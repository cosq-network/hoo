#include "../lib/hoo_generic_array.h"
#include "RuntimeRegistry.h"
#include "RuntimeFunctionStorage.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <iostream>

using namespace llvm;
using namespace llvm::orc;
using namespace hooc::runtime;

// ============================================================================
// JIT Symbol Registration Callback
// ============================================================================

/**
 * Register Array runtime functions with HoocJIT.
 *
 * This callback is invoked by HoocJIT during initialization to register
 * all Array functions as JIT symbols.
 *
 * @param jit Reference to the LLJIT instance
 * @param mainDylib Reference to the main JIT dynamic library
 */
void hoo_array_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    // Helper macro for registering a function
    #define REGISTER_ARRAY_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_array_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_array_##name), \
                JITSymbolFlags::Exported \
            );

    // Creation functions
    REGISTER_ARRAY_FUNC(new)
    REGISTER_ARRAY_FUNC(from_buffer)
    REGISTER_ARRAY_FUNC(repeat)

    // Basic operations
    REGISTER_ARRAY_FUNC(length)
    REGISTER_ARRAY_FUNC(get)
    REGISTER_ARRAY_FUNC(set)
    REGISTER_ARRAY_FUNC(push)
    REGISTER_ARRAY_FUNC(pop)

    // Type-specific push functions
    REGISTER_ARRAY_FUNC(push_int64)
    REGISTER_ARRAY_FUNC(push_double)
    REGISTER_ARRAY_FUNC(push_float)
    REGISTER_ARRAY_FUNC(push_bool)
    REGISTER_ARRAY_FUNC(push_char)
    REGISTER_ARRAY_FUNC(push_string)
    REGISTER_ARRAY_FUNC(push_object)
    REGISTER_ARRAY_FUNC(push_array)

    // Reference counting
    REGISTER_ARRAY_FUNC(retain)
    REGISTER_ARRAY_FUNC(release)
    REGISTER_ARRAY_FUNC(refcount)

    // Utility
    REGISTER_ARRAY_FUNC(empty)
    REGISTER_ARRAY_FUNC(clear)

    // Type-specific get functions
    REGISTER_ARRAY_FUNC(get_int64)
    REGISTER_ARRAY_FUNC(get_double)
    REGISTER_ARRAY_FUNC(get_float)
    REGISTER_ARRAY_FUNC(get_bool)
    REGISTER_ARRAY_FUNC(get_char)
    REGISTER_ARRAY_FUNC(get_string)
    REGISTER_ARRAY_FUNC(get_object)
    REGISTER_ARRAY_FUNC(get_array)

    #undef REGISTER_ARRAY_FUNC

    // Define all symbols in the JIT
    auto err = mainDylib.define(absoluteSymbols(symbols));
    if (err) {
        llvm::errs() << "ERROR: Failed to register Array functions with JIT: "
                     << toString(std::move(err)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

/**
 * Declare Array runtime functions in LLVM module.
 *
 * This callback is invoked by LLVMCodeGenerator during module creation
 * to declare LLVM function prototypes for all Array functions.
 *
 * The userData parameter points to a RuntimeFunctionStorage structure,
 * which this callback accesses to populate the array function storage
 * with the declared function pointers.
 * These pointers are then used during code generation when Array
 * operations need to generate calls to the runtime functions.
 *
 * @param module LLVM module to declare functions in
 * @param context LLVM context for type creation
 * @param userData Pointer to RuntimeFunctionStorage structure
 */
void hoo_array_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    if (!userData) {
        std::cerr << "ERROR: Array registration callback received null userData\n";
        return;
    }

    auto fullStorage = static_cast<RuntimeFunctionStorage*>(userData);
    auto& storage = fullStorage->arrays;  // Access the Array function storage

    // Create LLVM types
    auto ptrTy = PointerType::get(context, 0);  // void*
    auto i64Ty = Type::getInt64Ty(context);     // int64_t
    auto voidTy = Type::getVoidTy(context);     // void

    // Helper macro for declaring functions with specific signatures
    #define DECLARE_ARRAY_FN(name, returnType, ...) \
    do { \
        std::vector<Type*> params = {__VA_ARGS__}; \
        auto funcType = FunctionType::get(returnType, params, false); \
        storage.hoo_array_##name##_func = Function::Create( \
            funcType, Function::ExternalLinkage, "hoo_array_" #name, &module); \
    } while(0);

    // Creation functions
    DECLARE_ARRAY_FN(new, ptrTy)
    DECLARE_ARRAY_FN(from_buffer, ptrTy, ptrTy, i64Ty)
    DECLARE_ARRAY_FN(repeat, ptrTy, ptrTy, i64Ty)

    // Basic operations
    DECLARE_ARRAY_FN(length, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(set, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(push, i64Ty, ptrTy, ptrTy)
    DECLARE_ARRAY_FN(pop, i64Ty, ptrTy, ptrTy)

    // Type-specialized push functions
    DECLARE_ARRAY_FN(push_int64, i64Ty, ptrTy, i64Ty)
    DECLARE_ARRAY_FN(push_double, i64Ty, ptrTy, Type::getDoubleTy(context))
    DECLARE_ARRAY_FN(push_float, i64Ty, ptrTy, Type::getFloatTy(context))
    DECLARE_ARRAY_FN(push_bool, i64Ty, ptrTy, Type::getInt1Ty(context))
    DECLARE_ARRAY_FN(push_char, i64Ty, ptrTy, Type::getInt32Ty(context))
    DECLARE_ARRAY_FN(push_byte, i64Ty, ptrTy, Type::getInt8Ty(context))

    // Reference counting
    DECLARE_ARRAY_FN(retain, ptrTy, ptrTy)
    DECLARE_ARRAY_FN(release, voidTy, ptrTy)
    DECLARE_ARRAY_FN(refcount, i64Ty, ptrTy)

    // Utility
    DECLARE_ARRAY_FN(empty, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(clear, voidTy, ptrTy)

    // Type-specific get functions
    DECLARE_ARRAY_FN(get_int64, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_double, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_float, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_bool, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_char, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_string, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_object, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_ARRAY_FN(get_array, i64Ty, ptrTy, i64Ty, ptrTy)

    #undef DECLARE_ARRAY_FN
}

// ============================================================================
// Auto-Registration
// ============================================================================

/**
 * Static registration of Array runtime with the central registry.
 *
 * This macro creates a static object that registers the Array runtime
 * callbacks during C++ static initialization (before main()).
 *
 * The registry will later invoke these callbacks at appropriate times:
 * - JIT callback: During HoocJIT construction
 * - LLVM callback: During code generation for each module
 */
HOOC_REGISTER_RUNTIME(
    Array,
    hoo_array_register_with_jit,
    hoo_array_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function (for linker compatibility)
// ============================================================================

/**
 * Explicitly register Array runtime.
 * This function exists to ensure the static object is linked in.
 * It can be called from LLVMCodeGenerator to force registration.
 */
namespace hooc {

void _hoo_array_ensure_registration() {
    // This function forces the linker to include hoo_array_registration.cpp
    // The static object initialization in this file will run when this is called.
}

} // namespace hooc
