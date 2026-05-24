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
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_exception.h"

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

std::mutex gManagedObjectsMu;
std::unordered_set<uintptr_t> gManagedObjects;
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

void* tracked_alloc(size_t size, int64_t typeId) {
    void* obj = hoo_alloc(size, typeId);
    if (!obj) return nullptr;
    std::lock_guard<std::mutex> lk(gManagedObjectsMu);
    gManagedObjects.insert(reinterpret_cast<uintptr_t>(obj));
    return obj;
}

bool is_tracked_ptr(uintptr_t p) {
    if (p == 0) return false;
    std::lock_guard<std::mutex> lk(gManagedObjectsMu);
    return gManagedObjects.find(p) != gManagedObjects.end();
}

void* tracked_retain(uintptr_t p) {
    if (!is_tracked_ptr(p)) return reinterpret_cast<void*>(p);
    return hoo_retain(reinterpret_cast<void*>(p));
}

void tracked_release(uintptr_t p) {
    if (!is_tracked_ptr(p)) return;
    void* obj = reinterpret_cast<void*>(p);
    int64_t rc = hoo_get_refcount(obj);
    hoo_release(obj);
    if (rc <= 1) {
        std::lock_guard<std::mutex> lk(gManagedObjectsMu);
        gManagedObjects.erase(p);
    }
}

int64_t tracked_refcount(uintptr_t p) {
    if (!is_tracked_ptr(p)) return 0;
    return hoo_get_refcount(reinterpret_cast<void*>(p));
}

int64_t tracked_typeid(uintptr_t p) {
    if (!is_tracked_ptr(p)) return 0;
    return hoo_get_type_id(reinterpret_cast<void*>(p));
}

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
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(tracked_alloc(static_cast<size_t>(state->regs[1]), state->regs[2])));
    }
    uint64_t jit_hoo_retain(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(tracked_retain(static_cast<uintptr_t>(state->regs[1]))));
    }
    uint64_t jit_hoo_release(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        tracked_release(static_cast<uintptr_t>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_get_refcount(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(tracked_refcount(static_cast<uintptr_t>(state->regs[1])));
    }
    uint64_t jit_hoo_get_type_id(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(tracked_typeid(static_cast<uintptr_t>(state->regs[1])));
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
    uint64_t jit_hoo_character_from_utf8(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const char* bytes = reinterpret_cast<const char*>(state->memory + state->regs[1]);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_character_from_utf8(bytes, state->regs[2])));
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
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_character_data(reinterpret_cast<void*>(state->regs[1]))));
    }
    uint64_t jit_hoo_character_codepoint(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(hoo_character_codepoint(reinterpret_cast<void*>(state->regs[1])));
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
    uint64_t jit_hoo_array_new(void* /*state_ptr*/) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_array_new()));
    }
    uint64_t jit_hoo_array_push_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_array_push_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2]);
        return 0;
    }
    uint64_t jit_hoo_array_get_int64(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        int64_t* dest = reinterpret_cast<int64_t*>(state->memory + state->regs[3]);
        return static_cast<uint64_t>(hoo_array_get_int64(reinterpret_cast<void*>(state->regs[1]), state->regs[2], dest));
    }
    uint64_t jit_hoo_map_new(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hoo_map_new(state->regs[1])));
    }
    uint64_t jit_hoo_exception_runtime(void* /*state_ptr*/) {
        HooException exc = hoo_exception_runtime("hvm runtime exception");
        if (!exc) return 0;
        {
            std::lock_guard<std::mutex> lk(gManagedObjectsMu);
            gManagedObjects.insert(reinterpret_cast<uintptr_t>(exc));
        }
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(exc));
    }
    uint64_t jit_hoo_exception_clear(void* /*state_ptr*/) {
        hoo_exception_clear();
        return 0;
    }
    uint64_t jit_hoo_push_handler(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        const uint64_t handlerPc = static_cast<uint64_t>(state->regs[1]);
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
        try {
            hoo_exception_throw(reinterpret_cast<HooException>(state->regs[1]));
        } catch (...) {
            // Expected runtime throw path; handler transfer is performed by the JIT shadow stack.
        }
        return shadow_throw_to_handler(state, static_cast<uint64_t>(state->regs[1]), false);
    }
    uint64_t jit_hoo_rethrow(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        try {
            hoo_exception_rethrow();
        } catch (...) {
            // Expected runtime rethrow path; handler transfer is performed by the JIT shadow stack.
        }
        return shadow_throw_to_handler(state, 0, true);
    }

    // HVM internal sys calls (for interpreter)
    extern "C" uint64_t hooc_hvm_sys_alloc(uint64_t size, uint64_t typeId) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(tracked_alloc(static_cast<size_t>(size), static_cast<int64_t>(typeId))));
    }
    extern "C" uint64_t hooc_hvm_sys_retain(uint64_t obj) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(tracked_retain(static_cast<uintptr_t>(obj))));
    }
    extern "C" uint64_t hooc_hvm_sys_release(uint64_t obj) {
        tracked_release(static_cast<uintptr_t>(obj));
        return 0;
    }
    extern "C" uint64_t hooc_hvm_sys_refcount(uint64_t obj) {
        return static_cast<uint64_t>(tracked_refcount(static_cast<uintptr_t>(obj)));
    }
    extern "C" uint64_t hooc_hvm_sys_typeid(uint64_t obj) {
        return static_cast<uint64_t>(tracked_typeid(static_cast<uintptr_t>(obj)));
    }
    extern "C" uint64_t hooc_hvm_sys_exception_runtime(uint64_t /*reserved*/) {
        HooException exc = hoo_exception_runtime("hvm runtime exception");
        if (!exc) return 0;
        {
            std::lock_guard<std::mutex> lk(gManagedObjectsMu);
            gManagedObjects.insert(reinterpret_cast<uintptr_t>(exc));
        }
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
        try {
            hoo_exception_throw(reinterpret_cast<HooException>(exc));
        } catch (...) {
            // Expected runtime throw path; control transfer is completed by shadow handler routing.
        }
        return shadow_throw_to_handler(state, exc, false);
    }
    extern "C" uint64_t hooc_hvm_sys_rethrow_to_handler_state(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        try {
            hoo_exception_rethrow();
        } catch (...) {
            // Expected runtime rethrow path; control transfer is completed by shadow handler routing.
        }
        return shadow_throw_to_handler(state, 0, true);
    }
    extern "C" uint64_t hooc_hvm_sys_string_data(uint64_t strObj) {
        return static_cast<uint64_t>(
            reinterpret_cast<uintptr_t>(hoo_string_data(reinterpret_cast<void*>(strObj))));
    }
    extern "C" uint64_t hooc_hvm_sys_should_stop_state(void* state_ptr) {
        return shadow_should_stop_state(state_ptr);
    }

    void hooc_hvm_arc_retain_if_managed(uint64_t obj) {
        (void)tracked_retain(static_cast<uintptr_t>(obj));
    }
    void hooc_hvm_arc_release_if_managed(uint64_t obj) {
        tracked_release(static_cast<uintptr_t>(obj));
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
        {"_F_hoo_Character_from_utf8_p_p_i8", reinterpret_cast<void*>(&jit_hoo_character_from_utf8)},
        {"_F_hoo_Character_from_codepoint_p_i8", reinterpret_cast<void*>(&jit_hoo_character_from_codepoint)},
        {"_F_hoo_Character_length_i8_p", reinterpret_cast<void*>(&jit_hoo_character_length)},
        {"_F_hoo_Character_data_p_p", reinterpret_cast<void*>(&jit_hoo_character_data)},
        {"_F_hoo_Character_codepoint_i8_p", reinterpret_cast<void*>(&jit_hoo_character_codepoint)},
        {"_F_M_hoo_E_print_v_p", reinterpret_cast<void*>(&jit_hoo_print)},
        {"_F_M_hoo_E_println_v_p", reinterpret_cast<void*>(&jit_hoo_println)},
        {"_F_hoo_Array_new_p", reinterpret_cast<void*>(&jit_hoo_array_new)},
        {"_F_hoo_Array_push_i8_p_i8", reinterpret_cast<void*>(&jit_hoo_array_push_int64)},
        {"_F_hoo_Array_get_i8_p_i8_p", reinterpret_cast<void*>(&jit_hoo_array_get_int64)},
        {"_F_hoo_Map_new_p_i8", reinterpret_cast<void*>(&jit_hoo_map_new)},
        {"_F_hoo_exception_runtime_p", reinterpret_cast<void*>(&jit_hoo_exception_runtime)},
        {"_F_hoo_exception_clear_v", reinterpret_cast<void*>(&jit_hoo_exception_clear)},
        {"_F_hoo_push_handler_v_p", reinterpret_cast<void*>(&jit_hoo_push_handler)},
        {"_F_hoo_pop_handler_v", reinterpret_cast<void*>(&jit_hoo_pop_handler)},
        {"_F_hoo_throw_v_p", reinterpret_cast<void*>(&jit_hoo_throw)},
        {"_F_hoo_rethrow_v", reinterpret_cast<void*>(&jit_hoo_rethrow)},
    };
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
        if (llvm::JITEventListener* gdbListener = llvm::JITEventListener::createGDBRegistrationListener()) {
            jitDebugListener_.reset(gdbListener);
            rtLayer->registerJITEventListener(*jitDebugListener_);
        }
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
    return p.string();
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
        if (sym.type == hvm::Symbol::STT_FUNC) {
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
                              "_F_hoo_String_from_cstr_p_p");
    runtime->registerFunction("string_from_int64", reinterpret_cast<void*>(&hoo_string_from_int64),
                              "_F_hoo_String_from_int64_p_i8");
    runtime->registerFunction("string_from_double", reinterpret_cast<void*>(&hoo_string_from_double),
                              "_F_hoo_String_from_double_p_d");
    runtime->registerFunction("string_concat", reinterpret_cast<void*>(&hoo_string_concat),
                              "_F_hoo_String_concat_p_p_p");
    runtime->registerFunction("string_length", reinterpret_cast<void*>(&hoo_string_length),
                              "_F_hoo_String_length_i8_p");
    runtime->registerFunction("string_data", reinterpret_cast<void*>(&hoo_string_data),
                              "_F_hoo_String_data_p_p");
    runtime->registerFunction("string_to_characters", reinterpret_cast<void*>(&hoo_string_to_characters),
                              "_F_hoo_String_to_characters_p_p");
    runtime->registerFunction("string_join", reinterpret_cast<void*>(&hoo_string_join),
                              "_F_hoo_String_join_p_p");
    runtime->registerFunction("string_from_object", reinterpret_cast<void*>(&hoo_string_from_object),
                              "_F_hoo_String_from_object_p_p");
    runtime->registerFunction("string_from_any", reinterpret_cast<void*>(&hoo_string_from_any),
                              "_F_hoo_String_from_any_p_i8_i8");
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

    // 1. Exact match
    for (const auto& sym : symbols) {
        if (isFunc(sym) && sym.name == functionName && sym.section_index != -1) return &sym;
    }

    // 2. If it's a known non-mangled name, try common mangled forms
    std::string baseName = functionName;
    if (functionName.rfind("_F_", 0) == 0) {
        // Already mangled, try to extract base name for more flexible search if exact failed
        size_t start = 3;
        size_t end = functionName.find('_', start);
        if (end != std::string::npos) {
            baseName = functionName.substr(start, end - start);
        }
    }

    // 3. Robust prefix match for mangled functions (_F_baseName_...)
    std::string prefix = "_F_" + baseName + "_";
    for (const auto& sym : symbols) {
        if (isFunc(sym) && sym.name.rfind(prefix, 0) == 0 && sym.section_index != -1) return &sym;
    }

    // 4. Case-insensitive or fuzzy match (last resort for tests)
    for (const auto& sym : symbols) {
        if (isFunc(sym) && (sym.name.find(baseName) != std::string::npos) && sym.section_index != -1) return &sym;
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
    for (const auto& rs : buildRuntimeSymbols()) {
        if (rs.name && symbolName == rs.name && rs.addr) {
            auto fn = reinterpret_cast<StateAbiFn>(rs.addr);
            try {
                outValue = fn(&state);
                return true;
            } catch (const std::exception& ex) {
                setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                         "Runtime bridge exception in " + symbolName + ": " + ex.what());
                return false;
            } catch (...) {
                setError(ErrorPhase::Execute, ErrorCode::ExecutionFailed,
                         "Runtime bridge exception in " + symbolName);
                return false;
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
                writeReg(o.rd, readReg(o.rs) | (static_cast<uint64_t>(static_cast<uint16_t>(o.imm15)) << 32U));
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
                    default:
                        writeReg(o.rd, 0);
                        break;
                }
                break;
            }
            case hvm::Opcode::BREAK:
                lastError_ = "BREAK trap encountered";
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
    if (primaryModuleName_.empty()) {
        primaryModuleName_ = owned->getName();
    }

    if (!bootstrapRuntimeModules()) {
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!registerModuleInBundle(owned)) {
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!resolveAndLoadDependencies(*owned)) {
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
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!runPostLoadInitializers()) {
        rollbackModuleLoad(owned->getName());
        return false;
    }
    if (!configureJITDylibs()) {
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
    stateTy->setBody({llvm::ArrayType::get(i64, 32), i8Ptr, i8Ptr});
    llvm::PointerType* statePtrTy = llvm::PointerType::get(*context, 0);
    llvm::FunctionType* fnTy = llvm::FunctionType::get(i64, {statePtrTy}, false);

    const hvm::Section* text = hvmModule.getSection(".text");
    if (!text) {
        return llvm::createStringError(std::errc::invalid_argument, "missing .text section");
    }
    uint64_t rodataBase = 0;
    auto layoutIt = moduleLayouts_.find(hvmModule.getName());
    if (layoutIt != moduleLayouts_.end()) {
        rodataBase = layoutIt->second.rodataBase;
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
                    auto hi = builder.CreateShl(builder.getInt64(static_cast<uint16_t>(o.imm15)), builder.getInt64(32));
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
                    auto* sw = builder.CreateSwitch(builder.getInt64(static_cast<int64_t>(o.imm15)), syscallErr, 11);

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

                    builder.SetInsertPoint(syscallErr);
                    writeReg(o.rd, builder.getInt64(0));
                    builder.CreateBr(syscallDone);
                    builder.SetInsertPoint(syscallDone);
                } else if (op == hvm::Opcode::BREAK) {
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
        if (jdIt == moduleDylibs_.end() || !jdIt->second) continue;
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
            setError(ErrorPhase::Initialize, ErrorCode::ExecutionFailed,
                     "Failed to add LLVM IR module to ORC: " + moduleName, moduleName);
            return false;
        }
    }
    modulesMaterialized_ = true;
    return true;
}

int64_t HVMJIT::runViaJIT(const std::string& entryPoint) {
    if (!materializeModulesToJIT()) {
        return -1;
    }
    auto sym = jit_->lookup(entryPoint);
    if (!sym) {
        setError(ErrorPhase::Execute, ErrorCode::MissingEntryPoint,
                 "JIT entry point not found: " + entryPoint, primaryModuleName_, entryPoint);
        return -1;
    }
    using EntryFn = int64_t(*)(HVMState*);
    auto fn = reinterpret_cast<EntryFn>(static_cast<uintptr_t>(sym->getValue()));
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
    return rv;
}

int64_t HVMJIT::run(const std::string& entryPoint) {
    clearError();
    lastRunUsedJIT_ = false;
    stopExecutionRequested_.store(false, std::memory_order_relaxed);
    if (!ensureJIT()) {
        return -1;
    }
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

    int64_t jitResult = runViaJIT(entryPoint);
    if (jitResult != -1) {
        return jitResult;
    }
    // Fallback path for unsupported/failed JIT lowering.
    clearError();
    lastRunUsedJIT_ = false;
    auto primary = loadedModules_[primaryModuleName_];
    HVMState state{};
    state.io = &io_;
    state.memory = memory_.data();
    state.regs[31] = static_cast<int64_t>(memory_.size() - 16);
    const int64_t rv = executeFunction(primary, entryPoint, state);
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

    auto sym = jit_->lookup(mangledName);
    if (!sym) {
        auto mod = bundle_.findModuleBySymbolMangled(mangledName);
        if (mod) {
            if (const auto* s = mod->findSymbolMangled(mangledName)) {
                return reinterpret_cast<void*>(static_cast<uintptr_t>(s->address));
            }
        }
        setError(ErrorPhase::Resolve, ErrorCode::MissingDependency,
                 "Symbol not found: " + mangledName, "", mangledName);
        return nullptr;
    }

    return reinterpret_cast<void*>(static_cast<uintptr_t>(sym->getValue()));
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
