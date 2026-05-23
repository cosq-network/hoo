#include "hvm/HVMJIT.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <sstream>

#include "hvm/HOModuleBase.h"
#include "runtime/lib/hoo_runtime.h"
#include "runtime/lib/hoo_string.h"
#include "runtime/lib/hoo_io.h"
#include "runtime/lib/hoo_generic_array.h"
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_exception.h"

#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
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

std::mutex gManagedObjectsMu;
std::unordered_set<uintptr_t> gManagedObjects;

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

// JIT-compatible wrappers for runtime functions.
// These follow the int64_t(void* HVMState) convention used by the JIT translator.
extern "C" {
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
        hoo_push_handler(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_pop_handler(void* /*state_ptr*/) {
        hoo_pop_handler();
        return 0;
    }
    uint64_t jit_hoo_throw(void* state_ptr) {
        auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
        hoo_exception_throw(reinterpret_cast<void*>(state->regs[1]));
        return 0;
    }
    uint64_t jit_hoo_rethrow(void* /*state_ptr*/) {
        hoo_exception_rethrow();
        return 0;
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

    void hooc_hvm_arc_retain_if_managed(uint64_t obj) {
        (void)tracked_retain(static_cast<uintptr_t>(obj));
    }
    void hooc_hvm_arc_release_if_managed(uint64_t obj) {
        tracked_release(static_cast<uintptr_t>(obj));
    }
}

std::vector<RuntimeSymbolContract> buildRuntimeSymbols() {
    return {
        {"_F_hoo_alloc_p_i8_i8", reinterpret_cast<void*>(&jit_hoo_alloc)},
        {"_F_hoo_retain_p_p", reinterpret_cast<void*>(&jit_hoo_retain)},
        {"_F_hoo_release_v_p", reinterpret_cast<void*>(&jit_hoo_release)},
        {"_F_hoo_get_refcount_i8_p", reinterpret_cast<void*>(&jit_hoo_get_refcount)},
        {"_F_hoo_get_type_id_i8_p", reinterpret_cast<void*>(&jit_hoo_get_type_id)},
        {"_F_hoo_String_from_cstr_p_p", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"_F_hoo_String_concat_p_p_p", reinterpret_cast<void*>(&jit_hoo_string_concat)},
        {"_F_hoo_String_length_i8_p", reinterpret_cast<void*>(&jit_hoo_string_length)},
        {"_F_hoo_String_to_upper_p_p", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
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
        // Backward compatibility
        {"hoo_alloc", reinterpret_cast<void*>(&jit_hoo_alloc)},
        {"hoo_retain", reinterpret_cast<void*>(&jit_hoo_retain)},
        {"hoo_release", reinterpret_cast<void*>(&jit_hoo_release)},
        {"hoo_get_refcount", reinterpret_cast<void*>(&jit_hoo_get_refcount)},
        {"hoo_get_type_id", reinterpret_cast<void*>(&jit_hoo_get_type_id)},
        {"hoo_string_from_cstr", reinterpret_cast<void*>(&jit_hoo_string_from_cstr)},
        {"hoo_string_concat", reinterpret_cast<void*>(&jit_hoo_string_concat)},
        {"hoo_string_length", reinterpret_cast<void*>(&jit_hoo_string_length)},
        {"hoo_string_to_upper", reinterpret_cast<void*>(&jit_hoo_string_to_upper)},
        {"hoo_print", reinterpret_cast<void*>(&jit_hoo_print)},
        {"hoo_println", reinterpret_cast<void*>(&jit_hoo_println)},
        {"hoo_array_new", reinterpret_cast<void*>(&jit_hoo_array_new)},
        {"hoo_array_push_int64", reinterpret_cast<void*>(&jit_hoo_array_push_int64)},
        {"hoo_array_get_int64", reinterpret_cast<void*>(&jit_hoo_array_get_int64)},
        {"hoo_map_new", reinterpret_cast<void*>(&jit_hoo_map_new)},
        {"hoo_exception_runtime", reinterpret_cast<void*>(&jit_hoo_exception_runtime)},
        {"hoo_exception_clear", reinterpret_cast<void*>(&jit_hoo_exception_clear)},
        {"hoo_push_handler", reinterpret_cast<void*>(&jit_hoo_push_handler)},
        {"hoo_pop_handler", reinterpret_cast<void*>(&jit_hoo_pop_handler)},
        {"hoo_throw", reinterpret_cast<void*>(&jit_hoo_throw)},
        {"hoo_rethrow", reinterpret_cast<void*>(&jit_hoo_rethrow)},
    };
}
} // namespace

HVMJIT::HVMJIT(IOProvider& io)
    : tsc_(std::make_unique<llvm::LLVMContext>())
    , io_(io)
    , sourceCompiler_(std::make_unique<HooCompiler>()) {
    memory_.resize(16 * 1024 * 1024, 0); // 16 MB initial virtual memory.
    memoryTop_ = 0x10000;                // keep low page unmapped-like.
}

HVMJIT::~HVMJIT() = default;
