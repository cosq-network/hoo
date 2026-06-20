#include "hvm/HVMJIT.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <mutex>
#include <sstream>

#include "core/SymbolMangler.h"
#include "hvm/HOModuleBase.h"
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_string.h"
#include "runtime/lib/hoo_character.h"
#include "runtime/lib/hoo_io.h"
#include "runtime/lib/hoo_generic_array.h"
#include "runtime/lib/hoo_tensor.h"
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_any.h"
#include "runtime/lib/hoo_anyarray.h"
#include "runtime/lib/hoo_hashmap.h"
#include "runtime/lib/hoo_exception.h"
#include "runtime/lib/hoo_math.h"
#include "runtime/lib/hoo_fs.h"
#include "runtime/lib/hoo_system.h"
#include "runtime/lib/hoo_regex.h"
#include "runtime/lib/hoo_uuid.h"
#include "runtime/lib/hoo_encoding.h"
#include "runtime/lib/hoo_thread.h"
#include "runtime/lib/hoo_csv.h"
#include "runtime/lib/hoo_datetime.h"
#include "runtime/lib/hoo_hashing.h"
#include "runtime/lib/hoo_process.h"
#include "runtime/lib/hoo_compression.h"
#include "runtime/lib/hoo_args.h"
#include "runtime/lib/hoo_buffer.h"
#include "runtime/lib/hoo_net.h"
#include "runtime/lib/hoo_json.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#define HVM_RUNTIME_EXPORT __declspec(dllexport)
#else
#define HVM_RUNTIME_EXPORT
#endif
#ifdef _WIN32
#define NOMINMAX
#include <io.h>
#include <process.h>
#include <thread>
#include <windows.h>
#include <wincrypt.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/TargetSelect.h"

namespace fs = std::filesystem;

namespace hooc {

namespace {
std::vector<std::string> splitModulePath(const std::string& moduleName) {
    std::vector<std::string> parts;
    std::stringstream ss(moduleName);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

std::string extractLegacyBaseFunctionName(const std::string& symbolName) {
    if (symbolName.rfind("_F_", 0) != 0) {
        return symbolName;
    }
    const size_t start = 3;
    const size_t end = symbolName.find('_', start);
    if (end == std::string::npos) {
        return symbolName.substr(start);
    }
    return symbolName.substr(start, end - start);
}

std::vector<std::string> buildLookupCandidates(const std::string& symbolName,
                                               const std::string& moduleName) {
    std::vector<std::string> candidates;
    appendUnique(candidates, symbolName);

    const auto demangled = SymbolMangler::demangleSymbol(symbolName);
    std::string baseFunctionName = demangled.functionName;
    if (baseFunctionName.empty() && !demangled.className.empty()) {
        baseFunctionName = demangled.className;
    }
    if (baseFunctionName.empty()) {
        baseFunctionName = extractLegacyBaseFunctionName(symbolName);
    }

    if (symbolName.rfind("_F_", 0) == 0 && !baseFunctionName.empty()) {
        // Legacy tests often ask for plain "_F_name_sig" entry points, while
        // source compilation now emits module-qualified names. Try both a
        // top-level function reconstruction and a class/member reconstruction.
        MangledFunctionParams plainParams{};
        plainParams.modulePath = demangled.modulePath;
        if (plainParams.modulePath.empty() && !moduleName.empty()) {
            plainParams.modulePath = splitModulePath(moduleName);
        }
        plainParams.functionName = baseFunctionName;
        plainParams.functionModifiers = demangled.functionModifiers;
        plainParams.returnType = demangled.returnType;
        plainParams.parameterTypes = demangled.parameterTypes;
        plainParams.isStatic = demangled.isStatic;
        plainParams.isVirtual = demangled.isVirtual;
        appendUnique(candidates, SymbolMangler::mangleFunctionName(plainParams));

        MangledFunctionParams memberParams{};
        memberParams.modulePath = demangled.modulePath;
        if (memberParams.modulePath.empty() && !moduleName.empty()) {
            memberParams.modulePath = splitModulePath(moduleName);
        }
        memberParams.className = demangled.className;
        memberParams.baseClassName = demangled.baseClassName;
        memberParams.classModifiers = demangled.classModifiers;
        memberParams.functionName = baseFunctionName;
        memberParams.functionModifiers = demangled.functionModifiers;
        memberParams.returnType = demangled.returnType;
        memberParams.parameterTypes = demangled.parameterTypes;
        memberParams.isConstructor = demangled.isConstructor;
        memberParams.isDestructor = demangled.isDestructor;
        memberParams.isStatic = demangled.isStatic;
        memberParams.isVirtual = demangled.isVirtual;
        appendUnique(candidates, SymbolMangler::mangleFunctionName(memberParams));
    }

    return candidates;
}

struct RuntimeSymbolContract {
    const char* name;
    void* addr;
};

constexpr int16_t kSysAlloc = 1;
constexpr int16_t kSysRetain = 2;
constexpr int16_t kSysRelease = 3;
constexpr int16_t kSysRefcount = 4;
constexpr int16_t kSysTypeId = 5;
constexpr int16_t kSysExceptionRuntime = 6;
constexpr int16_t kSysPushHandler = 7;
constexpr int16_t kSysPopHandler = 8;
constexpr int16_t kSysThrowToHandler = 9;
constexpr int16_t kSysRethrowToHandler = 10;
constexpr int16_t kSysStringData = 11;
constexpr int16_t kSysThreadCreate = 12;
constexpr int16_t kSysThreadExit = 13;
constexpr int16_t kSysFutex = 14;
constexpr int16_t kSysGetTid = 15;
constexpr int16_t kSysOpen = 16;
constexpr int16_t kSysRead = 17;
constexpr int16_t kSysWrite = 18;
constexpr int16_t kSysClose = 19;
constexpr int16_t kSysLseek = 20;
constexpr int16_t kSysFstat = 21;
constexpr int16_t kSysClockGetTime = 22;
constexpr int16_t kSysGetRandom = 23;
constexpr uint64_t kNoHandlerPc = ~uint64_t{0};
constexpr size_t kMaxInboundTrampolineSlots = 8;
constexpr bool kEnableEscapeAllocaPromotion = false;
bool isArcUseDefGraphEnabled() {
    if (const char* v = std::getenv("HOOC_ENABLE_ARC_USEDEF")) {
        return std::string(v) == "1" || std::string(v) == "true" || std::string(v) == "TRUE";
    }
    return false;
}

bool isEscapeAllocaPromotionEnabled() {
    if (const char* v = std::getenv("HOOC_ENABLE_ESCAPE_ALLOCA")) {
        return std::string(v) == "1" || std::string(v) == "true" || std::string(v) == "TRUE";
    }
    return false;
}

bool isDwarfDebugInfoEnabled() {
    if (const char* v = std::getenv("HOOC_ENABLE_DWARF")) {
        return std::string(v) == "1" || std::string(v) == "true" || std::string(v) == "TRUE";
    }
    return false;
}

struct ARCUseDefGraph {
    // Map of instruction PC to "skip as no-op".
    std::unordered_set<uint64_t> skipPc;
};

ARCUseDefGraph buildARCUseDefGraph(const hvm::Section& text, uint64_t entryPc) {
    ARCUseDefGraph g;
    std::vector<std::pair<uint64_t, std::unique_ptr<hvm::HVMInstruction>>> stream;
    uint64_t pc = entryPc;
    while (pc < text.data.size()) {
        std::vector<uint8_t> slice;
        const size_t maxRead = static_cast<size_t>(std::min<uint64_t>(8, text.data.size() - pc));
        slice.insert(slice.end(), text.data.begin() + static_cast<ptrdiff_t>(pc),
                     text.data.begin() + static_cast<ptrdiff_t>(pc + maxRead));
        size_t used = 0;
        auto ins = hvm::HVMInstruction::decode(slice, used);
        if (!ins || used == 0) {
            break;
        }
        stream.emplace_back(pc, std::move(ins));
        if (stream.back().second && stream.back().second->getOpcode() == hvm::Opcode::RET) {
            break;
        }
        pc += used;
    }

    std::unordered_map<uint64_t, size_t> indexByPc;
    indexByPc.reserve(stream.size());
    for (size_t i = 0; i < stream.size(); ++i) {
        indexByPc.emplace(stream[i].first, i);
    }

    auto writesReg2 = [](const hvm::HVMInstruction& ins) -> bool {
        if (std::holds_alternative<hvm::OperandsI>(ins.getOperands())) {
            return std::get<hvm::OperandsI>(ins.getOperands()).rd == 2;
        }
        if (std::holds_alternative<hvm::OperandsR>(ins.getOperands())) {
            return std::get<hvm::OperandsR>(ins.getOperands()).rd == 2;
        }
        if (std::holds_alternative<hvm::OperandsJ>(ins.getOperands())) {
            return std::get<hvm::OperandsJ>(ins.getOperands()).rd == 2;
        }
        return false;
    };
    auto isArcSyscall = [](const hvm::HVMInstruction& ins) -> bool {
        if (ins.getOpcode() != hvm::Opcode::SYSCALL ||
            !std::holds_alternative<hvm::OperandsI>(ins.getOperands())) {
            return false;
        }
        const auto oi = std::get<hvm::OperandsI>(ins.getOperands());
        return oi.imm15 == kSysRetain || oi.imm15 == kSysRelease;
    };
    auto isControlBarrier = [](hvm::Opcode op) -> bool {
        return op == hvm::Opcode::RET || op == hvm::Opcode::CALL ||
               op == hvm::Opcode::TAILCALL || op == hvm::Opcode::JAL ||
               op == hvm::Opcode::JALR;
    };

    std::vector<std::vector<size_t>> succ(stream.size());
    for (size_t i = 0; i < stream.size(); ++i) {
        const auto& [pcI, insI] = stream[i];
        const auto op = insI->getOpcode();
        auto addSuccByPc = [&](uint64_t targetPc) {
            auto it = indexByPc.find(targetPc);
            if (it != indexByPc.end()) {
                succ[i].push_back(it->second);
            }
        };
        if (op == hvm::Opcode::RET || op == hvm::Opcode::CALL ||
            op == hvm::Opcode::TAILCALL || op == hvm::Opcode::JAL ||
            op == hvm::Opcode::JALR) {
            continue;
        }
        if (op == hvm::Opcode::JMP) {
            auto o = std::get<hvm::OperandsJ>(insI->getOperands());
            const uint64_t target = static_cast<uint64_t>(
                static_cast<int64_t>(pcI) + static_cast<int64_t>(o.offset) * 4);
            addSuccByPc(target);
            continue;
        }
        if (op == hvm::Opcode::BEQ || op == hvm::Opcode::BNE ||
            op == hvm::Opcode::BLT || op == hvm::Opcode::BLE) {
            auto o = std::get<hvm::OperandsB>(insI->getOperands());
            const uint64_t target = static_cast<uint64_t>(
                static_cast<int64_t>(pcI) + static_cast<int64_t>(o.imm15) * 4);
            addSuccByPc(target);
        }
        if (i + 1 < stream.size()) {
            addSuccByPc(stream[i + 1].first);
        }
    }

    struct PendingArc {
        size_t startIndex = 0;
        int16_t kind = 0;
    };
    std::vector<std::optional<PendingArc>> inPending(stream.size());
    std::vector<bool> inConflict(stream.size(), false);
    std::vector<size_t> worklist;
    worklist.push_back(0);

    while (!worklist.empty()) {
        const size_t idx = worklist.back();
        worklist.pop_back();
        if (idx >= stream.size()) {
            continue;
        }
        auto pending = inPending[idx];
        bool conflict = inConflict[idx];
        const auto& ins = *stream[idx].second;
        const auto op = ins.getOpcode();

        if (writesReg2(ins) || isControlBarrier(op)) {
            pending.reset();
            conflict = false;
        } else if (isArcSyscall(ins)) {
            const auto oi = std::get<hvm::OperandsI>(ins.getOperands());
            if (pending.has_value()) {
                const bool inverse =
                    (pending->kind == kSysRetain && oi.imm15 == kSysRelease) ||
                    (pending->kind == kSysRelease && oi.imm15 == kSysRetain);
                if (inverse && !conflict) {
                    g.skipPc.insert(stream[pending->startIndex].first);
                    g.skipPc.insert(stream[idx].first);
                    pending.reset();
                    conflict = false;
                } else {
                    pending = PendingArc{idx, oi.imm15};
                    conflict = false;
                }
            } else {
                pending = PendingArc{idx, oi.imm15};
                conflict = false;
            }
        }

        for (size_t next : succ[idx]) {
            if (next >= stream.size()) {
                continue;
            }
            bool changed = false;
            if (!inPending[next].has_value() && pending.has_value()) {
                inPending[next] = pending;
                inConflict[next] = conflict;
                changed = true;
            } else if (inPending[next].has_value() && pending.has_value()) {
                if (inPending[next]->startIndex != pending->startIndex ||
                    inPending[next]->kind != pending->kind) {
                    if (!inConflict[next]) {
                        inConflict[next] = true;
                        changed = true;
                    }
                } else if (inConflict[next] != conflict) {
                    inConflict[next] = inConflict[next] || conflict;
                    changed = true;
                }
            } else if (!pending.has_value() && inPending[next].has_value()) {
                if (!inConflict[next]) {
                    inConflict[next] = true;
                    changed = true;
                }
            } else if (conflict && !inConflict[next]) {
                inConflict[next] = true;
                changed = true;
            }
            if (changed) {
                worklist.push_back(next);
            }
        }
    }
    return g;
}

bool isSpecDevirtualizableName(const std::string& name) {
    const auto sym = SymbolMangler::demangleSymbol(name);
    for (const auto& mod : sym.classModifiers) {
        if (mod == "FINAL" || mod == "SINGLETON") {
            return true;
        }
    }
    return false;
}

std::mutex gShadowHandlersMu;

struct ShadowHandlerFrame {
    uint64_t handlerPc = 0;
    int64_t savedLr = 0;
    int64_t savedFp = 0;
    int64_t savedSp = 0;
};

std::unordered_map<void*, std::vector<ShadowHandlerFrame>> gShadowHandlers;
std::unordered_map<void*, uint64_t> gCurrentExceptionByState;
std::mutex gStateOwnerMu;
std::unordered_map<void*, HVMJIT*> gStateOwnerByPtr;
std::mutex gInboundTrampolineOwnerMu;
std::array<HVMJIT*, kMaxInboundTrampolineSlots> gInboundTrampolineOwnerBySlot{};
size_t gInboundNextSlot = 0;

uint64_t shadow_push_handler(HVMJIT::HVMState* state, uint64_t handlerPc) {
    if (!state) return 0;
    ShadowHandlerFrame frame;
    frame.handlerPc = handlerPc;
    frame.savedLr = state->regs[29];
    frame.savedFp = state->regs[30];
    frame.savedSp = state->regs[31];
    std::lock_guard<std::mutex> lk(gShadowHandlersMu);
    gShadowHandlers[state].push_back(frame);
    return 0;
}

uint64_t shadow_pop_handler(HVMJIT::HVMState* state) {
    if (!state) return 0;
    std::lock_guard<std::mutex> lk(gShadowHandlersMu);
    auto it = gShadowHandlers.find(state);
    if (it == gShadowHandlers.end() || it->second.empty()) return 0;
    it->second.pop_back();
    if (it->second.empty()) {
        gShadowHandlers.erase(it);
    }
    return 0;
}

uint64_t shadow_throw_to_handler(HVMJIT::HVMState* state, uint64_t exc, bool rethrow) {
    if (!state) return kNoHandlerPc;

    std::lock_guard<std::mutex> lk(gShadowHandlersMu);
    uint64_t effectiveExc = exc;
    if (rethrow) {
        auto exIt = gCurrentExceptionByState.find(state);
        if (exIt != gCurrentExceptionByState.end()) {
            effectiveExc = exIt->second;
        }
    } else {
        gCurrentExceptionByState[state] = exc;
    }

    auto it = gShadowHandlers.find(state);
    if (it == gShadowHandlers.end() || it->second.empty()) {
        return kNoHandlerPc;
    }

    const ShadowHandlerFrame frame = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) {
        gShadowHandlers.erase(it);
    }

    state->regs[29] = frame.savedLr;
    state->regs[30] = frame.savedFp;
    state->regs[31] = frame.savedSp;
    state->regs[1] = static_cast<int64_t>(effectiveExc);
    return frame.handlerPc;
}

void shadow_clear_state(HVMJIT::HVMState* state) {
    if (!state) return;
    std::lock_guard<std::mutex> lk(gShadowHandlersMu);
    gShadowHandlers.erase(state);
    gCurrentExceptionByState.erase(state);
}

uint64_t shadow_should_stop_state(void* state_ptr) {
    if (!state_ptr) return 0;
    std::lock_guard<std::mutex> lk(gStateOwnerMu);
    auto it = gStateOwnerByPtr.find(state_ptr);
    if (it == gStateOwnerByPtr.end() || !it->second) {
        return 0;
    }
    return it->second->getStopExecutionRequested() ? 1 : 0;
}

// JIT-compatible wrappers for runtime functions.
// These follow the int64_t(void* HVMState) convention used by the JIT translator.
extern "C" {
    uint64_t hooc_hvm_inbound_trampoline_dispatch(size_t slot, uint64_t arg0) {
        HVMJIT* owner = nullptr;
        {
            std::lock_guard<std::mutex> lk(gInboundTrampolineOwnerMu);
            if (slot >= kMaxInboundTrampolineSlots) {
                return static_cast<uint64_t>(-1);
            }
            owner = gInboundTrampolineOwnerBySlot[slot];
        }
        if (!owner) {
            return static_cast<uint64_t>(-1);
        }
        return static_cast<uint64_t>(owner->invokeInboundCallback(slot, {arg0}));
    }
    uint64_t hooc_hvm_inbound_trampoline_dispatch2(size_t slot, uint64_t arg0, uint64_t arg1) {
        HVMJIT* owner = nullptr;
        {
            std::lock_guard<std::mutex> lk(gInboundTrampolineOwnerMu);
            if (slot >= kMaxInboundTrampolineSlots) {
                return static_cast<uint64_t>(-1);
            }
            owner = gInboundTrampolineOwnerBySlot[slot];
        }
        if (!owner) {
            return static_cast<uint64_t>(-1);
        }
        return static_cast<uint64_t>(owner->invokeInboundCallback(slot, {arg0, arg1}));
    }
    uint64_t hooc_hvm_inbound_trampoline_0(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(0, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_1(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(1, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_2(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(2, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_3(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(3, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_4(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(4, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_5(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(5, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_6(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(6, arg0); }
    uint64_t hooc_hvm_inbound_trampoline_7(uint64_t arg0) { return hooc_hvm_inbound_trampoline_dispatch(7, arg0); }
    uint64_t hooc_hvm_inbound_trampoline2_0(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(0, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_1(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(1, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_2(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(2, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_3(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(3, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_4(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(4, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_5(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(5, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_6(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(6, arg0, arg1);
    }
    uint64_t hooc_hvm_inbound_trampoline2_7(uint64_t arg0, uint64_t arg1) {
        return hooc_hvm_inbound_trampoline_dispatch2(7, arg0, arg1);
    }

    uint64_t jit_hoo_alloc(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        // Unified allocation: use the runtime's C heap allocator (hoo_alloc).
        // Returns a real C heap pointer instead of an HVM bump offset.
        size_t size = static_cast<size_t>(state->regs[1]);
        int64_t typeId = state->regs[2];
        void* obj = hoo_alloc(size, typeId);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(obj));
    }
    uint64_t jit_hoo_retain(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_retain(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_release(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_get_refcount(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_get_refcount(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_get_type_id(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_get_type_id(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_string_from_cstr(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* cstr = reinterpret_cast<const char*>(state->memory + state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_cstr(cstr)));
    }
    uint64_t jit_hoo_string_from_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_int64(state->regs[1])));
    }
    uint64_t jit_hoo_string_from_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double val;
        std::memcpy(&val, &state->regs[1], sizeof(double));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_double(val)));
    }
    uint64_t jit_hoo_string_concat(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_concat(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2]))));
    }
    uint64_t jit_hoo_string_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_string_to_upper(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_to_upper(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_string_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_data(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_string_to_characters(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_to_characters(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_string_join(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_join(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_string_from_object(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_object(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_string_from_any(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_any(state->regs[1], state->regs[2])));
    }
    uint64_t jit_hoo_string_from_bytes(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* data = reinterpret_cast<const char*>(state->memory + state->regs[1]);
        int64_t length = state->regs[2];
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_bytes(data, length)));
    }
    uint64_t jit_hoo_string_from_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_bool(state->regs[1])));
    }
    uint64_t jit_hoo_character_from_utf8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* bytes = reinterpret_cast<const char*>(state->memory + state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_character_from_utf8(bytes, state->regs[2])));
    }
    uint64_t jit_hoo_character_from_utf8_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooString str = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_character_from_utf8(hoo_string_data(str), hoo_string_length(str))));
    }
    uint64_t jit_hoo_character_from_codepoint(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_character_from_codepoint(state->regs[1])));
    }
    uint64_t jit_hoo_character_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_character_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_character_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooCharacter ch = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_cstr(hoo_character_data(ch))));
    }
    uint64_t jit_hoo_character_codepoint(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_character_codepoint(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_character_print(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_character_print(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_character_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_character_release(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_print(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_print(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_println(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_println(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_readline(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_readline()));
    }
    uint64_t jit_hoo_readchar(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_readchar());
    }
    uint64_t jit_hoo_array_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_new()));
    }
    uint64_t jit_hoo_array_push_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooArray new_arr = hoo_array_push_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2]);
        state->regs[1] = static_cast<int64_t>(reinterpret_cast<intptr_t>(new_arr));
        return 0;
    }
    uint64_t jit_hoo_array_get_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t dest = 0;
        hoo_array_get_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2], &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_hoo_buffer_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_buffer_new(0)));
    }
    uint64_t jit_hoo_buffer_from_bytes(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* data = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_buffer_from_bytes(reinterpret_cast<const uint8_t*>(data), state->regs[2])));
    }
    uint64_t jit_hoo_buffer_copy(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_buffer_copy(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_buffer_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_buffer_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_buffer_capacity(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_buffer_capacity(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_buffer_byte_at(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_buffer_byte_at(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_hoo_buffer_set_byte(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_buffer_set_byte(reinterpret_cast<void*>(state->regs[1]), state->regs[2], state->regs[3]));
    }
    uint64_t jit_hoo_buffer_append(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* data = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_buffer_append(reinterpret_cast<void*>(state->regs[1]),
                              reinterpret_cast<const uint8_t*>(data), state->regs[3])));
    }
    uint64_t jit_hoo_buffer_append_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_buffer_append_buffer(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2]))));
    }
    uint64_t jit_hoo_buffer_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_buffer_clear(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hoo_buffer_slice(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_buffer_slice(reinterpret_cast<void*>(state->regs[1]), state->regs[2], state->regs[3])));
    }
    uint64_t jit_hoo_buffer_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_buffer_data(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_map_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_map_new(static_cast<int>(state->regs[1]), static_cast<int>(state->regs[2]))));
    }
    uint64_t jit_hoo_exception_runtime(void* /*state_ptr*/) {
        HooException exc = hoo_exception_runtime("hvm runtime exception");
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(exc));
    }
    uint64_t jit_hoo_exception_clear(void* /*state_ptr*/) {
        hoo_exception_clear();
        return 0;
    }
    uint64_t jit_hoo_push_handler(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint64_t handlerPc = static_cast<uint64_t>(state->regs[2]);
        hoo_push_handler(reinterpret_cast<void*>(handlerPc));
        return shadow_push_handler(state, handlerPc);
    }
    uint64_t jit_hoo_pop_handler(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_pop_handler();
        return shadow_pop_handler(state);
    }
    uint64_t jit_hoo_throw(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
#ifdef _WIN32
        const uint64_t exc = static_cast<uint64_t>(state->regs[2]);
        hoo_exception_set_current(reinterpret_cast<HooException>(exc));
        return shadow_throw_to_handler(state, exc, false);
#else
        try {
            hoo_exception_throw(reinterpret_cast<HooException>(state->regs[1]));
        } catch (...) {
        }
        return shadow_throw_to_handler(state, static_cast<uint64_t>(state->regs[1]), false);
#endif
    }
    uint64_t jit_hoo_rethrow(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
#ifdef _WIN32
        return shadow_throw_to_handler(state, 0, true);
#else
        try {
            hoo_exception_rethrow();
        } catch (...) {
        }
        return shadow_throw_to_handler(state, 0, true);
#endif
    }

    // ── String aliases (match codegen-generated _F_string_*_v_p names) ──────
    uint64_t jit_string_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_new()));
    }
    uint64_t jit_string_is_empty(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_is_empty(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_string_to_lower(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_to_lower(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_string_equals(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_equals(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2])));
    }
    uint64_t jit_string_contains(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_contains(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2])));
    }
    uint64_t jit_string_starts_with(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_starts_with(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2])));
    }
    uint64_t jit_string_trim(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_trim(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_string_repeat(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_repeat(static_cast<char>(state->regs[1]), state->regs[2])));
    }
    uint64_t jit_string_index_of(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_string_index_of(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2])));
    }
    // ── Array aliases (match codegen-generated _F_array_*_v_p names) ─────────
    uint64_t jit_array_push_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double val;
        std::memcpy(&val, &state->regs[2], sizeof(double));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_push_double(reinterpret_cast<void*>(state->regs[1]), val)));
    }
    uint64_t jit_array_get_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double dest = 0.0;
        hoo_array_get_double(reinterpret_cast<void*>(state->regs[1]), state->regs[2], &dest);
        uint64_t bits;
        std::memcpy(&bits, &dest, sizeof(double));
        return bits;
    }
    uint64_t jit_array_push_int64_plain(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_push_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2])));
    }
    uint64_t jit_array_get_int64_plain(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t dest = 0;
        hoo_array_get_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2], &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_array_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_array_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_array_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_array_clear(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_array_empty(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_array_empty(reinterpret_cast<void*>(state->regs[1])));
    }
    // ── Object field access helpers ──────────────────────────────────────
    uint64_t jit_object_get_field(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_object_get_field(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_object_set_field(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_object_set_field(reinterpret_cast<void*>(state->regs[1]), state->regs[2], state->regs[3]);
        return 0;
    }
    uint64_t jit_array_set_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_array_set(reinterpret_cast<void*>(state->regs[1]), state->regs[2], reinterpret_cast<void*>(&state->regs[3])));
    }
    uint64_t jit_array_push_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* cstr = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_push_string(reinterpret_cast<void*>(state->regs[1]), cstr)));
    }
    uint64_t jit_array_get_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* dest = nullptr;
        hoo_array_get_string(reinterpret_cast<void*>(state->regs[1]), state->regs[2], &dest);
        if (!dest) return 0;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_cstr(dest)));
    }
    uint64_t jit_array_push_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_push_bool(reinterpret_cast<void*>(state->regs[1]), state->regs[2])));
    }
    uint64_t jit_array_get_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t dest = 0;
        hoo_array_get_bool(reinterpret_cast<void*>(state->regs[1]), state->regs[2], &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_array_push_array(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_array_push_array(reinterpret_cast<void*>(state->regs[1]),
                                reinterpret_cast<void*>(state->regs[2]))));
    }
    uint64_t jit_array_push_object(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_array_push_object(reinterpret_cast<void*>(state->regs[1]),
                                 reinterpret_cast<void*>(state->regs[2]))));
    }
    uint64_t jit_tensor_new1(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_tensor_new1(state->regs[1], state->regs[2])));
    }
    uint64_t jit_tensor_new2(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_tensor_new2(state->regs[1], state->regs[2], state->regs[3])));
    }
    uint64_t jit_tensor_new3(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_tensor_new3(state->regs[1], state->regs[2], state->regs[3], state->regs[5])));
    }
    uint64_t jit_tensor_push_value(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_tensor_push_value(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_tensor_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_tensor_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_tensor_dim(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_tensor_dim(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_tensor_get_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_tensor_get_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_tensor_get_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double value = hoo_tensor_get_double(reinterpret_cast<void*>(state->regs[1]), state->regs[2]);
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        return bits;
    }
    uint64_t jit_tensor_binary(void* state_ptr, HooTensor (*fn)(HooTensor, HooTensor)) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            fn(reinterpret_cast<void*>(state->regs[1]), reinterpret_cast<void*>(state->regs[2]))));
    }
    uint64_t jit_tensor_add(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_add); }
    uint64_t jit_tensor_sub(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_sub); }
    uint64_t jit_tensor_element_mul(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_element_mul); }
    uint64_t jit_tensor_element_div(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_element_div); }
    uint64_t jit_tensor_matmul(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_matmul); }
    uint64_t jit_tensor_eq(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_eq); }
    uint64_t jit_tensor_ne(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_ne); }
    uint64_t jit_tensor_lt(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_lt); }
    uint64_t jit_tensor_le(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_le); }
    uint64_t jit_tensor_gt(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_gt); }
    uint64_t jit_tensor_ge(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_ge); }
    uint64_t jit_tensor_and(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_and); }
    uint64_t jit_tensor_or(void* state_ptr) { return jit_tensor_binary(state_ptr, &hoo_tensor_or); }
    uint64_t jit_tensor_not(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_tensor_not(reinterpret_cast<void*>(state->regs[1]))));
    }
    // ── Map aliases (match codegen-generated _F_map_*_v_p names) ─────────────
    uint64_t jit_map_new_plain(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_map_new(static_cast<int>(state->regs[1]), static_cast<int>(state->regs[2]))));
    }
    uint64_t jit_map_new_types(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_map_new(static_cast<int>(state->regs[1]), static_cast<int>(state->regs[2]))));
    }
    uint64_t jit_map_set_int64_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        int64_t val = static_cast<int64_t>(state->regs[3]);
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), &key, &val));
    }
    uint64_t jit_map_get_int64_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        int64_t dest = 0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), &key, &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_map_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_map_count(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_map_set_string_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        int64_t val = static_cast<int64_t>(state->regs[3]);
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), key, &val));
    }
    uint64_t jit_map_contains_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        return static_cast<uint64_t>(hoo_map_contains_key(reinterpret_cast<void*>(state->regs[1]), &key));
    }
    uint64_t jit_map_remove_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        return static_cast<uint64_t>(hoo_map_remove(reinterpret_cast<void*>(state->regs[1]), &key));
    }
    uint64_t jit_map_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_map_clear(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_map_empty(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_map_is_empty(reinterpret_cast<void*>(state->regs[1])));
    }
    // ── Map extended typed operations ─────────────────────────────────────────
    uint64_t jit_map_set_int64_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        double val;
        std::memcpy(&val, &state->regs[3], sizeof(double));
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), &key, &val));
    }
    uint64_t jit_map_get_int64_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        double dest = 0.0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), &key, &dest);
        uint64_t bits;
        std::memcpy(&bits, &dest, sizeof(double));
        return bits;
    }
    uint64_t jit_map_set_int64_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        const char* val = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), &key, val));
    }
    uint64_t jit_map_get_int64_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        const char* dest = nullptr;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), &key, &dest);
        if (!dest) return 0;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_cstr(dest)));
    }
    uint64_t jit_map_set_int64_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        int64_t val = static_cast<int64_t>(state->regs[3]);
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), &key, &val));
    }
    uint64_t jit_map_get_int64_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t key = static_cast<int64_t>(state->regs[2]);
        int64_t dest = 0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), &key, &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_map_set_string_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        double val;
        std::memcpy(&val, &state->regs[3], sizeof(double));
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), key, &val));
    }
    uint64_t jit_map_get_string_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        double dest = 0.0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), key, &dest);
        uint64_t bits;
        std::memcpy(&bits, &dest, sizeof(double));
        return bits;
    }
    uint64_t jit_map_set_string_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* val = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), key, val));
    }
    uint64_t jit_map_get_string_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* dest = nullptr;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), key, &dest);
        if (!dest) return 0;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_cstr(dest)));
    }
    uint64_t jit_map_set_string_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        int64_t val = static_cast<int64_t>(state->regs[3]);
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), key, &val));
    }
    uint64_t jit_map_get_string_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        int64_t dest = 0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), key, &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_map_set_int8_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int8_t key = static_cast<int8_t>(state->regs[2]);
        int64_t val = static_cast<int64_t>(state->regs[3]);
        return static_cast<uint64_t>(hoo_map_set(reinterpret_cast<void*>(state->regs[1]), &key, &val));
    }
    uint64_t jit_map_get_int8_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int8_t key = static_cast<int8_t>(state->regs[2]);
        int64_t dest = 0;
        hoo_map_try_get(reinterpret_cast<void*>(state->regs[1]), &key, &dest);
        return static_cast<uint64_t>(dest);
    }
    uint64_t jit_map_key_type(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_map_key_type(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_map_value_type(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_map_value_type(reinterpret_cast<void*>(state->regs[1])));
    }

    uint64_t jit_anyarray_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_anyarray_new()));
    }
    uint64_t jit_anyarray_new_capacity(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_anyarray_new_capacity(state->regs[1])));
    }
    uint64_t jit_anyarray_length(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_anyarray_length(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_anyarray_push(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_anyarray_push(
            reinterpret_cast<void*>(state->regs[1]),
            static_cast<int64_t>(state->regs[2]),
            state->regs[3]));
    }
    uint64_t jit_anyarray_set(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_anyarray_set(
            reinterpret_cast<void*>(state->regs[1]),
            static_cast<int64_t>(state->regs[2]),
            static_cast<int64_t>(state->regs[3]),
            state->regs[5]));
    }
    uint64_t jit_anyarray_get_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooAnyValue value{0, 0};
        if (!hoo_anyarray_get(reinterpret_cast<void*>(state->regs[1]), static_cast<int64_t>(state->regs[2]), &value)) {
            return 0;
        }
        return value.data;
    }
    uint64_t jit_anyarray_pop_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooAnyValue value{0, 0};
        if (!hoo_anyarray_pop(reinterpret_cast<void*>(state->regs[1]), &value)) {
            return 0;
        }
        return value.data;
    }
    uint64_t jit_anyarray_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_anyarray_clear(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_anyarray_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_anyarray_release(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }

    uint64_t jit_hashmap_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
            hoo_hashmap_new(static_cast<int64_t>(state->regs[1]), static_cast<int64_t>(state->regs[2]))));
    }
    uint64_t jit_hashmap_count(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_hashmap_count(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_hashmap_set_fixed(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_hashmap_set_fixed_i8(
            reinterpret_cast<void*>(state->regs[1]),
            static_cast<int64_t>(state->regs[2]),
            state->regs[3]));
    }
    uint64_t jit_hashmap_get_fixed_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        uint64_t value = 0;
        hoo_hashmap_get_fixed_i8(reinterpret_cast<void*>(state->regs[1]), static_cast<int64_t>(state->regs[2]), &value);
        return value;
    }
    uint64_t jit_hashmap_set_any(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_hashmap_set_any_i8(
            reinterpret_cast<void*>(state->regs[1]),
            static_cast<int64_t>(state->regs[2]),
            static_cast<int64_t>(state->regs[3]),
            state->regs[5]));
    }
    uint64_t jit_hashmap_get_any_data(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooAnyValue value{0, 0};
        if (!hoo_hashmap_get_any_i8(reinterpret_cast<void*>(state->regs[1]), static_cast<int64_t>(state->regs[2]), &value)) {
            return 0;
        }
        return value.data;
    }
    uint64_t jit_hashmap_remove(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_hashmap_remove_i8(
            reinterpret_cast<void*>(state->regs[1]),
            static_cast<int64_t>(state->regs[2])));
    }
    uint64_t jit_hashmap_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_hashmap_clear(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hashmap_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_hashmap_release(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    // ── Math functions ────────────────────────────────────────────────────────
    uint64_t jit_math_abs_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_abs_int64(state->regs[1]));
    }
    uint64_t jit_math_abs_int8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(static_cast<int64_t>(hoo_math_abs_int8(static_cast<int8_t>(state->regs[1]))));
    }
    uint64_t jit_math_abs_byte(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_abs_byte(static_cast<uint8_t>(state->regs[1])));
    }
    uint64_t jit_math_abs_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_abs_double(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_abs_f8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_abs_f8(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_min_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_min_int64(state->regs[1], state->regs[2]));
    }
    uint64_t jit_math_min_int8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(static_cast<int64_t>(hoo_math_min_int8(static_cast<int8_t>(state->regs[1]), static_cast<int8_t>(state->regs[2]))));
    }
    uint64_t jit_math_min_byte(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_min_byte(static_cast<uint8_t>(state->regs[1]), static_cast<uint8_t>(state->regs[2])));
    }
    uint64_t jit_math_min_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double a, b;
        std::memcpy(&a, &state->regs[1], sizeof(double));
        std::memcpy(&b, &state->regs[2], sizeof(double));
        double result = hoo_math_min_double(a, b);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_min_f8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double a, b;
        std::memcpy(&a, &state->regs[1], sizeof(double));
        std::memcpy(&b, &state->regs[2], sizeof(double));
        double result = hoo_math_min_f8(a, b);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_max_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_max_int64(state->regs[1], state->regs[2]));
    }
    uint64_t jit_math_max_int8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(static_cast<int64_t>(hoo_math_max_int8(static_cast<int8_t>(state->regs[1]), static_cast<int8_t>(state->regs[2]))));
    }
    uint64_t jit_math_max_byte(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_max_byte(static_cast<uint8_t>(state->regs[1]), static_cast<uint8_t>(state->regs[2])));
    }
    uint64_t jit_math_max_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double a, b;
        std::memcpy(&a, &state->regs[1], sizeof(double));
        std::memcpy(&b, &state->regs[2], sizeof(double));
        double result = hoo_math_max_double(a, b);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_max_f8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double a, b;
        std::memcpy(&a, &state->regs[1], sizeof(double));
        std::memcpy(&b, &state->regs[2], sizeof(double));
        double result = hoo_math_max_f8(a, b);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_sign_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_sign_int64(state->regs[1]));
    }
    uint64_t jit_math_sign_int8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(static_cast<int64_t>(hoo_math_sign_int8(static_cast<int8_t>(state->regs[1]))));
    }
    uint64_t jit_math_sign_byte(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_sign_byte(static_cast<uint8_t>(state->regs[1])));
    }
    uint64_t jit_math_sign_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_sign_double(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_sign_f8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_sign_f8(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_gcd(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_gcd(state->regs[1], state->regs[2]));
    }
    uint64_t jit_math_factorial(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_factorial(state->regs[1]));
    }
    uint64_t jit_math_fibonacci(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_fibonacci(state->regs[1]));
    }
    uint64_t jit_math_is_even(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_is_even(state->regs[1]));
    }
    uint64_t jit_math_is_odd(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_is_odd(state->regs[1]));
    }
    uint64_t jit_math_is_prime(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_is_prime(state->regs[1]));
    }
    uint64_t jit_math_lcm(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_lcm(state->regs[1], state->regs[2]));
    }
    uint64_t jit_math_sqrt(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_sqrt(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_pi(void* /*state_ptr*/) {
        double result = hoo_math_get_pi();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_e(void* /*state_ptr*/) {
        double result = hoo_math_get_e();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_tau(void* /*state_ptr*/) {
        double result = hoo_math_get_tau();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_inf(void* /*state_ptr*/) {
        double result = hoo_math_get_inf();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_neg_inf(void* /*state_ptr*/) {
        double result = hoo_math_get_neg_inf();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_get_nan(void* /*state_ptr*/) {
        double result = hoo_math_get_nan();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_pow(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double base, exp;
        std::memcpy(&base, &state->regs[1], sizeof(double));
        std::memcpy(&exp, &state->regs[2], sizeof(double));
        double result = hoo_math_pow(base, exp);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_clamp(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double val, min, max;
        std::memcpy(&val, &state->regs[1], sizeof(double));
        std::memcpy(&min, &state->regs[2], sizeof(double));
        std::memcpy(&max, &state->regs[3], sizeof(double));
        double result = hoo_math_clamp(val, min, max);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_floor(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_floor(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_cbrt(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_cbrt(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_hypot(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double x, y;
        std::memcpy(&x, &state->regs[1], sizeof(double));
        std::memcpy(&y, &state->regs[2], sizeof(double));
        double result = hoo_math_hypot(x, y);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_ceil(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_ceil(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_sin(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_sin(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_cos(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_cos(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_tan(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_tan(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_atan2(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double y, x;
        std::memcpy(&y, &state->regs[1], sizeof(double));
        std::memcpy(&x, &state->regs[2], sizeof(double));
        double result = hoo_math_atan2(y, x);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_unary_asin(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_asin(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_unary_acos(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_acos(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_unary_atan(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_atan(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_sinh(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_sinh(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_cosh(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_cosh(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_tanh(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_tanh(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_exp(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_exp(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_exp2(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_exp2(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_expm1(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_expm1(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_log(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_log(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_log10(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_log10(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_log2(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_log2(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_log1p(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_log1p(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_round(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_round(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_trunc(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_trunc(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_math_fract(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double arg; std::memcpy(&arg, &state->regs[1], sizeof(double));
        double result = hoo_math_fract(arg);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_random_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_math_random_new()));
    }
    uint64_t jit_random_new_with_seed(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_math_random_new_with_seed(state->regs[1])));
    }
    uint64_t jit_random_next_int(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_random_next_int(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_random_next_int_max(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_random_next_int_max(reinterpret_cast<void*>(state->regs[1]), state->regs[2]));
    }
    uint64_t jit_random_next_double(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        double result = hoo_math_random_next_double(reinterpret_cast<void*>(state->regs[1]));
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_random_next_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_random_next_bool(reinterpret_cast<void*>(state->regs[1])));
    }
    uint64_t jit_random_next_bytes(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_math_random_next_bytes(
            reinterpret_cast<void*>(state->regs[1]),
            reinterpret_cast<void*>(state->regs[2]),
            state->regs[3]));
    }
    uint64_t jit_random_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_math_random_release(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    // ── Standard library (hoo module namespace) ───────────────────────────────
    uint64_t jit_system_get_env(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* val = hoo_system_get_env(name);
        if (!val) return 0;
        void* str = hoo_string_from_cstr(val);
        hoo_system_free_string(val);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_set_env(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* value = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(hoo_system_set_env(name, value));
    }
    uint64_t jit_system_unset_env(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_system_unset_env(name));
    }
    uint64_t jit_system_hostname(void* /*state_ptr*/) {
        char* hostname = hoo_system_hostname();
        if (!hostname) return 0;
        void* str = hoo_string_from_cstr(hostname);
        hoo_system_free_string(hostname);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_os_name(void* /*state_ptr*/) {
        char* os = hoo_system_os_name();
        if (!os) return 0;
        void* str = hoo_string_from_cstr(os);
        hoo_system_free_string(os);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_os_version(void* /*state_ptr*/) {
        char* ver = hoo_system_os_version();
        if (!ver) return 0;
        void* str = hoo_string_from_cstr(ver);
        hoo_system_free_string(ver);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_cpu_count(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_system_cpu_count());
    }
    uint64_t jit_system_process_id(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_system_process_id());
    }
    uint64_t jit_system_uptime_ms(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_system_uptime_ms());
    }
    uint64_t jit_system_exit(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t code = state->regs[1];
        hoo_system_exit(code);
        return 0;
    }
    uint64_t jit_system_exec(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* command = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* out = hoo_system_exec(command);
        if (!out) return 0;
        void* str = hoo_string_from_cstr(out);
        hoo_system_free_string(out);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_exec_status(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* command = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_system_exec_status(command));
    }
    uint64_t jit_system_user_home(void* /*state_ptr*/) {
        char* home = hoo_system_user_home();
        if (!home) return 0;
        void* str = hoo_string_from_cstr(home);
        hoo_system_free_string(home);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_user_name(void* /*state_ptr*/) {
        char* name = hoo_system_user_name();
        if (!name) return 0;
        void* str = hoo_string_from_cstr(name);
        hoo_system_free_string(name);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_current_dir(void* /*state_ptr*/) {
        char* dir = hoo_system_current_dir();
        if (!dir) return 0;
        void* str = hoo_string_from_cstr(dir);
        hoo_system_free_string(dir);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_system_set_current_dir(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_system_set_current_dir(path));
    }
    uint64_t jit_system_total_memory(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_system_total_memory());
    }
    uint64_t jit_system_free_memory(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_system_free_memory());
    }
    uint64_t jit_fs_exists(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_fs_exists(path));
    }
    uint64_t jit_fs_read_text(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* text = hoo_fs_read_text(path);
        if (!text) return 0;
        void* str = hoo_string_from_cstr(text);
        free(text);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_fs_is_file(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_fs_is_file(path));
    }
    uint64_t jit_fs_is_dir(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_fs_is_dir(path));
    }
    uint64_t jit_fs_size(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_fs_size(path));
    }
    uint64_t jit_fs_last_modified(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        int64_t ts = hoo_fs_last_modified(path);
        if (ts <= 0) return 0;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_string_from_int64(ts)));
    }
    uint64_t jit_fs_delete(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        hoo_fs_delete(path);
        return 0;
    }
    uint64_t jit_fs_rename(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* old_path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* new_path = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        hoo_fs_rename(old_path, new_path);
        return 0;
    }
    uint64_t jit_fs_copy(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* src = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* dst = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        hoo_fs_copy(src, dst);
        return 0;
    }
    uint64_t jit_fs_write_text(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* content = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        hoo_fs_write_text(path, content);
        return 0;
    }
    uint64_t jit_fs_append_text(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* content = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        hoo_fs_append_text(path, content);
        return 0;
    }
    uint64_t jit_fs_read_bytes_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_fs_read_bytes_buffer(path)));
    }
    uint64_t jit_fs_write_bytes_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[2]);
        hoo_fs_write_bytes_buffer(path, buf);
        return 0;
    }
    uint64_t jit_fs_mkdir(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        hoo_fs_mkdir(path);
        return 0;
    }
    uint64_t jit_fs_mkdirs(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        hoo_fs_mkdirs(path);
        return 0;
    }
    uint64_t jit_fs_rmdir(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        hoo_fs_rmdir(path);
        return 0;
    }
    uint64_t jit_fs_list_dir(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        int64_t count = 0;
        char** entries = hoo_fs_list_dir(path, &count);
        void* arr = hoo_array_new();
        for (int64_t i = 0; i < count; ++i) {
            void* str = hoo_string_from_cstr(entries[i]);
            hoo_retain(str);
            arr = hoo_array_push_object(arr, str);
            hoo_release(str);
        }
        if (arr && count > 0) {
            ((int64_t*)arr)[2] = HOO_TYPE_STRING;
        }
        hoo_fs_free_list(entries, count);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(arr));
    }
    uint64_t jit_fs_temp_dir(void* /*state_ptr*/) {
        char* dir = hoo_fs_temp_dir();
        if (!dir) return 0;
        void* str = hoo_string_from_cstr(dir);
        hoo_fs_free_string(dir);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_fs_create_temp_file(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* prefix = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* path = hoo_fs_create_temp_file(prefix);
        if (!path) return 0;
        void* str = hoo_string_from_cstr(path);
        hoo_fs_free_string(path);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    // Default-value variants for readText/readBytes (fallback when file missing)
    uint64_t jit_fs_read_text_default(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        void* fallback = reinterpret_cast<void*>(state->regs[2]);
        char* text = hoo_fs_read_text(path);
        if (text) {
            void* str = hoo_string_from_cstr(text);
            free(text);
            return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
        }
        hoo_retain(fallback);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(fallback));
    }
    uint64_t jit_fs_read_bytes_buffer_default(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        void* fallback = reinterpret_cast<void*>(state->regs[2]);
        HooBuffer buf = hoo_fs_read_bytes_buffer(path);
        if (buf) {
            return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(buf));
        }
        hoo_retain(fallback);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(fallback));
    }
    uint64_t jit_regex_compile(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* pattern = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_regex_compile(pattern)));
    }
    uint64_t jit_regex_match(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooRegex re = reinterpret_cast<HooRegex>(state->regs[1]);
        const char* text = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(hoo_regex_match(re, text));
    }
    uint64_t jit_uuid_v4(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_uuid_v4()));
    }
    uint64_t jit_uuid_to_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooUUID id = reinterpret_cast<HooUUID>(state->regs[1]);
        char* cstr = hoo_uuid_to_string(id);
        if (!cstr) return 0;
        void* str = hoo_string_from_cstr(cstr);
        hoo_uuid_free_string(cstr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    // ── Thread module ──────────────────────────────────────────────────
    uint64_t jit_thread_spawn(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        auto func = reinterpret_cast<int64_t (*)(void*)>(state->regs[1]);
        void* arg = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_thread_spawn(func, arg));
    }
    uint64_t jit_thread_join(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_thread_join(static_cast<int64_t>(state->regs[1])));
    }
    uint64_t jit_thread_self(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_thread_self());
    }
    uint64_t jit_thread_mutex_create(void* /*state_ptr*/) {
        return reinterpret_cast<uint64_t>(hoo_thread_mutex_create());
    }
    uint64_t jit_thread_mutex_lock(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_thread_mutex_lock(reinterpret_cast<HooMutex>(state->regs[1])));
    }
    uint64_t jit_thread_mutex_unlock(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_thread_mutex_unlock(reinterpret_cast<HooMutex>(state->regs[1])));
    }
    uint64_t jit_thread_mutex_destroy(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_thread_mutex_destroy(reinterpret_cast<HooMutex>(state->regs[1])));
    }

    uint64_t jit_encoding_base64_encode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        char* encoded = hoo_encoding_base64_encode(data, len);
        if (!encoded) return 0;
        void* str = hoo_string_from_cstr(encoded);
        hoo_encoding_free_string(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_base64_decode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* encoded = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        uint8_t* out = nullptr;
        int64_t len = hoo_encoding_base64_decode(encoded, &out);
        if (len < 0 || !out) return 0;
        void* str = hoo_string_from_cstr(reinterpret_cast<const char*>(out));
        hoo_encoding_free_bytes(out);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_hex_encode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        char* encoded = hoo_encoding_hex_encode(data, len);
        if (!encoded) return 0;
        void* str = hoo_string_from_cstr(encoded);
        hoo_encoding_free_string(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_hex_decode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* hex = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        uint8_t* out = nullptr;
        int64_t len = hoo_encoding_hex_decode(hex, &out);
        if (len < 0 || !out) return 0;
        void* str = hoo_string_from_cstr(reinterpret_cast<const char*>(out));
        hoo_encoding_free_bytes(out);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_url_encode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* raw = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* encoded = hoo_encoding_url_encode(raw);
        if (!encoded) return 0;
        void* str = hoo_string_from_cstr(encoded);
        hoo_encoding_free_string(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_url_decode(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* raw = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* decoded = hoo_encoding_url_decode(raw);
        if (!decoded) return 0;
        void* str = hoo_string_from_cstr(decoded);
        hoo_encoding_free_string(decoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_base64_encode_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        char* encoded = hoo_encoding_base64_encode_buffer(buf);
        if (!encoded) return 0;
        void* str = hoo_string_from_cstr(encoded);
        hoo_encoding_free_string(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_base64_decode_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* encoded = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        HooBuffer buf = hoo_encoding_base64_decode_buffer(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(buf));
    }
    uint64_t jit_encoding_hex_encode_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        char* encoded = hoo_encoding_hex_encode_buffer(buf);
        if (!encoded) return 0;
        void* str = hoo_string_from_cstr(encoded);
        hoo_encoding_free_string(encoded);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_encoding_hex_decode_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* hex = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        HooBuffer buf = hoo_encoding_hex_decode_buffer(hex);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(buf));
    }

    // ── CSV module (instance-based, self in regs[1]) ──────────────────────
    uint64_t jit_csv_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = hoo_csv_new();
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(handle));
    }
    uint64_t jit_hoo_csv_from_opts(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int32_t delimiter = static_cast<int32_t>(state->regs[1]);
        int32_t quote_char = static_cast<int32_t>(state->regs[2]);
        void* handle = hoo_csv_from_opts(delimiter, quote_char);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(handle));
    }
    uint64_t jit_csv_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        hoo_csv_release(handle);
        return 0;
    }
    uint64_t jit_csv_parse(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* csv = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        void* result = hoo_csv_parse(handle, csv);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_generate(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data_arr = reinterpret_cast<void*>(state->regs[2]);
        void* str = hoo_csv_generate(handle, data_arr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_csv_read_file(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        void* result = hoo_csv_read_file(handle, path);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_write_file(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        void* data_arr = reinterpret_cast<void*>(state->regs[3]);
        return static_cast<uint64_t>(hoo_csv_write_file(handle, path, data_arr));
    }
    uint64_t jit_csv_escape(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        int32_t c = static_cast<int32_t>(state->regs[2]);
        return static_cast<uint64_t>(hoo_csv_escape(handle, c));
    }
    uint64_t jit_csv_count(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(hoo_csv_count(handle, data, column));
    }
    uint64_t jit_csv_sum(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(hoo_csv_sum(handle, data, column));
    }
    uint64_t jit_csv_avg(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        void* result = hoo_csv_avg(handle, data, column);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_min(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        void* result = hoo_csv_min(handle, data, column);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_max(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        void* result = hoo_csv_max(handle, data, column);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_select(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        void* columns = reinterpret_cast<void*>(state->regs[3]);
        void* result = hoo_csv_select(handle, data, columns);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_filter(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        const char* op = hoo_string_data(reinterpret_cast<void*>(state->regs[4]));
        const char* value = hoo_string_data(reinterpret_cast<void*>(state->regs[5]));
        void* result = hoo_csv_filter(handle, data, column, op, value);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_sort(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        int64_t ascending = state->regs[4];
        void* result = hoo_csv_sort(handle, data, column, ascending);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }
    uint64_t jit_csv_describe(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        void* data = reinterpret_cast<void*>(state->regs[2]);
        const char* column = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        void* result = hoo_csv_describe(handle, data, column);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
    }

    // ── Datetime module ─────────────────────────────────────────────────────
    uint64_t jit_datetime_now(void* /*state_ptr*/) {
        return reinterpret_cast<uint64_t>(hoo_datetime_now());
    }
    uint64_t jit_datetime_now_seconds(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_datetime_now_seconds());
    }
    uint64_t jit_datetime_now_precise(void* /*state_ptr*/) {
        double result = hoo_datetime_now_precise();
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_datetime_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return reinterpret_cast<uint64_t>(hoo_datetime_new(state->regs[1]));
    }
    uint64_t jit_datetime_format(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        const char* fmt = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return reinterpret_cast<uint64_t>(hoo_datetime_format(dt, fmt));
    }
    uint64_t jit_datetime_iso8601(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_iso8601(dt));
    }
    uint64_t jit_datetime_parse(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* str = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* fmt = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return reinterpret_cast<uint64_t>(hoo_datetime_parse(str, fmt));
    }
    uint64_t jit_datetime_from_iso8601(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* str = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return reinterpret_cast<uint64_t>(hoo_datetime_from_iso8601(str));
    }
    uint64_t jit_datetime_add_days(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_add_days(dt, state->regs[2]));
    }
    uint64_t jit_datetime_add_hours(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_add_hours(dt, state->regs[2]));
    }
    uint64_t jit_datetime_add_minutes(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_add_minutes(dt, state->regs[2]));
    }
    uint64_t jit_datetime_add_seconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_add_seconds(dt, state->regs[2]));
    }
    uint64_t jit_datetime_add_milliseconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* dt = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_add_milliseconds(dt, state->regs[2]));
    }
    uint64_t jit_datetime_diff_days(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* from = reinterpret_cast<void*>(state->regs[1]);
        void* to = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_diff_days(from, to));
    }
    uint64_t jit_datetime_diff_hours(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* from = reinterpret_cast<void*>(state->regs[1]);
        void* to = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_diff_hours(from, to));
    }
    uint64_t jit_datetime_diff_seconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* from = reinterpret_cast<void*>(state->regs[1]);
        void* to = reinterpret_cast<void*>(state->regs[2]);
        double result = hoo_datetime_diff_seconds(from, to);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_datetime_compare(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* a = reinterpret_cast<void*>(state->regs[1]);
        void* b = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_instance_compare(a, b));
    }

    // ── DateTime instance method bridges ────────────────────────────────────
    uint64_t jit_datetime_inst_format(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        const char* fmt = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_format(self, fmt));
    }
    uint64_t jit_datetime_inst_iso8601(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_iso8601(self));
    }
    uint64_t jit_datetime_inst_getTimestamp(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(hoo_datetime_get_timestamp(self));
    }
    uint64_t jit_datetime_inst_addDays(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_add_days(self, state->regs[2]));
    }
    uint64_t jit_datetime_inst_addHours(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_add_hours(self, state->regs[2]));
    }
    uint64_t jit_datetime_inst_addMinutes(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_add_minutes(self, state->regs[2]));
    }
    uint64_t jit_datetime_inst_addSeconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_add_seconds(self, state->regs[2]));
    }
    uint64_t jit_datetime_inst_addMilliseconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        return reinterpret_cast<uint64_t>(hoo_datetime_instance_add_milliseconds(self, state->regs[2]));
    }
    uint64_t jit_datetime_inst_diffDays(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        void* other = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_instance_diff_days(self, other));
    }
    uint64_t jit_datetime_inst_diffHours(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        void* other = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_instance_diff_hours(self, other));
    }
    uint64_t jit_datetime_inst_diffSeconds(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        void* other = reinterpret_cast<void*>(state->regs[2]);
        double result = hoo_datetime_instance_diff_seconds(self, other);
        uint64_t bits; std::memcpy(&bits, &result, sizeof(double));
        return bits;
    }
    uint64_t jit_datetime_inst_compare(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* self = reinterpret_cast<void*>(state->regs[1]);
        void* other = reinterpret_cast<void*>(state->regs[2]);
        return static_cast<uint64_t>(hoo_datetime_instance_compare(self, other));
    }

    // ── Path module ─────────────────────────────────────────────────────────
    uint64_t jit_path_dirname(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_dirname(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_basename(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_basename(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_extension(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_extension(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_stem(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_stem(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_normalize(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_normalize(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_absolute(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_path_absolute(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_join(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* a = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* b = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        char* result = hoo_path_join(a, b);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_relative(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        const char* base = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        char* result = hoo_path_relative(path, base);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_path_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_path_is_absolute(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_path_is_absolute(path));
    }
    uint64_t jit_path_is_relative(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_path_is_relative(path));
    }
    uint64_t jit_path_has_extension(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(hoo_path_has_extension(path));
    }
    uint64_t jit_path_separator(void* /*state_ptr*/) {
        return static_cast<uint64_t>(static_cast<uint8_t>(hoo_path_separator()));
    }
    uint64_t jit_path_list_separator(void* /*state_ptr*/) {
        return static_cast<uint64_t>(static_cast<uint8_t>(hoo_path_list_separator()));
    }

    // ── Hashing module ──────────────────────────────────────────────────────
    uint64_t jit_hashing_sha256(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        char* result = hoo_hashing_sha256(data, len);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_sha1(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        char* result = hoo_hashing_sha1(data, len);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_md5(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        char* result = hoo_hashing_md5(data, len);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_sha256_file(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* path = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_hashing_sha256_file(path);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_crc32(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t len = state->regs[2];
        return static_cast<uint64_t>(hoo_hashing_crc32(data, len));
    }
    uint64_t jit_hashing_hmac_sha256(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* key = reinterpret_cast<const uint8_t*>(state->regs[1]);
        int64_t key_len = state->regs[2];
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[3]);
        int64_t data_len = state->regs[4];
        char* result = hoo_hashing_hmac_sha256(key, key_len, data, data_len);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_sha256_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        char* result = hoo_hashing_sha256_buffer(buf);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_sha1_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        char* result = hoo_hashing_sha1_buffer(buf);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_md5_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        char* result = hoo_hashing_md5_buffer(buf);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_hashing_crc32_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer buf = reinterpret_cast<HooBuffer>(state->regs[1]);
        return static_cast<uint64_t>(hoo_hashing_crc32_buffer(buf));
    }
    uint64_t jit_hashing_hmac_sha256_buffer(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooBuffer key = reinterpret_cast<HooBuffer>(state->regs[1]);
        HooBuffer data = reinterpret_cast<HooBuffer>(state->regs[2]);
        char* result = hoo_hashing_hmac_sha256_buffer(key, data);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_hashing_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }

    // ── Process module ──────────────────────────────────────────────────────
    uint64_t jit_process_kill(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_process_kill(state->regs[1], state->regs[2]));
    }
    uint64_t jit_process_self_pid(void* /*state_ptr*/) {
        return static_cast<uint64_t>(hoo_process_self_pid());
    }
    uint64_t jit_process_capture(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* cmd = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        char* result = hoo_process_capture(cmd);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        hoo_process_free_string(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }

    // ── Compression module (instance-based, self in regs[1]) ────────────────
    uint64_t jit_compression_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* comp = hoo_compression_new();
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(comp));
    }
    uint64_t jit_compression_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* comp = reinterpret_cast<void*>(state->regs[1]);
        hoo_compression_release(comp);
        return 0;
    }
    uint64_t jit_compression_gzip_compress(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[2]);
        int64_t data_len = state->regs[3];
        uint8_t* out_data = nullptr;
        int64_t out_len = 0;
        int64_t ok = hoo_compression_gzip_compress(data, data_len, &out_data, &out_len);
        if (ok != 0 || !out_data) return 0;
        void* str = hoo_string_from_bytes(reinterpret_cast<const char*>(out_data), out_len);
        hoo_compression_free_bytes(out_data);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_compression_gzip_decompress(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        uint8_t* out_data = nullptr;
        int64_t out_len = 0;
        void* self = reinterpret_cast<void*>(state->regs[1]);
        (void)self;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[2]);
        int64_t data_len = state->regs[3];
        int64_t ok = hoo_compression_gzip_decompress(data, data_len, &out_data, &out_len);
        if (ok != 0 || !out_data) return 0;
        void* str = hoo_string_from_bytes(reinterpret_cast<const char*>(out_data), out_len);
        hoo_compression_free_bytes(out_data);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_compression_deflate_compress(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        uint8_t* out_data = nullptr;
        int64_t out_len = 0;
        void* self = reinterpret_cast<void*>(state->regs[1]);
        (void)self;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[2]);
        int64_t data_len = state->regs[3];
        int64_t ok = hoo_compression_deflate_compress(data, data_len, &out_data, &out_len);
        if (ok != 0 || !out_data) return 0;
        void* str = hoo_string_from_bytes(reinterpret_cast<const char*>(out_data), out_len);
        hoo_compression_free_bytes(out_data);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_compression_deflate_decompress(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        uint8_t* out_data = nullptr;
        int64_t out_len = 0;
        void* self = reinterpret_cast<void*>(state->regs[1]);
        (void)self;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(state->regs[2]);
        int64_t data_len = state->regs[3];
        int64_t ok = hoo_compression_deflate_decompress(data, data_len, &out_data, &out_len);
        if (ok != 0 || !out_data) return 0;
        void* str = hoo_string_from_bytes(reinterpret_cast<const char*>(out_data), out_len);
        hoo_compression_free_bytes(out_data);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }

    // ── Args module ─────────────────────────────────────────────────────────
    uint64_t jit_args_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* args = hoo_args_new();
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(args));
    }
    uint64_t jit_args_count(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(hoo_args_count(handle));
    }
    uint64_t jit_args_get(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* val = hoo_args_get(handle, state->regs[2]);
        if (!val) return 0;
        void* str = hoo_string_from_cstr(val);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_args_has(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(hoo_args_has(handle, key));
    }
    uint64_t jit_args_value(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* val = hoo_args_value(handle, key);
        if (!val) return 0;
        void* str = hoo_string_from_cstr(val);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_args_program_name(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_args_program_name(handle);
        void* str = hoo_string_from_cstr(name);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }

    // ── Args argparse-style API ─────────────────────────────────────────────
    uint64_t jit_args_add_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* short_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        const char* long_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[5]));
        const char* help = hoo_string_data(reinterpret_cast<void*>(state->regs[6]));
        const char* default_val = hoo_string_data(reinterpret_cast<void*>(state->regs[7]));
        hoo_args_add_string(handle, name, short_opt, long_opt, help, default_val);
        return 0;
    }
    uint64_t jit_args_add_int(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* short_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        const char* long_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[5]));
        const char* help = hoo_string_data(reinterpret_cast<void*>(state->regs[6]));
        int64_t default_val = state->regs[7];
        hoo_args_add_int(handle, name, short_opt, long_opt, help, default_val);
        return 0;
    }
    uint64_t jit_args_add_flag(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* short_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        const char* long_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[5]));
        const char* help = hoo_string_data(reinterpret_cast<void*>(state->regs[6]));
        hoo_args_add_flag(handle, name, short_opt, long_opt, help);
        return 0;
    }
    uint64_t jit_args_add_float(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* short_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        const char* long_opt = hoo_string_data(reinterpret_cast<void*>(state->regs[5]));
        const char* help = hoo_string_data(reinterpret_cast<void*>(state->regs[6]));
        double default_val;
        std::memcpy(&default_val, &state->regs[7], sizeof(double));
        hoo_args_add_float(handle, name, short_opt, long_opt, help, default_val);
        return 0;
    }
    uint64_t jit_args_add_positional(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* help = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        hoo_args_add_positional(handle, name, help);
        return 0;
    }
    uint64_t jit_args_parse(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(hoo_args_parse(handle));
    }
    uint64_t jit_args_get_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* val = hoo_args_get_string(handle, name);
        void* str = hoo_string_from_cstr(val);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_args_get_int(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(hoo_args_get_int(handle, name));
    }
    uint64_t jit_args_get_bool(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(hoo_args_get_bool(handle, name));
    }
    uint64_t jit_args_get_float(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        const char* name = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        double val = hoo_args_get_float(handle, name);
        uint64_t bits;
        std::memcpy(&bits, &val, sizeof(double));
        return bits;
    }
    uint64_t jit_args_help_text(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        char* help = hoo_args_help_text(handle);
        if (!help) return 0;
        void* str = hoo_string_from_cstr(help);
        free(help);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_args_clear(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        void* handle = reinterpret_cast<void*>(state->regs[1]);
        hoo_args_clear(handle);
        return 0;
    }

    // ── Net module ──────────────────────────────────────────────────────────
    uint64_t jit_net_url_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* url_str = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_url_new(url_str)));
    }
    uint64_t jit_net_url_get_scheme(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_get_scheme(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_get_host(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_get_host(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_get_port(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        return static_cast<uint64_t>(hoo_net_url_get_port(url));
    }
    uint64_t jit_net_url_get_path(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_get_path(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_get_query(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_get_query(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_get_fragment(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_get_fragment(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_to_string(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooURL url = reinterpret_cast<HooURL>(state->regs[1]);
        char* result = hoo_net_url_to_string(url);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_url_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_net_url_release(reinterpret_cast<HooURL>(state->regs[1]));
        return 0;
    }
    uint64_t jit_net_http_client_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_http_client_new()));
    }
    uint64_t jit_net_http_client_set_header(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        const char* key = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* val = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(hoo_net_http_client_set_header(client, key, val));
    }
    uint64_t jit_net_http_client_set_timeout(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        hoo_net_http_client_set_timeout(client, state->regs[2]);
        return 0;
    }
    uint64_t jit_net_http_client_get(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        const char* url = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_http_client_get(client, url)));
    }
    uint64_t jit_net_http_client_post(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        const char* url = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* body = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_http_client_post(client, url, body)));
    }
    uint64_t jit_net_http_client_put(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        const char* url = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        const char* body = hoo_string_data(reinterpret_cast<void*>(state->regs[3]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_http_client_put(client, url, body)));
    }
    uint64_t jit_net_http_client_delete(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpClient client = reinterpret_cast<HooHttpClient>(state->regs[1]);
        const char* url = hoo_string_data(reinterpret_cast<void*>(state->regs[2]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_net_http_client_delete(client, url)));
    }
    uint64_t jit_net_http_response_get_status_code(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_net_http_response_get_status_code(reinterpret_cast<HooHttpResponse>(state->regs[1])));
    }
    uint64_t jit_net_http_response_get_body(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpResponse resp = reinterpret_cast<HooHttpResponse>(state->regs[1]);
        char* result = hoo_net_http_response_get_body(resp);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_http_response_get_status_text(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        HooHttpResponse resp = reinterpret_cast<HooHttpResponse>(state->regs[1]);
        char* result = hoo_net_http_response_get_status_text(resp);
        if (!result) return 0;
        void* str = hoo_string_from_cstr(result);
        std::free(result);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(str));
    }
    uint64_t jit_net_http_response_is_success(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_net_http_response_is_success(reinterpret_cast<HooHttpResponse>(state->regs[1])));
    }
    uint64_t jit_net_http_response_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_net_http_response_release(reinterpret_cast<HooHttpResponse>(state->regs[1]));
        return 0;
    }
    uint64_t jit_net_http_client_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_net_http_client_release(reinterpret_cast<HooHttpClient>(state->regs[1]));
        return 0;
    }

    // ── JSON free functions ─────────────────────────────────────────────────
    uint64_t jit_json_serialize_hashmap(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        auto* map = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_serialize_hashmap(map)));
    }
    uint64_t jit_json_serialize_anyarray(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        auto* array = reinterpret_cast<void*>(state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_serialize_anyarray(array)));
    }
    uint64_t jit_json_deserialize_hashmap(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* json = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_deserialize_hashmap(json)));
    }
    uint64_t jit_json_deserialize_anyarray(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* json = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_deserialize_anyarray(json)));
    }
    uint64_t jit_json_minify(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* json = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_minify(json)));
    }
    uint64_t jit_json_beautify(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* json = hoo_string_data(reinterpret_cast<void*>(state->regs[1]));
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_json_beautify(json)));
    }

    // Base pointer for translating HVM memory offsets to real addresses.
    // The HVM uses offset-addressed memory (std::vector<uint8_t> memory_),
    // but POSIX syscalls need real virtual addresses.  This global lets the
    // extern "C" helpers below compute real addresses at runtime.
    static uint8_t* g_hvm_memory = nullptr;
    void hvm_set_memory_base(uint8_t* base) { g_hvm_memory = base; }
    uint8_t* hvm_get_memory_base() { return g_hvm_memory; }

    // HVM internal sys calls (for interpreter)
    extern "C" uint64_t hooc_hvm_sys_alloc(uint64_t size, uint64_t typeId) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_alloc(static_cast<size_t>(size), static_cast<int64_t>(typeId))));
    }
    extern "C" uint64_t hooc_hvm_sys_retain(uint64_t obj) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_retain(reinterpret_cast<void*>(obj))));
    }
    extern "C" uint64_t hooc_hvm_sys_release(uint64_t obj) {
        hoo_release(reinterpret_cast<void*>(obj));
        return 0;
    }
    extern "C" uint64_t hooc_hvm_sys_refcount(uint64_t obj) {
        return static_cast<uint64_t>(hoo_get_refcount(reinterpret_cast<void*>(obj)));
    }
    extern "C" uint64_t hooc_hvm_sys_typeid(uint64_t obj) {
        return static_cast<uint64_t>(hoo_get_type_id(reinterpret_cast<void*>(obj)));
    }
    extern "C" uint64_t hooc_hvm_sys_exception_runtime(uint64_t /*reserved*/) {
        HooException exc = hoo_exception_runtime("hvm runtime exception");
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(exc));
    }
    extern "C" uint64_t hooc_hvm_sys_push_handler_state(void* state_ptr, uint64_t handler_pc) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return shadow_push_handler(state, handler_pc);
    }
    extern "C" uint64_t hooc_hvm_sys_pop_handler_state(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return shadow_pop_handler(state);
    }
    extern "C" uint64_t hooc_hvm_sys_throw_to_handler_state(void* state_ptr, uint64_t exc) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
#ifdef _WIN32
        hoo_exception_set_current(reinterpret_cast<HooException>(exc));
        return shadow_throw_to_handler(state, exc, false);
#else
        try {
            hoo_exception_throw(reinterpret_cast<HooException>(exc));
        } catch (...) {
        }
        return shadow_throw_to_handler(state, exc, false);
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_rethrow_to_handler_state(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
#ifdef _WIN32
        return shadow_throw_to_handler(state, 0, true);
#else
        try {
            hoo_exception_rethrow();
        } catch (...) {
        }
        return shadow_throw_to_handler(state, 0, true);
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_string_data(uint64_t strObj) {
        return static_cast<uint64_t>(
            reinterpret_cast<uintptr_t>(hoo_string_data(reinterpret_cast<void*>(strObj))));
    }
    extern "C" HVM_RUNTIME_EXPORT uint64_t hooc_hvm_sys_should_stop_state(void* state_ptr) {
        return shadow_should_stop_state(state_ptr);
    }

    // ── Platform OS services (syscalls 12–23) ───────────────────────
    extern "C" uint64_t hooc_hvm_sys_thread_create(uint64_t entry, uint64_t arg) {
#ifdef _WIN32
        auto fn = reinterpret_cast<int64_t (*)(uint64_t)>(entry);
        std::thread t([fn, arg]() { fn(arg); });
        t.detach();
        return 0;
#else
        pthread_t thread;
        auto* p = new std::pair<uint64_t, uint64_t>(entry, arg);
        auto* wrapper = +[](void* raw) -> void* {
            auto* tp = static_cast<std::pair<uint64_t, uint64_t>*>(raw);
            auto fn = reinterpret_cast<int64_t (*)(uint64_t)>(tp->first);
            fn(tp->second);
            delete tp;
            return nullptr;
        };
        if (pthread_create(&thread, nullptr, wrapper, p) != 0) {
            delete p;
            return -1;
        }
        pthread_detach(thread);
        return 0;
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_thread_exit(uint64_t retval) {
        (void)retval;
#ifdef _WIN32
        _endthread();
#else
        pthread_exit(nullptr);
#endif
        return 0;
    }
    extern "C" uint64_t hooc_hvm_sys_futex(uint64_t uaddr, uint64_t op, uint64_t val) {
        (void)uaddr; (void)op; (void)val;
        return -1;
    }
    extern "C" uint64_t hooc_hvm_sys_get_tid() {
#ifdef _WIN32
        return static_cast<uint64_t>(GetCurrentThreadId());
#elif defined(__APPLE__)
        uint64_t tid = 0;
        pthread_threadid_np(pthread_self(), &tid);
        return tid;
#else
        return static_cast<uint64_t>(pthread_self());
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_open(uint64_t path, uint64_t flags, uint64_t mode) {
        const char* realPath = g_hvm_memory
            ? reinterpret_cast<const char*>(g_hvm_memory + path)
            : reinterpret_cast<const char*>(path);
#ifdef _WIN32
        int osflags = _O_BINARY;
        if ((flags & 3) == 0) osflags |= _O_RDONLY;
        else if ((flags & 3) == 1) osflags |= _O_WRONLY;
        else if ((flags & 3) == 2) osflags |= _O_RDWR;
        if (flags & 0x0008) osflags |= _O_APPEND;
        // Handle both Linux-style (0x40/0x80) and macOS-style (0x200/0x400/0x800) flags.
        // 0x200 is O_CREAT on macOS but O_TRUNC on Linux — resolve by checking Linux O_CREAT (0x40).
        bool hasLinuxCreate = (flags & 0x0040) != 0;
        if (hasLinuxCreate)      osflags |= _O_CREAT;
        if (flags & 0x0080)      osflags |= _O_EXCL;
        if (flags & 0x0200)      osflags |= hasLinuxCreate ? _O_TRUNC : _O_CREAT;
        if (flags & 0x0400)      osflags |= _O_TRUNC;
        if (flags & 0x0800)      osflags |= _O_EXCL;
        return static_cast<uint64_t>(_open(realPath, osflags, static_cast<int>(mode)));
#else
        return static_cast<uint64_t>(::open(realPath,
                                            static_cast<int>(flags),
                                            static_cast<mode_t>(mode)));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_read(uint64_t fd, uint64_t buf, uint64_t count) {
        void* realBuf = g_hvm_memory
            ? g_hvm_memory + buf
            : reinterpret_cast<void*>(buf);
#ifdef _WIN32
        return static_cast<uint64_t>(_read(static_cast<int>(fd),
                                           realBuf,
                                           static_cast<unsigned int>(count)));
#else
        return static_cast<uint64_t>(::read(static_cast<int>(fd),
                                            realBuf,
                                            static_cast<size_t>(count)));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
        const void* realBuf = g_hvm_memory
            ? g_hvm_memory + buf
            : reinterpret_cast<const void*>(buf);
#ifdef _WIN32
        return static_cast<uint64_t>(_write(static_cast<int>(fd),
                                            realBuf,
                                            static_cast<unsigned int>(count)));
#else
        return static_cast<uint64_t>(::write(static_cast<int>(fd),
                                             realBuf,
                                             static_cast<size_t>(count)));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_close(uint64_t fd) {
#ifdef _WIN32
        return static_cast<uint64_t>(_close(static_cast<int>(fd)));
#else
        return static_cast<uint64_t>(::close(static_cast<int>(fd)));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence) {
#ifdef _WIN32
        return static_cast<uint64_t>(_lseek(static_cast<int>(fd),
                                            static_cast<long>(offset),
                                            static_cast<int>(whence)));
#else
        return static_cast<uint64_t>(::lseek(static_cast<int>(fd),
                                             static_cast<off_t>(offset),
                                             static_cast<int>(whence)));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_fstat(uint64_t fd, uint64_t buf) {
#ifdef _WIN32
        struct _stat* realBuf = g_hvm_memory
            ? reinterpret_cast<struct _stat*>(g_hvm_memory + buf)
            : reinterpret_cast<struct _stat*>(buf);
        return static_cast<uint64_t>(_fstat(static_cast<int>(fd), realBuf));
#else
        struct stat* realBuf = g_hvm_memory
            ? reinterpret_cast<struct stat*>(g_hvm_memory + buf)
            : reinterpret_cast<struct stat*>(buf);
        return static_cast<uint64_t>(::fstat(static_cast<int>(fd), realBuf));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_clock_gettime(uint64_t clk_id, uint64_t ts_ptr) {
        struct timespec* realTs = g_hvm_memory
            ? reinterpret_cast<struct timespec*>(g_hvm_memory + ts_ptr)
            : reinterpret_cast<struct timespec*>(ts_ptr);
#ifdef _WIN32
        (void)clk_id;
        FILETIME ft;
        GetSystemTimePreciseAsFileTime(&ft);
        ULARGE_INTEGER ui;
        ui.LowPart = ft.dwLowDateTime;
        ui.HighPart = ft.dwHighDateTime;
        constexpr uint64_t EPOCH_DIFF = 116444736000000000ULL;
        uint64_t ns100 = ui.QuadPart - EPOCH_DIFF;
        realTs->tv_sec = static_cast<time_t>(ns100 / 10000000ULL);
        realTs->tv_nsec = static_cast<long>((ns100 % 10000000ULL) * 100);
        return 0;
#else
        return static_cast<uint64_t>(::clock_gettime(static_cast<clockid_t>(clk_id), realTs));
#endif
    }
    extern "C" uint64_t hooc_hvm_sys_getrandom(uint64_t buf, uint64_t len) {
        void* realBuf = g_hvm_memory
            ? g_hvm_memory + buf
            : reinterpret_cast<void*>(buf);
#ifdef _WIN32
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CryptGenRandom(hProv, (DWORD)len, (BYTE*)realBuf);
            CryptReleaseContext(hProv, 0);
            return len;
        }
        return 0;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        ::arc4random_buf(realBuf, static_cast<size_t>(len));
        return len;
#else
        return static_cast<uint64_t>(::getrandom(realBuf, static_cast<size_t>(len), 0));
#endif
    }

    HVM_RUNTIME_EXPORT void hooc_hvm_arc_retain_if_managed(uint64_t obj) {
        void* ptr = reinterpret_cast<void*>(obj);
        if (hoo_is_managed_object(ptr)) {
            hoo_retain(ptr);
        }
    }
    HVM_RUNTIME_EXPORT void hooc_hvm_arc_release_if_managed(uint64_t obj) {
        void* ptr = reinterpret_cast<void*>(obj);
        if (hoo_is_managed_object(ptr)) {
            hoo_release(ptr);
        }
    }
}

using InboundTrampolineFn = uint64_t(*)(uint64_t);
constexpr InboundTrampolineFn kInboundTrampolines[kMaxInboundTrampolineSlots] = {
    &hooc_hvm_inbound_trampoline_0,
    &hooc_hvm_inbound_trampoline_1,
    &hooc_hvm_inbound_trampoline_2,
    &hooc_hvm_inbound_trampoline_3,
    &hooc_hvm_inbound_trampoline_4,
    &hooc_hvm_inbound_trampoline_5,
    &hooc_hvm_inbound_trampoline_6,
    &hooc_hvm_inbound_trampoline_7,
};
using InboundTrampolineFn2 = uint64_t(*)(uint64_t, uint64_t);
constexpr InboundTrampolineFn2 kInboundTrampolines2[kMaxInboundTrampolineSlots] = {
    &hooc_hvm_inbound_trampoline2_0,
    &hooc_hvm_inbound_trampoline2_1,
    &hooc_hvm_inbound_trampoline2_2,
    &hooc_hvm_inbound_trampoline2_3,
    &hooc_hvm_inbound_trampoline2_4,
    &hooc_hvm_inbound_trampoline2_5,
    &hooc_hvm_inbound_trampoline2_6,
    &hooc_hvm_inbound_trampoline2_7,
};

std::vector<RuntimeSymbolContract> buildRuntimeSymbols() {
    return {
        {"_F_hoo_alloc_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_alloc)},
        {"_F_hoo_retain_p_p", reinterpret_cast<void*>(&jit_hoo_retain)},
        {"_F_hoo_release_v_p", reinterpret_cast<void*>(&jit_hoo_release)},
        {"_F_hoo_get_refcount_i8_p", reinterpret_cast<void*>(&jit_hoo_get_refcount)},
        {"_F_hoo_get_type_id_i8_p", reinterpret_cast<void*>(&jit_hoo_get_type_id)},
        {"_F_hoo_String_from_cstr_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_hoo_String_from_int64_p_i8", reinterpret_cast<void*>(&jit_hoo_string_from_int64)},
        {"_F_hoo_String_from_double_p_d", reinterpret_cast<void*>(&jit_hoo_string_from_double)},
        {"_F_hoo_String_concat_p_p_p", reinterpret_cast<void*>(&jit_hoo_string_concat)},
        {"_F_hoo_String_length_i8_p", reinterpret_cast<void*>(&jit_hoo_string_length)},
        {"_F_hoo_String_to_upper_p_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"_F_hoo_String_data_p_p", reinterpret_cast<void*>(&jit_hoo_string_data)},
        {"_F_hoo_String_to_characters_p_p", reinterpret_cast<void*>(&jit_hoo_string_to_characters)},
        {"_F_hoo_String_join_p_p", reinterpret_cast<void*>(&jit_hoo_string_join)},
        {"_F_hoo_String_from_object_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_object)},
        {"_F_hoo_String_from_any_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_string_from_any)},
        // String class methods (hoo module, String class)
        {"_F_M_hoo_E_String_new_static_p", reinterpret_cast<void*>(&jit_string_new)},
        {"_F_M_hoo_E_String_fromCStr_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_M_hoo_E_String_fromInt64_static_p_i8", reinterpret_cast<void*>(&jit_hoo_string_from_int64)},
        {"_F_M_hoo_E_String_fromDouble_static_p_d", reinterpret_cast<void*>(&jit_hoo_string_from_double)},
        {"_F_M_hoo_E_String_fromAny_static_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_string_from_any)},
        {"_F_M_hoo_E_String_fromObject_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_object)},
        {"_F_M_hoo_E_String_join_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_join)},
        {"_F_M_hoo_E_String_repeat_static_p_p_p", reinterpret_cast<void*>(&jit_string_repeat)},
        {"_F_M_hoo_E_String_concat_p_p", reinterpret_cast<void*>(&jit_hoo_string_concat)},
        {"_F_M_hoo_E_String_length_i8", reinterpret_cast<void*>(&jit_hoo_string_length)},
        {"_F_M_hoo_E_String_data_p", reinterpret_cast<void*>(&jit_hoo_string_data)},
        {"_F_M_hoo_E_String_isEmpty_i8", reinterpret_cast<void*>(&jit_string_is_empty)},
        {"_F_M_hoo_E_String_toUpper_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"_F_M_hoo_E_String_toLower_p", reinterpret_cast<void*>(&jit_string_to_lower)},
        {"_F_M_hoo_E_String_equals_i8_p", reinterpret_cast<void*>(&jit_string_equals)},
        {"_F_M_hoo_E_String_contains_i8_p", reinterpret_cast<void*>(&jit_string_contains)},
        {"_F_M_hoo_E_String_startsWith_i8_p", reinterpret_cast<void*>(&jit_string_starts_with)},
        {"_F_M_hoo_E_String_trim_p", reinterpret_cast<void*>(&jit_string_trim)},
        {"_F_M_hoo_E_String_indexOf_i8_p", reinterpret_cast<void*>(&jit_string_index_of)},
        {"_F_M_hoo_E_String_toCharacters_p", reinterpret_cast<void*>(&jit_hoo_string_to_characters)},
        // String class method redirect names (snake_case, from Hooc source string_*())
        {"_F_M_hoo_E_String_from_cstr_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_M_hoo_E_String_from_int64_static_p_i8", reinterpret_cast<void*>(&jit_hoo_string_from_int64)},
        {"_F_M_hoo_E_String_from_double_static_p_d", reinterpret_cast<void*>(&jit_hoo_string_from_double)},
        {"_F_M_hoo_E_String_from_any_static_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_string_from_any)},
        {"_F_M_hoo_E_String_from_object_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_object)},
        {"_F_M_hoo_E_String_is_empty_i8", reinterpret_cast<void*>(&jit_string_is_empty)},
        {"_F_M_hoo_E_String_to_upper_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"_F_M_hoo_E_String_to_lower_p", reinterpret_cast<void*>(&jit_string_to_lower)},
        {"_F_M_hoo_E_String_starts_with_i8_p", reinterpret_cast<void*>(&jit_string_starts_with)},
        {"_F_M_hoo_E_String_index_of_i8_p", reinterpret_cast<void*>(&jit_string_index_of)},
        {"_F_M_hoo_E_String_to_characters_p", reinterpret_cast<void*>(&jit_hoo_string_to_characters)},
        // CamelCase aliases
        {"_F_M_hoo_E_String_fromCstr_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_M_hoo_E_String_fromInt64_static_p_i8", reinterpret_cast<void*>(&jit_hoo_string_from_int64)},
        {"_F_M_hoo_E_String_fromDouble_static_p_d", reinterpret_cast<void*>(&jit_hoo_string_from_double)},
        {"_F_M_hoo_E_String_fromAny_static_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_string_from_any)},
        {"_F_M_hoo_E_String_fromObject_static_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_object)},
        {"_F_M_hoo_E_String_isEmpty_i8", reinterpret_cast<void*>(&jit_string_is_empty)},
        {"_F_M_hoo_E_String_toUpper_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"_F_M_hoo_E_String_toLower_p", reinterpret_cast<void*>(&jit_string_to_lower)},
        {"_F_M_hoo_E_String_startsWith_i8_p", reinterpret_cast<void*>(&jit_string_starts_with)},
        {"_F_M_hoo_E_String_indexOf_i8_p", reinterpret_cast<void*>(&jit_string_index_of)},
        {"_F_M_hoo_E_String_toCharacters_p", reinterpret_cast<void*>(&jit_hoo_string_to_characters)},
        {"_F_hoo_Character_from_utf8_p_p_i8", reinterpret_cast<void*>(&jit_hoo_character_from_utf8)},
        {"_F_hoo_Character_from_codepoint_p_i8", reinterpret_cast<void*>(&jit_hoo_character_from_codepoint)},
        {"_F_hoo_Character_length_i8_p", reinterpret_cast<void*>(&jit_hoo_character_length)},
        {"_F_hoo_Character_data_p_p", reinterpret_cast<void*>(&jit_hoo_character_data)},
        {"_F_hoo_Character_codepoint_i8_p", reinterpret_cast<void*>(&jit_hoo_character_codepoint)},
        // Character hoo-module-qualified symbols (codegen prefix-based dispatch)
        // Character instance-method symbols (no static methods — all instance calls)
        {"_F_M_hoo_E_character_new_v_p", reinterpret_cast<void*>(&jit_hoo_character_from_codepoint)},
        {"_F_M_hoo_E_character_fromUtf8_v_p", reinterpret_cast<void*>(&jit_hoo_character_from_utf8_string)},
        {"_F_M_hoo_E_character_codepoint_v", reinterpret_cast<void*>(&jit_hoo_character_codepoint)},
        {"_F_M_hoo_E_character_length_v", reinterpret_cast<void*>(&jit_hoo_character_length)},
        {"_F_M_hoo_E_character_data_v", reinterpret_cast<void*>(&jit_hoo_character_data)},
        {"_F_M_hoo_E_character_print_v", reinterpret_cast<void*>(&jit_hoo_character_print)},
        {"_F_M_hoo_E_character_release_v", reinterpret_cast<void*>(&jit_hoo_character_release)},
        {"_F_M_hoo_E_print_v_p", reinterpret_cast<void*>(&jit_hoo_print)},
        {"_F_M_hoo_E_println_v_p", reinterpret_cast<void*>(&jit_hoo_println)},
        {"_F_hoo_Array_new_p", reinterpret_cast<void*>(&jit_hoo_array_new)},
        {"_F_hoo_Array_pushInt64_p_i8", reinterpret_cast<void*>(&jit_array_push_int64_plain)},
        {"_F_hoo_Array_pushString_p_p", reinterpret_cast<void*>(&jit_array_push_string)},
        {"_F_hoo_Array_pushBool_p_i8", reinterpret_cast<void*>(&jit_array_push_bool)},
        {"_F_hoo_Array_pushDouble_p_d", reinterpret_cast<void*>(&jit_array_push_double)},
        {"_F_hoo_Array_pushArray_p_p", reinterpret_cast<void*>(&jit_array_push_array)},
        {"_F_hoo_Array_pushObject_p_p", reinterpret_cast<void*>(&jit_array_push_object)},
        {"_F_hoo_Array_push_i8_p_i8", reinterpret_cast<void*>(&jit_hoo_array_push_int64)},
        {"_F_hoo_Array_get_i8_p_i8_p", reinterpret_cast<void*>(&jit_hoo_array_get_int64)},
        {"_F_hoo_Map_new_p_i8", reinterpret_cast<void*>(&jit_hoo_map_new)},
        {"_F_hoo_exception_runtime_p", reinterpret_cast<void*>(&jit_hoo_exception_runtime)},
        {"_F_hoo_exception_clear_v", reinterpret_cast<void*>(&jit_hoo_exception_clear)},
        {"_F_hoo_push_handler_v_p", reinterpret_cast<void*>(&jit_hoo_push_handler)},
        {"_F_hoo_pop_handler_v", reinterpret_cast<void*>(&jit_hoo_pop_handler)},
        {"_F_hoo_throw_v_p", reinterpret_cast<void*>(&jit_hoo_throw)},
        {"_F_hoo_rethrow_v", reinterpret_cast<void*>(&jit_hoo_rethrow)},
        // String aliases (codegen-generated plain names)
        {"_F_string_new_v", reinterpret_cast<void*>(&jit_string_new)},
        {"_F_M_hoo_E_string_new_v", reinterpret_cast<void*>(&jit_string_new)},
        {"_F_string_length_v_p", reinterpret_cast<void*>(&jit_hoo_string_length)},
        {"_F_string_is_empty_v_p", reinterpret_cast<void*>(&jit_string_is_empty)},
        {"_F_string_concat_v_p_p", reinterpret_cast<void*>(&jit_hoo_string_concat)},
        {"_F_string_data_v_p", reinterpret_cast<void*>(&jit_hoo_string_data)},
        {"_F_string_from_cstr_v_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_string_to_lower_v_p", reinterpret_cast<void*>(&jit_string_to_lower)},
        {"_F_string_to_upper_v_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"_F_string_equals_v_p_p", reinterpret_cast<void*>(&jit_string_equals)},
        {"_F_string_contains_v_p_p", reinterpret_cast<void*>(&jit_string_contains)},
        {"_F_string_starts_with_v_p_p", reinterpret_cast<void*>(&jit_string_starts_with)},
        {"_F_string_trim_v_p", reinterpret_cast<void*>(&jit_string_trim)},
        {"_F_string_repeat_v_p_p", reinterpret_cast<void*>(&jit_string_repeat)},
        {"_F_string_index_of_v_p_p", reinterpret_cast<void*>(&jit_string_index_of)},
        // Array aliases (codegen-generated plain names)
        {"_F_array_new_v", reinterpret_cast<void*>(&jit_hoo_array_new)},
        {"_F_array_push_double_v_p_p", reinterpret_cast<void*>(&jit_array_push_double)},
        {"_F_array_get_double_v_p_p", reinterpret_cast<void*>(&jit_array_get_double)},
        {"_F_array_push_int64_v_p_p", reinterpret_cast<void*>(&jit_array_push_int64_plain)},
        {"_F_array_get_int64_v_p_p", reinterpret_cast<void*>(&jit_array_get_int64_plain)},
        {"_F_array_length_v_p", reinterpret_cast<void*>(&jit_array_length)},
        {"_F_array_clear_v_p", reinterpret_cast<void*>(&jit_array_clear)},
        {"_F_array_empty_v_p", reinterpret_cast<void*>(&jit_array_empty)},
        {"_F_array_push_string_v_p_p", reinterpret_cast<void*>(&jit_array_push_string)},
        {"_F_array_get_string_v_p_p", reinterpret_cast<void*>(&jit_array_get_string)},
        {"_F_array_push_bool_v_p_p", reinterpret_cast<void*>(&jit_array_push_bool)},
        {"_F_array_get_bool_v_p_p", reinterpret_cast<void*>(&jit_array_get_bool)},
        {"_F_array_set_v_p_i8_p", reinterpret_cast<void*>(&jit_array_set_int64)},
        {"_F_hoo_Tensor_new1_p_i8_i8", reinterpret_cast<void*>(&jit_tensor_new1)},
        {"_F_hoo_Tensor_new2_p_i8_i8_i8", reinterpret_cast<void*>(&jit_tensor_new2)},
        {"_F_hoo_Tensor_new3_p_i8_i8_i8_i8", reinterpret_cast<void*>(&jit_tensor_new3)},
        {"_F_hoo_Tensor_pushValue_i8_p_i8", reinterpret_cast<void*>(&jit_tensor_push_value)},
        {"_F_hoo_Tensor_length_i8_p", reinterpret_cast<void*>(&jit_tensor_length)},
        {"_F_hoo_Tensor_dim_i8_p_i8", reinterpret_cast<void*>(&jit_tensor_dim)},
        {"_F_hoo_Tensor_getInt64_i8_p_i8", reinterpret_cast<void*>(&jit_tensor_get_int64)},
        {"_F_hoo_Tensor_getDouble_d_p_i8", reinterpret_cast<void*>(&jit_tensor_get_double)},
        {"_F_hoo_Tensor_add_p_p_p", reinterpret_cast<void*>(&jit_tensor_add)},
        {"_F_hoo_Tensor_sub_p_p_p", reinterpret_cast<void*>(&jit_tensor_sub)},
        {"_F_hoo_Tensor_elementMul_p_p_p", reinterpret_cast<void*>(&jit_tensor_element_mul)},
        {"_F_hoo_Tensor_elementDiv_p_p_p", reinterpret_cast<void*>(&jit_tensor_element_div)},
        {"_F_hoo_Tensor_matmul_p_p_p", reinterpret_cast<void*>(&jit_tensor_matmul)},
        {"_F_hoo_Tensor_eq_p_p_p", reinterpret_cast<void*>(&jit_tensor_eq)},
        {"_F_hoo_Tensor_ne_p_p_p", reinterpret_cast<void*>(&jit_tensor_ne)},
        {"_F_hoo_Tensor_lt_p_p_p", reinterpret_cast<void*>(&jit_tensor_lt)},
        {"_F_hoo_Tensor_le_p_p_p", reinterpret_cast<void*>(&jit_tensor_le)},
        {"_F_hoo_Tensor_gt_p_p_p", reinterpret_cast<void*>(&jit_tensor_gt)},
        {"_F_hoo_Tensor_ge_p_p_p", reinterpret_cast<void*>(&jit_tensor_ge)},
        {"_F_hoo_Tensor_and_p_p_p", reinterpret_cast<void*>(&jit_tensor_and)},
        {"_F_hoo_Tensor_or_p_p_p", reinterpret_cast<void*>(&jit_tensor_or)},
        {"_F_hoo_Tensor_not_p_p", reinterpret_cast<void*>(&jit_tensor_not)},
        // Object field access helpers
        {"_F_object_get_field_p_i8", reinterpret_cast<void*>(&jit_object_get_field)},
        {"_F_object_set_field_v_p_i8_p", reinterpret_cast<void*>(&jit_object_set_field)},
        // Map aliases (codegen-generated plain names)
        {"_F_map_new_v_p", reinterpret_cast<void*>(&jit_map_new_plain)},
        {"_F_map_set_int64_int64_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_int64_int64)},
        {"_F_map_get_int64_int64_v_p_p", reinterpret_cast<void*>(&jit_map_get_int64_int64)},
        {"_F_map_set_int64_double_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_int64_double)},
        {"_F_map_get_int64_double_v_p_p", reinterpret_cast<void*>(&jit_map_get_int64_double)},
        {"_F_map_set_int64_string_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_int64_string)},
        {"_F_map_get_int64_string_v_p_p", reinterpret_cast<void*>(&jit_map_get_int64_string)},
        {"_F_map_set_int64_bool_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_int64_bool)},
        {"_F_map_get_int64_bool_v_p_p", reinterpret_cast<void*>(&jit_map_get_int64_bool)},
        {"_F_map_set_string_int64_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_string_int64)},
        {"_F_map_set_string_double_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_string_double)},
        {"_F_map_get_string_double_v_p_p", reinterpret_cast<void*>(&jit_map_get_string_double)},
        {"_F_map_set_string_string_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_string_string)},
        {"_F_map_get_string_string_v_p_p", reinterpret_cast<void*>(&jit_map_get_string_string)},
        {"_F_map_set_string_bool_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_string_bool)},
        {"_F_map_get_string_bool_v_p_p", reinterpret_cast<void*>(&jit_map_get_string_bool)},
        {"_F_map_set_int8_int64_v_p_p_p", reinterpret_cast<void*>(&jit_map_set_int8_int64)},
        {"_F_map_get_int8_int64_v_p_p", reinterpret_cast<void*>(&jit_map_get_int8_int64)},
        {"_F_map_contains_int64_v_p_p", reinterpret_cast<void*>(&jit_map_contains_int64)},
        {"_F_map_remove_int64_v_p_p", reinterpret_cast<void*>(&jit_map_remove_int64)},
        {"_F_map_clear_v_p", reinterpret_cast<void*>(&jit_map_clear)},
        {"_F_map_empty_v_p", reinterpret_cast<void*>(&jit_map_empty)},
        {"_F_map_key_type_v_p", reinterpret_cast<void*>(&jit_map_key_type)},
        {"_F_map_value_type_v_p", reinterpret_cast<void*>(&jit_map_value_type)},
        // ISSUE-033 any/AnyArray/HashMap intrinsic symbols
        {"_F_hoo_anyarray_new_p", reinterpret_cast<void*>(&jit_anyarray_new)},
        {"_F_hoo_anyarray_new_capacity_p_i8", reinterpret_cast<void*>(&jit_anyarray_new_capacity)},
        {"_F_hoo_anyarray_length_i8_p", reinterpret_cast<void*>(&jit_anyarray_length)},
        {"_F_hoo_anyarray_push_i8_p_i8_i8", reinterpret_cast<void*>(&jit_anyarray_push)},
        {"_F_hoo_anyarray_set_i8_p_i8_i8_i8", reinterpret_cast<void*>(&jit_anyarray_set)},
        {"_F_hoo_anyarray_get_data_i8_p_i8", reinterpret_cast<void*>(&jit_anyarray_get_data)},
        {"_F_hoo_anyarray_pop_data_i8_p", reinterpret_cast<void*>(&jit_anyarray_pop_data)},
        {"_F_hoo_anyarray_clear_v_p", reinterpret_cast<void*>(&jit_anyarray_clear)},
        {"_F_M_hoo_E_anyarray_new_v", reinterpret_cast<void*>(&jit_anyarray_new)},
        {"_F_M_hoo_E_anyarray_new_v_p", reinterpret_cast<void*>(&jit_anyarray_new_capacity)},
        {"_F_M_hoo_E_anyarray_release_v", reinterpret_cast<void*>(&jit_anyarray_release)},
        {"_F_hoo_hashmap_new_p_i8_i8", reinterpret_cast<void*>(&jit_hashmap_new)},
        {"_F_hoo_hashmap_count_i8_p", reinterpret_cast<void*>(&jit_hashmap_count)},
        {"_F_hoo_hashmap_set_fixed_i8_p_i8_i8", reinterpret_cast<void*>(&jit_hashmap_set_fixed)},
        {"_F_hoo_hashmap_get_fixed_data_i8_p_i8", reinterpret_cast<void*>(&jit_hashmap_get_fixed_data)},
        {"_F_hoo_hashmap_set_any_i8_p_i8_i8_i8", reinterpret_cast<void*>(&jit_hashmap_set_any)},
        {"_F_hoo_hashmap_get_any_data_i8_p_i8", reinterpret_cast<void*>(&jit_hashmap_get_any_data)},
        {"_F_hoo_hashmap_remove_i8_p_i8", reinterpret_cast<void*>(&jit_hashmap_remove)},
        {"_F_hoo_hashmap_clear_v_p", reinterpret_cast<void*>(&jit_hashmap_clear)},
        {"_F_M_hoo_E_hashmap_release_v", reinterpret_cast<void*>(&jit_hashmap_release)},
        // Array hoo-module-qualified symbols (codegen redirects array_* to hoo module)
        {"_F_M_hoo_E_array_new_v", reinterpret_cast<void*>(&jit_hoo_array_new)},
        {"_F_M_hoo_E_array_push_double_v_p_p", reinterpret_cast<void*>(&jit_array_push_double)},
        {"_F_M_hoo_E_array_get_double_v_p_p", reinterpret_cast<void*>(&jit_array_get_double)},
        {"_F_M_hoo_E_array_push_int64_v_p_p", reinterpret_cast<void*>(&jit_array_push_int64_plain)},
        {"_F_M_hoo_E_array_get_int64_v_p_p", reinterpret_cast<void*>(&jit_array_get_int64_plain)},
        {"_F_M_hoo_E_array_length_v_p", reinterpret_cast<void*>(&jit_array_length)},
        {"_F_M_hoo_E_array_clear_v_p", reinterpret_cast<void*>(&jit_array_clear)},
        {"_F_M_hoo_E_array_empty_v_p", reinterpret_cast<void*>(&jit_array_empty)},
        {"_F_M_hoo_E_array_push_string_v_p_p", reinterpret_cast<void*>(&jit_array_push_string)},
        {"_F_M_hoo_E_array_get_string_v_p_p", reinterpret_cast<void*>(&jit_array_get_string)},
        {"_F_M_hoo_E_array_push_bool_v_p_p", reinterpret_cast<void*>(&jit_array_push_bool)},
        {"_F_M_hoo_E_array_get_bool_v_p_p", reinterpret_cast<void*>(&jit_array_get_bool)},
        // CamelCase aliases
        {"_F_M_hoo_E_array_pushDouble_v_p_p", reinterpret_cast<void*>(&jit_array_push_double)},
        {"_F_M_hoo_E_array_getDouble_v_p_p", reinterpret_cast<void*>(&jit_array_get_double)},
        {"_F_M_hoo_E_array_pushInt64_v_p_p", reinterpret_cast<void*>(&jit_array_push_int64_plain)},
        {"_F_M_hoo_E_array_getInt64_v_p_p", reinterpret_cast<void*>(&jit_array_get_int64_plain)},
        {"_F_M_hoo_E_array_pushString_v_p_p", reinterpret_cast<void*>(&jit_array_push_string)},
        {"_F_M_hoo_E_array_getString_v_p_p", reinterpret_cast<void*>(&jit_array_get_string)},
        {"_F_M_hoo_E_array_pushBool_v_p_p", reinterpret_cast<void*>(&jit_array_push_bool)},
        {"_F_M_hoo_E_array_getBool_v_p_p", reinterpret_cast<void*>(&jit_array_get_bool)},
        // Buffer hoo-module-qualified symbols (codegen redirects buffer_* to hoo module)
        {"_F_M_hoo_E_buffer_new_v", reinterpret_cast<void*>(&jit_hoo_buffer_new)},
        {"_F_M_hoo_E_buffer_fromBytes_p_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_from_bytes)},
        {"_F_M_hoo_E_buffer_copy_v", reinterpret_cast<void*>(&jit_hoo_buffer_copy)},
        {"_F_M_hoo_E_buffer_length_v", reinterpret_cast<void*>(&jit_hoo_buffer_length)},
        {"_F_M_hoo_E_buffer_capacity_v", reinterpret_cast<void*>(&jit_hoo_buffer_capacity)},
        {"_F_M_hoo_E_buffer_byteAt_v_p", reinterpret_cast<void*>(&jit_hoo_buffer_byte_at)},
        {"_F_M_hoo_E_buffer_setByte_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_set_byte)},
        {"_F_M_hoo_E_buffer_append_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_append)},
        {"_F_M_hoo_E_buffer_appendBuffer_v_p", reinterpret_cast<void*>(&jit_hoo_buffer_append_buffer)},
        {"_F_M_hoo_E_buffer_clear_v", reinterpret_cast<void*>(&jit_hoo_buffer_clear)},
        {"_F_M_hoo_E_buffer_slice_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_slice)},
        {"_F_M_hoo_E_buffer_data_v", reinterpret_cast<void*>(&jit_hoo_buffer_data)},
        // CamelCase aliases
        {"_F_M_hoo_E_buffer_new_v", reinterpret_cast<void*>(&jit_hoo_buffer_new)},
        {"_F_M_hoo_E_buffer_fromBytes_p_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_from_bytes)},
        {"_F_M_hoo_E_buffer_copy_v", reinterpret_cast<void*>(&jit_hoo_buffer_copy)},
        {"_F_M_hoo_E_buffer_length_v", reinterpret_cast<void*>(&jit_hoo_buffer_length)},
        {"_F_M_hoo_E_buffer_capacity_v", reinterpret_cast<void*>(&jit_hoo_buffer_capacity)},
        {"_F_M_hoo_E_buffer_byteAt_v_p", reinterpret_cast<void*>(&jit_hoo_buffer_byte_at)},
        {"_F_M_hoo_E_buffer_setByte_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_set_byte)},
        {"_F_M_hoo_E_buffer_append_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_append)},
        {"_F_M_hoo_E_buffer_appendBuffer_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_append_buffer)},
        {"_F_M_hoo_E_buffer_clear_v", reinterpret_cast<void*>(&jit_hoo_buffer_clear)},
        {"_F_M_hoo_E_buffer_slice_v_p_p", reinterpret_cast<void*>(&jit_hoo_buffer_slice)},
        {"_F_M_hoo_E_buffer_data_v", reinterpret_cast<void*>(&jit_hoo_buffer_data)},
        // Map hoo-module-qualified symbols (codegen redirects map_* to hoo module)
        {"_F_M_hoo_E_map_new_v_p", reinterpret_cast<void*>(&jit_map_new_plain)},
        {"_F_M_hoo_E_map_new_v_p_p", reinterpret_cast<void*>(&jit_map_new_types)},
        {"_F_M_hoo_E_map_set_int64_int64_v_p_p", reinterpret_cast<void*>(&jit_map_set_int64_int64)},
        {"_F_M_hoo_E_map_get_int64_int64_v_p", reinterpret_cast<void*>(&jit_map_get_int64_int64)},
        {"_F_M_hoo_E_map_length_v", reinterpret_cast<void*>(&jit_map_length)},
        {"_F_M_hoo_E_map_set_string_int64_v_p_p", reinterpret_cast<void*>(&jit_map_set_string_int64)},
        {"_F_M_hoo_E_map_contains_int64_v_p", reinterpret_cast<void*>(&jit_map_contains_int64)},
        {"_F_M_hoo_E_map_remove_int64_v_p", reinterpret_cast<void*>(&jit_map_remove_int64)},
        {"_F_M_hoo_E_map_clear_v", reinterpret_cast<void*>(&jit_map_clear)},
        {"_F_M_hoo_E_map_empty_v", reinterpret_cast<void*>(&jit_map_empty)},
        {"_F_M_hoo_E_map_key_type_v", reinterpret_cast<void*>(&jit_map_key_type)},
        {"_F_M_hoo_E_map_value_type_v", reinterpret_cast<void*>(&jit_map_value_type)},
        // CamelCase aliases
        {"_F_M_hoo_E_map_new_v_p_p", reinterpret_cast<void*>(&jit_map_new_types)},
        {"_F_M_hoo_E_map_setInt64Int64_v_p_p", reinterpret_cast<void*>(&jit_map_set_int64_int64)},
        {"_F_M_hoo_E_map_getInt64Int64_v_p", reinterpret_cast<void*>(&jit_map_get_int64_int64)},
        {"_F_M_hoo_E_map_setInt64Double_v_p_p", reinterpret_cast<void*>(&jit_map_set_int64_double)},
        {"_F_M_hoo_E_map_getInt64Double_v_p", reinterpret_cast<void*>(&jit_map_get_int64_double)},
        {"_F_M_hoo_E_map_setInt64String_v_p_p", reinterpret_cast<void*>(&jit_map_set_int64_string)},
        {"_F_M_hoo_E_map_getInt64String_v_p", reinterpret_cast<void*>(&jit_map_get_int64_string)},
        {"_F_M_hoo_E_map_setInt64Bool_v_p_p", reinterpret_cast<void*>(&jit_map_set_int64_bool)},
        {"_F_M_hoo_E_map_getInt64Bool_v_p", reinterpret_cast<void*>(&jit_map_get_int64_bool)},
        {"_F_M_hoo_E_map_setStringInt64_v_p_p", reinterpret_cast<void*>(&jit_map_set_string_int64)},
        {"_F_M_hoo_E_map_setStringDouble_v_p_p", reinterpret_cast<void*>(&jit_map_set_string_double)},
        {"_F_M_hoo_E_map_getStringDouble_v_p", reinterpret_cast<void*>(&jit_map_get_string_double)},
        {"_F_M_hoo_E_map_setStringString_v_p_p", reinterpret_cast<void*>(&jit_map_set_string_string)},
        {"_F_M_hoo_E_map_getStringString_v_p", reinterpret_cast<void*>(&jit_map_get_string_string)},
        {"_F_M_hoo_E_map_setStringBool_v_p_p", reinterpret_cast<void*>(&jit_map_set_string_bool)},
        {"_F_M_hoo_E_map_getStringBool_v_p", reinterpret_cast<void*>(&jit_map_get_string_bool)},
        {"_F_M_hoo_E_map_setInt8Int64_v_p_p", reinterpret_cast<void*>(&jit_map_set_int8_int64)},
        {"_F_M_hoo_E_map_getInt8Int64_v_p", reinterpret_cast<void*>(&jit_map_get_int8_int64)},
        {"_F_M_hoo_E_map_containsInt64_v_p", reinterpret_cast<void*>(&jit_map_contains_int64)},
        {"_F_M_hoo_E_map_removeInt64_v_p", reinterpret_cast<void*>(&jit_map_remove_int64)},
        {"_F_M_hoo_E_map_keyType_v", reinterpret_cast<void*>(&jit_map_key_type)},
        {"_F_M_hoo_E_map_valueType_v", reinterpret_cast<void*>(&jit_map_value_type)},
        // Math functions (hoo module namespace, as codegen redirects them)
        // Math functions (simplified names for class-based Math.abs() syntax)
        {"_F_M_hoo_E_math_abs_v_p", reinterpret_cast<void*>(&jit_math_abs_int64)},
        {"_F_M_hoo_E_math_min_v_p_p", reinterpret_cast<void*>(&jit_math_min_int64)},
        {"_F_M_hoo_E_math_max_v_p_p", reinterpret_cast<void*>(&jit_math_max_int64)},
        {"_F_M_hoo_E_math_sign_v_p", reinterpret_cast<void*>(&jit_math_sign_int64)},
        // Math functions (singleton class mangled names)
        // Math functions (free function snake_case names)
        {"_F_M_hoo_E_math_abs_i8_p", reinterpret_cast<void*>(&jit_math_abs_int64)},
        {"_F_M_hoo_E_math_abs_i1_p", reinterpret_cast<void*>(&jit_math_abs_int8)},
        {"_F_M_hoo_E_math_abs_u1_p", reinterpret_cast<void*>(&jit_math_abs_byte)},
        {"_F_M_hoo_E_math_abs_d_p", reinterpret_cast<void*>(&jit_math_abs_double)},
        {"_F_M_hoo_E_math_abs_e_p", reinterpret_cast<void*>(&jit_math_abs_f8)},
        {"_F_M_hoo_E_math_min_i8_p_p", reinterpret_cast<void*>(&jit_math_min_int64)},
        {"_F_M_hoo_E_math_min_i1_p_p", reinterpret_cast<void*>(&jit_math_min_int8)},
        {"_F_M_hoo_E_math_min_u1_p_p", reinterpret_cast<void*>(&jit_math_min_byte)},
        {"_F_M_hoo_E_math_min_d_p_p", reinterpret_cast<void*>(&jit_math_min_double)},
        {"_F_M_hoo_E_math_min_e_p_p", reinterpret_cast<void*>(&jit_math_min_f8)},
        {"_F_M_hoo_E_math_max_i8_p_p", reinterpret_cast<void*>(&jit_math_max_int64)},
        {"_F_M_hoo_E_math_max_i1_p_p", reinterpret_cast<void*>(&jit_math_max_int8)},
        {"_F_M_hoo_E_math_max_u1_p_p", reinterpret_cast<void*>(&jit_math_max_byte)},
        {"_F_M_hoo_E_math_max_d_p_p", reinterpret_cast<void*>(&jit_math_max_double)},
        {"_F_M_hoo_E_math_max_e_p_p", reinterpret_cast<void*>(&jit_math_max_f8)},
        {"_F_M_hoo_E_math_sign_i8_p", reinterpret_cast<void*>(&jit_math_sign_int64)},
        {"_F_M_hoo_E_math_sign_i1_p", reinterpret_cast<void*>(&jit_math_sign_int8)},
        {"_F_M_hoo_E_math_sign_u1_p", reinterpret_cast<void*>(&jit_math_sign_byte)},
        {"_F_M_hoo_E_math_sign_d_p", reinterpret_cast<void*>(&jit_math_sign_double)},
        {"_F_M_hoo_E_math_sign_e_p", reinterpret_cast<void*>(&jit_math_sign_f8)},
        {"_F_M_hoo_E_math_gcd_i8_p_p", reinterpret_cast<void*>(&jit_math_gcd)},
        {"_F_M_hoo_E_math_factorial_i8_p", reinterpret_cast<void*>(&jit_math_factorial)},
        {"_F_M_hoo_E_math_fibonacci_i8_p", reinterpret_cast<void*>(&jit_math_fibonacci)},
        {"_F_M_hoo_E_math_is_even_i8_p", reinterpret_cast<void*>(&jit_math_is_even)},
        {"_F_M_hoo_E_math_is_odd_i8_p", reinterpret_cast<void*>(&jit_math_is_odd)},
        {"_F_M_hoo_E_math_is_prime_i8_p", reinterpret_cast<void*>(&jit_math_is_prime)},
        {"_F_M_hoo_E_math_lcm_i8_p_p", reinterpret_cast<void*>(&jit_math_lcm)},
        {"_F_M_hoo_E_math_sqrt_d_p", reinterpret_cast<void*>(&jit_math_sqrt)},
        {"_F_M_hoo_E_math_get_pi_d", reinterpret_cast<void*>(&jit_math_get_pi)},
        {"_F_M_hoo_E_math_get_e_d", reinterpret_cast<void*>(&jit_math_get_e)},
        {"_F_M_hoo_E_math_get_tau_d", reinterpret_cast<void*>(&jit_math_get_tau)},
        {"_F_M_hoo_E_math_get_inf_d", reinterpret_cast<void*>(&jit_math_get_inf)},
        {"_F_M_hoo_E_math_get_neg_inf_d", reinterpret_cast<void*>(&jit_math_get_neg_inf)},
        {"_F_M_hoo_E_math_get_nan_d", reinterpret_cast<void*>(&jit_math_get_nan)},
        {"_F_M_hoo_E_math_pow_d_p_p", reinterpret_cast<void*>(&jit_math_pow)},
        {"_F_M_hoo_E_math_cbrt_d_p", reinterpret_cast<void*>(&jit_math_cbrt)},
        {"_F_M_hoo_E_math_hypot_d_p_p", reinterpret_cast<void*>(&jit_math_hypot)},
        {"_F_M_hoo_E_math_floor_d_p", reinterpret_cast<void*>(&jit_math_floor)},
        {"_F_M_hoo_E_math_ceil_d_p", reinterpret_cast<void*>(&jit_math_ceil)},
        {"_F_M_hoo_E_math_round_d_p", reinterpret_cast<void*>(&jit_math_round)},
        {"_F_M_hoo_E_math_trunc_d_p", reinterpret_cast<void*>(&jit_math_trunc)},
        {"_F_M_hoo_E_math_fract_d_p", reinterpret_cast<void*>(&jit_math_fract)},
        {"_F_M_hoo_E_math_sin_d_p", reinterpret_cast<void*>(&jit_math_sin)},
        {"_F_M_hoo_E_math_cos_d_p", reinterpret_cast<void*>(&jit_math_cos)},
        {"_F_M_hoo_E_math_tan_d_p", reinterpret_cast<void*>(&jit_math_tan)},
        {"_F_M_hoo_E_math_asin_d_p", reinterpret_cast<void*>(&jit_math_unary_asin)},
        {"_F_M_hoo_E_math_acos_d_p", reinterpret_cast<void*>(&jit_math_unary_acos)},
        {"_F_M_hoo_E_math_atan_d_p", reinterpret_cast<void*>(&jit_math_unary_atan)},
        {"_F_M_hoo_E_math_atan2_d_p_p", reinterpret_cast<void*>(&jit_math_atan2)},
        {"_F_M_hoo_E_math_sinh_d_p", reinterpret_cast<void*>(&jit_math_sinh)},
        {"_F_M_hoo_E_math_cosh_d_p", reinterpret_cast<void*>(&jit_math_cosh)},
        {"_F_M_hoo_E_math_tanh_d_p", reinterpret_cast<void*>(&jit_math_tanh)},
        {"_F_M_hoo_E_math_exp_d_p", reinterpret_cast<void*>(&jit_math_exp)},
        {"_F_M_hoo_E_math_exp2_d_p", reinterpret_cast<void*>(&jit_math_exp2)},
        {"_F_M_hoo_E_math_expm1_d_p", reinterpret_cast<void*>(&jit_math_expm1)},
        {"_F_M_hoo_E_math_log_d_p", reinterpret_cast<void*>(&jit_math_log)},
        {"_F_M_hoo_E_math_log10_d_p", reinterpret_cast<void*>(&jit_math_log10)},
        {"_F_M_hoo_E_math_log2_d_p", reinterpret_cast<void*>(&jit_math_log2)},
        {"_F_M_hoo_E_math_log1p_d_p", reinterpret_cast<void*>(&jit_math_log1p)},
        {"_F_M_hoo_E_math_clamp_d_p_p_p", reinterpret_cast<void*>(&jit_math_clamp)},
        // Random module (instance-based, prefix-style)
        {"_F_M_hoo_E_random_new_v", reinterpret_cast<void*>(&jit_random_new)},
        {"_F_M_hoo_E_random_new_v_p", reinterpret_cast<void*>(&jit_random_new_with_seed)},
        {"_F_M_hoo_E_random_nextInt_v", reinterpret_cast<void*>(&jit_random_next_int)},
        {"_F_M_hoo_E_random_nextIntMax_v_p", reinterpret_cast<void*>(&jit_random_next_int_max)},
        {"_F_M_hoo_E_random_nextDouble_v", reinterpret_cast<void*>(&jit_random_next_double)},
        {"_F_M_hoo_E_random_nextBool_v", reinterpret_cast<void*>(&jit_random_next_bool)},
        {"_F_M_hoo_E_random_nextBytes_v_p_p", reinterpret_cast<void*>(&jit_random_next_bytes)},
        {"_F_M_hoo_E_random_release_v", reinterpret_cast<void*>(&jit_random_release)},
        // System module (free function snake_case names)
        {"_F_M_hoo_E_system_get_env_p_p", reinterpret_cast<void*>(&jit_system_get_env)},
        {"_F_M_hoo_E_system_set_env_p_p_p", reinterpret_cast<void*>(&jit_system_set_env)},
        {"_F_M_hoo_E_system_unset_env_p_p", reinterpret_cast<void*>(&jit_system_unset_env)},
        {"_F_M_hoo_E_system_hostname_p", reinterpret_cast<void*>(&jit_system_hostname)},
        {"_F_M_hoo_E_system_os_name_p", reinterpret_cast<void*>(&jit_system_os_name)},
        {"_F_M_hoo_E_system_os_version_p", reinterpret_cast<void*>(&jit_system_os_version)},
        {"_F_M_hoo_E_system_cpu_count_p", reinterpret_cast<void*>(&jit_system_cpu_count)},
        {"_F_M_hoo_E_system_process_id_p", reinterpret_cast<void*>(&jit_system_process_id)},
        {"_F_M_hoo_E_system_uptime_ms_p", reinterpret_cast<void*>(&jit_system_uptime_ms)},
        {"_F_M_hoo_E_system_exit_p_p", reinterpret_cast<void*>(&jit_system_exit)},
        {"_F_M_hoo_E_system_exec_p_p", reinterpret_cast<void*>(&jit_system_exec)},
        {"_F_M_hoo_E_system_exec_status_p_p", reinterpret_cast<void*>(&jit_system_exec_status)},
        {"_F_M_hoo_E_system_user_home_p", reinterpret_cast<void*>(&jit_system_user_home)},
        {"_F_M_hoo_E_system_user_name_p", reinterpret_cast<void*>(&jit_system_user_name)},
        {"_F_M_hoo_E_system_current_dir_p", reinterpret_cast<void*>(&jit_system_current_dir)},
        {"_F_M_hoo_E_system_set_current_dir_p_p", reinterpret_cast<void*>(&jit_system_set_current_dir)},
        {"_F_M_hoo_E_system_total_memory_p", reinterpret_cast<void*>(&jit_system_total_memory)},
        {"_F_M_hoo_E_system_free_memory_p", reinterpret_cast<void*>(&jit_system_free_memory)},
        // Fs functions (hoo module namespace, both Fs.methodName and fs_methodName syntax)
        {"_F_M_hoo_E_fs_exists_v_p", reinterpret_cast<void*>(&jit_fs_exists)},
        {"_F_M_hoo_E_fs_exists_p_p", reinterpret_cast<void*>(&jit_fs_exists)},
        {"_F_M_hoo_E_fs_is_file_v_p", reinterpret_cast<void*>(&jit_fs_is_file)},
        {"_F_M_hoo_E_fs_is_file_p_p", reinterpret_cast<void*>(&jit_fs_is_file)},
        {"_F_M_hoo_E_fs_is_dir_v_p", reinterpret_cast<void*>(&jit_fs_is_dir)},
        {"_F_M_hoo_E_fs_is_dir_p_p", reinterpret_cast<void*>(&jit_fs_is_dir)},
        {"_F_M_hoo_E_fs_size_v_p", reinterpret_cast<void*>(&jit_fs_size)},
        {"_F_M_hoo_E_fs_size_p_p", reinterpret_cast<void*>(&jit_fs_size)},
        {"_F_M_hoo_E_fs_last_modified_v_p", reinterpret_cast<void*>(&jit_fs_last_modified)},
        {"_F_M_hoo_E_fs_last_modified_p_p", reinterpret_cast<void*>(&jit_fs_last_modified)},
        {"_F_M_hoo_E_fs_delete_v_p", reinterpret_cast<void*>(&jit_fs_delete)},
        {"_F_M_hoo_E_fs_delete_p_p", reinterpret_cast<void*>(&jit_fs_delete)},
        {"_F_M_hoo_E_fs_rename_v_p_p", reinterpret_cast<void*>(&jit_fs_rename)},
        {"_F_M_hoo_E_fs_rename_p_p_p", reinterpret_cast<void*>(&jit_fs_rename)},
        {"_F_M_hoo_E_fs_copy_v_p_p", reinterpret_cast<void*>(&jit_fs_copy)},
        {"_F_M_hoo_E_fs_copy_p_p_p", reinterpret_cast<void*>(&jit_fs_copy)},
        {"_F_M_hoo_E_fs_read_text_v_p", reinterpret_cast<void*>(&jit_fs_read_text)},
        {"_F_M_hoo_E_fs_read_text_p_p", reinterpret_cast<void*>(&jit_fs_read_text)},
        {"_F_M_hoo_E_fs_read_text_v_p_p", reinterpret_cast<void*>(&jit_fs_read_text_default)},
        {"_F_M_hoo_E_fs_read_text_p_p_p", reinterpret_cast<void*>(&jit_fs_read_text_default)},
        {"_F_M_hoo_E_fs_write_text_v_p_p", reinterpret_cast<void*>(&jit_fs_write_text)},
        {"_F_M_hoo_E_fs_write_text_p_p_p", reinterpret_cast<void*>(&jit_fs_write_text)},
        {"_F_M_hoo_E_fs_append_text_v_p_p", reinterpret_cast<void*>(&jit_fs_append_text)},
        {"_F_M_hoo_E_fs_append_text_p_p_p", reinterpret_cast<void*>(&jit_fs_append_text)},
        {"_F_M_hoo_E_fs_read_bytes_buffer_v_p", reinterpret_cast<void*>(&jit_fs_read_bytes_buffer)},
        {"_F_M_hoo_E_fs_read_bytes_buffer_p_p", reinterpret_cast<void*>(&jit_fs_read_bytes_buffer)},
        {"_F_M_hoo_E_fs_read_bytes_buffer_v_p_p", reinterpret_cast<void*>(&jit_fs_read_bytes_buffer_default)},
        {"_F_M_hoo_E_fs_read_bytes_buffer_p_p_p", reinterpret_cast<void*>(&jit_fs_read_bytes_buffer_default)},
        {"_F_M_hoo_E_fs_write_bytes_buffer_v_p_p", reinterpret_cast<void*>(&jit_fs_write_bytes_buffer)},
        {"_F_M_hoo_E_fs_write_bytes_buffer_p_p_p", reinterpret_cast<void*>(&jit_fs_write_bytes_buffer)},
        {"_F_M_hoo_E_fs_mkdir_v_p", reinterpret_cast<void*>(&jit_fs_mkdir)},
        {"_F_M_hoo_E_fs_mkdir_p_p", reinterpret_cast<void*>(&jit_fs_mkdir)},
        {"_F_M_hoo_E_fs_mkdirs_v_p", reinterpret_cast<void*>(&jit_fs_mkdirs)},
        {"_F_M_hoo_E_fs_mkdirs_p_p", reinterpret_cast<void*>(&jit_fs_mkdirs)},
        {"_F_M_hoo_E_fs_rmdir_v_p", reinterpret_cast<void*>(&jit_fs_rmdir)},
        {"_F_M_hoo_E_fs_rmdir_p_p", reinterpret_cast<void*>(&jit_fs_rmdir)},
        {"_F_M_hoo_E_fs_list_dir_v_p", reinterpret_cast<void*>(&jit_fs_list_dir)},
        {"_F_M_hoo_E_fs_list_dir_p_p", reinterpret_cast<void*>(&jit_fs_list_dir)},
        {"_F_M_hoo_E_fs_temp_dir_v", reinterpret_cast<void*>(&jit_fs_temp_dir)},
        {"_F_M_hoo_E_fs_temp_dir_p", reinterpret_cast<void*>(&jit_fs_temp_dir)},
        {"_F_M_hoo_E_fs_create_temp_file_v_p", reinterpret_cast<void*>(&jit_fs_create_temp_file)},
        {"_F_M_hoo_E_fs_create_temp_file_p_p", reinterpret_cast<void*>(&jit_fs_create_temp_file)},

        {"_F_M_hoo_E_regex_compile_v_p", reinterpret_cast<void*>(&jit_regex_compile)},
        {"_F_M_hoo_E_regex_match_v_p_p", reinterpret_cast<void*>(&jit_regex_match)},
        {"_F_M_hoo_E_uuid_v4_v", reinterpret_cast<void*>(&jit_uuid_v4)},
        {"_F_M_hoo_E_uuid_to_string_v_p", reinterpret_cast<void*>(&jit_uuid_to_string)},
        {"_F_M_hoo_E_Uuid_N_v4_p", reinterpret_cast<void*>(&jit_uuid_v4)},
        {"_F_M_hoo_E_Uuid_N_to_string_p_p", reinterpret_cast<void*>(&jit_uuid_to_string)},
        {"_F_M_hoo_E_encoding_base64_encode_p_p_p", reinterpret_cast<void*>(&jit_encoding_base64_encode)},
        {"_F_M_hoo_E_encoding_base64_decode_p_p", reinterpret_cast<void*>(&jit_encoding_base64_decode)},
        {"_F_M_hoo_E_encoding_hex_encode_p_p_p", reinterpret_cast<void*>(&jit_encoding_hex_encode)},
        {"_F_M_hoo_E_encoding_hex_decode_p_p", reinterpret_cast<void*>(&jit_encoding_hex_decode)},
        {"_F_M_hoo_E_encoding_url_encode_p_p", reinterpret_cast<void*>(&jit_encoding_url_encode)},
        {"_F_M_hoo_E_encoding_url_decode_p_p", reinterpret_cast<void*>(&jit_encoding_url_decode)},
        {"_F_M_hoo_E_encoding_base64_encode_buffer_p_p", reinterpret_cast<void*>(&jit_encoding_base64_encode_buffer)},
        {"_F_M_hoo_E_encoding_base64_decode_buffer_p_p", reinterpret_cast<void*>(&jit_encoding_base64_decode_buffer)},
        {"_F_M_hoo_E_encoding_hex_encode_buffer_p_p", reinterpret_cast<void*>(&jit_encoding_hex_encode_buffer)},
        {"_F_M_hoo_E_encoding_hex_decode_buffer_p_p", reinterpret_cast<void*>(&jit_encoding_hex_decode_buffer)},
        {"_F_M_hoo_E_thread_spawn_v_p_p", reinterpret_cast<void*>(&jit_thread_spawn)},
        {"_F_M_hoo_E_thread_join_v_p", reinterpret_cast<void*>(&jit_thread_join)},
        {"_F_M_hoo_E_thread_self_v", reinterpret_cast<void*>(&jit_thread_self)},
        {"_F_M_hoo_E_thread_mutex_create_v", reinterpret_cast<void*>(&jit_thread_mutex_create)},
        {"_F_M_hoo_E_thread_mutex_lock_v_p", reinterpret_cast<void*>(&jit_thread_mutex_lock)},
        {"_F_M_hoo_E_thread_mutex_unlock_v_p", reinterpret_cast<void*>(&jit_thread_mutex_unlock)},
        {"_F_M_hoo_E_thread_mutex_destroy_v_p", reinterpret_cast<void*>(&jit_thread_mutex_destroy)},

        // CSV module (instance-based, prefix-style)
        {"_F_M_hoo_E_csv_new_v", reinterpret_cast<void*>(&jit_csv_new)},
        {"_F_M_hoo_E_csv_from_opts_p_p_p", reinterpret_cast<void*>(&jit_hoo_csv_from_opts)},
        {"_F_M_hoo_E_csv_release_v", reinterpret_cast<void*>(&jit_csv_release)},
        {"_F_M_hoo_E_csv_parse_v_p", reinterpret_cast<void*>(&jit_csv_parse)},
        {"_F_M_hoo_E_csv_generate_v_p", reinterpret_cast<void*>(&jit_csv_generate)},
        {"_F_M_hoo_E_csv_readFile_v_p", reinterpret_cast<void*>(&jit_csv_read_file)},
        {"_F_M_hoo_E_csv_writeFile_v_p_p", reinterpret_cast<void*>(&jit_csv_write_file)},
        {"_F_M_hoo_E_csv_escape_v_p", reinterpret_cast<void*>(&jit_csv_escape)},
        {"_F_M_hoo_E_csv_count_v_p_p", reinterpret_cast<void*>(&jit_csv_count)},
        {"_F_M_hoo_E_csv_sum_v_p_p", reinterpret_cast<void*>(&jit_csv_sum)},
        {"_F_M_hoo_E_csv_avg_v_p_p", reinterpret_cast<void*>(&jit_csv_avg)},
        {"_F_M_hoo_E_csv_min_v_p_p", reinterpret_cast<void*>(&jit_csv_min)},
        {"_F_M_hoo_E_csv_max_v_p_p", reinterpret_cast<void*>(&jit_csv_max)},
        {"_F_M_hoo_E_csv_select_v_p_p", reinterpret_cast<void*>(&jit_csv_select)},
        {"_F_M_hoo_E_csv_filter_v_p_p_p_p", reinterpret_cast<void*>(&jit_csv_filter)},
        {"_F_M_hoo_E_csv_sort_v_p_p_p", reinterpret_cast<void*>(&jit_csv_sort)},
        {"_F_M_hoo_E_csv_describe_v_p_p", reinterpret_cast<void*>(&jit_csv_describe)},
        // Datetime module — free function (_p return for free function dispatch)
        {"_F_M_hoo_E_datetime_now_p", reinterpret_cast<void*>(&jit_datetime_now)},
        {"_F_M_hoo_E_datetime_new_p_p", reinterpret_cast<void*>(&jit_datetime_new)},
        {"_F_M_hoo_E_datetime_now_seconds_p", reinterpret_cast<void*>(&jit_datetime_now_seconds)},
        {"_F_M_hoo_E_datetime_now_precise_p", reinterpret_cast<void*>(&jit_datetime_now_precise)},
        {"_F_M_hoo_E_datetime_format_p_p_p", reinterpret_cast<void*>(&jit_datetime_format)},
        {"_F_M_hoo_E_datetime_iso8601_p_p", reinterpret_cast<void*>(&jit_datetime_iso8601)},
        {"_F_M_hoo_E_datetime_parse_p_p_p", reinterpret_cast<void*>(&jit_datetime_parse)},
        {"_F_M_hoo_E_datetime_from_iso8601_p_p", reinterpret_cast<void*>(&jit_datetime_from_iso8601)},
        {"_F_M_hoo_E_datetime_add_days_p_p_p", reinterpret_cast<void*>(&jit_datetime_add_days)},
        {"_F_M_hoo_E_datetime_add_hours_p_p_p", reinterpret_cast<void*>(&jit_datetime_add_hours)},
        {"_F_M_hoo_E_datetime_add_minutes_p_p_p", reinterpret_cast<void*>(&jit_datetime_add_minutes)},
        {"_F_M_hoo_E_datetime_add_seconds_p_p_p", reinterpret_cast<void*>(&jit_datetime_add_seconds)},
        {"_F_M_hoo_E_datetime_add_milliseconds_p_p_p", reinterpret_cast<void*>(&jit_datetime_add_milliseconds)},
        {"_F_M_hoo_E_datetime_diff_days_p_p_p", reinterpret_cast<void*>(&jit_datetime_diff_days)},
        {"_F_M_hoo_E_datetime_diff_hours_p_p_p", reinterpret_cast<void*>(&jit_datetime_diff_hours)},
        {"_F_M_hoo_E_datetime_diff_seconds_p_p_p", reinterpret_cast<void*>(&jit_datetime_diff_seconds)},
        {"_F_M_hoo_E_datetime_compare_p_p_p", reinterpret_cast<void*>(&jit_datetime_compare)},


        // DateTime instance methods
        {"_F_M_hoo_E_datetime_inst_format_p_p", reinterpret_cast<void*>(&jit_datetime_inst_format)},
        {"_F_M_hoo_E_datetime_inst_iso8601_p", reinterpret_cast<void*>(&jit_datetime_inst_iso8601)},
        {"_F_M_hoo_E_datetime_inst_getTimestamp_i8", reinterpret_cast<void*>(&jit_datetime_inst_getTimestamp)},
        {"_F_M_hoo_E_datetime_inst_addDays_p_p", reinterpret_cast<void*>(&jit_datetime_inst_addDays)},
        {"_F_M_hoo_E_datetime_inst_addHours_p_p", reinterpret_cast<void*>(&jit_datetime_inst_addHours)},
        {"_F_M_hoo_E_datetime_inst_addMinutes_p_p", reinterpret_cast<void*>(&jit_datetime_inst_addMinutes)},
        {"_F_M_hoo_E_datetime_inst_addSeconds_p_p", reinterpret_cast<void*>(&jit_datetime_inst_addSeconds)},
        {"_F_M_hoo_E_datetime_inst_addMilliseconds_p_p", reinterpret_cast<void*>(&jit_datetime_inst_addMilliseconds)},
        {"_F_M_hoo_E_datetime_inst_diffDays_i8_p", reinterpret_cast<void*>(&jit_datetime_inst_diffDays)},
        {"_F_M_hoo_E_datetime_inst_diffHours_i8_p", reinterpret_cast<void*>(&jit_datetime_inst_diffHours)},
        {"_F_M_hoo_E_datetime_inst_diffSeconds_d_p", reinterpret_cast<void*>(&jit_datetime_inst_diffSeconds)},
        {"_F_M_hoo_E_datetime_inst_compare_i8_p", reinterpret_cast<void*>(&jit_datetime_inst_compare)},

        // Path module
        {"_F_M_hoo_E_path_dirname_v_p", reinterpret_cast<void*>(&jit_path_dirname)},
        {"_F_M_hoo_E_path_basename_v_p", reinterpret_cast<void*>(&jit_path_basename)},
        {"_F_M_hoo_E_path_extension_v_p", reinterpret_cast<void*>(&jit_path_extension)},
        {"_F_M_hoo_E_path_stem_v_p", reinterpret_cast<void*>(&jit_path_stem)},
        {"_F_M_hoo_E_path_join_v_p_p", reinterpret_cast<void*>(&jit_path_join)},
        {"_F_M_hoo_E_path_normalize_v_p", reinterpret_cast<void*>(&jit_path_normalize)},
        {"_F_M_hoo_E_path_absolute_v_p", reinterpret_cast<void*>(&jit_path_absolute)},
        {"_F_M_hoo_E_path_relative_v_p_p", reinterpret_cast<void*>(&jit_path_relative)},
        {"_F_M_hoo_E_path_is_absolute_v_p", reinterpret_cast<void*>(&jit_path_is_absolute)},
        {"_F_M_hoo_E_path_is_relative_v_p", reinterpret_cast<void*>(&jit_path_is_relative)},
        {"_F_M_hoo_E_path_has_extension_v_p", reinterpret_cast<void*>(&jit_path_has_extension)},
        {"_F_M_hoo_E_path_separator_v", reinterpret_cast<void*>(&jit_path_separator)},
        {"_F_M_hoo_E_path_list_separator_v", reinterpret_cast<void*>(&jit_path_list_separator)},

        // Hashing module
        {"_F_M_hoo_E_hashing_sha256_p_p_p", reinterpret_cast<void*>(&jit_hashing_sha256)},
        {"_F_M_hoo_E_hashing_sha1_p_p_p", reinterpret_cast<void*>(&jit_hashing_sha1)},
        {"_F_M_hoo_E_hashing_md5_p_p_p", reinterpret_cast<void*>(&jit_hashing_md5)},
        {"_F_M_hoo_E_hashing_sha256_file_p_p", reinterpret_cast<void*>(&jit_hashing_sha256_file)},
        {"_F_M_hoo_E_hashing_crc32_p_p_p", reinterpret_cast<void*>(&jit_hashing_crc32)},
        {"_F_M_hoo_E_hashing_hmac_sha256_p_p_p_p_p", reinterpret_cast<void*>(&jit_hashing_hmac_sha256)},
        {"_F_M_hoo_E_hashing_sha256_buffer_p_p", reinterpret_cast<void*>(&jit_hashing_sha256_buffer)},
        {"_F_M_hoo_E_hashing_sha1_buffer_p_p", reinterpret_cast<void*>(&jit_hashing_sha1_buffer)},
        {"_F_M_hoo_E_hashing_md5_buffer_p_p", reinterpret_cast<void*>(&jit_hashing_md5_buffer)},
        {"_F_M_hoo_E_hashing_crc32_buffer_p_p", reinterpret_cast<void*>(&jit_hashing_crc32_buffer)},
        {"_F_M_hoo_E_hashing_hmac_sha256_buffer_p_p_p", reinterpret_cast<void*>(&jit_hashing_hmac_sha256_buffer)},

        // Process module
        {"_F_M_hoo_E_process_kill_v_p_p", reinterpret_cast<void*>(&jit_process_kill)},
        {"_F_M_hoo_E_process_self_pid_v", reinterpret_cast<void*>(&jit_process_self_pid)},
        {"_F_M_hoo_E_process_capture_v_p", reinterpret_cast<void*>(&jit_process_capture)},

        // Compression module (instance-based, prefix-style)
        {"_F_M_hoo_E_compression_new_v", reinterpret_cast<void*>(&jit_compression_new)},
        {"_F_M_hoo_E_compression_release_v", reinterpret_cast<void*>(&jit_compression_release)},
        {"_F_M_hoo_E_compression_gzipCompress_v_p_p", reinterpret_cast<void*>(&jit_compression_gzip_compress)},
        {"_F_M_hoo_E_compression_gzipDecompress_v_p_p", reinterpret_cast<void*>(&jit_compression_gzip_decompress)},
        {"_F_M_hoo_E_compression_deflateCompress_v_p_p", reinterpret_cast<void*>(&jit_compression_deflate_compress)},
        {"_F_M_hoo_E_compression_deflateDecompress_v_p_p", reinterpret_cast<void*>(&jit_compression_deflate_decompress)},
        // Args module (prefix-based instance methods)
        {"_F_M_hoo_E_args_new_v", reinterpret_cast<void*>(&jit_args_new)},
        {"_F_M_hoo_E_args_count_v", reinterpret_cast<void*>(&jit_args_count)},
        {"_F_M_hoo_E_args_get_v_p", reinterpret_cast<void*>(&jit_args_get)},
        {"_F_M_hoo_E_args_has_v_p", reinterpret_cast<void*>(&jit_args_has)},
        {"_F_M_hoo_E_args_value_v_p", reinterpret_cast<void*>(&jit_args_value)},
        {"_F_M_hoo_E_args_programName_v", reinterpret_cast<void*>(&jit_args_program_name)},
        // Args argparse-style API
        {"_F_M_hoo_E_args_addString_v_p_p_p_p_p", reinterpret_cast<void*>(&jit_args_add_string)},
        {"_F_M_hoo_E_args_addInt_v_p_p_p_p_p", reinterpret_cast<void*>(&jit_args_add_int)},
        {"_F_M_hoo_E_args_addFlag_v_p_p_p_p", reinterpret_cast<void*>(&jit_args_add_flag)},
        {"_F_M_hoo_E_args_addFloat_v_p_p_p_p_p", reinterpret_cast<void*>(&jit_args_add_float)},
        {"_F_M_hoo_E_args_addPositional_v_p_p", reinterpret_cast<void*>(&jit_args_add_positional)},
        {"_F_M_hoo_E_args_parse_v", reinterpret_cast<void*>(&jit_args_parse)},
        {"_F_M_hoo_E_args_getString_v_p", reinterpret_cast<void*>(&jit_args_get_string)},
        {"_F_M_hoo_E_args_getInt_v_p", reinterpret_cast<void*>(&jit_args_get_int)},
        {"_F_M_hoo_E_args_getBool_v_p", reinterpret_cast<void*>(&jit_args_get_bool)},
        {"_F_M_hoo_E_args_getFloat_v_p", reinterpret_cast<void*>(&jit_args_get_float)},
        {"_F_M_hoo_E_args_helpText_v", reinterpret_cast<void*>(&jit_args_help_text)},
        {"_F_M_hoo_E_args_clear_v", reinterpret_cast<void*>(&jit_args_clear)},
        // Net module
        {"_F_M_hoo_E_net_url_new_v_p", reinterpret_cast<void*>(&jit_net_url_new)},
        {"_F_M_hoo_E_net_url_get_scheme_v_p", reinterpret_cast<void*>(&jit_net_url_get_scheme)},
        {"_F_M_hoo_E_net_url_get_host_v_p", reinterpret_cast<void*>(&jit_net_url_get_host)},
        {"_F_M_hoo_E_net_url_get_port_v_p", reinterpret_cast<void*>(&jit_net_url_get_port)},
        {"_F_M_hoo_E_net_url_get_path_v_p", reinterpret_cast<void*>(&jit_net_url_get_path)},
        {"_F_M_hoo_E_net_url_get_query_v_p", reinterpret_cast<void*>(&jit_net_url_get_query)},
        {"_F_M_hoo_E_net_url_get_fragment_v_p", reinterpret_cast<void*>(&jit_net_url_get_fragment)},
        {"_F_M_hoo_E_net_url_to_string_v_p", reinterpret_cast<void*>(&jit_net_url_to_string)},
        {"_F_M_hoo_E_net_url_release_v_p", reinterpret_cast<void*>(&jit_net_url_release)},
        // Net module (camelCase aliases)
        {"_F_M_hoo_E_net_url_getScheme_v_p", reinterpret_cast<void*>(&jit_net_url_get_scheme)},
        {"_F_M_hoo_E_net_url_getHost_v_p", reinterpret_cast<void*>(&jit_net_url_get_host)},
        {"_F_M_hoo_E_net_url_getPort_v_p", reinterpret_cast<void*>(&jit_net_url_get_port)},
        {"_F_M_hoo_E_net_url_getPath_v_p", reinterpret_cast<void*>(&jit_net_url_get_path)},
        {"_F_M_hoo_E_net_url_getQuery_v_p", reinterpret_cast<void*>(&jit_net_url_get_query)},
        {"_F_M_hoo_E_net_url_getFragment_v_p", reinterpret_cast<void*>(&jit_net_url_get_fragment)},
        {"_F_M_hoo_E_net_url_toString_v_p", reinterpret_cast<void*>(&jit_net_url_to_string)},
        // Instance-call aliases where "this" is implicit and not part of the mangled parameter list.
        {"_F_M_hoo_E_net_url_getScheme_v", reinterpret_cast<void*>(&jit_net_url_get_scheme)},
        {"_F_M_hoo_E_net_url_getHost_v", reinterpret_cast<void*>(&jit_net_url_get_host)},
        {"_F_M_hoo_E_net_url_getPort_v", reinterpret_cast<void*>(&jit_net_url_get_port)},
        {"_F_M_hoo_E_net_url_getPath_v", reinterpret_cast<void*>(&jit_net_url_get_path)},
        {"_F_M_hoo_E_net_url_getQuery_v", reinterpret_cast<void*>(&jit_net_url_get_query)},
        {"_F_M_hoo_E_net_url_getFragment_v", reinterpret_cast<void*>(&jit_net_url_get_fragment)},
        {"_F_M_hoo_E_net_url_toString_v", reinterpret_cast<void*>(&jit_net_url_to_string)},
        {"_F_M_hoo_E_net_url_release_v", reinterpret_cast<void*>(&jit_net_url_release)},
        {"_F_M_hoo_E_net_http_client_new_v", reinterpret_cast<void*>(&jit_net_http_client_new)},
        {"_F_M_hoo_E_net_http_client_set_header_v_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_header)},
        {"_F_M_hoo_E_net_http_client_set_timeout_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_timeout)},
        {"_F_M_hoo_E_net_http_client_get_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_get)},
        {"_F_M_hoo_E_net_http_client_post_v_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_post)},
        {"_F_M_hoo_E_net_http_client_put_v_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_put)},
        {"_F_M_hoo_E_net_http_client_delete_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_delete)},
        // Net module (camelCase aliases)
        {"_F_M_hoo_E_net_http_client_setHeader_v_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_header)},
        {"_F_M_hoo_E_net_http_client_setTimeout_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_timeout)},
        {"_F_M_hoo_E_net_http_client_setHeader_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_header)},
        {"_F_M_hoo_E_net_http_client_setTimeout_v_p", reinterpret_cast<void*>(&jit_net_http_client_set_timeout)},
        {"_F_M_hoo_E_net_http_client_get_v_p", reinterpret_cast<void*>(&jit_net_http_client_get)},
        {"_F_M_hoo_E_net_http_client_post_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_post)},
        {"_F_M_hoo_E_net_http_client_put_v_p_p", reinterpret_cast<void*>(&jit_net_http_client_put)},
        {"_F_M_hoo_E_net_http_client_delete_v_p", reinterpret_cast<void*>(&jit_net_http_client_delete)},
        {"_F_M_hoo_E_net_http_response_get_status_code_v_p", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_net_http_response_get_body_v_p", reinterpret_cast<void*>(&jit_net_http_response_get_body)},
        {"_F_M_hoo_E_net_http_response_is_success_v_p", reinterpret_cast<void*>(&jit_net_http_response_is_success)},
        {"_F_M_hoo_E_net_http_response_release_v_p", reinterpret_cast<void*>(&jit_net_http_response_release)},
        {"_F_M_hoo_E_net_http_client_release_v_p", reinterpret_cast<void*>(&jit_net_http_client_release)},
        // Net module Http camelCase aliases
        {"_F_M_hoo_E_net_http_response_statusCode_v_p", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_net_http_response_getStatusCode_v_p", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_net_http_response_getBody_v_p", reinterpret_cast<void*>(&jit_net_http_response_get_body)},
        {"_F_M_hoo_E_net_http_response_isSuccess_v_p", reinterpret_cast<void*>(&jit_net_http_response_is_success)},
        {"_F_M_hoo_E_net_http_response_statusCode_v", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_net_http_response_getStatusCode_v", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_net_http_response_getBody_v", reinterpret_cast<void*>(&jit_net_http_response_get_body)},
        {"_F_M_hoo_E_net_http_response_isSuccess_v", reinterpret_cast<void*>(&jit_net_http_response_is_success)},
        {"_F_M_hoo_E_net_http_response_release_v", reinterpret_cast<void*>(&jit_net_http_response_release)},
        {"_F_M_hoo_E_net_http_client_release_v", reinterpret_cast<void*>(&jit_net_http_client_release)},
        // URL class methods
        {"_F_M_hoo_E_URL_new_static_p_p", reinterpret_cast<void*>(&jit_net_url_new)},
        {"_F_M_hoo_E_URL_scheme_p", reinterpret_cast<void*>(&jit_net_url_get_scheme)},
        {"_F_M_hoo_E_URL_host_p", reinterpret_cast<void*>(&jit_net_url_get_host)},
        {"_F_M_hoo_E_URL_port_i8", reinterpret_cast<void*>(&jit_net_url_get_port)},
        {"_F_M_hoo_E_URL_path_p", reinterpret_cast<void*>(&jit_net_url_get_path)},
        {"_F_M_hoo_E_URL_query_p", reinterpret_cast<void*>(&jit_net_url_get_query)},
        {"_F_M_hoo_E_URL_fragment_p", reinterpret_cast<void*>(&jit_net_url_get_fragment)},
        {"_F_M_hoo_E_URL_to_string_p", reinterpret_cast<void*>(&jit_net_url_to_string)},
        {"_F_M_hoo_E_URL_release_v", reinterpret_cast<void*>(&jit_net_url_release)},
        // HttpResponse class methods
        {"_F_M_hoo_E_HttpResponse_status_code_i8", reinterpret_cast<void*>(&jit_net_http_response_get_status_code)},
        {"_F_M_hoo_E_HttpResponse_status_text_p", reinterpret_cast<void*>(&jit_net_http_response_get_status_text)},
        {"_F_M_hoo_E_HttpResponse_body_p", reinterpret_cast<void*>(&jit_net_http_response_get_body)},
        {"_F_M_hoo_E_HttpResponse_is_success_i8", reinterpret_cast<void*>(&jit_net_http_response_is_success)},
        {"_F_M_hoo_E_HttpResponse_release_v", reinterpret_cast<void*>(&jit_net_http_response_release)},
        // HttpClient class methods
        {"_F_M_hoo_E_HttpClient_new_static_p", reinterpret_cast<void*>(&jit_net_http_client_new)},
        {"_F_M_hoo_E_HttpClient_set_header_i8_p_p", reinterpret_cast<void*>(&jit_net_http_client_set_header)},
        {"_F_M_hoo_E_HttpClient_set_timeout_v_p", reinterpret_cast<void*>(&jit_net_http_client_set_timeout)},
        {"_F_M_hoo_E_HttpClient_get_p_p", reinterpret_cast<void*>(&jit_net_http_client_get)},
        {"_F_M_hoo_E_HttpClient_post_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_post)},
        {"_F_M_hoo_E_HttpClient_put_p_p_p", reinterpret_cast<void*>(&jit_net_http_client_put)},
        {"_F_M_hoo_E_HttpClient_delete_p_p", reinterpret_cast<void*>(&jit_net_http_client_delete)},
        {"_F_M_hoo_E_HttpClient_release_v", reinterpret_cast<void*>(&jit_net_http_client_release)},
        // Process class methods
        {"_F_M_hoo_E_Process_self_pid_static_i8", reinterpret_cast<void*>(&jit_process_self_pid)},
        // CamelCase aliases
        {"_F_M_hoo_E_Process_selfPid_static_i8", reinterpret_cast<void*>(&jit_process_self_pid)},
        {"_F_M_hoo_E_Process_capture_static_p_p", reinterpret_cast<void*>(&jit_process_capture)},
        {"_F_M_hoo_E_Process_kill_static_i8_p_p", reinterpret_cast<void*>(&jit_process_kill)},
        // Console class methods
        {"_F_M_hoo_E_Console_print_static_v_p", reinterpret_cast<void*>(&jit_hoo_print)},
        {"_F_M_hoo_E_Console_println_static_v_p", reinterpret_cast<void*>(&jit_hoo_println)},
        {"_F_M_hoo_E_Console_readline_static_p", reinterpret_cast<void*>(&jit_hoo_readline)},
        {"_F_M_hoo_E_Console_readchar_static_i8", reinterpret_cast<void*>(&jit_hoo_readchar)},
        // JSON free functions
        {"_F_M_hoo_E_json_serialize_hashmap_p_p", reinterpret_cast<void*>(&jit_json_serialize_hashmap)},
        {"_F_M_hoo_E_json_serialize_anyarray_p_p", reinterpret_cast<void*>(&jit_json_serialize_anyarray)},
        {"_F_M_hoo_E_json_deserialize_hashmap_p_p", reinterpret_cast<void*>(&jit_json_deserialize_hashmap)},
        {"_F_M_hoo_E_json_deserialize_anyarray_p_p", reinterpret_cast<void*>(&jit_json_deserialize_anyarray)},
        {"_F_M_hoo_E_json_minify_p_p", reinterpret_cast<void*>(&jit_json_minify)},
        {"_F_M_hoo_E_json_beautify_p_p", reinterpret_cast<void*>(&jit_json_beautify)},
    };
}

void* lookupPlainRuntimeSymbolAddress(const std::string& name) {
    static const std::unordered_map<std::string, void*> kPlainRuntimeSymbols{
        {"hoo_alloc", reinterpret_cast<void*>(&hoo_alloc)},
        {"hoo_retain", reinterpret_cast<void*>(&hoo_retain)},
        {"hoo_release", reinterpret_cast<void*>(&hoo_release)},
        {"hoo_get_refcount", reinterpret_cast<void*>(&hoo_get_refcount)},
        {"hoo_get_type_id", reinterpret_cast<void*>(&hoo_get_type_id)},
        {"hoo_string_from_cstr", reinterpret_cast<void*>(&hoo_string_from_cstr)},
        {"hoo_string_from_int64", reinterpret_cast<void*>(&hoo_string_from_int64)},
        {"hoo_string_from_double", reinterpret_cast<void*>(&hoo_string_from_double)},
        {"hoo_string_concat", reinterpret_cast<void*>(&hoo_string_concat)},
        {"hoo_string_length", reinterpret_cast<void*>(&hoo_string_length)},
        {"hoo_string_data", reinterpret_cast<void*>(&hoo_string_data)},
        {"hoo_string_to_characters", reinterpret_cast<void*>(&hoo_string_to_characters)},
        {"hoo_array_new", reinterpret_cast<void*>(&hoo_array_new)},
        {"hoo_array_push_int64", reinterpret_cast<void*>(&hoo_array_push_int64)},
        {"hoo_array_get_int64", reinterpret_cast<void*>(&hoo_array_get_int64)},
        {"hoo_map_new", reinterpret_cast<void*>(&hoo_map_new)},
        {"hoo_map_retain", reinterpret_cast<void*>(&hoo_map_retain)},
        {"hoo_map_release", reinterpret_cast<void*>(&hoo_map_release)},
        {"hoo_map_count", reinterpret_cast<void*>(&hoo_map_count)},
        {"hoo_map_is_empty", reinterpret_cast<void*>(&hoo_map_is_empty)},
        {"hoo_map_key_type", reinterpret_cast<void*>(&hoo_map_key_type)},
        {"hoo_map_value_type", reinterpret_cast<void*>(&hoo_map_value_type)},
        {"hoo_map_contains_key", reinterpret_cast<void*>(&hoo_map_contains_key)},
        {"hoo_map_set", reinterpret_cast<void*>(&hoo_map_set)},
        {"hoo_map_try_get", reinterpret_cast<void*>(&hoo_map_try_get)},
        {"hoo_map_remove", reinterpret_cast<void*>(&hoo_map_remove)},
        {"hoo_map_clear", reinterpret_cast<void*>(&hoo_map_clear)},
        {"hoo_anyarray_new", reinterpret_cast<void*>(&hoo_anyarray_new)},
        {"hoo_anyarray_new_capacity", reinterpret_cast<void*>(&hoo_anyarray_new_capacity)},
        {"hoo_anyarray_length", reinterpret_cast<void*>(&hoo_anyarray_length)},
        {"hoo_anyarray_push", reinterpret_cast<void*>(&hoo_anyarray_push)},
        {"hoo_anyarray_set", reinterpret_cast<void*>(&hoo_anyarray_set)},
        {"hoo_anyarray_get", reinterpret_cast<void*>(&hoo_anyarray_get)},
        {"hoo_anyarray_pop", reinterpret_cast<void*>(&hoo_anyarray_pop)},
        {"hoo_anyarray_clear", reinterpret_cast<void*>(&hoo_anyarray_clear)},
        {"hoo_hashmap_new", reinterpret_cast<void*>(&hoo_hashmap_new)},
        {"hoo_hashmap_count", reinterpret_cast<void*>(&hoo_hashmap_count)},
        {"hoo_hashmap_set_fixed_i8", reinterpret_cast<void*>(&hoo_hashmap_set_fixed_i8)},
        {"hoo_hashmap_get_fixed_i8", reinterpret_cast<void*>(&hoo_hashmap_get_fixed_i8)},
        {"hoo_hashmap_set_any_i8", reinterpret_cast<void*>(&hoo_hashmap_set_any_i8)},
        {"hoo_hashmap_get_any_i8", reinterpret_cast<void*>(&hoo_hashmap_get_any_i8)},
        {"hoo_hashmap_remove_i8", reinterpret_cast<void*>(&hoo_hashmap_remove_i8)},
        {"hoo_hashmap_clear", reinterpret_cast<void*>(&hoo_hashmap_clear)},
        {"hoo_buffer_new", reinterpret_cast<void*>(&hoo_buffer_new)},
        {"hoo_buffer_from_bytes", reinterpret_cast<void*>(&hoo_buffer_from_bytes)},
        {"hoo_csv_from_opts", reinterpret_cast<void*>(&hoo_csv_from_opts)},
        {"hoo_buffer_copy", reinterpret_cast<void*>(&hoo_buffer_copy)},
        {"hoo_buffer_length", reinterpret_cast<void*>(&hoo_buffer_length)},
        {"hoo_buffer_capacity", reinterpret_cast<void*>(&hoo_buffer_capacity)},
        {"hoo_buffer_byte_at", reinterpret_cast<void*>(&hoo_buffer_byte_at)},
        {"hoo_buffer_set_byte", reinterpret_cast<void*>(&hoo_buffer_set_byte)},
        {"hoo_buffer_append", reinterpret_cast<void*>(&hoo_buffer_append)},
        {"hoo_buffer_append_buffer", reinterpret_cast<void*>(&hoo_buffer_append_buffer)},
        {"hoo_buffer_clear", reinterpret_cast<void*>(&hoo_buffer_clear)},
        {"hoo_buffer_slice", reinterpret_cast<void*>(&hoo_buffer_slice)},
        {"hoo_buffer_data", reinterpret_cast<void*>(&hoo_buffer_data)},
        {"hoo_buffer_retain", reinterpret_cast<void*>(&hoo_buffer_retain)},
        {"hoo_buffer_release", reinterpret_cast<void*>(&hoo_buffer_release)},
    };
    auto it = kPlainRuntimeSymbols.find(name);
    return it == kPlainRuntimeSymbols.end() ? nullptr : it->second;
}

bool validateRuntimeSymbolMap(const std::vector<RuntimeSymbolContract>& runtimeSymbols) {
    if (runtimeSymbols.empty()) {
        return false;
    }
    std::unordered_set<std::string> required{
        "_F_hoo_alloc_p_i8_i8",
        "_F_hoo_retain_p_p",
        "_F_hoo_release_v_p",
        "_F_hoo_push_handler_v_p",
        "_F_hoo_pop_handler_v",
        "_F_hoo_throw_v_p",
        "_F_hoo_rethrow_v",
    };
    for (const auto& rs : runtimeSymbols) {
        if (!rs.name || rs.addr == nullptr) continue;
        required.erase(rs.name);
    }
    return required.empty();
}
} // namespace

HVMJIT::HVMJIT(IOProvider& io)
    : tsc_(std::make_unique<llvm::LLVMContext>())
    , io_(io)
    , sourceCompiler_(std::make_unique<HooCompiler>()) {
    memory_.resize(16 * 1024 * 1024, 0); // 16 MB initial virtual memory.
    memoryTop_ = 0x10000;                // keep low page unmapped-like.
    hvm_set_memory_base(memory_.data()); // expose base to extern "C" helpers
}

HVMJIT::~HVMJIT() {
    if (jit_ && jitDebugListener_) {
        if (auto* rtLayer = llvm::dyn_cast<llvm::orc::RTDyldObjectLinkingLayer>(&jit_->getObjLinkingLayer())) {
            rtLayer->unregisterJITEventListener(*jitDebugListener_);
        }
    }
}
std::string HVMJIT::canonicalizePath(const std::string& path) const {
    if (path.empty()) {
        return path;
    }
    std::error_code ec;
    fs::path p(path);
    fs::path normalized = p.lexically_normal();
    if (normalized.is_absolute()) {
        return normalized.string();
    }
    fs::path abs = fs::absolute(normalized, ec);
    if (ec) {
        return normalized.string();
    }
    return abs.lexically_normal().string();
}

bool HVMJIT::setLoaderState(const std::string& moduleName, LoaderState state) {
    if (moduleName.empty()) {
        return false;
    }
    auto it = moduleStates_.find(moduleName);
    if (it == moduleStates_.end()) {
        moduleStates_[moduleName] = state;
        return true;
    }
    if (state == LoaderState::Failed) {
        it->second = state;
        return true;
    }
    if (static_cast<int>(state) < static_cast<int>(it->second)) {
        return false;
    }
    it->second = state;
    return true;
}

void HVMJIT::setError(ErrorPhase phase, ErrorCode code, const std::string& message,
                      const std::string& moduleName, const std::string& symbolName,
                      const std::string& path) {
    lastError_ = message;
    lastErrorInfo_ = ErrorInfo{phase, code, moduleName, symbolName, path, message};
}

bool HVMJIT::ensureJIT() {
    if (jit_) {
        return true;
    }

    // Initialize native target for JIT execution (only once per process)
    static std::once_flag initFlag;
    std::call_once(initFlag, []() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
    });

    auto jitExpected = llvm::orc::LLJITBuilder().create();
    if (!jitExpected) {
        setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                 "Failed to create LLVM LLJIT instance");
        return false;
    }
    jit_ = std::move(*jitExpected);

    // Expose JITed symbols/objects to host debuggers when supported by the
    // underlying ORC object linking layer.
    if (auto* rtLayer = llvm::dyn_cast<llvm::orc::RTDyldObjectLinkingLayer>(&jit_->getObjLinkingLayer())) {
#ifndef _WIN32
        if (llvm::JITEventListener* gdbListener = llvm::JITEventListener::createGDBRegistrationListener()) {
            jitDebugListener_.reset(gdbListener);
            rtLayer->registerJITEventListener(*jitDebugListener_);
        }
#else
        (void)rtLayer;
#endif
    }

    auto processSymbols = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        jit_->getDataLayout().getGlobalPrefix());
    if (!processSymbols) {
        setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                 "Failed to create process symbol generator");
        return false;
    }
    jit_->getMainJITDylib().addGenerator(std::move(*processSymbols));
    return true;
}

bool HVMJIT::isSourcePath(const std::string& path) const {
    return fs::path(path).extension() == ".hoo";
}

bool HVMJIT::isBytecodePath(const std::string& path) const {
    return fs::path(path).extension() == ".ho";
}

std::string HVMJIT::moduleNameToPath(const std::string& moduleName) const {
    auto parts = splitModulePath(moduleName);
    if (parts.empty()) {
        return moduleName;
    }

    fs::path p;
    for (const auto& part : parts) {
        p /= part;
    }
    p += ".ho";
    return p.generic_string();
}

bool HVMJIT::loadInput(const std::string& pathOrModuleName) {
    clearError();

    if (isSourcePath(pathOrModuleName)) {
        return loadSource(pathOrModuleName);
    }
    if (isBytecodePath(pathOrModuleName)) {
        return loadBytecode(pathOrModuleName);
    }

    if (loadBytecode(pathOrModuleName)) {
        return true;
    }

    const std::string bytecodeError = lastError_;
    clearError();
    const std::string asModulePath = moduleNameToPath(pathOrModuleName);
    if (loadBytecode(asModulePath)) {
        return true;
    }

    setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
             "Failed to load input '" + pathOrModuleName + "'. " + bytecodeError,
             "", "", pathOrModuleName);
    return false;
}

bool HVMJIT::loadSource(const std::string& sourcePath) {
    clearError();

    const auto source = io_.readFile(sourcePath);
    if (!source.has_value()) {
        setError(ErrorPhase::Parse, ErrorCode::IoReadFailed,
                 "Unable to read source file: " + sourcePath, "", "", sourcePath);
        return false;
    }

    const std::string moduleName = fs::path(sourcePath).stem().string();
    return loadSourceCode(moduleName, *source);
}

bool HVMJIT::loadSourceCode(const std::string& moduleName, const std::string& sourceCode) {
    clearError();

    if (sourceCode.empty()) {
        setError(ErrorPhase::Parse, ErrorCode::ParseFailed,
                 "Source code is empty", moduleName, "", "");
        return false;
    }

    auto module = sourceCompiler_->compile(moduleName, sourceCode);
    if (!module) {
        setError(ErrorPhase::Parse, ErrorCode::ParseFailed,
                 "Failed to compile source to HVM: " + sourceCompiler_->getLastError(),
                 moduleName, "", "");
        return false;
    }

    return loadModule(std::move(module));
}

bool HVMJIT::parseAndLoadModuleFromPath(const std::string& path, std::shared_ptr<hvm::HOModule>& outModule) {
    const std::string canonicalPath = canonicalizePath(path);
    const auto bytes = io_.readBinaryFile(path);
    if (!bytes.has_value()) {
        setError(ErrorPhase::Parse, ErrorCode::IoReadFailed,
                 "Unable to read module bytes: " + path, "", "", canonicalPath);
        return false;
    }

    auto parsed = hvm::HOModule::parse(*bytes);
    if (!parsed) {
        setError(ErrorPhase::Parse, ErrorCode::ParseFailed,
                 "Failed to parse module: " + path, "", "", canonicalPath);
        return false;
    }

    parsed->setSourcePath(canonicalPath);
    outModule = std::shared_ptr<hvm::HOModule>(parsed.release());
    setLoaderState(outModule->getName(), LoaderState::Parsed);
    if (!validateModule(*outModule, path)) {
        setLoaderState(outModule->getName(), LoaderState::Failed);
        return false;
    }
    setLoaderState(outModule->getName(), LoaderState::Validated);
    return true;
}

bool HVMJIT::loadBytecode(const std::string& modulePath) {
    clearError();
    std::shared_ptr<hvm::HOModule> parsed;
    if (!parseAndLoadModuleFromPath(modulePath, parsed)) {
        return false;
    }
    if (parsed->getName().empty()) {
        parsed->setName(fs::path(modulePath).stem().string());
    }
    return loadModule(std::unique_ptr<hvm::HOModule>(new hvm::HOModule(*parsed)));
}

bool HVMJIT::validateModule(const hvm::HOModule& module, const std::string& sourcePath) {
    if (module.getMagic() != hvm::HOModule::MAGIC ||
        module.getVersionMajor() != hvm::HOModule::VERSION_MAJOR ||
        module.getVersionMinor() != hvm::HOModule::VERSION_MINOR) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidHeader,
                 "Invalid module header/version for: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }
    if (module.getEndianness() != hvm::Endianness::Little) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidHeader,
                 "Unsupported endianness in module: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }
    if (module.getPointerSize() != 8) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidHeader,
                 "Unsupported pointer size in module: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }

    const hvm::Section* text = module.getSection(".text");
    if (!text || text->data.empty()) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidSection,
                 "Missing or empty .text section: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }
    const uint32_t textRequired = hvm::SectionFlags::ALLOC | hvm::SectionFlags::EXECUTE;
    if ((text->flags & textRequired) != textRequired) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidSection,
                 ".text section must be ALLOC|EXECUTE: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }
    if (text->virtual_size != 0 && text->virtual_size < text->data.size()) {
        setError(ErrorPhase::Validate, ErrorCode::InvalidSection,
                 "Invalid .text virtual size: " + sourcePath,
                 module.getName(), "", sourcePath);
        setLoaderState(module.getName(), LoaderState::Failed);
        return false;
    }

    const uint64_t textSize = static_cast<uint64_t>(text->data.size());
    for (const auto& sym : module.getSymbols()) {
        if (sym.type == hvm::Symbol::STT_FUNC && sym.value >= textSize) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidSymbol,
                     "Function symbol offset out of .text bounds: " + sym.name,
                     module.getName(), sym.name, sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }

    const size_t symbolCount = module.getSymbols().size();
    for (const auto& exp : module.getExports()) {
        if (exp.symbol_index >= symbolCount) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidMetadata,
                     "Export symbol index out of range: " + exp.name,
                     module.getName(), exp.name, sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }
    for (const auto& fm : module.getFunctionMetadata()) {
        if (fm.symbol_index >= symbolCount) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidMetadata,
                     "Function metadata symbol index out of range: " + fm.name,
                     module.getName(), fm.name, sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }
    for (const auto& imp : module.getImports()) {
        if (imp.library.empty()) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidMetadata,
                     "Import library field cannot be empty for symbol: " + imp.name,
                     module.getName(), imp.name, sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }

    if (const hvm::Section* data = module.getSection(".data")) {
        if ((data->flags & hvm::SectionFlags::WRITE) == 0) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidSection,
                     ".data section must be writable: " + sourcePath,
                     module.getName(), "", sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }
    if (const hvm::Section* rodata = module.getSection(".rodata")) {
        if ((rodata->flags & hvm::SectionFlags::WRITE) != 0) {
            setError(ErrorPhase::Validate, ErrorCode::InvalidSection,
                     ".rodata section must not be writable: " + sourcePath,
                     module.getName(), "", sourcePath);
            setLoaderState(module.getName(), LoaderState::Failed);
            return false;
        }
    }

    return true;
}

std::optional<std::string> HVMJIT::resolveImportModulePath(
    const std::shared_ptr<hvm::HOModule>& importer,
    const std::string& importModuleName) const {
    if (importModuleName.empty()) {
        return std::nullopt;
    }

    std::vector<std::string> candidates;
    candidates.push_back(moduleNameToPath(importModuleName));

    if (importer && !importer->getSourcePath().empty()) {
        fs::path source(importer->getSourcePath());
        fs::path dir = source.parent_path();
        if (!dir.empty()) {
            candidates.push_back((dir / moduleNameToPath(importModuleName)).string());
        }
    }

    for (const auto& path : candidates) {
        auto bytes = io_.readBinaryFile(path);
        if (bytes.has_value()) {
            return path;
        }
    }
    return std::nullopt;
}

void HVMJIT::buildModuleRegistryEntry(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) {
        return;
    }
    ModuleRegistryEntry entry;
    for (const auto& s : module->getSymbols()) {
        entry.symbolsByName[s.name] = s;
    }
    for (const auto& e : module->getExports()) {
        entry.exportsByName[e.name] = e;
    }
    entry.imports = module->getImports();
    for (const auto& fm : module->getFunctionMetadata()) {
        entry.functionMetaByName[fm.name] = fm;
    }
    moduleRegistry_[module->getName()] = std::move(entry);
}

bool HVMJIT::registerModuleInBundle(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) {
        setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                 "Cannot register null module");
        return false;
    }

    const std::string& moduleName = module->getName();
    if (moduleName.empty()) {
        setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                 "Module has empty name");
        return false;
    }

    if (!bundle_.hasModule(moduleName)) {
        bundle_.addModule(module);
    }

    for (const auto& sym : module->getSymbols()) {
        hvm::SymbolType kind = hvm::SymbolType::NoType;
        switch (sym.type) {
            case hvm::Symbol::STT_FUNC: kind = hvm::SymbolType::Function; break;
            case hvm::Symbol::STT_OBJECT: kind = hvm::SymbolType::Object; break;
            case hvm::Symbol::STT_TYPE: kind = hvm::SymbolType::Type; break;
            case hvm::Symbol::STT_TLS: kind = hvm::SymbolType::TLS; break;
            default: break;
        }

        const std::string mangled = sym.name;
        bundle_.registerExport(moduleName, sym.name, mangled, kind);
    }
    buildModuleRegistryEntry(module);
    functionNameByOffset_[moduleName].clear();
    for (const auto& sym : module->getSymbols()) {
        if (sym.type == hvm::Symbol::STT_FUNC && sym.section_index >= 0) {
            functionNameByOffset_[moduleName][sym.value] = sym.name;
        }
    }

    return true;
}

bool HVMJIT::resolveAndLoadDependencies(const hvm::HOModule& root) {
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::vector<std::string> visitPath;

    std::function<bool(const std::shared_ptr<hvm::HOModule>&)> visit = [&](const std::shared_ptr<hvm::HOModule>& module) {
        if (!module) {
            setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                     "Dependency traversal found null module");
            return false;
        }
        const std::string name = module->getName();
        if (visited.count(name)) {
            return true;
        }
        if (visiting.count(name)) {
            auto beginIt = std::find(visitPath.begin(), visitPath.end(), name);
            std::string cycle;
            if (beginIt != visitPath.end()) {
                for (auto it = beginIt; it != visitPath.end(); ++it) {
                    if (!cycle.empty()) cycle += " -> ";
                    cycle += *it;
                }
                cycle += " -> " + name;
            } else {
                cycle = name;
            }
            setError(ErrorPhase::Resolve, ErrorCode::CircularDependency,
                     "Circular dependency detected: " + cycle, name);
            return false;
        }
        visiting.insert(name);
        visitPath.push_back(name);
        setLoaderState(name, LoaderState::Registered);

        for (const auto& imp : module->getImports()) {
            if (imp.library.rfind("hoo.", 0) == 0 || imp.library == "hoo") {
                continue;
            }
            if (isNativeImport(imp)) {
                continue;
            }

            const std::string depModuleName = imp.library;
            if (depModuleName.empty()) {
                continue;
            }

            std::shared_ptr<hvm::HOModule> depModule;
            auto loadedIt = loadedModules_.find(depModuleName);
            if (loadedIt != loadedModules_.end()) {
                depModule = loadedIt->second;
            } else {
                const auto depPathOpt = resolveImportModulePath(module, depModuleName);
                if (!depPathOpt.has_value()) {
                    setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                             "Unable to resolve dependency module: " + depModuleName,
                             module->getName(), imp.name, depModuleName);
                    return false;
                }
                if (!parseAndLoadModuleFromPath(*depPathOpt, depModule)) {
                    return false;
                }
                if (depModule->getName().empty()) {
                    depModule->setName(depModuleName);
                }
                setLoaderState(depModule->getName(), LoaderState::Discovered);
                loadedModules_[depModule->getName()] = depModule;
                if (!registerModuleInBundle(depModule)) {
                    return false;
                }
            }

            module->addDependency(depModule->getName(), hvm::ModuleType::Compiled, false);
            auto& deps = moduleDependencies_[module->getName()];
            if (std::find(deps.begin(), deps.end(), depModule->getName()) == deps.end()) {
                deps.push_back(depModule->getName());
            }
            if (!visit(depModule)) {
                return false;
            }
        }

        visiting.erase(name);
        if (!visitPath.empty()) visitPath.pop_back();
        visited.insert(name);
        setLoaderState(name, LoaderState::DependenciesResolved);
        return true;
    };

    auto rootIt = loadedModules_.find(root.getName());
    if (rootIt == loadedModules_.end()) {
        setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                 "Root module not tracked: " + root.getName(), root.getName());
        return false;
    }
    return visit(rootIt->second);
}

bool HVMJIT::initializeDependencyGraphPostOrder() {
    auto ordered = bundle_.resolveDependencyOrder();
    std::vector<std::shared_ptr<hvm::HOModuleBase>> allModules;
    allModules.reserve(ordered.size());
    for (const auto& mod : ordered) {
        allModules.push_back(mod);
    }

    for (const auto& mod : ordered) {
        mod->resolveDependencyOrder(allModules);
    }

    for (const auto& module : ordered) {
        if (!module) {
            continue;
        }
        const std::string& moduleName = module->getName();
        if (initializedModules_.count(moduleName)) {
            continue;
        }
        initializedModules_.insert(moduleName);
        setLoaderState(moduleName, LoaderState::Ready);
    }
    buildLogicalSearchOrder();
    return true;
}

void HVMJIT::buildLogicalSearchOrder() {
    moduleSearchOrder_.clear();
    for (const auto& [moduleName, _] : loadedModules_) {
        std::vector<std::string> order;
        order.push_back(moduleName);
        auto depIt = moduleDependencies_.find(moduleName);
        if (depIt != moduleDependencies_.end()) {
            for (const auto& dep : depIt->second) {
                if (std::find(order.begin(), order.end(), dep) == order.end()) {
                    order.push_back(dep);
                }
            }
        }
        order.push_back("hoo");
        order.push_back("__process__");
        moduleSearchOrder_[moduleName] = std::move(order);
    }
}

bool HVMJIT::bootstrapRuntimeModules() {
    if (bundle_.hasModule("hoo")) {
        return true;
    }
    const auto runtimeSymbols = buildRuntimeSymbols();
    if (!validateRuntimeSymbolMap(runtimeSymbols)) {
        setError(ErrorPhase::Initialize, ErrorCode::RuntimeBootstrapFailed,
                 "Mandatory runtime intrinsic symbols are unavailable");
        return false;
    }

    auto runtime = hvm::StaticHOModule::create("hoo");
    runtime->registerFunction("alloc", reinterpret_cast<void*>(&hoo_alloc), "_F_hoo_alloc_p_i8_i8");
    runtime->registerFunction("retain", reinterpret_cast<void*>(&hoo_retain), "_F_hoo_retain_p_p");
    runtime->registerFunction("release", reinterpret_cast<void*>(&hoo_release), "_F_hoo_release_v_p");
    runtime->registerFunction("get_refcount", reinterpret_cast<void*>(&hoo_get_refcount), "_F_hoo_get_refcount_i8_p");
    runtime->registerFunction("get_type_id", reinterpret_cast<void*>(&hoo_get_type_id), "_F_hoo_get_type_id_i8_p");

    runtime->registerFunction("string_from_cstr", reinterpret_cast<void*>(&hoo_string_from_cstr),
                              "_F_M_hoo_E_String_fromCStr_static_p_p");
    runtime->registerFunction("string_from_int64", reinterpret_cast<void*>(&hoo_string_from_int64),
                              "_F_M_hoo_E_String_fromInt64_static_p_i8");
    runtime->registerFunction("string_from_double", reinterpret_cast<void*>(&hoo_string_from_double),
                              "_F_M_hoo_E_String_fromDouble_static_p_d");
    runtime->registerFunction("string_concat", reinterpret_cast<void*>(&hoo_string_concat),
                              "_F_M_hoo_E_String_concat_p_p");
    runtime->registerFunction("string_length", reinterpret_cast<void*>(&hoo_string_length),
                              "_F_M_hoo_E_String_length_i8");
    runtime->registerFunction("string_data", reinterpret_cast<void*>(&hoo_string_data),
                              "_F_M_hoo_E_String_data_p");
    runtime->registerFunction("string_to_characters", reinterpret_cast<void*>(&hoo_string_to_characters),
                              "_F_M_hoo_E_String_toCharacters_p");
    runtime->registerFunction("string_join", reinterpret_cast<void*>(&hoo_string_join),
                              "_F_M_hoo_E_String_join_static_p_p");
    runtime->registerFunction("string_from_object", reinterpret_cast<void*>(&hoo_string_from_object),
                              "_F_M_hoo_E_String_fromObject_static_p_p");
    runtime->registerFunction("string_from_any", reinterpret_cast<void*>(&hoo_string_from_any),
                              "_F_M_hoo_E_String_fromAny_static_p_i8_i8");
    runtime->registerFunction("string_new", reinterpret_cast<void*>(&hoo_string_new),
                              "_F_M_hoo_E_String_new_static_p");
    runtime->registerFunction("string_is_empty", reinterpret_cast<void*>(&hoo_string_is_empty),
                              "_F_M_hoo_E_String_isEmpty_i8");
    runtime->registerFunction("string_to_lower", reinterpret_cast<void*>(&hoo_string_to_lower),
                              "_F_M_hoo_E_String_toLower_p");
    runtime->registerFunction("string_equals", reinterpret_cast<void*>(&hoo_string_equals),
                              "_F_M_hoo_E_String_equals_i8_p");
    runtime->registerFunction("string_contains", reinterpret_cast<void*>(&hoo_string_contains),
                              "_F_M_hoo_E_String_contains_i8_p");
    runtime->registerFunction("string_starts_with", reinterpret_cast<void*>(&hoo_string_starts_with),
                              "_F_M_hoo_E_String_startsWith_i8_p");
    runtime->registerFunction("string_trim", reinterpret_cast<void*>(&hoo_string_trim),
                              "_F_M_hoo_E_String_trim_p");
    runtime->registerFunction("string_repeat", reinterpret_cast<void*>(&hoo_string_repeat),
                              "_F_M_hoo_E_String_repeat_static_p_p_p");
    runtime->registerFunction("string_index_of", reinterpret_cast<void*>(&hoo_string_index_of),
                              "_F_M_hoo_E_String_indexOf_i8_p");
    // String class redirect names (snake_case)
    runtime->registerFunction("string_from_cstr_snake", reinterpret_cast<void*>(&hoo_string_from_cstr),
                              "_F_M_hoo_E_String_from_cstr_static_p_p");
    runtime->registerFunction("string_from_int64_snake", reinterpret_cast<void*>(&hoo_string_from_int64),
                              "_F_M_hoo_E_String_from_int64_static_p_i8");
    runtime->registerFunction("string_from_double_snake", reinterpret_cast<void*>(&hoo_string_from_double),
                              "_F_M_hoo_E_String_from_double_static_p_d");
    runtime->registerFunction("string_from_any_snake", reinterpret_cast<void*>(&hoo_string_from_any),
                              "_F_M_hoo_E_String_from_any_static_p_i8_i8");
    runtime->registerFunction("string_from_object_snake", reinterpret_cast<void*>(&hoo_string_from_object),
                              "_F_M_hoo_E_String_from_object_static_p_p");
    runtime->registerFunction("string_is_empty_snake", reinterpret_cast<void*>(&hoo_string_is_empty),
                              "_F_M_hoo_E_String_is_empty_i8");
    runtime->registerFunction("string_to_upper_snake", reinterpret_cast<void*>(&hoo_string_to_upper),
                              "_F_M_hoo_E_String_to_upper_p");
    runtime->registerFunction("string_to_lower_snake", reinterpret_cast<void*>(&hoo_string_to_lower),
                              "_F_M_hoo_E_String_to_lower_p");
    runtime->registerFunction("string_starts_with_snake", reinterpret_cast<void*>(&hoo_string_starts_with),
                              "_F_M_hoo_E_String_starts_with_i8_p");
    runtime->registerFunction("string_index_of_snake", reinterpret_cast<void*>(&hoo_string_index_of),
                              "_F_M_hoo_E_String_index_of_i8_p");
    runtime->registerFunction("string_to_characters_snake", reinterpret_cast<void*>(&hoo_string_to_characters),
                              "_F_M_hoo_E_String_to_characters_p");
    runtime->registerFunction("character_from_utf8", reinterpret_cast<void*>(&hoo_character_from_utf8),
                              "_F_hoo_Character_from_utf8_p_p_i8");
    runtime->registerFunction("character_from_codepoint", reinterpret_cast<void*>(&hoo_character_from_codepoint),
                              "_F_hoo_Character_from_codepoint_p_i8");
    runtime->registerFunction("character_length", reinterpret_cast<void*>(&hoo_character_length),
                              "_F_hoo_Character_length_i8_p");
    runtime->registerFunction("character_data", reinterpret_cast<void*>(&hoo_character_data),
                              "_F_hoo_Character_data_p_p");
    runtime->registerFunction("character_codepoint", reinterpret_cast<void*>(&hoo_character_codepoint),
                              "_F_hoo_Character_codepoint_i8_p");
    runtime->registerFunction("print", reinterpret_cast<void*>(&hoo_print), "_F_M_hoo_E_print_v_p");
    runtime->registerFunction("println", reinterpret_cast<void*>(&hoo_println), "_F_M_hoo_E_println_v_p");
    runtime->registerFunction("array_new", reinterpret_cast<void*>(&hoo_array_new), "_F_hoo_Array_new_p");
    runtime->registerFunction("array_push_int64", reinterpret_cast<void*>(&hoo_array_push_int64),
                              "_F_hoo_Array_push_i8_p_i8");
    runtime->registerFunction("array_get_int64", reinterpret_cast<void*>(&hoo_array_get_int64),
                              "_F_hoo_Array_get_i8_p_i8_p");
    runtime->registerFunction("map_new", reinterpret_cast<void*>(&hoo_map_new), "_F_hoo_Map_new_p_i8");

    runtime->registerFunction("buffer_new", reinterpret_cast<void*>(&hoo_buffer_new), "_F_hoo_Buffer_new_p");
    runtime->registerFunction("buffer_from_bytes", reinterpret_cast<void*>(&hoo_buffer_from_bytes),
                              "_F_hoo_Buffer_from_bytes_p_p_i8");
    runtime->registerFunction("buffer_copy", reinterpret_cast<void*>(&hoo_buffer_copy), "_F_hoo_Buffer_copy_p_p");
    runtime->registerFunction("buffer_length", reinterpret_cast<void*>(&hoo_buffer_length), "_F_hoo_Buffer_length_i8_p");
    runtime->registerFunction("buffer_capacity", reinterpret_cast<void*>(&hoo_buffer_capacity), "_F_hoo_Buffer_capacity_i8_p");
    runtime->registerFunction("buffer_byte_at", reinterpret_cast<void*>(&hoo_buffer_byte_at), "_F_hoo_Buffer_byte_at_i8_p_i8");
    runtime->registerFunction("buffer_set_byte", reinterpret_cast<void*>(&hoo_buffer_set_byte), "_F_hoo_Buffer_set_byte_i8_p_i8_i8");
    runtime->registerFunction("buffer_append", reinterpret_cast<void*>(&hoo_buffer_append), "_F_hoo_Buffer_append_p_p_p_i8");
    runtime->registerFunction("buffer_append_buffer", reinterpret_cast<void*>(&hoo_buffer_append_buffer), "_F_hoo_Buffer_append_buffer_p_p_p");
    runtime->registerFunction("buffer_clear", reinterpret_cast<void*>(&hoo_buffer_clear), "_F_hoo_Buffer_clear_i8_p");
    runtime->registerFunction("buffer_slice", reinterpret_cast<void*>(&hoo_buffer_slice), "_F_hoo_Buffer_slice_p_p_i8_i8");
    runtime->registerFunction("buffer_data", reinterpret_cast<void*>(&hoo_buffer_data), "_F_hoo_Buffer_data_p_p");
    runtime->registerFunction("buffer_retain", reinterpret_cast<void*>(&hoo_buffer_retain), "_F_hoo_Buffer_retain_p_p");
    runtime->registerFunction("buffer_release", reinterpret_cast<void*>(&hoo_buffer_release), "_F_hoo_Buffer_release_v_p");

    runtime->registerFunction("push_handler", reinterpret_cast<void*>(&hoo_push_handler), "_F_hoo_push_handler_v_p");
    runtime->registerFunction("pop_handler", reinterpret_cast<void*>(&hoo_pop_handler), "_F_hoo_pop_handler_v");
    runtime->registerFunction("throw", reinterpret_cast<void*>(&hoo_exception_throw), "_F_hoo_throw_v_p");
    runtime->registerFunction("rethrow", reinterpret_cast<void*>(&hoo_exception_rethrow), "_F_hoo_rethrow_v");

    bundle_.addModule(runtime);
    return true;
}

bool HVMJIT::registerRuntimeSymbolsInJITDylib() {
    if (!jit_) {
        return false;
    }
    auto it = moduleDylibs_.find("hoo");
    if (it == moduleDylibs_.end() || !it->second) {
        return false;
    }

    llvm::orc::MangleAndInterner mangle(jit_->getExecutionSession(), jit_->getDataLayout());
    llvm::orc::SymbolMap symbols;
    for (const auto& rs : buildRuntimeSymbols()) {
        if (!rs.name || rs.addr == nullptr) {
            continue;
        }
        symbols[mangle(rs.name)] =
            llvm::orc::ExecutorSymbolDef(llvm::orc::ExecutorAddr::fromPtr(rs.addr), llvm::JITSymbolFlags::Exported);
    }

    if (auto err = it->second->define(llvm::orc::absoluteSymbols(symbols))) {
        setError(ErrorPhase::Initialize, ErrorCode::RuntimeBootstrapFailed,
                 "Failed to define runtime symbols in hoo JITDylib");
        return false;
    }
    return true;
}

bool HVMJIT::isNativeImport(const hvm::ImportEntry& imp) const {
    if (imp.import_type == hvm::ImportEntry::IT_NATIVE) {
        return true;
    }
    if (imp.library == "__process__") {
        return true;
    }
    return imp.library.find(".so") != std::string::npos ||
           imp.library.find(".dylib") != std::string::npos ||
           imp.library.find(".dll") != std::string::npos;
}

bool HVMJIT::preloadNativeLibrariesFromImports() {
    for (const auto& [moduleName, module] : loadedModules_) {
        if (!module) {
            continue;
        }
        for (const auto& imp : module->getImports()) {
            if (!isNativeImport(imp) || imp.library.empty() || imp.library == "__process__") {
                continue;
            }
            if (loadedNativeLibraries_.count(imp.library)) {
                continue;
            }
            if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(imp.library.c_str())) {
                setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                         "Failed to load native library: " + imp.library,
                         moduleName, imp.name, imp.library);
                return false;
            }
            loadedNativeLibraries_.insert(imp.library);
        }
    }
    return true;
}

bool HVMJIT::resolveNativeImportSymbol(const hvm::ImportEntry& imp, const std::string& importerModuleName,
                                       uint64_t* outAddr) {
    if (!isNativeImport(imp)) {
        return false;
    }
    if (imp.name.empty()) {
        return true;
    }
    if (!imp.library.empty() && imp.library != "__process__" && !loadedNativeLibraries_.count(imp.library)) {
        if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(imp.library.c_str())) {
            setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                     "Failed to load native library for symbol resolution: " + imp.library,
                     importerModuleName, imp.name, imp.library);
            return false;
        }
        loadedNativeLibraries_.insert(imp.library);
    }
    void* addr = llvm::sys::DynamicLibrary::SearchForAddressOfSymbol(imp.name.c_str());
    if (!addr) {
        setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                 "Native symbol not found: " + imp.name,
                 importerModuleName, imp.name, imp.library);
        return false;
    }
    if (outAddr) {
        *outAddr = reinterpret_cast<uint64_t>(addr);
    }
    return true;
}

bool HVMJIT::configureJITDylibs() {
    if (!jit_) {
        return false;
    }
    if (!preloadNativeLibrariesFromImports()) {
        return false;
    }

    moduleDylibs_.clear();
    for (const auto& [moduleName, _] : loadedModules_) {
        if (moduleName.empty()) {
            continue;
        }
        if (moduleName == "__process__") {
            continue;
        }
        auto jdExpected = jit_->createJITDylib(moduleName);
        if (!jdExpected) {
            auto* existing = jit_->getExecutionSession().getJITDylibByName(moduleName);
            if (!existing) {
                setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                         "Failed to create/retrieve JITDylib for module: " + moduleName,
                         moduleName);
                return false;
            }
            moduleDylibs_[moduleName] = existing;
        } else {
            moduleDylibs_[moduleName] = &(*jdExpected);
        }
    }

    auto hooExpected = jit_->createJITDylib("hoo");
    if (!hooExpected) {
        auto* existing = jit_->getExecutionSession().getJITDylibByName("hoo");
        if (!existing) {
            setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                     "Failed to create/retrieve hoo JITDylib", "hoo");
            return false;
        }
        moduleDylibs_["hoo"] = existing;
    } else {
        moduleDylibs_["hoo"] = &(*hooExpected);
    }

    for (const auto& [moduleName, order] : moduleSearchOrder_) {
        auto jdIt = moduleDylibs_.find(moduleName);
        if (jdIt == moduleDylibs_.end() || !jdIt->second) {
            continue;
        }
        auto* from = jdIt->second;
        auto processSymbols = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            jit_->getDataLayout().getGlobalPrefix());
        if (!processSymbols) {
            setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                     "Failed to create process symbol generator for module dylib: " + moduleName,
                     moduleName);
            return false;
        }
        from->addGenerator(std::move(*processSymbols));

        for (const auto& targetName : order) {
            if (targetName == moduleName || targetName == "__process__") {
                continue;
            }
            auto toIt = moduleDylibs_.find(targetName);
            if (toIt == moduleDylibs_.end() || !toIt->second) {
                continue;
            }
            from->addToLinkOrder(*toIt->second);
        }
    }

    if (!registerRuntimeSymbolsInJITDylib()) {
        return false;
    }

    return true;
}

bool HVMJIT::hasExportedOrDefinedSymbol(const std::string& moduleName, const std::string& symbolName) const {
    auto modIt = loadedModules_.find(moduleName);
    if (modIt == loadedModules_.end()) {
        return false;
    }
    const auto& mod = modIt->second;
    if (mod->getSymbol(symbolName) != nullptr) {
        return true;
    }
    auto regIt = moduleRegistry_.find(moduleName);
    if (regIt == moduleRegistry_.end()) {
        return false;
    }
    return regIt->second.exportsByName.find(symbolName) != regIt->second.exportsByName.end();
}

bool HVMJIT::validateImportsAgainstDependencies() {
    for (const auto& [moduleName, module] : loadedModules_) {
        if (!module) {
            continue;
        }
        for (const auto& imp : module->getImports()) {
            if (imp.library.rfind("hoo.", 0) == 0 || imp.library == "hoo") {
                continue;
            }
            if (isNativeImport(imp)) {
                if (!resolveNativeImportSymbol(imp, moduleName, nullptr)) {
                    return false;
                }
                continue;
            }
            if (!loadedModules_.count(imp.library)) {
                setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                         "Imported module not loaded: " + imp.library,
                         moduleName, imp.name, imp.library);
                return false;
            }
            if (!imp.name.empty() && !hasExportedOrDefinedSymbol(imp.library, imp.name)) {
                setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                         "Imported symbol not found in dependency module: " + imp.name,
                         moduleName, imp.name, imp.library);
                return false;
            }
        }
    }
    return true;
}

void HVMJIT::rollbackModuleLoad(const std::string& moduleName) {
    if (moduleName.empty()) {
        return;
    }
    loadedModules_.erase(moduleName);
    moduleRegistry_.erase(moduleName);
    moduleCanonicalPathByName_.erase(moduleName);
    moduleDependencies_.erase(moduleName);
    moduleSearchOrder_.erase(moduleName);
    moduleDylibs_.erase(moduleName);
    moduleLayouts_.erase(moduleName);
    moduleStates_[moduleName] = LoaderState::Failed;
    initializedModules_.erase(moduleName);
    {
        std::lock_guard<std::mutex> lk(moduleInitOnceMu_);
        moduleInitOnceFlags_.erase(moduleName);
    }
    {
        std::lock_guard<std::mutex> lk(moduleVTableOnceMu_);
        moduleVTableOnceFlags_.erase(moduleName);
    }
    for (auto it = initializedVTableClasses_.begin(); it != initializedVTableClasses_.end();) {
        if (it->rfind(moduleName + "::", 0) == 0) {
            it = initializedVTableClasses_.erase(it);
        } else {
            ++it;
        }
    }
    bundle_.removeModule(moduleName);
}

bool HVMJIT::loadU64(uint64_t addr, uint64_t& out) const {
    if (addr > memory_.size() || addr + 8 > memory_.size()) {
        return false;
    }
    out = 0;
    for (int i = 0; i < 8; ++i) {
        out |= static_cast<uint64_t>(memory_[addr + i]) << (8U * i);
    }
    return true;
}

bool HVMJIT::isSupportedForIRLowering(hvm::Opcode op, uint16_t func) const {
    switch (op) {
        case hvm::Opcode::NOP:
        case hvm::Opcode::MOV:
        case hvm::Opcode::MOVZ:
        case hvm::Opcode::LUI:
        case hvm::Opcode::ADDI:
        case hvm::Opcode::NOT:
        case hvm::Opcode::BEQ:
        case hvm::Opcode::BNE:
        case hvm::Opcode::BLT:
        case hvm::Opcode::BLE:
        case hvm::Opcode::JMP:
        case hvm::Opcode::JAL:
        case hvm::Opcode::JALR:
        case hvm::Opcode::RET:
        case hvm::Opcode::LD_B:
        case hvm::Opcode::LD_BU:
        case hvm::Opcode::LD_H:
        case hvm::Opcode::LD_HU:
        case hvm::Opcode::LD_W:
        case hvm::Opcode::LD_WU:
        case hvm::Opcode::LD_D:
        case hvm::Opcode::ST_B:
        case hvm::Opcode::ST_H:
        case hvm::Opcode::ST_W:
        case hvm::Opcode::ST_D:
        case hvm::Opcode::ENTER:
        case hvm::Opcode::LEAVE:
        case hvm::Opcode::ADJSP:
        case hvm::Opcode::FRAME:
        case hvm::Opcode::PUSH:
        case hvm::Opcode::POP:
        case hvm::Opcode::LDA:
        case hvm::Opcode::CALL:
        case hvm::Opcode::TAILCALL:
        case hvm::Opcode::SYSCALL:
        case hvm::Opcode::BREAK:
            return true;
        case hvm::Opcode::ARITH:
            return func == 0 || func == 1 || func == 2 || func == 5 || func == 6 || func == 7;
        case hvm::Opcode::FLOAT_ARITH:
            return func <= 3;
        case hvm::Opcode::SHIFT:
            return func <= 2;
        case hvm::Opcode::LOGIC:
            return func <= 2;
        case hvm::Opcode::CMP:
            return func <= 3;
        case hvm::Opcode::FCMP:
            return func <= 2;
        default:
            return false;
    }
}

bool HVMJIT::storeU64(uint64_t addr, uint64_t value) {
    if (addr > memory_.size() || addr + 8 > memory_.size()) {
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        memory_[addr + i] = static_cast<uint8_t>((value >> (8U * i)) & 0xFFU);
    }
    return true;
}

const hvm::Symbol* HVMJIT::findFunctionSymbol(const hvm::HOModule& module, const std::string& functionName) const {
    const auto& symbols = module.getSymbols();
    auto isFunc = [](const hvm::Symbol& s) { return s.type == hvm::Symbol::STT_FUNC; };
    const auto candidates = buildLookupCandidates(functionName, module.getName());

    // 1. Exact match.
    for (const auto& candidate : candidates) {
        for (const auto& sym : symbols) {
            if (isFunc(sym) && sym.name == candidate && sym.section_index != -1) {
                return &sym;
            }
        }
    }

    // 2. Prefix match.
    for (const auto& candidate : candidates) {
        if (candidate.empty()) {
            continue;
        }
        std::string prefix = candidate.rfind("_F_", 0) == 0
            ? candidate + "_"
            : "_F_" + candidate + "_";
        for (const auto& sym : symbols) {
            if (isFunc(sym) && sym.name.rfind(prefix, 0) == 0 && sym.section_index != -1) {
                return &sym;
            }
        }
    }

    // 3. Fuzzy containment fallback.
    const std::string baseName = extractLegacyBaseFunctionName(functionName);
    for (const auto& sym : symbols) {
        if (isFunc(sym) && sym.name.find(baseName) != std::string::npos && sym.section_index != -1) {
            return &sym;
        }
    }

    return nullptr;
}

bool HVMJIT::mapModuleSections(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) {
        return false;
    }
    const std::string& name = module->getName();
    if (moduleLayouts_.find(name) != moduleLayouts_.end()) {
        return true;
    }

    ModuleMemoryLayout layout;
    auto alignUp = [](uint64_t x, uint64_t a) {
        return (x + (a - 1)) & ~(a - 1);
    };
    memoryTop_ = alignUp(memoryTop_, 16);

    auto mapSection = [&](const char* secName, uint64_t& base, uint64_t& size, bool writable) -> bool {
        const hvm::Section* sec = module->getSection(secName);
        if (!sec) {
            base = 0;
            size = 0;
            return true;
        }
        size = static_cast<uint64_t>(sec->data.size());
        if (size == 0 && sec->virtual_size > 0) {
            size = sec->virtual_size;
        }
        if (size == 0) {
            base = 0;
            return true;
        }

        memoryTop_ = alignUp(memoryTop_, std::max<uint64_t>(16, sec->alignment));
        base = memoryTop_;
        if (base + size >= memory_.size()) {
            lastError_ = "Out of virtual memory while mapping section " + std::string(secName) +
                         " of module " + name;
            return false;
        }
        if (!sec->data.empty()) {
            std::copy(sec->data.begin(), sec->data.end(), memory_.begin() + static_cast<size_t>(base));
        } else if (writable) {
            std::fill(memory_.begin() + static_cast<size_t>(base),
                      memory_.begin() + static_cast<size_t>(base + size), 0);
        }
        memoryTop_ = base + size;
        return true;
    };

    if (!mapSection(".text", layout.textBase, layout.textSize, false)) return false;
    if (!mapSection(".rodata", layout.rodataBase, layout.rodataSize, false)) return false;
    if (!mapSection(".data", layout.dataBase, layout.dataSize, true)) return false;
    if (!mapSection(".bss", layout.bssBase, layout.bssSize, true)) return false;

    moduleLayouts_[name] = layout;
    return true;
}

bool HVMJIT::invokeStateAbiSymbol(const std::string& symbolName, HVMState& state, uint64_t& outValue) {
    using StateAbiFn = uint64_t(*)(void*);
    for (const auto& candidate : buildLookupCandidates(symbolName, "hoo")) {
        for (const auto& rs : buildRuntimeSymbols()) {
            if (rs.name && candidate == rs.name && rs.addr) {
                auto fn = reinterpret_cast<StateAbiFn>(rs.addr);
                try {
                    outValue = fn(&state);
                    return true;
                } catch (const std::exception& ex) {
                    setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                             "Runtime bridge exception in " + candidate + ": " + ex.what());
                    return false;
                } catch (...) {
                    setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                             "Runtime bridge exception in " + candidate);
                    return false;
                }
            }
        }
    }
    return false;
}

int64_t HVMJIT::executeFunction(const std::shared_ptr<hvm::HOModule>& module, const std::string& functionName, HVMState& state) {
    if (!module) {
        lastError_ = "Null module in executeFunction";
        return -1;
    }
    auto layoutIt = moduleLayouts_.find(module->getName());
    if (layoutIt == moduleLayouts_.end()) {
        lastError_ = "Module layout not found for " + module->getName();
        return -1;
    }
    const ModuleMemoryLayout& layout = layoutIt->second;
    const hvm::Section* text = module->getSection(".text");
    if (!text) {
        lastError_ = "Module has no .text section: " + module->getName();
        return -1;
    }

    const hvm::Symbol* sym = findFunctionSymbol(*module, functionName);
    if (!sym) {
        uint64_t runtimeRet = 0;
        if (invokeStateAbiSymbol(functionName, state, runtimeRet)) {
            return static_cast<int64_t>(runtimeRet);
        }
        if (hasError()) {
            return -1;
        }
        // Cross-module resolution: search all loaded modules
        for (const auto& [name, mod] : loadedModules_) {
            if (mod.get() == module.get()) continue;
            sym = findFunctionSymbol(*mod, functionName);
            if (sym) {
                return executeFunction(mod, functionName, state);
            }
        }
        lastError_ = "Function symbol not found: " + functionName;
        return -1;
    }

    // Imported symbols (section_index == -1) need cross-module resolution
    if (sym->section_index == -1) {
        uint64_t runtimeRet = 0;
        if (invokeStateAbiSymbol(functionName, state, runtimeRet)) {
            return static_cast<int64_t>(runtimeRet);
        }
        if (hasError()) {
            return -1;
        }
        for (const auto& [name, mod] : loadedModules_) {
            if (mod.get() == module.get()) continue;
            const hvm::Symbol* realSym = findFunctionSymbol(*mod, functionName);
            if (realSym && realSym->section_index != -1) {
                return executeFunction(mod, functionName, state);
            }
        }
        lastError_ = "Import symbol not resolved: " + functionName;
        return -1;
    }

    uint64_t pc = sym->value;
    const uint64_t textSize = text->data.size();
    const bool arcUseDefEnabled = isArcUseDefGraphEnabled();
    const ARCUseDefGraph arcUseDef = arcUseDefEnabled
                                         ? buildARCUseDefGraph(*text, pc)
                                         : ARCUseDefGraph{};
    stopExecutionRequested_.store(false, std::memory_order_relaxed);

    while (pc < textSize) {
        if (stopExecutionRequested_.load(std::memory_order_relaxed)) {
            lastError_ = "Execution stopped by inspector";
            std::lock_guard<std::mutex> lk(lastRegistersMu_);
            for (size_t i = 0; i < 32; ++i) lastRegisters_[i] = state.regs[i];
            return -1;
        }
        std::vector<uint8_t> slice;
        const size_t maxRead = static_cast<size_t>(std::min<uint64_t>(8, textSize - pc));
        slice.insert(slice.end(), text->data.begin() + static_cast<ptrdiff_t>(pc),
                     text->data.begin() + static_cast<ptrdiff_t>(pc + maxRead));
        size_t used = 0;
        auto ins = hvm::HVMInstruction::decode(slice, used);
        if (!ins || used == 0) {
            lastError_ = "Failed to decode instruction at PC=" + std::to_string(pc);
            return -1;
        }

        auto readReg = [&](uint8_t r) -> uint64_t {
            if (r == 0) return 0;
            return static_cast<uint64_t>(state.regs[r]);
        };
        auto writeReg = [&](uint8_t r, uint64_t v) {
            if (r == 0) return;
            state.regs[r] = static_cast<int64_t>(v);
        };
        auto regAsF64 = [&](uint8_t r) -> double {
            const uint64_t bits = readReg(r);
            double out = 0.0;
            std::memcpy(&out, &bits, sizeof(out));
            return out;
        };
        auto f64AsBits = [&](double v) -> uint64_t {
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(bits));
            return bits;
        };

        const uint64_t nextPc = pc + used;
        if (arcUseDefEnabled && arcUseDef.skipPc.count(pc)) {
            pc = nextPc;
            continue;
        }
        bool jumped = false;
        captureInspectorSnapshot(state, pc, module->getName(), functionName, ins->getMnemonic(), false);

        switch (ins->getOpcode()) {
            case hvm::Opcode::NOP: break;
            case hvm::Opcode::MOV: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                writeReg(o.rd, readReg(o.rs1));
                break;
            }
            case hvm::Opcode::MOVZ: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                writeReg(o.rd, readReg(o.rs) | static_cast<uint16_t>(o.imm15));
                break;
            }
            case hvm::Opcode::LUI: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                writeReg(o.rd, readReg(o.rs) | (static_cast<uint64_t>(static_cast<uint16_t>(o.imm15)) << 49U));
                break;
            }
            case hvm::Opcode::ADDI: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                writeReg(o.rd, readReg(o.rs) + static_cast<int64_t>(o.imm15));
                break;
            }
            case hvm::Opcode::NOT: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                writeReg(o.rd, ~readReg(o.rs1));
                break;
            }
            case hvm::Opcode::ARITH: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const int64_t a = static_cast<int64_t>(readReg(o.rs1));
                const int64_t b = static_cast<int64_t>(readReg(o.rs2));
                switch (o.func) {
                    case 0: writeReg(o.rd, static_cast<uint64_t>(a + b)); break;
                    case 1: writeReg(o.rd, static_cast<uint64_t>(a - b)); break;
                    case 2: writeReg(o.rd, static_cast<uint64_t>(a * b)); break;
                    case 5:
                        if (b == 0) { lastError_ = "Division by zero"; return -1; }
                        writeReg(o.rd, static_cast<uint64_t>(a / b)); break;
                    case 6: {
                        const uint64_t ua = readReg(o.rs1), ub = readReg(o.rs2);
                        if (ub == 0) { lastError_ = "Unsigned division by zero"; return -1; }
                        writeReg(o.rd, ua / ub); break;
                    }
                    case 7:
                        if (b == 0) { lastError_ = "Modulo by zero"; return -1; }
                        writeReg(o.rd, static_cast<uint64_t>(a % b)); break;
                    default:
                        lastError_ = "Unsupported ARITH func: " + std::to_string(o.func);
                        return -1;
                }
                break;
            }
            case hvm::Opcode::SHIFT: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const uint64_t a = readReg(o.rs1);
                const uint64_t sh = readReg(o.rs2) & 63ULL;
                switch (o.func) {
                    case 0: writeReg(o.rd, a << sh); break;
                    case 1: writeReg(o.rd, a >> sh); break;
                    case 2: writeReg(o.rd, static_cast<uint64_t>(static_cast<int64_t>(a) >> sh)); break;
                    default:
                        lastError_ = "Unsupported SHIFT func: " + std::to_string(o.func);
                        return -1;
                }
                break;
            }
            case hvm::Opcode::LOGIC: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const uint64_t a = readReg(o.rs1), b = readReg(o.rs2);
                switch (o.func) {
                    case 0: writeReg(o.rd, a & b); break;
                    case 1: writeReg(o.rd, a | b); break;
                    case 2: writeReg(o.rd, a ^ b); break;
                    default:
                        lastError_ = "Unsupported LOGIC func: " + std::to_string(o.func);
                        return -1;
                }
                break;
            }
            case hvm::Opcode::CMP: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const int64_t a = static_cast<int64_t>(readReg(o.rs1));
                const int64_t b = static_cast<int64_t>(readReg(o.rs2));
                switch (o.func) {
                    case 0: writeReg(o.rd, a == b ? 1 : 0); break;
                    case 1: writeReg(o.rd, a != b ? 1 : 0); break;
                    case 2: writeReg(o.rd, a < b ? 1 : 0); break;
                    case 3: writeReg(o.rd, a <= b ? 1 : 0); break;
                    default:
                        lastError_ = "Unsupported CMP func: " + std::to_string(o.func);
                        return -1;
                }
                break;
            }
            case hvm::Opcode::FLOAT_ARITH: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const double a = regAsF64(o.rs1);
                const double b = regAsF64(o.rs2);
                double out = 0.0;
                switch (o.func) {
                    case 0: out = a + b; break;
                    case 1: out = a - b; break;
                    case 2: out = a * b; break;
                    case 3: out = a / b; break;
                    default:
                        lastError_ = "Unsupported FLOAT_ARITH func: " + std::to_string(o.func);
                        return -1;
                }
                writeReg(o.rd, f64AsBits(out));
                break;
            }
            case hvm::Opcode::FCMP: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                const double a = regAsF64(o.rs1);
                const double b = regAsF64(o.rs2);
                switch (o.func) {
                    case 0: writeReg(o.rd, a == b ? 1 : 0); break;
                    case 1: writeReg(o.rd, a < b ? 1 : 0); break;
                    case 2: writeReg(o.rd, a <= b ? 1 : 0); break;
                    default:
                        lastError_ = "Unsupported FCMP func: " + std::to_string(o.func);
                        return -1;
                }
                break;
            }
            case hvm::Opcode::BEQ:
            case hvm::Opcode::BNE:
            case hvm::Opcode::BLT:
            case hvm::Opcode::BLE: {
                auto o = std::get<hvm::OperandsB>(ins->getOperands());
                const int64_t a = static_cast<int64_t>(readReg(o.rs1));
                const int64_t b = static_cast<int64_t>(readReg(o.rs2));
                bool cond = false;
                if (ins->getOpcode() == hvm::Opcode::BEQ) cond = (a == b);
                else if (ins->getOpcode() == hvm::Opcode::BNE) cond = (a != b);
                else if (ins->getOpcode() == hvm::Opcode::BLT) cond = (a < b);
                else cond = (a <= b);
                if (cond) {
                    if (static_cast<int64_t>(o.imm15) < 0 &&
                        stopExecutionRequested_.load(std::memory_order_relaxed)) {
                        lastError_ = "Execution stopped by inspector";
                        return -1;
                    }
                    pc = static_cast<uint64_t>(static_cast<int64_t>(pc) + static_cast<int64_t>(o.imm15) * 4);
                    jumped = true;
                }
                break;
            }
            case hvm::Opcode::JMP: {
                auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                if (static_cast<int64_t>(o.offset) < 0 &&
                    stopExecutionRequested_.load(std::memory_order_relaxed)) {
                    lastError_ = "Execution stopped by inspector";
                    return -1;
                }
                pc = static_cast<uint64_t>(static_cast<int64_t>(pc) + static_cast<int64_t>(o.offset) * 4);
                jumped = true;
                break;
            }
            case hvm::Opcode::JAL:
            case hvm::Opcode::CALL: {
                if (stopExecutionRequested_.load(std::memory_order_relaxed)) {
                    lastError_ = "Execution stopped by inspector";
                    return -1;
                }
                auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                writeReg(o.rd, nextPc);
                const uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(pc) + static_cast<int64_t>(o.offset) * 4);
                const std::string calleeName = [&]() -> std::string {
                    for (const auto& s : module->getSymbols()) {
                        if ((s.type == hvm::Symbol::STT_FUNC || s.type == hvm::Symbol::STT_NOTYPE) &&
                            s.value == target) return s.name;
                    }
                    return "";
                }();
                if (calleeName.empty()) {
                    lastError_ = "CALL target unresolved at PC=" + std::to_string(pc);
                    return -1;
                }
                // Check if the callee is an import (undefined) symbol and resolve it
                // via the runtime ABI table first to avoid incorrect fuzzy matching in
                // executeFunction's findFunctionSymbol.
                {
                    const hvm::Symbol* calleeSym = nullptr;
                    for (const auto& s : module->getSymbols()) {
                        if (s.name == calleeName) { calleeSym = &s; break; }
                    }
                    if (calleeSym && calleeSym->section_index == -1) {
                        uint64_t runtimeRet = 0;
                        if (invokeStateAbiSymbol(calleeName, state, runtimeRet)) {
                            writeReg(1, runtimeRet);
                            break;
                        }
                        if (hasError()) return -1;
                    }
                }
                int64_t rv = -1;
                try {
                    rv = executeFunction(module, calleeName, state);
                } catch (const std::exception& ex) {
                    lastError_ = "CALL bridge exception: " + std::string(ex.what());
                    return -1;
                } catch (...) {
                    lastError_ = "CALL bridge exception";
                    return -1;
                }
                if (rv == -1 && !lastError_.empty()) return -1;
                writeReg(1, static_cast<uint64_t>(rv));
                break;
            }
            case hvm::Opcode::JALR: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                writeReg(o.rd, nextPc);
                const uint64_t targetPc =
                    static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15));
                const hvm::Symbol* maybeCalleeSym = [&]() -> const hvm::Symbol* {
                    for (const auto& s : module->getSymbols()) {
                        if ((s.type == hvm::Symbol::STT_FUNC || s.type == hvm::Symbol::STT_NOTYPE) &&
                            s.value == targetPc) {
                            return &s;
                        }
                    }
                    return nullptr;
                }();
                const bool devirtEligible =
                    maybeCalleeSym && isSpecDevirtualizableName(maybeCalleeSym->name) &&
                    maybeCalleeSym->section_index != -1;
                if (devirtEligible) {
                    if (stopExecutionRequested_.load(std::memory_order_relaxed)) {
                        lastError_ = "Execution stopped by inspector";
                        return -1;
                    }
                    int64_t rv = executeFunction(module, maybeCalleeSym->name, state);
                    if (rv == -1 && !lastError_.empty()) {
                        return -1;
                    }
                    writeReg(1, static_cast<uint64_t>(rv));
                } else {
                    pc = targetPc;
                    jumped = true;
                }
                break;
            }
            case hvm::Opcode::TAILCALL: {
                auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                const uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(pc) + static_cast<int64_t>(o.offset) * 4);
                const std::string calleeName = [&]() -> std::string {
                    for (const auto& s : module->getSymbols()) {
                        if ((s.type == hvm::Symbol::STT_FUNC || s.type == hvm::Symbol::STT_NOTYPE) &&
                            s.value == target) return s.name;
                    }
                    return "";
                }();
                if (calleeName.empty()) {
                    lastError_ = "TAILCALL target unresolved at PC=" + std::to_string(pc);
                    return -1;
                }
                try {
                    return executeFunction(module, calleeName, state);
                } catch (const std::exception& ex) {
                    lastError_ = "TAILCALL bridge exception: " + std::string(ex.what());
                    return -1;
                } catch (...) {
                    lastError_ = "TAILCALL bridge exception";
                    return -1;
                }
            }
            case hvm::Opcode::LD_B:
            case hvm::Opcode::LD_BU:
            case hvm::Opcode::LD_H:
            case hvm::Opcode::LD_HU:
            case hvm::Opcode::LD_W:
            case hvm::Opcode::LD_WU: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                const uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15));
                uint64_t v = 0;
                if (ins->getOpcode() == hvm::Opcode::LD_B || ins->getOpcode() == hvm::Opcode::LD_BU) {
                    if (addr >= memory_.size()) {
                        lastError_ = "Invalid LD.B address: " + std::to_string(addr);
                        return -1;
                    }
                    v = memory_[addr];
                    if (ins->getOpcode() == hvm::Opcode::LD_B) {
                        v = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(v)));
                    }
                } else if (ins->getOpcode() == hvm::Opcode::LD_H || ins->getOpcode() == hvm::Opcode::LD_HU) {
                    if (addr + 2 > memory_.size()) {
                        lastError_ = "Invalid LD.H address: " + std::to_string(addr);
                        return -1;
                    }
                    uint16_t h = static_cast<uint16_t>(memory_[addr]) |
                                 static_cast<uint16_t>(static_cast<uint16_t>(memory_[addr + 1]) << 8U);
                    v = (ins->getOpcode() == hvm::Opcode::LD_H)
                            ? static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(h)))
                            : static_cast<uint64_t>(h);
                } else {
                    if (addr + 4 > memory_.size()) {
                        lastError_ = "Invalid LD.W address: " + std::to_string(addr);
                        return -1;
                    }
                    uint32_t w = static_cast<uint32_t>(memory_[addr]) |
                                 (static_cast<uint32_t>(memory_[addr + 1]) << 8U) |
                                 (static_cast<uint32_t>(memory_[addr + 2]) << 16U) |
                                 (static_cast<uint32_t>(memory_[addr + 3]) << 24U);
                    v = (ins->getOpcode() == hvm::Opcode::LD_W)
                            ? static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(w)))
                            : static_cast<uint64_t>(w);
                }
                writeReg(o.rd, v);
                break;
            }
            case hvm::Opcode::RET:
                {
                    std::lock_guard<std::mutex> lk(lastRegistersMu_);
                    for (size_t i = 0; i < 32; ++i) lastRegisters_[i] = state.regs[i];
                }
                return state.regs[1];
            case hvm::Opcode::LD_D: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                const uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15));
                if ((addr & 7ULL) != 0) {
                    lastError_ = "Unaligned LD.D address: " + std::to_string(addr);
                    return -1;
                }
                uint64_t val = 0;
                if (!loadU64(addr, val)) {
                    lastError_ = "Invalid LD.D address: " + std::to_string(addr);
                    return -1;
                }
                writeReg(o.rd, val);
                break;
            }
            case hvm::Opcode::ST_B:
            case hvm::Opcode::ST_H:
            case hvm::Opcode::ST_W: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                const uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15));
                const uint64_t data = readReg(o.rd);
                if (ins->getOpcode() == hvm::Opcode::ST_B) {
                    if (addr >= memory_.size()) {
                        lastError_ = "Invalid ST.B address: " + std::to_string(addr);
                        return -1;
                    }
                    memory_[addr] = static_cast<uint8_t>(data & 0xFFU);
                } else if (ins->getOpcode() == hvm::Opcode::ST_H) {
                    if (addr + 2 > memory_.size()) {
                        lastError_ = "Invalid ST.H address: " + std::to_string(addr);
                        return -1;
                    }
                    memory_[addr] = static_cast<uint8_t>(data & 0xFFU);
                    memory_[addr + 1] = static_cast<uint8_t>((data >> 8U) & 0xFFU);
                } else {
                    if (addr + 4 > memory_.size()) {
                        lastError_ = "Invalid ST.W address: " + std::to_string(addr);
                        return -1;
                    }
                    memory_[addr] = static_cast<uint8_t>(data & 0xFFU);
                    memory_[addr + 1] = static_cast<uint8_t>((data >> 8U) & 0xFFU);
                    memory_[addr + 2] = static_cast<uint8_t>((data >> 16U) & 0xFFU);
                    memory_[addr + 3] = static_cast<uint8_t>((data >> 24U) & 0xFFU);
                }
                break;
            }
            case hvm::Opcode::ST_D: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                const uint64_t addr = static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15));
                if ((addr & 7ULL) != 0) {
                    lastError_ = "Unaligned ST.D address: " + std::to_string(addr);
                    return -1;
                }
                uint64_t oldVal = 0;
                if (!loadU64(addr, oldVal)) {
                    lastError_ = "Invalid ST.D address: " + std::to_string(addr);
                    return -1;
                }
                uint64_t newVal = readReg(o.rd);
                if (newVal == oldVal) {
                    break;
                }
                if (newVal != 0) {
                    hooc_hvm_arc_retain_if_managed(newVal);
                }
                if (!storeU64(addr, newVal)) {
                    lastError_ = "Invalid ST.D address: " + std::to_string(addr);
                    return -1;
                }
                if (oldVal != 0) {
                    hooc_hvm_arc_release_if_managed(oldVal);
                }
                break;
            }
            case hvm::Opcode::LDA: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                if (o.rs == 0 && layout.rodataBase != 0 && o.imm15 >= 0 &&
                    static_cast<uint64_t>(o.imm15) < layout.rodataSize) {
                    writeReg(o.rd, layout.rodataBase + static_cast<uint64_t>(o.imm15));
                } else if (o.rs == 1 && layout.dataBase != 0) {
                    writeReg(o.rd, layout.dataBase + static_cast<uint64_t>(static_cast<int16_t>(o.imm15)));
                } else {
                    writeReg(o.rd, static_cast<uint64_t>(static_cast<int64_t>(readReg(o.rs)) + static_cast<int64_t>(o.imm15)));
                }
                break;
            }
            case hvm::Opcode::ENTER: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                uint64_t sp = static_cast<uint64_t>(state.regs[31]);
                sp -= 16;
                if (!storeU64(sp + 0, static_cast<uint64_t>(state.regs[29])) ||
                    !storeU64(sp + 8, static_cast<uint64_t>(state.regs[30]))) {
                    lastError_ = "Stack write failed in ENTER";
                    return -1;
                }
                state.regs[30] = static_cast<int64_t>(sp);
                sp -= static_cast<int64_t>(o.imm15);
                state.regs[31] = static_cast<int64_t>(sp);
                break;
            }
            case hvm::Opcode::LEAVE: {
                uint64_t sp = static_cast<uint64_t>(state.regs[30]);
                uint64_t lr = 0, fp = 0;
                if (!loadU64(sp + 0, lr) || !loadU64(sp + 8, fp)) {
                    lastError_ = "Stack read failed in LEAVE";
                    return -1;
                }
                state.regs[29] = static_cast<int64_t>(lr);
                state.regs[30] = static_cast<int64_t>(fp);
                state.regs[31] = static_cast<int64_t>(sp + 16);
                break;
            }
            case hvm::Opcode::ADJSP: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                state.regs[31] += static_cast<int64_t>(o.imm15);
                break;
            }
            case hvm::Opcode::FRAME: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                writeReg(o.rd, static_cast<uint64_t>(state.regs[30] + static_cast<int64_t>(o.imm15)));
                break;
            }
            case hvm::Opcode::PUSH: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                state.regs[31] -= 8;
                if (!storeU64(static_cast<uint64_t>(state.regs[31]), readReg(o.rd))) {
                    lastError_ = "Stack write failed in PUSH";
                    return -1;
                }
                break;
            }
            case hvm::Opcode::POP: {
                auto o = std::get<hvm::OperandsR>(ins->getOperands());
                uint64_t v = 0;
                if (!loadU64(static_cast<uint64_t>(state.regs[31]), v)) {
                    lastError_ = "Stack read failed in POP";
                    return -1;
                }
                writeReg(o.rd, v);
                state.regs[31] += 8;
                break;
            }
            case hvm::Opcode::SYSCALL: {
                auto o = std::get<hvm::OperandsI>(ins->getOperands());
                if ((o.imm15 == kSysRetain || o.imm15 == kSysRelease) && nextPc < textSize) {
                    std::vector<uint8_t> nextSlice;
                    size_t nextMaxRead = static_cast<size_t>(std::min<uint64_t>(8, textSize - nextPc));
                    nextSlice.insert(nextSlice.end(), text->data.begin() + static_cast<ptrdiff_t>(nextPc),
                                     text->data.begin() + static_cast<ptrdiff_t>(nextPc + nextMaxRead));
                    size_t nextUsed = 0;
                    auto nextIns = hvm::HVMInstruction::decode(nextSlice, nextUsed);
                    if (nextIns && nextUsed > 0 && nextIns->getOpcode() == hvm::Opcode::SYSCALL) {
                        auto no = std::get<hvm::OperandsI>(nextIns->getOperands());
                        if ((o.imm15 == kSysRetain && no.imm15 == kSysRelease) ||
                            (o.imm15 == kSysRelease && no.imm15 == kSysRetain)) {
                            pc = nextPc + nextUsed;
                            jumped = true;
                            break;
                        }
                    }
                }
                switch (o.imm15) {
                    case kSysAlloc: {
                        writeReg(o.rd, hooc_hvm_sys_alloc(readReg(2), readReg(3)));
                        break;
                    }
                    case kSysRetain: {
                        writeReg(o.rd, hooc_hvm_sys_retain(readReg(2)));
                        break;
                    }
                    case kSysRelease: {
                        writeReg(o.rd, hooc_hvm_sys_release(readReg(2)));
                        break;
                    }
                    case kSysRefcount: {
                        writeReg(o.rd, hooc_hvm_sys_refcount(readReg(2)));
                        break;
                    }
                    case kSysTypeId: {
                        writeReg(o.rd, hooc_hvm_sys_typeid(readReg(2)));
                        break;
                    }
                    case kSysExceptionRuntime: {
                        writeReg(o.rd, hooc_hvm_sys_exception_runtime(0));
                        break;
                    }
                    case kSysPushHandler: {
                        writeReg(o.rd, hooc_hvm_sys_push_handler_state(&state, readReg(2)));
                        break;
                    }
                    case kSysPopHandler: {
                        writeReg(o.rd, hooc_hvm_sys_pop_handler_state(&state));
                        break;
                    }
                    case kSysThrowToHandler: {
                        const uint64_t targetPc = hooc_hvm_sys_throw_to_handler_state(&state, readReg(2));
                        writeReg(o.rd, targetPc);
                        if (targetPc != kNoHandlerPc) {
                            pc = targetPc;
                            jumped = true;
                        } else {
                            lastError_ = "Unhandled exception trap (no registered handler)";
                            return -1;
                        }
                        break;
                    }
                    case kSysRethrowToHandler: {
                        const uint64_t targetPc = hooc_hvm_sys_rethrow_to_handler_state(&state);
                        writeReg(o.rd, targetPc);
                        if (targetPc != kNoHandlerPc) {
                            pc = targetPc;
                            jumped = true;
                        } else {
                            lastError_ = "Unhandled rethrow trap (no registered handler)";
                            return -1;
                        }
                        break;
                    }
                    case kSysStringData: {
                        writeReg(o.rd, hooc_hvm_sys_string_data(readReg(2)));
                        break;
                    }
                    case kSysThreadCreate: {
                        writeReg(o.rd, hooc_hvm_sys_thread_create(readReg(2), readReg(3)));
                        break;
                    }
                    case kSysThreadExit: {
                        hooc_hvm_sys_thread_exit(readReg(2));
                        writeReg(o.rd, 0);
                        break;
                    }
                    case kSysFutex: {
                        writeReg(o.rd, hooc_hvm_sys_futex(readReg(2), readReg(3), readReg(4)));
                        break;
                    }
                    case kSysGetTid: {
                        writeReg(o.rd, hooc_hvm_sys_get_tid());
                        break;
                    }
                    case kSysOpen: {
                        writeReg(o.rd, hooc_hvm_sys_open(readReg(2), readReg(3), readReg(4)));
                        break;
                    }
                    case kSysRead: {
                        writeReg(o.rd, hooc_hvm_sys_read(readReg(2), readReg(3), readReg(4)));
                        break;
                    }
                    case kSysWrite: {
                        writeReg(o.rd, hooc_hvm_sys_write(readReg(2), readReg(3), readReg(4)));
                        break;
                    }
                    case kSysClose: {
                        writeReg(o.rd, hooc_hvm_sys_close(readReg(2)));
                        break;
                    }
                    case kSysLseek: {
                        writeReg(o.rd, hooc_hvm_sys_lseek(readReg(2), readReg(3), readReg(4)));
                        break;
                    }
                    case kSysFstat: {
                        writeReg(o.rd, hooc_hvm_sys_fstat(readReg(2), readReg(3)));
                        break;
                    }
                    case kSysClockGetTime: {
                        writeReg(o.rd, hooc_hvm_sys_clock_gettime(readReg(2), readReg(3)));
                        break;
                    }
                    case kSysGetRandom: {
                        writeReg(o.rd, hooc_hvm_sys_getrandom(readReg(2), readReg(3)));
                        break;
                    }
                    default:
                        writeReg(o.rd, 0);
                        break;
                }
                break;
            }
            case hvm::Opcode::BREAK:
                lastError_ = "BREAK trap encountered";
                state.trapHit = true;
                return -1;
            default:
                lastError_ = "Unsupported opcode in interpreter: " + ins->getMnemonic();
                return -1;
        }

        if (!jumped) {
            pc = nextPc;
        }
    }
    captureInspectorSnapshot(state, pc, module->getName(), functionName, "HALT", true);
    {
        std::lock_guard<std::mutex> lk(lastRegistersMu_);
        for (size_t i = 0; i < 32; ++i) lastRegisters_[i] = state.regs[i];
    }
    return state.regs[1];
}

void HVMJIT::captureInspectorSnapshot(const HVMState& state, uint64_t pc, const std::string& moduleName,
                                      const std::string& functionName, const std::string& opcode, bool halted) {
    if (!inspectorCaptureEnabled_) {
        return;
    }
    InspectorSnapshot s;
    for (size_t i = 0; i < 32; ++i) {
        s.regs[i] = state.regs[i];
    }
    s.pc = pc;
    s.moduleName = moduleName;
    s.functionName = functionName;
    s.opcode = opcode;
    s.halted = halted;
    inspectorTrace_.push_back(std::move(s));
}

bool HVMJIT::initializeModules() {
    if (modulesInitialized_) {
        return true;
    }
    auto ordered = bundle_.resolveDependencyOrder();
    for (const auto& base : ordered) {
        auto mod = std::dynamic_pointer_cast<hvm::HOModule>(base);
        if (!mod) {
            continue;
        }
        if (!mapModuleSections(mod)) {
            return false;
        }
        if (!runModuleVTableInitializers(mod)) {
            return false;
        }
        if (!runModuleInitializer(mod)) {
            return false;
        }
    }
    modulesInitialized_ = true;
    return true;
}

std::shared_ptr<std::once_flag> HVMJIT::getOrCreateModuleInitOnceFlag(const std::string& moduleName) {
    std::lock_guard<std::mutex> lk(moduleInitOnceMu_);
    auto it = moduleInitOnceFlags_.find(moduleName);
    if (it != moduleInitOnceFlags_.end()) {
        return it->second;
    }
    auto inserted = moduleInitOnceFlags_.emplace(moduleName, std::make_shared<std::once_flag>());
    return inserted.first->second;
}

std::shared_ptr<std::once_flag> HVMJIT::getOrCreateModuleVTableOnceFlag(const std::string& moduleName) {
    std::lock_guard<std::mutex> lk(moduleVTableOnceMu_);
    auto it = moduleVTableOnceFlags_.find(moduleName);
    if (it != moduleVTableOnceFlags_.end()) {
        return it->second;
    }
    auto inserted = moduleVTableOnceFlags_.emplace(moduleName, std::make_shared<std::once_flag>());
    return inserted.first->second;
}

bool HVMJIT::runModuleVTableInitializers(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) {
        return false;
    }

    auto onceFlag = getOrCreateModuleVTableOnceFlag(module->getName());
    bool ok = true;
    std::string err;
    std::call_once(*onceFlag, [&]() {
        struct VTableTask {
            std::shared_ptr<hvm::HOModule> owner;
            std::string symbolName;
            std::string className;
            std::string baseClassName;
        };

        auto parseTask = [](const std::shared_ptr<hvm::HOModule>& owner, const hvm::Symbol& s)
            -> std::optional<VTableTask> {
            static const std::string kPrefix = "_F_";
            static const std::string kSuffix = "_vtable_init_v";
            if (s.type != hvm::Symbol::STT_FUNC || s.section_index == -1) {
                return std::nullopt;
            }
            if (s.name.rfind(kPrefix, 0) != 0 || s.name.size() <= kPrefix.size() + kSuffix.size()) {
                return std::nullopt;
            }
            if (s.name.rfind(kSuffix) != s.name.size() - kSuffix.size()) {
                return std::nullopt;
            }

            VTableTask task;
            task.owner = owner;
            task.symbolName = s.name;

            auto demangled = SymbolMangler::demangleSymbol(s.name);
            if (!demangled.className.empty()) {
                task.className = demangled.className;
                task.baseClassName = demangled.baseClassName;
                return task;
            }

            std::string core = s.name.substr(kPrefix.size(), s.name.size() - kPrefix.size() - kSuffix.size());
            const size_t sep = core.find('_');
            if (sep == std::string::npos) {
                task.className = core;
            } else {
                task.className = core.substr(0, sep);
                task.baseClassName = core.substr(sep + 1);
            }
            return task;
        };

        std::vector<VTableTask> allTasks;
        for (const auto& [_, mod] : loadedModules_) {
            if (!mod) continue;
            for (const auto& s : mod->getSymbols()) {
                auto t = parseTask(mod, s);
                if (t.has_value()) {
                    allTasks.push_back(*t);
                }
            }
        }

        std::unordered_map<std::string, VTableTask> byClass;
        std::unordered_map<std::string, VTableTask> moduleTasks;
        for (const auto& t : allTasks) {
            byClass[t.owner->getName() + "::" + t.className] = t;
            if (t.owner->getName() == module->getName()) {
                moduleTasks[t.symbolName] = t;
            }
        }

        std::unordered_set<std::string> visiting;
        std::unordered_set<std::string> visited;
        std::function<bool(const VTableTask&)> dfs = [&](const VTableTask& t) -> bool {
            const std::string key = t.owner->getName() + "::" + t.symbolName;
            if (visited.count(key)) return true;
            if (visiting.count(key)) {
                err = "Circular vtable init dependency: " + key;
                return false;
            }
            visiting.insert(key);

            if (!t.baseClassName.empty()) {
                const std::string localBase = t.owner->getName() + "::" + t.baseClassName;
                auto baseIt = byClass.find(localBase);
                if (baseIt == byClass.end()) {
                    for (const auto& [_, cand] : byClass) {
                        if (cand.className == t.baseClassName) {
                            baseIt = byClass.find(cand.owner->getName() + "::" + cand.className);
                            break;
                        }
                    }
                }
                if (baseIt != byClass.end()) {
                    if (!dfs(baseIt->second)) {
                        return false;
                    }
                }
            }

            HVMState state{};
            state.io = &io_;
            state.memory = memory_.data();
            state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
            if (executeFunction(t.owner, t.symbolName, state) == -1 && !lastError_.empty()) {
                err = "VTable init failed for " + t.owner->getName() + ": " + lastError_;
                return false;
            }
            initializedVTableClasses_.insert(t.owner->getName() + "::" + t.className);
            visiting.erase(key);
            visited.insert(key);
            return true;
        };

        for (const auto& [_, t] : moduleTasks) {
            if (!dfs(t)) {
                ok = false;
                break;
            }
        }
    });

    if (!ok) {
        setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                 err.empty() ? ("VTable init failed for module " + module->getName()) : err,
                 module->getName());
        return false;
    }
    return true;
}

bool HVMJIT::runModuleInitializer(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) {
        return false;
    }
    auto onceFlag = getOrCreateModuleInitOnceFlag(module->getName());
    bool ok = true;
    std::string err;
    std::call_once(*onceFlag, [&]() {
        HVMState state{};
        state.io = &io_;
        state.memory = memory_.data();
        state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
        const auto* initSym = findFunctionSymbol(*module, "_F_module_init_v");
        if (!initSym) return;
        if (executeFunction(module, initSym->name, state) == -1 && !lastError_.empty()) {
            ok = false;
            err = "Module init failed for " + module->getName() + ": " + lastError_;
        }
    });

    if (!ok) {
        setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                 err.empty() ? ("Module init failed for " + module->getName()) : err,
                 module->getName(), "_F_module_init_v");
        return false;
    }
    return true;
}

bool HVMJIT::runPostLoadInitializers() {
    auto ordered = bundle_.resolveDependencyOrder();
    for (const auto& base : ordered) {
        auto mod = std::dynamic_pointer_cast<hvm::HOModule>(base);
        if (!mod) {
            continue;
        }
        if (!mapModuleSections(mod)) {
            return false;
        }
        if (!runModuleVTableInitializers(mod)) {
            return false;
        }
        if (!runModuleInitializer(mod)) {
            return false;
        }
    }
    modulesInitialized_ = true;
    return true;
}

bool HVMJIT::loadModule(const std::string& path) {
    return loadBytecode(path);
}

bool HVMJIT::loadModule(std::unique_ptr<hvm::HOModule> module) {
    clearError();
    if (!module) {
        setError(ErrorPhase::Parse, ErrorCode::ParseFailed,
                 "Cannot load null HOModule");
        return false;
    }

    if (!ensureJIT()) {
        return false;
    }
    if (!validateModule(*module, module->getSourcePath())) {
        return false;
    }

    if (module->getName().empty()) {
        module->setName("unnamed_module");
    }
    setLoaderState(module->getName(), LoaderState::Discovered);

    const std::string moduleName = module->getName();
    const std::string canonicalPath = canonicalizePath(module->getSourcePath());
    moduleCanonicalPathByName_[moduleName] = canonicalPath;

    std::shared_ptr<hvm::HOModule> owned(std::move(module));
    std::unordered_set<std::string> preExistingModules;
    for (const auto& [name, _] : loadedModules_) {
        preExistingModules.insert(name);
    }

    loadedModules_[owned->getName()] = owned;
    moduleLayouts_.erase(owned->getName());
    moduleDylibs_.erase(owned->getName());

    if (primaryModuleName_.empty()) {
        primaryModuleName_ = owned->getName();
    }
    const bool prevModulesInitialized = modulesInitialized_;
    const bool prevModulesMaterialized = modulesMaterialized_;
    modulesInitialized_ = false;
    modulesMaterialized_ = false;

    auto restoreFlags = [&]() {
        modulesInitialized_ = prevModulesInitialized;
        modulesMaterialized_ = prevModulesMaterialized;
    };

    if (!bootstrapRuntimeModules()) {
        restoreFlags();
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!registerModuleInBundle(owned)) {
        restoreFlags();
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!resolveAndLoadDependencies(*owned)) {
        restoreFlags();
        std::vector<std::string> toRollback;
        for (const auto& [name, _] : loadedModules_) {
            if (!preExistingModules.count(name)) {
                toRollback.push_back(name);
            }
        }
        for (const auto& name : toRollback) rollbackModuleLoad(name);
        return false;
    }
    if (!validateImportsAgainstDependencies()) {
        restoreFlags();
        std::vector<std::string> toRollback;
        for (const auto& [name, _] : loadedModules_) {
            if (!preExistingModules.count(name)) {
                toRollback.push_back(name);
            }
        }
        for (const auto& name : toRollback) rollbackModuleLoad(name);
        return false;
    }
    if (!initializeDependencyGraphPostOrder()) {
        restoreFlags();
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!runPostLoadInitializers()) {
        restoreFlags();
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!configureJITDylibs()) {
        restoreFlags();
        rollbackModuleLoad(owned->getName());
        return false;
    }
    return true;
}

llvm::Expected<llvm::orc::ThreadSafeModule> HVMJIT::translateModule(hvm::HOModule& hvmModule) {
    const bool dwarfEnabled = isDwarfDebugInfoEnabled();
    auto context = std::make_unique<llvm::LLVMContext>();
    auto module = std::make_unique<llvm::Module>(hvmModule.getName(), *context);
    std::unique_ptr<llvm::DIBuilder> diBuilder;
    llvm::DIFile* diFile = nullptr;
    if (dwarfEnabled) {
        diBuilder = std::make_unique<llvm::DIBuilder>(*module);
        const std::string srcPath = hvmModule.getSourcePath().empty()
                                        ? (hvmModule.getName() + ".hoo")
                                        : hvmModule.getSourcePath();
        const fs::path srcFs(srcPath);
        diFile = diBuilder->createFile(srcFs.filename().string(), srcFs.parent_path().string());
        auto* diCU = diBuilder->createCompileUnit(
            llvm::dwarf::DW_LANG_C_plus_plus_11, diFile, "HVM-JIT", false, "", 0);
        (void)diCU;
    }
    llvm::IRBuilder<> builder(*context);
    llvm::Type* i64 = builder.getInt64Ty();
    llvm::Type* i8Ptr = llvm::PointerType::get(*context, 0);
    llvm::StructType* stateTy = llvm::StructType::create(*context, "hvm.state");
    stateTy->setBody({llvm::ArrayType::get(i64, 32), i8Ptr, i8Ptr, builder.getInt1Ty()});
    llvm::PointerType* statePtrTy = llvm::PointerType::get(*context, 0);
    llvm::FunctionType* fnTy = llvm::FunctionType::get(i64, {statePtrTy}, false);

    const hvm::Section* text = hvmModule.getSection(".text");
    if (!text) {
        return llvm::createStringError(std::errc::invalid_argument, "missing .text section");
    }
    uint64_t rodataBase = 0;
    uint64_t dataBase = 0;
    auto layoutIt = moduleLayouts_.find(hvmModule.getName());
    if (layoutIt != moduleLayouts_.end()) {
        rodataBase = layoutIt->second.rodataBase;
        dataBase = layoutIt->second.dataBase;
    }
    uint64_t rodataSize = 0;
    if (const hvm::Section* rodata = hvmModule.getSection(".rodata")) {
        rodataSize = rodata->data.empty() ? rodata->virtual_size : rodata->data.size();
    }

    std::unordered_map<std::string, llvm::Function*> fnMap;
    std::unordered_map<std::string, const hvm::FunctionMetadata*> fnMetaByName;
    for (const auto& fm : hvmModule.getFunctionMetadata()) {
        fnMetaByName[fm.name] = &fm;
    }
    for (const auto& sym : hvmModule.getSymbols()) {
        if (sym.type != hvm::Symbol::STT_FUNC) continue;
        fnMap[sym.name] = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, sym.name, module.get());
    }

    for (const auto& sym : hvmModule.getSymbols()) {
        if (sym.type != hvm::Symbol::STT_FUNC) continue;
        // Skip undefined symbols (section_index == -1) — these are imports resolved by the JIT linker
        if (sym.section_index < 0) continue;
        auto* fn = fnMap[sym.name];
        uint32_t lineStart = 1;
        auto fit = fnMetaByName.find(sym.name);
        if (fit != fnMetaByName.end() && fit->second && fit->second->source_line > 0) {
            lineStart = fit->second->source_line;
        }
        llvm::DISubprogram* sp = nullptr;
        if (dwarfEnabled && diBuilder && diFile) {
            auto* subTy = diBuilder->createSubroutineType(diBuilder->getOrCreateTypeArray({}));
            sp = diBuilder->createFunction(
                diFile, sym.name, sym.name, diFile, lineStart, subTy, lineStart,
                llvm::DINode::FlagPrototyped, llvm::DISubprogram::SPFlagDefinition);
            fn->setSubprogram(sp);
        }
        auto* entry = llvm::BasicBlock::Create(*context, "entry", fn);
        builder.SetInsertPoint(entry);
        auto* stateArg = fn->arg_begin();

        auto regPtr = [&](uint8_t r) {
            auto* regsArr = builder.CreateStructGEP(stateTy, stateArg, 0);
            auto* zero = builder.getInt64(0);
            return builder.CreateInBoundsGEP(llvm::ArrayType::get(i64, 32), regsArr, {zero, builder.getInt64(r)});
        };
        auto memBase = [&]() -> llvm::Value* {
            return builder.CreateLoad(i8Ptr, builder.CreateStructGEP(stateTy, stateArg, 1));
        };
        auto memAddr = [&](llvm::Value* addr64) -> llvm::Value* {
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), memBase(), addr64);
        };
        auto readReg = [&](uint8_t r) -> llvm::Value* {
            if (r == 0) return builder.getInt64(0);
            return builder.CreateLoad(i64, regPtr(r));
        };
        auto writeReg = [&](uint8_t r, llvm::Value* v) {
            if (r == 0) return;
            builder.CreateStore(v, regPtr(r));
        };
        auto allocCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_alloc", llvm::FunctionType::get(i64, {i64, i64}, false));
        auto retainCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_retain", llvm::FunctionType::get(i64, {i64}, false));
        auto releaseCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_release", llvm::FunctionType::get(i64, {i64}, false));
        auto refcountCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_refcount", llvm::FunctionType::get(i64, {i64}, false));
        auto typeIdCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_typeid", llvm::FunctionType::get(i64, {i64}, false));
        auto arcRetainCallee = module->getOrInsertFunction(
            "hooc_hvm_arc_retain_if_managed", llvm::FunctionType::get(builder.getVoidTy(), {i64}, false));
        auto arcReleaseCallee = module->getOrInsertFunction(
            "hooc_hvm_arc_release_if_managed", llvm::FunctionType::get(builder.getVoidTy(), {i64}, false));
        auto pushHandlerStateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_push_handler_state", llvm::FunctionType::get(i64, {statePtrTy, i64}, false));
        auto popHandlerStateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_pop_handler_state", llvm::FunctionType::get(i64, {statePtrTy}, false));
        auto throwToHandlerStateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_throw_to_handler_state", llvm::FunctionType::get(i64, {statePtrTy, i64}, false));
        auto rethrowToHandlerStateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_rethrow_to_handler_state", llvm::FunctionType::get(i64, {statePtrTy}, false));
        auto stringDataCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_string_data", llvm::FunctionType::get(i64, {i64}, false));
        auto shouldStopStateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_should_stop_state", llvm::FunctionType::get(i64, {statePtrTy}, false));

        // Platform OS services (syscalls 12–23)
        auto threadCreateCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_thread_create", llvm::FunctionType::get(i64, {i64, i64}, false));
        auto threadExitCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_thread_exit", llvm::FunctionType::get(i64, {i64}, false));
        auto futexCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_futex", llvm::FunctionType::get(i64, {i64, i64, i64}, false));
        auto getTidCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_get_tid", llvm::FunctionType::get(i64, false));
        auto openCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_open", llvm::FunctionType::get(i64, {i64, i64, i64}, false));
        auto readCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_read", llvm::FunctionType::get(i64, {i64, i64, i64}, false));
        auto writeCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_write", llvm::FunctionType::get(i64, {i64, i64, i64}, false));
        auto closeCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_close", llvm::FunctionType::get(i64, {i64}, false));
        auto lseekCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_lseek", llvm::FunctionType::get(i64, {i64, i64, i64}, false));
        auto fstatCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_fstat", llvm::FunctionType::get(i64, {i64, i64}, false));
        auto clockGetTimeCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_clock_gettime", llvm::FunctionType::get(i64, {i64, i64}, false));
        auto getRandomCallee = module->getOrInsertFunction(
            "hooc_hvm_sys_getrandom", llvm::FunctionType::get(i64, {i64, i64}, false));

        uint64_t pc = sym.value;
        const uint64_t textSize = text->data.size();
        std::unordered_map<uint64_t, llvm::BasicBlock*> blockByPc;
        std::vector<uint64_t> work{pc};
        blockByPc[pc] = llvm::BasicBlock::Create(*context, "pc_" + std::to_string(pc), fn);
        builder.CreateBr(blockByPc[pc]);

        std::vector<uint64_t> validPcs;
        {
            uint64_t scanPc = pc;
            while (scanPc < textSize) {
                validPcs.push_back(scanPc);
                std::vector<uint8_t> scanSlice;
                size_t scanRead = static_cast<size_t>(std::min<uint64_t>(8, textSize - scanPc));
                scanSlice.insert(scanSlice.end(), text->data.begin() + static_cast<ptrdiff_t>(scanPc),
                                 text->data.begin() + static_cast<ptrdiff_t>(scanPc + scanRead));
                size_t used = 0;
                auto scanIns = hvm::HVMInstruction::decode(scanSlice, used);
                if (!scanIns || used == 0 || scanIns->getOpcode() == hvm::Opcode::RET) {
                    break;
                }
                scanPc += used;
            }
        }

        auto ensureBlock = [&](uint64_t targetPc) {
            if (targetPc >= textSize) return;
            if (!blockByPc.count(targetPc)) {
                blockByPc[targetPc] = llvm::BasicBlock::Create(*context, "pc_" + std::to_string(targetPc), fn);
                work.push_back(targetPc);
            }
        };
        auto allocEscapes = [&](uint64_t allocPc, uint8_t allocReg) -> bool {
            std::unordered_map<uint64_t, std::pair<std::unique_ptr<hvm::HVMInstruction>, size_t>> decodedByPc;
            decodedByPc.reserve(validPcs.size());
            for (uint64_t vpc : validPcs) {
                if (vpc >= textSize) return true;
                std::vector<uint8_t> scanSlice;
                size_t maxRead = static_cast<size_t>(std::min<uint64_t>(8, textSize - vpc));
                scanSlice.insert(scanSlice.end(), text->data.begin() + static_cast<ptrdiff_t>(vpc),
                                 text->data.begin() + static_cast<ptrdiff_t>(vpc + maxRead));
                size_t used = 0;
                auto sin = hvm::HVMInstruction::decode(scanSlice, used);
                if (!sin || used == 0) return true;
                decodedByPc.emplace(vpc, std::make_pair(std::move(sin), used));
            }
            auto hasPc = [&](uint64_t targetPc) -> bool {
                return decodedByPc.find(targetPc) != decodedByPc.end();
            };
            auto nextPcOf = [&](uint64_t curPc) -> uint64_t {
                auto it = decodedByPc.find(curPc);
                if (it == decodedByPc.end()) return textSize;
                return curPc + static_cast<uint64_t>(it->second.second);
            };
            auto isWriteToAllocReg = [&](const hvm::HVMInstruction& ins) -> bool {
                if (std::holds_alternative<hvm::OperandsI>(ins.getOperands())) {
                    return std::get<hvm::OperandsI>(ins.getOperands()).rd == allocReg;
                }
                if (std::holds_alternative<hvm::OperandsR>(ins.getOperands())) {
                    return std::get<hvm::OperandsR>(ins.getOperands()).rd == allocReg;
                }
                if (std::holds_alternative<hvm::OperandsJ>(ins.getOperands())) {
                    return std::get<hvm::OperandsJ>(ins.getOperands()).rd == allocReg;
                }
                return false;
            };
            auto isReadOfAllocReg = [&](const hvm::HVMInstruction& ins) -> bool {
                const auto op = ins.getOpcode();
                if (std::holds_alternative<hvm::OperandsI>(ins.getOperands())) {
                    const auto oi = std::get<hvm::OperandsI>(ins.getOperands());
                    if (oi.rs == allocReg) return true;
                    if ((op == hvm::Opcode::ST_B || op == hvm::Opcode::ST_H ||
                         op == hvm::Opcode::ST_W || op == hvm::Opcode::ST_D) &&
                        oi.rd == allocReg) {
                        return true;
                    }
                } else if (std::holds_alternative<hvm::OperandsR>(ins.getOperands())) {
                    const auto orr = std::get<hvm::OperandsR>(ins.getOperands());
                    if (orr.rs1 == allocReg || orr.rs2 == allocReg) return true;
                    if (op == hvm::Opcode::PUSH && orr.rd == allocReg) return true;
                } else if (std::holds_alternative<hvm::OperandsB>(ins.getOperands())) {
                    const auto ob = std::get<hvm::OperandsB>(ins.getOperands());
                    if (ob.rs1 == allocReg || ob.rs2 == allocReg) return true;
                }
                if (op == hvm::Opcode::SYSCALL && allocReg == 2) {
                    return true;
                }
                return false;
            };

            std::vector<std::pair<uint64_t, bool>> wl;
            std::unordered_set<uint64_t> visitedLive;
            std::unordered_set<uint64_t> visitedDead;
            wl.emplace_back(allocPc, true);

            auto pushState = [&](uint64_t nextPc, bool live) {
                if (!hasPc(nextPc)) return;
                auto& seen = live ? visitedLive : visitedDead;
                if (seen.insert(nextPc).second) {
                    wl.emplace_back(nextPc, live);
                }
            };

            while (!wl.empty()) {
                auto [curPc, live] = wl.back();
                wl.pop_back();
                auto dit = decodedByPc.find(curPc);
                if (dit == decodedByPc.end()) {
                    return true;
                }
                const auto& ins = *dit->second.first;
                const auto op = ins.getOpcode();
                if (live && isReadOfAllocReg(ins)) {
                    return true;
                }

                bool nextLive = live;
                if (live && isWriteToAllocReg(ins)) {
                    nextLive = false;
                }

                if (live && (op == hvm::Opcode::CALL || op == hvm::Opcode::TAILCALL ||
                             op == hvm::Opcode::JAL || op == hvm::Opcode::JALR)) {
                    return true;
                }
                if (op == hvm::Opcode::RET) {
                    continue;
                }
                if (op == hvm::Opcode::JMP) {
                    auto oj = std::get<hvm::OperandsJ>(ins.getOperands());
                    uint64_t targetPc = static_cast<uint64_t>(
                        static_cast<int64_t>(curPc) + static_cast<int64_t>(oj.offset) * 4);
                    pushState(targetPc, nextLive);
                    continue;
                }
                if (op == hvm::Opcode::BEQ || op == hvm::Opcode::BNE ||
                    op == hvm::Opcode::BLT || op == hvm::Opcode::BLE) {
                    auto ob = std::get<hvm::OperandsB>(ins.getOperands());
                    uint64_t targetPc = static_cast<uint64_t>(
                        static_cast<int64_t>(curPc) + static_cast<int64_t>(ob.imm15) * 4);
                    pushState(targetPc, nextLive);
                    pushState(nextPcOf(curPc), nextLive);
                    continue;
                }
                pushState(nextPcOf(curPc), nextLive);
            }
            return false;
        };

        while (!work.empty()) {
            uint64_t curr = work.back();
            work.pop_back();
            builder.SetInsertPoint(blockByPc[curr]);
            {
                auto* shouldStop = builder.CreateCall(shouldStopStateCallee, {stateArg});
                auto* stopNow = builder.CreateICmpNE(shouldStop, builder.getInt64(0));
                auto* stopBB = llvm::BasicBlock::Create(*context, "stop_now", fn);
                auto* contBB = llvm::BasicBlock::Create(*context, "stop_cont", fn);
                builder.CreateCondBr(stopNow, stopBB, contBB);
                builder.SetInsertPoint(stopBB);
                builder.CreateRet(builder.getInt64(-1));
                builder.SetInsertPoint(contBB);
            }
            uint64_t ipc = curr;
            while (ipc < textSize) {
                std::vector<uint8_t> slice;
                size_t maxRead = static_cast<size_t>(std::min<uint64_t>(8, textSize - ipc));
                slice.insert(slice.end(), text->data.begin() + static_cast<ptrdiff_t>(ipc),
                             text->data.begin() + static_cast<ptrdiff_t>(ipc + maxRead));
                size_t used = 0;
                auto ins = hvm::HVMInstruction::decode(slice, used);
                if (!ins || used == 0) {
                    builder.CreateRet(builder.getInt64(-1));
                    break;
                }
                uint64_t nextPc = ipc + used;
                auto op = ins->getOpcode();
                if (dwarfEnabled && sp) {
                    const uint32_t line = lineStart + static_cast<uint32_t>((ipc - sym.value) / 4U);
                    builder.SetCurrentDebugLocation(llvm::DILocation::get(*context, line, 1, sp));
                }

                if (op == hvm::Opcode::RET) {
                    builder.CreateRet(readReg(1));
                    break;
                } else if (op == hvm::Opcode::NOP) {
                } else if (op == hvm::Opcode::MOV) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    writeReg(o.rd, readReg(o.rs1));
                } else if (op == hvm::Opcode::MOVZ) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    writeReg(o.rd, builder.CreateOr(readReg(o.rs), builder.getInt64(static_cast<uint16_t>(o.imm15))));
                } else if (op == hvm::Opcode::LUI) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto hi = builder.CreateShl(builder.getInt64(static_cast<uint16_t>(o.imm15)), builder.getInt64(49));
                    writeReg(o.rd, builder.CreateOr(readReg(o.rs), hi));
                } else if (op == hvm::Opcode::ADDI) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    writeReg(o.rd, builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15))));
                } else if (op == hvm::Opcode::NOT) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    writeReg(o.rd, builder.CreateXor(readReg(o.rs1), builder.getInt64(~0ULL)));
                } else if (op == hvm::Opcode::ARITH) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto a = readReg(o.rs1);
                    auto b = readReg(o.rs2);
                    llvm::Value* out = nullptr;
                    if (o.func == 0) out = builder.CreateAdd(a, b);
                    else if (o.func == 1) out = builder.CreateSub(a, b);
                    else if (o.func == 2) out = builder.CreateMul(a, b);
                    else if (o.func == 5 || o.func == 6 || o.func == 7) {
                        auto* zero = builder.getInt64(0);
                        auto* isZero = builder.CreateICmpEQ(b, zero);
                        auto* okBB = llvm::BasicBlock::Create(*context, "arith_ok", fn);
                        auto* errBB = llvm::BasicBlock::Create(*context, "arith_err", fn);
                        builder.CreateCondBr(isZero, errBB, okBB);
                        builder.SetInsertPoint(errBB);
                        builder.CreateRet(builder.getInt64(-1));
                        builder.SetInsertPoint(okBB);
                        if (o.func == 5) out = builder.CreateSDiv(a, b);
                        else if (o.func == 6) out = builder.CreateUDiv(a, b);
                        else out = builder.CreateSRem(a, b);
                    }
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, out);
                } else if (op == hvm::Opcode::SHIFT) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto sh = builder.CreateAnd(readReg(o.rs2), builder.getInt64(63));
                    llvm::Value* out = nullptr;
                    if (o.func == 0) out = builder.CreateShl(readReg(o.rs1), sh);
                    else if (o.func == 1) out = builder.CreateLShr(readReg(o.rs1), sh);
                    else if (o.func == 2) out = builder.CreateAShr(readReg(o.rs1), sh);
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, out);
                } else if (op == hvm::Opcode::LOGIC) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    llvm::Value* out = nullptr;
                    if (o.func == 0) out = builder.CreateAnd(readReg(o.rs1), readReg(o.rs2));
                    else if (o.func == 1) out = builder.CreateOr(readReg(o.rs1), readReg(o.rs2));
                    else if (o.func == 2) out = builder.CreateXor(readReg(o.rs1), readReg(o.rs2));
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, out);
                } else if (op == hvm::Opcode::CMP) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    llvm::Value* pred = nullptr;
                    if (o.func == 0) pred = builder.CreateICmpEQ(readReg(o.rs1), readReg(o.rs2));
                    else if (o.func == 1) pred = builder.CreateICmpNE(readReg(o.rs1), readReg(o.rs2));
                    else if (o.func == 2) pred = builder.CreateICmpSLT(readReg(o.rs1), readReg(o.rs2));
                    else if (o.func == 3) pred = builder.CreateICmpSLE(readReg(o.rs1), readReg(o.rs2));
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, builder.CreateZExt(pred, i64));
                } else if (op == hvm::Opcode::FLOAT_ARITH) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto* a = builder.CreateBitCast(readReg(o.rs1), builder.getDoubleTy());
                    auto* b = builder.CreateBitCast(readReg(o.rs2), builder.getDoubleTy());
                    llvm::Value* out = nullptr;
                    if (o.func == 0) out = builder.CreateFAdd(a, b);
                    else if (o.func == 1) out = builder.CreateFSub(a, b);
                    else if (o.func == 2) out = builder.CreateFMul(a, b);
                    else if (o.func == 3) out = builder.CreateFDiv(a, b);
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, builder.CreateBitCast(out, i64));
                } else if (op == hvm::Opcode::FCMP) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto* a = builder.CreateBitCast(readReg(o.rs1), builder.getDoubleTy());
                    auto* b = builder.CreateBitCast(readReg(o.rs2), builder.getDoubleTy());
                    llvm::Value* pred = nullptr;
                    if (o.func == 0) pred = builder.CreateFCmpOEQ(a, b);
                    else if (o.func == 1) pred = builder.CreateFCmpOLT(a, b);
                    else if (o.func == 2) pred = builder.CreateFCmpOLE(a, b);
                    else { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, builder.CreateZExt(pred, i64));
                } else if (op == hvm::Opcode::JMP) {
                    auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                    uint64_t tgt = static_cast<uint64_t>(static_cast<int64_t>(ipc) + static_cast<int64_t>(o.offset) * 4);
                    ensureBlock(tgt);
                    if (!blockByPc.count(tgt)) { builder.CreateRet(builder.getInt64(-1)); break; }
                    builder.CreateBr(blockByPc[tgt]);
                    break;
                } else if (op == hvm::Opcode::BEQ || op == hvm::Opcode::BNE || op == hvm::Opcode::BLT || op == hvm::Opcode::BLE) {
                    auto o = std::get<hvm::OperandsB>(ins->getOperands());
                    uint64_t tgt = static_cast<uint64_t>(static_cast<int64_t>(ipc) + static_cast<int64_t>(o.imm15) * 4);
                    ensureBlock(tgt);
                    ensureBlock(nextPc);
                    if (!blockByPc.count(tgt) || !blockByPc.count(nextPc)) { builder.CreateRet(builder.getInt64(-1)); break; }
                    llvm::Value* cond = nullptr;
                    if (op == hvm::Opcode::BEQ) cond = builder.CreateICmpEQ(readReg(o.rs1), readReg(o.rs2));
                    else if (op == hvm::Opcode::BNE) cond = builder.CreateICmpNE(readReg(o.rs1), readReg(o.rs2));
                    else if (op == hvm::Opcode::BLT) cond = builder.CreateICmpSLT(readReg(o.rs1), readReg(o.rs2));
                    else cond = builder.CreateICmpSLE(readReg(o.rs1), readReg(o.rs2));
                    builder.CreateCondBr(cond, blockByPc[tgt], blockByPc[nextPc]);
                    break;
                } else if (op == hvm::Opcode::JAL) {
                    auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                    uint64_t tgt = static_cast<uint64_t>(static_cast<int64_t>(ipc) + static_cast<int64_t>(o.offset) * 4);
                    ensureBlock(tgt);
                    if (!blockByPc.count(tgt)) { builder.CreateRet(builder.getInt64(-1)); break; }
                    writeReg(o.rd, builder.getInt64(nextPc));
                    builder.CreateBr(blockByPc[tgt]);
                    break;
                } else if (op == hvm::Opcode::JALR) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* tgt = builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15)));
                    writeReg(o.rd, builder.getInt64(nextPc));
                    auto* errBB = llvm::BasicBlock::Create(*context, "jalr_err", fn);
                    auto* sw = builder.CreateSwitch(tgt, errBB, static_cast<unsigned>(validPcs.size()));
                    for (uint64_t vp : validPcs) {
                        ensureBlock(vp);
                        sw->addCase(builder.getInt64(vp), blockByPc[vp]);
                    }
                    builder.SetInsertPoint(errBB);
                    builder.CreateRet(builder.getInt64(-1));
                    break;
                } else if (op == hvm::Opcode::CALL) {
                    auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                    uint64_t tgt = static_cast<uint64_t>(static_cast<int64_t>(ipc) + static_cast<int64_t>(o.offset) * 4);
                    auto fnNameIt = functionNameByOffset_[hvmModule.getName()].find(tgt);

                    std::string calleeName;
                    if (fnNameIt != functionNameByOffset_[hvmModule.getName()].end()) {
                        calleeName = fnNameIt->second;
                    } else {
                        // Look for symbol at target in module symbols (might be IMPORT)
                        for (const auto& sym : hvmModule.getSymbols()) {
                            if (sym.value == tgt && (sym.type == hvm::Symbol::STT_FUNC || sym.type == hvm::Symbol::STT_NOTYPE)) {
                                calleeName = sym.name;
                                break;
                            }
                        }
                    }

                    if (calleeName.empty()) {
                        builder.CreateRet(builder.getInt64(-1));
                        break;
                    }

                    llvm::Function* callee = module->getFunction(calleeName);
                    if (!callee) {
                        callee = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, calleeName, module.get());
                    }

                    writeReg(o.rd, builder.getInt64(nextPc));
                    auto* rv = builder.CreateCall(callee, {stateArg});
                    writeReg(1, rv);
                } else if (op == hvm::Opcode::TAILCALL) {
                    auto o = std::get<hvm::OperandsJ>(ins->getOperands());
                    uint64_t tgt = static_cast<uint64_t>(static_cast<int64_t>(ipc) + static_cast<int64_t>(o.offset) * 4);
                    auto fnNameIt = functionNameByOffset_[hvmModule.getName()].find(tgt);

                    std::string calleeName;
                    if (fnNameIt != functionNameByOffset_[hvmModule.getName()].end()) {
                        calleeName = fnNameIt->second;
                    } else {
                        for (const auto& sym : hvmModule.getSymbols()) {
                            if (sym.value == tgt && (sym.type == hvm::Symbol::STT_FUNC || sym.type == hvm::Symbol::STT_NOTYPE)) {
                                calleeName = sym.name;
                                break;
                            }
                        }
                    }

                    if (calleeName.empty()) {
                        builder.CreateRet(builder.getInt64(-1));
                        break;
                    }

                    llvm::Function* callee = module->getFunction(calleeName);
                    if (!callee) {
                        callee = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, calleeName, module.get());
                    }

                    auto* rv = builder.CreateCall(callee, {stateArg});
                    builder.CreateRet(rv);
                    break;
                }
 else if (op == hvm::Opcode::LD_B || op == hvm::Opcode::LD_BU || op == hvm::Opcode::LD_H ||
                           op == hvm::Opcode::LD_HU || op == hvm::Opcode::LD_W || op == hvm::Opcode::LD_WU) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* addr = builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15)));
                    if (op == hvm::Opcode::LD_B || op == hvm::Opcode::LD_BU) {
                        auto* v = builder.CreateLoad(builder.getInt8Ty(), memAddr(addr));
                        writeReg(o.rd, op == hvm::Opcode::LD_B ? builder.CreateSExt(v, i64) : builder.CreateZExt(v, i64));
                    } else if (op == hvm::Opcode::LD_H || op == hvm::Opcode::LD_HU) {
                        auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                        auto* v = builder.CreateLoad(builder.getInt16Ty(), ptr);
                        writeReg(o.rd, op == hvm::Opcode::LD_H ? builder.CreateSExt(v, i64) : builder.CreateZExt(v, i64));
                    } else {
                        auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                        auto* v = builder.CreateLoad(builder.getInt32Ty(), ptr);
                        writeReg(o.rd, op == hvm::Opcode::LD_W ? builder.CreateSExt(v, i64) : builder.CreateZExt(v, i64));
                    }
                } else if (op == hvm::Opcode::LD_D) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* addr = builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15)));
                    auto* misaligned = builder.CreateICmpNE(builder.CreateAnd(addr, builder.getInt64(7)), builder.getInt64(0));
                    auto* okBB = llvm::BasicBlock::Create(*context, "ldd_ok", fn);
                    auto* errBB = llvm::BasicBlock::Create(*context, "ldd_err", fn);
                    builder.CreateCondBr(misaligned, errBB, okBB);
                    builder.SetInsertPoint(errBB);
                    builder.CreateRet(builder.getInt64(-1));
                    builder.SetInsertPoint(okBB);
                    auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                    auto* v = builder.CreateLoad(i64, ptr);
                    writeReg(o.rd, v);
                } else if (op == hvm::Opcode::ST_B || op == hvm::Opcode::ST_H || op == hvm::Opcode::ST_W) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* addr = builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15)));
                    auto* val = readReg(o.rd);
                    if (op == hvm::Opcode::ST_B) {
                        builder.CreateStore(builder.CreateTrunc(val, builder.getInt8Ty()), memAddr(addr));
                    } else if (op == hvm::Opcode::ST_H) {
                        auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                        builder.CreateStore(builder.CreateTrunc(val, builder.getInt16Ty()), ptr);
                    } else {
                        auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                        builder.CreateStore(builder.CreateTrunc(val, builder.getInt32Ty()), ptr);
                    }
                } else if (op == hvm::Opcode::ST_D) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* addr = builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15)));
                    auto* misaligned = builder.CreateICmpNE(builder.CreateAnd(addr, builder.getInt64(7)), builder.getInt64(0));
                    auto* okBB = llvm::BasicBlock::Create(*context, "std_ok", fn);
                    auto* errBB = llvm::BasicBlock::Create(*context, "std_err", fn);
                    builder.CreateCondBr(misaligned, errBB, okBB);
                    builder.SetInsertPoint(errBB);
                    builder.CreateRet(builder.getInt64(-1));
                    builder.SetInsertPoint(okBB);
                    auto* ptr = builder.CreateBitCast(memAddr(addr), llvm::PointerType::get(*context, 0));
                    auto* oldVal = builder.CreateLoad(i64, ptr);
                    auto* newVal = readReg(o.rd);
                    auto* sameVal = builder.CreateICmpEQ(newVal, oldVal);
                    auto* sameContBB = llvm::BasicBlock::Create(*context, "std_same", fn);
                    auto* diffBB = llvm::BasicBlock::Create(*context, "std_diff", fn);
                    builder.CreateCondBr(sameVal, sameContBB, diffBB);
                    builder.SetInsertPoint(diffBB);
                    auto* newNonNull = builder.CreateICmpNE(newVal, builder.getInt64(0));
                    auto* retainBB = llvm::BasicBlock::Create(*context, "std_retain", fn);
                    auto* storeBB = llvm::BasicBlock::Create(*context, "std_store", fn);
                    builder.CreateCondBr(newNonNull, retainBB, storeBB);
                    builder.SetInsertPoint(retainBB);
                    builder.CreateCall(arcRetainCallee, {newVal});
                    builder.CreateBr(storeBB);
                    builder.SetInsertPoint(storeBB);
                    builder.CreateStore(newVal, ptr);
                    auto* oldNonNull = builder.CreateICmpNE(oldVal, builder.getInt64(0));
                    auto* releaseBB = llvm::BasicBlock::Create(*context, "std_release", fn);
                    auto* contBB = llvm::BasicBlock::Create(*context, "std_cont", fn);
                    builder.CreateCondBr(oldNonNull, releaseBB, contBB);
                    builder.SetInsertPoint(releaseBB);
                    builder.CreateCall(arcReleaseCallee, {oldVal});
                    builder.CreateBr(contBB);
                    builder.SetInsertPoint(contBB);
                    builder.CreateBr(sameContBB);
                    builder.SetInsertPoint(sameContBB);
                } else if (op == hvm::Opcode::LDA) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    if (o.rs == 0 && rodataBase != 0 && o.imm15 >= 0 &&
                        static_cast<uint64_t>(o.imm15) < rodataSize) {
                        writeReg(o.rd, builder.getInt64(rodataBase + static_cast<uint64_t>(o.imm15)));
                    } else if (o.rs == 1 && dataBase != 0) {
                        writeReg(o.rd, builder.getInt64(dataBase + static_cast<uint64_t>(static_cast<int16_t>(o.imm15))));
                    } else {
                        writeReg(o.rd, builder.CreateAdd(readReg(o.rs), builder.getInt64(static_cast<int64_t>(o.imm15))));
                    }
                } else if (op == hvm::Opcode::ENTER) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* sp = readReg(31);
                    auto* sp2 = builder.CreateSub(sp, builder.getInt64(16));
                    // store old lr/fp
                    auto* pLr = builder.CreateBitCast(memAddr(sp2), llvm::PointerType::get(*context, 0));
                    builder.CreateStore(readReg(29), pLr);
                    auto* pFp = builder.CreateBitCast(memAddr(builder.CreateAdd(sp2, builder.getInt64(8))), llvm::PointerType::get(*context, 0));
                    builder.CreateStore(readReg(30), pFp);
                    writeReg(30, sp2);
                    writeReg(31, builder.CreateSub(sp2, builder.getInt64(static_cast<int64_t>(o.imm15))));
                } else if (op == hvm::Opcode::LEAVE) {
                    auto* fp = readReg(30);
                    auto* pLr = builder.CreateBitCast(memAddr(fp), llvm::PointerType::get(*context, 0));
                    auto* pFp = builder.CreateBitCast(memAddr(builder.CreateAdd(fp, builder.getInt64(8))), llvm::PointerType::get(*context, 0));
                    writeReg(29, builder.CreateLoad(i64, pLr));
                    writeReg(30, builder.CreateLoad(i64, pFp));
                    writeReg(31, builder.CreateAdd(fp, builder.getInt64(16)));
                } else if (op == hvm::Opcode::ADJSP) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    writeReg(31, builder.CreateAdd(readReg(31), builder.getInt64(static_cast<int64_t>(o.imm15))));
                } else if (op == hvm::Opcode::FRAME) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    writeReg(o.rd, builder.CreateAdd(readReg(30), builder.getInt64(static_cast<int64_t>(o.imm15))));
                } else if (op == hvm::Opcode::PUSH) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto* sp = builder.CreateSub(readReg(31), builder.getInt64(8));
                    auto* p = builder.CreateBitCast(memAddr(sp), llvm::PointerType::get(*context, 0));
                    builder.CreateStore(readReg(o.rd), p);
                    writeReg(31, sp);
                } else if (op == hvm::Opcode::POP) {
                    auto o = std::get<hvm::OperandsR>(ins->getOperands());
                    auto* sp = readReg(31);
                    auto* p = builder.CreateBitCast(memAddr(sp), llvm::PointerType::get(*context, 0));
                    writeReg(o.rd, builder.CreateLoad(i64, p));
                    writeReg(31, builder.CreateAdd(sp, builder.getInt64(8)));
                } else if (op == hvm::Opcode::SYSCALL) {
                    auto o = std::get<hvm::OperandsI>(ins->getOperands());
                    auto* syscallErr = llvm::BasicBlock::Create(*context, "sys_err", fn);
                    auto* syscallDone = llvm::BasicBlock::Create(*context, "sys_done", fn);
                    auto* sw = builder.CreateSwitch(builder.getInt64(static_cast<int64_t>(o.imm15)), syscallErr, 23);

                    auto* allocBB = llvm::BasicBlock::Create(*context, "sys_alloc", fn);
                    sw->addCase(builder.getInt64(kSysAlloc), allocBB);
                    builder.SetInsertPoint(allocBB);
                    llvm::Value* allocRet = nullptr;
                    auto escape = allocEscapes(nextPc, o.rd);
                    if (!escape && isEscapeAllocaPromotionEnabled()) {
                        // Escape analysis: if allocation result cannot escape the current
                        // linear region, keep storage on stack and avoid runtime heap call.
                        auto* bytes = readReg(2);
                        auto* tmp = builder.CreateAlloca(builder.getInt8Ty(), bytes, "esc_alloca");
                        builder.CreateMemSet(tmp, builder.getInt8(0), bytes, llvm::Align(1));
                        allocRet = builder.CreatePtrToInt(tmp, i64);
                    } else {
                        allocRet = builder.CreateCall(allocCallee, {readReg(2), readReg(3)});
                    }
                    writeReg(o.rd, allocRet);
                    builder.CreateBr(syscallDone);

                    auto* retainBB = llvm::BasicBlock::Create(*context, "sys_retain", fn);
                    sw->addCase(builder.getInt64(kSysRetain), retainBB);
                    builder.SetInsertPoint(retainBB);
                    auto* retainRet = builder.CreateCall(retainCallee, {readReg(2)});
                    writeReg(o.rd, retainRet);
                    builder.CreateBr(syscallDone);

                    auto* releaseBB = llvm::BasicBlock::Create(*context, "sys_release", fn);
                    sw->addCase(builder.getInt64(kSysRelease), releaseBB);
                    builder.SetInsertPoint(releaseBB);
                    auto* releaseRet = builder.CreateCall(releaseCallee, {readReg(2)});
                    writeReg(o.rd, releaseRet);
                    builder.CreateBr(syscallDone);

                    auto* refBB = llvm::BasicBlock::Create(*context, "sys_ref", fn);
                    sw->addCase(builder.getInt64(kSysRefcount), refBB);
                    builder.SetInsertPoint(refBB);
                    auto* refVal = builder.CreateCall(refcountCallee, {readReg(2)});
                    writeReg(o.rd, refVal);
                    builder.CreateBr(syscallDone);

                    auto* typeBB = llvm::BasicBlock::Create(*context, "sys_type", fn);
                    sw->addCase(builder.getInt64(kSysTypeId), typeBB);
                    builder.SetInsertPoint(typeBB);
                    auto* t = builder.CreateCall(typeIdCallee, {readReg(2)});
                    writeReg(o.rd, t);
                    builder.CreateBr(syscallDone);

                    auto* excBB = llvm::BasicBlock::Create(*context, "sys_exc_runtime", fn);
                    sw->addCase(builder.getInt64(kSysExceptionRuntime), excBB);
                    builder.SetInsertPoint(excBB);
                    auto excCall = module->getOrInsertFunction(
                        "hooc_hvm_sys_exception_runtime", llvm::FunctionType::get(i64, {i64}, false));
                    auto* exc = builder.CreateCall(excCall, {builder.getInt64(0)});
                    writeReg(o.rd, exc);
                    builder.CreateBr(syscallDone);

                    auto* pushHandlerBB = llvm::BasicBlock::Create(*context, "sys_push_handler", fn);
                    sw->addCase(builder.getInt64(kSysPushHandler), pushHandlerBB);
                    builder.SetInsertPoint(pushHandlerBB);
                    auto* pushRet = builder.CreateCall(pushHandlerStateCallee, {stateArg, readReg(2)});
                    writeReg(o.rd, pushRet);
                    builder.CreateBr(syscallDone);

                    auto* popHandlerBB = llvm::BasicBlock::Create(*context, "sys_pop_handler", fn);
                    sw->addCase(builder.getInt64(kSysPopHandler), popHandlerBB);
                    builder.SetInsertPoint(popHandlerBB);
                    auto* popRet = builder.CreateCall(popHandlerStateCallee, {stateArg});
                    writeReg(o.rd, popRet);
                    builder.CreateBr(syscallDone);

                    auto* throwHandlerBB = llvm::BasicBlock::Create(*context, "sys_throw_handler", fn);
                    sw->addCase(builder.getInt64(kSysThrowToHandler), throwHandlerBB);
                    builder.SetInsertPoint(throwHandlerBB);
                    auto* throwPc = builder.CreateCall(throwToHandlerStateCallee, {stateArg, readReg(2)});
                    writeReg(o.rd, throwPc);
                    auto* throwUnhandled = llvm::BasicBlock::Create(*context, "sys_throw_unhandled", fn);
                    auto* throwSwitch = llvm::BasicBlock::Create(*context, "sys_throw_switch", fn);
                    builder.CreateCondBr(
                        builder.CreateICmpEQ(throwPc, builder.getInt64(kNoHandlerPc)),
                        throwUnhandled, throwSwitch);
                    builder.SetInsertPoint(throwUnhandled);
                    builder.CreateRet(builder.getInt64(-1));
                    builder.SetInsertPoint(throwSwitch);
                    auto* throwSw = builder.CreateSwitch(throwPc, throwUnhandled, static_cast<unsigned>(validPcs.size()));
                    for (uint64_t vp : validPcs) {
                        ensureBlock(vp);
                        throwSw->addCase(builder.getInt64(vp), blockByPc[vp]);
                    }

                    auto* rethrowHandlerBB = llvm::BasicBlock::Create(*context, "sys_rethrow_handler", fn);
                    sw->addCase(builder.getInt64(kSysRethrowToHandler), rethrowHandlerBB);
                    builder.SetInsertPoint(rethrowHandlerBB);
                    auto* rethrowPc = builder.CreateCall(rethrowToHandlerStateCallee, {stateArg});
                    writeReg(o.rd, rethrowPc);
                    auto* rethrowUnhandled = llvm::BasicBlock::Create(*context, "sys_rethrow_unhandled", fn);
                    auto* rethrowSwitch = llvm::BasicBlock::Create(*context, "sys_rethrow_switch", fn);
                    builder.CreateCondBr(
                        builder.CreateICmpEQ(rethrowPc, builder.getInt64(kNoHandlerPc)),
                        rethrowUnhandled, rethrowSwitch);
                    builder.SetInsertPoint(rethrowUnhandled);
                    builder.CreateRet(builder.getInt64(-1));
                    builder.SetInsertPoint(rethrowSwitch);
                    auto* rethrowSw = builder.CreateSwitch(rethrowPc, rethrowUnhandled, static_cast<unsigned>(validPcs.size()));
                    for (uint64_t vp : validPcs) {
                        ensureBlock(vp);
                        rethrowSw->addCase(builder.getInt64(vp), blockByPc[vp]);
                    }

                    auto* stringDataBB = llvm::BasicBlock::Create(*context, "sys_string_data", fn);
                    sw->addCase(builder.getInt64(kSysStringData), stringDataBB);
                    builder.SetInsertPoint(stringDataBB);
                    auto* strData = builder.CreateCall(stringDataCallee, {readReg(2)});
                    writeReg(o.rd, strData);
                    builder.CreateBr(syscallDone);

                    // ── Platform OS services (syscalls 12–23) ──────────────
                    auto* threadCreateBB = llvm::BasicBlock::Create(*context, "sys_thread_create", fn);
                    sw->addCase(builder.getInt64(kSysThreadCreate), threadCreateBB);
                    builder.SetInsertPoint(threadCreateBB);
                    auto* tcRet = builder.CreateCall(threadCreateCallee, {readReg(2), readReg(3)});
                    writeReg(o.rd, tcRet);
                    builder.CreateBr(syscallDone);

                    auto* threadExitBB = llvm::BasicBlock::Create(*context, "sys_thread_exit", fn);
                    sw->addCase(builder.getInt64(kSysThreadExit), threadExitBB);
                    builder.SetInsertPoint(threadExitBB);
                    auto* teRet = builder.CreateCall(threadExitCallee, {readReg(2)});
                    writeReg(o.rd, teRet);
                    builder.CreateBr(syscallDone);

                    auto* futexBB = llvm::BasicBlock::Create(*context, "sys_futex", fn);
                    sw->addCase(builder.getInt64(kSysFutex), futexBB);
                    builder.SetInsertPoint(futexBB);
                    auto* futexRet = builder.CreateCall(futexCallee, {readReg(2), readReg(3), readReg(4)});
                    writeReg(o.rd, futexRet);
                    builder.CreateBr(syscallDone);

                    auto* getTidBB = llvm::BasicBlock::Create(*context, "sys_get_tid", fn);
                    sw->addCase(builder.getInt64(kSysGetTid), getTidBB);
                    builder.SetInsertPoint(getTidBB);
                    auto* gtRet = builder.CreateCall(getTidCallee, {});
                    writeReg(o.rd, gtRet);
                    builder.CreateBr(syscallDone);

                    auto* openBB = llvm::BasicBlock::Create(*context, "sys_open", fn);
                    sw->addCase(builder.getInt64(kSysOpen), openBB);
                    builder.SetInsertPoint(openBB);
                    auto* openRet = builder.CreateCall(openCallee, {readReg(2), readReg(3), readReg(4)});
                    writeReg(o.rd, openRet);
                    builder.CreateBr(syscallDone);

                    auto* readBB = llvm::BasicBlock::Create(*context, "sys_read", fn);
                    sw->addCase(builder.getInt64(kSysRead), readBB);
                    builder.SetInsertPoint(readBB);
                    auto* readRet = builder.CreateCall(readCallee, {readReg(2), readReg(3), readReg(4)});
                    writeReg(o.rd, readRet);
                    builder.CreateBr(syscallDone);

                    auto* writeBB = llvm::BasicBlock::Create(*context, "sys_write", fn);
                    sw->addCase(builder.getInt64(kSysWrite), writeBB);
                    builder.SetInsertPoint(writeBB);
                    auto* writeRet = builder.CreateCall(writeCallee, {readReg(2), readReg(3), readReg(4)});
                    writeReg(o.rd, writeRet);
                    builder.CreateBr(syscallDone);

                    auto* closeBB = llvm::BasicBlock::Create(*context, "sys_close", fn);
                    sw->addCase(builder.getInt64(kSysClose), closeBB);
                    builder.SetInsertPoint(closeBB);
                    auto* closeRet = builder.CreateCall(closeCallee, {readReg(2)});
                    writeReg(o.rd, closeRet);
                    builder.CreateBr(syscallDone);

                    auto* lseekBB = llvm::BasicBlock::Create(*context, "sys_lseek", fn);
                    sw->addCase(builder.getInt64(kSysLseek), lseekBB);
                    builder.SetInsertPoint(lseekBB);
                    auto* lseekRet = builder.CreateCall(lseekCallee, {readReg(2), readReg(3), readReg(4)});
                    writeReg(o.rd, lseekRet);
                    builder.CreateBr(syscallDone);

                    auto* fstatBB = llvm::BasicBlock::Create(*context, "sys_fstat", fn);
                    sw->addCase(builder.getInt64(kSysFstat), fstatBB);
                    builder.SetInsertPoint(fstatBB);
                    auto* fstatRet = builder.CreateCall(fstatCallee, {readReg(2), readReg(3)});
                    writeReg(o.rd, fstatRet);
                    builder.CreateBr(syscallDone);

                    auto* clockGetTimeBB = llvm::BasicBlock::Create(*context, "sys_clock_gettime", fn);
                    sw->addCase(builder.getInt64(kSysClockGetTime), clockGetTimeBB);
                    builder.SetInsertPoint(clockGetTimeBB);
                    auto* cgtRet = builder.CreateCall(clockGetTimeCallee, {readReg(2), readReg(3)});
                    writeReg(o.rd, cgtRet);
                    builder.CreateBr(syscallDone);

                    auto* getRandomBB = llvm::BasicBlock::Create(*context, "sys_getrandom", fn);
                    sw->addCase(builder.getInt64(kSysGetRandom), getRandomBB);
                    builder.SetInsertPoint(getRandomBB);
                    auto* grRet = builder.CreateCall(getRandomCallee, {readReg(2), readReg(3)});
                    writeReg(o.rd, grRet);
                    builder.CreateBr(syscallDone);

                    builder.SetInsertPoint(syscallErr);
                    writeReg(o.rd, builder.getInt64(0));
                    builder.CreateBr(syscallDone);
                    builder.SetInsertPoint(syscallDone);
                } else if (op == hvm::Opcode::BREAK) {
                    builder.CreateStore(builder.getInt1(true), builder.CreateStructGEP(stateTy, stateArg, 3));
                    builder.CreateRet(builder.getInt64(-1));
                    break;
                } else {
                    builder.CreateRet(builder.getInt64(-1));
                    break;
                }

                ipc = nextPc;
                if (ipc >= textSize) {
                    builder.CreateRet(readReg(1));
                    break;
                }
                if (blockByPc.count(ipc) && blockByPc[ipc] != builder.GetInsertBlock()) {
                    builder.CreateBr(blockByPc[ipc]);
                    break;
                }
                if (!blockByPc.count(ipc)) {
                    blockByPc[ipc] = llvm::BasicBlock::Create(*context, "pc_" + std::to_string(ipc), fn);
                    work.push_back(ipc);
                    builder.CreateBr(blockByPc[ipc]);
                    break;
                }
            }
        }
    }
    if (dwarfEnabled && diBuilder) {
        diBuilder->finalize();
    }

    if (llvm::verifyModule(*module, &llvm::errs())) {
        return llvm::createStringError(std::errc::invalid_argument, "generated HVM IR module verification failed");
    }
    return llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
}

bool HVMJIT::ensureJITFunctionTable(const std::shared_ptr<hvm::HOModule>& module) {
    if (!module) return false;
    const hvm::Section* text = module->getSection(".text");
    if (!text) return false;
    for (const auto& sym : module->getSymbols()) {
        if (sym.type != hvm::Symbol::STT_FUNC) continue;
        uint64_t pc = sym.value;
        while (pc < text->data.size()) {
            std::vector<uint8_t> slice;
            size_t maxRead = static_cast<size_t>(std::min<uint64_t>(8, text->data.size() - pc));
            slice.insert(slice.end(), text->data.begin() + static_cast<ptrdiff_t>(pc),
                         text->data.begin() + static_cast<ptrdiff_t>(pc + maxRead));
            size_t used = 0;
            auto ins = hvm::HVMInstruction::decode(slice, used);
            if (!ins || used == 0) return false;
            uint16_t func = 0;
            if (ins->getFormat() == hvm::InstructionFormat::R) {
                func = std::get<hvm::OperandsR>(ins->getOperands()).func;
            }

            if (!isSupportedForIRLowering(ins->getOpcode(), func)) {
                return false;
            }
            if (ins->getOpcode() == hvm::Opcode::RET) break;
            pc += used;
        }
    }
    return true;
}

bool HVMJIT::materializeModulesToJIT() {
    
    if (modulesMaterialized_) return true;
    for (const auto& [moduleName, module] : loadedModules_) {
        auto jdIt = moduleDylibs_.find(moduleName);
        if (jdIt == moduleDylibs_.end() || !jdIt->second) {
            
            continue;
        }
        
        if (!ensureJITFunctionTable(module)) {
            
            setError(ErrorPhase::Initialize, ErrorCode::UnsupportedInstruction,
                     "Module contains instructions outside current JIT-lowering subset: " + moduleName,
                     moduleName);
            modulesMaterialized_ = false;
            return false;
        }
        
        auto tsmOrErr = translateModule(*module);
        if (!tsmOrErr) {
            
            setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                     "Failed to translate module to LLVM IR: " + moduleName, moduleName);
            return false;
        }
        

        if (auto err = jit_->addIRModule(*jdIt->second, std::move(*tsmOrErr))) {
            auto errStr = llvm::toString(std::move(err));
            
            setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                     "Failed to add LLVM IR module to ORC: " + moduleName + " - " + errStr, moduleName);
            return false;
        }
        
    }
    modulesMaterialized_ = true;
    
    return true;
}

int64_t HVMJIT::runViaJIT(const std::string& entryPoint) {
    
    
    for (const auto& [name, _] : loadedModules_) {
        auto jdIt = moduleDylibs_.find(name);
        
    }
    if (!materializeModulesToJIT()) {
        
        return -1;
    }
    // Reset the HVM heap allocator for this run
    std::optional<uintptr_t> resolvedAddress;
    // Search the primary module's JITDylib (NOT the main dylib)
    auto primaryJdIt = moduleDylibs_.find(primaryModuleName_);
    auto candidates = buildLookupCandidates(entryPoint, primaryModuleName_);
    
    for (const auto& candidate : candidates) {
        // Try searching the primary module's dylib first
        if (primaryJdIt != moduleDylibs_.end() && primaryJdIt->second) {
            auto sym = jit_->lookup(*primaryJdIt->second, candidate);
            if (sym) {
                
                resolvedAddress = static_cast<uintptr_t>(sym->getValue());
                break;
            }
            consumeError(sym.takeError());
        }
        // Fallback: try the main JITDylib
        auto sym = jit_->lookup(candidate);
        if (sym) {
            
            resolvedAddress = static_cast<uintptr_t>(sym->getValue());
            break;
        } else {
            auto err = llvm::toString(sym.takeError());
            
        }
    }
    if (!resolvedAddress.has_value()) {
        setError(ErrorPhase::Execute, ErrorCode::MissingEntryPoint,
                 "JIT entry point not found: " + entryPoint, primaryModuleName_, entryPoint);
        return -1;
    }
    using EntryFn = int64_t(*)(HVMState*);
    auto fn = reinterpret_cast<EntryFn>(*resolvedAddress);
    HVMState state{};
    state.io = &io_;
    state.memory = memory_.data();
    state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
    lastRunUsedJIT_ = true;
    {
        std::lock_guard<std::mutex> lk(gStateOwnerMu);
        gStateOwnerByPtr[&state] = this;
    }
    const int64_t rv = fn(&state);
    {
        std::lock_guard<std::mutex> lk(gStateOwnerMu);
        gStateOwnerByPtr.erase(&state);
    }
    shadow_clear_state(&state);
    if (state.trapHit) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed, "BREAK trap encountered");
        return -1;
    }
    return rv;
}

int64_t HVMJIT::run(const std::string& entryPoint) {
    clearError();
    lastRunUsedJIT_ = false;
    stopExecutionRequested_.store(false, std::memory_order_relaxed);
    if (loadedModules_.empty()) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                 "No module loaded");
        return -1;
    }
    if (primaryModuleName_.empty()) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                 "Primary module is not set");
        return -1;
    }

    if (!initializeModules()) {
        return -1;
    }

    if (!ensureJIT()) {
        return -1;
    }
    std::string jitError;
    int64_t jitResult = runViaJIT(entryPoint);
    
    if (jitResult != -1) {
        return jitResult;
    }
    // JIT returned -1. Capture the JIT error but fall through to interpreter
    // if the error is a lookup/missing-entry failure (the interpreter has a
    // more flexible symbol resolution). Only skip fallback for hard JIT
    // translation/compilation errors.
    {
        jitError = lastError_;
        auto errInfo = lastErrorInfo_;
        if (errInfo && errInfo->code != ErrorCode::MissingEntryPoint) {
            
            return -1;
        }
    }
    
    clearError();
    lastRunUsedJIT_ = false;
    auto primary = loadedModules_[primaryModuleName_];
    HVMState state{};
    state.io = &io_;
    state.memory = memory_.data();
    state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
    const int64_t rv = executeFunction(primary, entryPoint, state);
    if (rv == -1 && !jitError.empty()) {
        lastError_ = "[JIT] " + jitError + " | [Interp] " + lastError_;
    }
    shadow_clear_state(&state);
    return rv;
}

void* HVMJIT::createInboundTrampoline(const std::string& moduleName, const std::string& functionName,
                                      size_t arity) {
    std::lock_guard<std::mutex> lk(inboundTrampolineMu_);
    const std::string key = moduleName + "::" + functionName;
    auto existing = inboundTrampolineIndexByTarget_.find(key);
    if (existing != inboundTrampolineIndexByTarget_.end()) {
        auto targetIt = inboundTrampolineTargets_.find(existing->second);
        if (targetIt != inboundTrampolineTargets_.end() && targetIt->second.arity != arity) {
            setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                     "Inbound trampoline arity mismatch for existing target: " + key);
            return nullptr;
        }
        if (arity == 1) {
            return reinterpret_cast<void*>(kInboundTrampolines[existing->second]);
        }
        if (arity == 2) {
            return reinterpret_cast<void*>(kInboundTrampolines2[existing->second]);
        }
        setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                 "Unsupported inbound trampoline arity: " + std::to_string(arity));
        return nullptr;
    }
    auto modIt = loadedModules_.find(moduleName);
    if (modIt == loadedModules_.end() || !modIt->second) {
        setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                 "Module not loaded for inbound trampoline: " + moduleName, moduleName, functionName);
        return nullptr;
    }
    if (!findFunctionSymbol(*modIt->second, functionName)) {
        setError(ErrorPhase::Resolve, ErrorCode::MissingEntryPoint,
                 "Function not found for inbound trampoline: " + functionName, moduleName, functionName);
        return nullptr;
    }
    if (arity == 0 || arity > 2) {
        setError(ErrorPhase::Resolve, ErrorCode::InvalidMetadata,
                 "Unsupported inbound trampoline arity: " + std::to_string(arity));
        return nullptr;
    }
    size_t slot = 0;
    {
        std::lock_guard<std::mutex> ownerLk(gInboundTrampolineOwnerMu);
        if (gInboundNextSlot >= kMaxInboundTrampolineSlots) {
            setError(ErrorPhase::Resolve, ErrorCode::ExecutionFailed,
                     "Inbound trampoline slot limit reached");
            return nullptr;
        }
        slot = gInboundNextSlot++;
        gInboundTrampolineOwnerBySlot[slot] = this;
    }
    inboundTrampolineTargets_[slot] = InboundTarget{moduleName, functionName, arity};
    inboundTrampolineIndexByTarget_[key] = slot;
    if (arity == 1) {
        return reinterpret_cast<void*>(kInboundTrampolines[slot]);
    }
    return reinterpret_cast<void*>(kInboundTrampolines2[slot]);
}

int64_t HVMJIT::invokeInboundCallback(size_t slot, const std::vector<uint64_t>& args) {
    InboundTarget target;
    {
        std::lock_guard<std::mutex> lk(inboundTrampolineMu_);
        auto it = inboundTrampolineTargets_.find(slot);
        if (it == inboundTrampolineTargets_.end()) {
            return -1;
        }
        target = it->second;
    }
    if (args.size() != target.arity) {
        return -1;
    }
    auto modIt = loadedModules_.find(target.moduleName);
    if (modIt == loadedModules_.end() || !modIt->second) {
        return -1;
    }
    if (!mapModuleSections(modIt->second)) {
        return -1;
    }
    HVMState state{};
    state.io = &io_;
    state.memory = memory_.data();
    state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
    for (size_t i = 0; i < args.size() && i < 7; ++i) {
        state.regs[1 + i] = static_cast<int64_t>(args[i]);
    }
    int64_t rv = -1;
    try {
        rv = executeFunction(modIt->second, target.functionName, state);
    } catch (const std::exception& ex) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                 "Inbound callback execution exception: " + std::string(ex.what()),
                 target.moduleName, target.functionName);
        shadow_clear_state(&state);
        return -1;
    } catch (...) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                 "Inbound callback execution exception", target.moduleName, target.functionName);
        shadow_clear_state(&state);
        return -1;
    }
    shadow_clear_state(&state);
    return rv;
}

bool HVMJIT::buildInspectorTrace(const std::string& entryPoint) {
    clearError();
    resetInspector();
    if (!ensureJIT()) {
        return false;
    }
    if (loadedModules_.empty() || primaryModuleName_.empty()) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed, "No module loaded for inspector trace");
        return false;
    }
    if (!initializeModules()) {
        return false;
    }
    auto primary = loadedModules_[primaryModuleName_];
    if (!primary) {
        setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed, "Primary module unavailable for inspector trace");
        return false;
    }
    HVMState state{};
    state.io = &io_;
    state.memory = memory_.data();
    state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
    inspectorCaptureEnabled_ = true;
    (void)executeFunction(primary, entryPoint, state);
    inspectorCaptureEnabled_ = false;
    shadow_clear_state(&state);
    return !inspectorTrace_.empty();
}

void HVMJIT::stopExecution() {
    stopExecutionRequested_.store(true, std::memory_order_relaxed);
}

std::array<int64_t, 32> HVMJIT::getRegisters() const {
    std::lock_guard<std::mutex> lk(lastRegistersMu_);
    return lastRegisters_;
}

std::vector<uint8_t> HVMJIT::readVirtualMemory(uint64_t addr, size_t size) const {
    if (size == 0 || addr >= memory_.size()) {
        return {};
    }
    size_t end = static_cast<size_t>(std::min<uint64_t>(memory_.size(), addr + size));
    return std::vector<uint8_t>(memory_.begin() + static_cast<ptrdiff_t>(addr),
                                memory_.begin() + static_cast<ptrdiff_t>(end));
}

bool HVMJIT::inspectorStep() {
    if (inspectorTrace_.empty()) {
        return false;
    }
    if (inspectorCursor_ + 1 >= inspectorTrace_.size()) {
        return false;
    }
    ++inspectorCursor_;
    return true;
}

std::optional<HVMJIT::InspectorSnapshot> HVMJIT::getInspectorSnapshot() const {
    if (inspectorTrace_.empty() || inspectorCursor_ >= inspectorTrace_.size()) {
        return std::nullopt;
    }
    return inspectorTrace_[inspectorCursor_];
}

void HVMJIT::resetInspector() {
    inspectorCaptureEnabled_ = false;
    inspectorTrace_.clear();
    inspectorCursor_ = 0;
    stopExecutionRequested_.store(false, std::memory_order_relaxed);
}

void* HVMJIT::getSymbolAddress(const std::string& mangledName) {
    clearError();
    if (!ensureJIT()) {
        return nullptr;
    }

    for (const auto& candidate : buildLookupCandidates(mangledName, primaryModuleName_)) {
        auto sym = jit_->lookup(candidate);
        if (sym) {
            return reinterpret_cast<void*>(static_cast<uintptr_t>(sym->getValue()));
        }
        consumeError(sym.takeError());

        auto mod = bundle_.findModuleBySymbolMangled(candidate);
        if (mod) {
            if (const auto* s = mod->findSymbolMangled(candidate)) {
                return reinterpret_cast<void*>(static_cast<uintptr_t>(s->address));
            }
        }
    }

    if (void* runtimeAddr = lookupPlainRuntimeSymbolAddress(mangledName)) {
        return runtimeAddr;
    }

    setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
             "Symbol not found: " + mangledName, "", mangledName);
    return nullptr;
}

bool HVMJIT::hasModuleJITDylib(const std::string& moduleName) const {
    auto it = moduleDylibs_.find(moduleName);
    return it != moduleDylibs_.end() && it->second != nullptr;
}

std::vector<std::string> HVMJIT::getModuleLogicalSearchOrder(const std::string& moduleName) const {
    auto it = moduleSearchOrder_.find(moduleName);
    if (it == moduleSearchOrder_.end()) {
        return {};
    }
    return it->second;
}

} // namespace hooc
