#include "../lib/hoo_string.h"
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
 * Register String runtime functions with HoocJIT.
 *
 * This callback is invoked by HoocJIT during initialization to register
 * all String functions as JIT symbols.
 *
 * @param jit Reference to the LLJIT instance
 * @param mainDylib Reference to the main JIT dynamic library
 */
void hoo_string_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    // Helper macro for registering a function
    #define REGISTER_STRING_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_string_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_string_##name), \
                JITSymbolFlags::Exported \
            );

    // Creation functions
    REGISTER_STRING_FUNC(from_cstr)
    REGISTER_STRING_FUNC(new)
    REGISTER_STRING_FUNC(from_bytes)
    REGISTER_STRING_FUNC(repeat)

    // Manipulation
    REGISTER_STRING_FUNC(concat)
    REGISTER_STRING_FUNC(substring)
    REGISTER_STRING_FUNC(to_upper)
    REGISTER_STRING_FUNC(to_lower)
    REGISTER_STRING_FUNC(trim)
    REGISTER_STRING_FUNC(replace)
    REGISTER_STRING_FUNC(split)

    // Query
    REGISTER_STRING_FUNC(length)
    REGISTER_STRING_FUNC(data)
    REGISTER_STRING_FUNC(byte_at)
    REGISTER_STRING_FUNC(is_empty)
    REGISTER_STRING_FUNC(index_of)
    REGISTER_STRING_FUNC(last_index_of)
    REGISTER_STRING_FUNC(contains)
    REGISTER_STRING_FUNC(starts_with)
    REGISTER_STRING_FUNC(ends_with)

    // Comparison
    REGISTER_STRING_FUNC(compare)
    REGISTER_STRING_FUNC(equals)
    REGISTER_STRING_FUNC(equals_ignore_case)

    // Reference counting
    REGISTER_STRING_FUNC(retain)
    REGISTER_STRING_FUNC(release)
    REGISTER_STRING_FUNC(refcount)

    // Conversion
    REGISTER_STRING_FUNC(from_int64)
    REGISTER_STRING_FUNC(from_double)
    REGISTER_STRING_FUNC(from_bool)
    REGISTER_STRING_FUNC(to_int64)
    REGISTER_STRING_FUNC(to_double)
    REGISTER_STRING_FUNC(format)

    // Debugging
    REGISTER_STRING_FUNC(print)
    REGISTER_STRING_FUNC(println)
    REGISTER_STRING_FUNC(debug)

    #undef REGISTER_STRING_FUNC

    // Define all symbols in the JIT
    auto err = mainDylib.define(absoluteSymbols(symbols));
    if (err) {
        llvm::errs() << "ERROR: Failed to register String functions with JIT: "
                     << toString(std::move(err)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

/**
 * Declare String runtime functions in LLVM module.
 *
 * This callback is invoked by LLVMCodeGenerator during module creation
 * to declare LLVM function prototypes for all String functions.
 *
 * The userData parameter points to a RuntimeFunctionStorage structure,
 * which this callback accesses to populate the string function storage
 * with the declared function pointers.
 * These pointers are then used during code generation when String
 * operations need to generate calls to the runtime functions.
 *
 * @param module LLVM module to declare functions in
 * @param context LLVM context for type creation
 * @param userData Pointer to RuntimeFunctionStorage structure
 */
void hoo_string_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    if (!userData) {
        std::cerr << "ERROR: String registration callback received null userData\n";
        return;
    }

    auto fullStorage = static_cast<RuntimeFunctionStorage*>(userData);
    auto& storage = fullStorage->strings;  // Access the String function storage

    // Create LLVM types
    auto ptrTy = PointerType::get(context, 0);  // void*
    auto i8Ty = Type::getInt8Ty(context);       // byte
    auto i64Ty = Type::getInt64Ty(context);     // int64_t
    auto f64Ty = Type::getDoubleTy(context);    // double
    auto voidTy = Type::getVoidTy(context);     // void

    // Helper macro for declaring functions with specific signatures
    #define DECLARE_STRING_FN(name, returnType, ...) \
    do { \
        std::vector<Type*> params = {__VA_ARGS__}; \
        auto funcType = FunctionType::get(returnType, params, false); \
        storage.hoo_string_##name##_func = Function::Create( \
            funcType, Function::ExternalLinkage, "hoo_string_" #name, &module); \
    } while(0);

    // Creation functions
    DECLARE_STRING_FN(from_cstr, ptrTy, ptrTy)
    DECLARE_STRING_FN(new, ptrTy)
    DECLARE_STRING_FN(from_bytes, ptrTy, ptrTy, i64Ty)
    DECLARE_STRING_FN(repeat, ptrTy, i8Ty, i64Ty)

    // Manipulation
    DECLARE_STRING_FN(concat, ptrTy, ptrTy, ptrTy)
    DECLARE_STRING_FN(substring, ptrTy, ptrTy, i64Ty, i64Ty)
    DECLARE_STRING_FN(to_upper, ptrTy, ptrTy)
    DECLARE_STRING_FN(to_lower, ptrTy, ptrTy)
    DECLARE_STRING_FN(trim, ptrTy, ptrTy)
    DECLARE_STRING_FN(replace, ptrTy, ptrTy, ptrTy, ptrTy)
    DECLARE_STRING_FN(split, ptrTy, ptrTy, ptrTy)

    // Query
    DECLARE_STRING_FN(length, i64Ty, ptrTy)
    DECLARE_STRING_FN(data, ptrTy, ptrTy)
    DECLARE_STRING_FN(byte_at, i64Ty, ptrTy, i64Ty)
    DECLARE_STRING_FN(is_empty, i64Ty, ptrTy)
    DECLARE_STRING_FN(index_of, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(last_index_of, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(contains, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(starts_with, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(ends_with, i64Ty, ptrTy, ptrTy)

    // Comparison
    DECLARE_STRING_FN(compare, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(equals, i64Ty, ptrTy, ptrTy)
    DECLARE_STRING_FN(equals_ignore_case, i64Ty, ptrTy, ptrTy)

    // Reference counting
    DECLARE_STRING_FN(retain, ptrTy, ptrTy)
    DECLARE_STRING_FN(release, voidTy, ptrTy)
    DECLARE_STRING_FN(refcount, i64Ty, ptrTy)

    // Conversion
    DECLARE_STRING_FN(from_int64, ptrTy, i64Ty)
    DECLARE_STRING_FN(from_double, ptrTy, f64Ty)
    DECLARE_STRING_FN(from_bool, ptrTy, i64Ty)
    DECLARE_STRING_FN(to_int64, i64Ty, ptrTy)
    DECLARE_STRING_FN(to_double, f64Ty, ptrTy)
    DECLARE_STRING_FN(format, ptrTy, ptrTy)

    // Debugging
    DECLARE_STRING_FN(print, voidTy, ptrTy)
    DECLARE_STRING_FN(println, voidTy, ptrTy)
    DECLARE_STRING_FN(debug, ptrTy, ptrTy)

    #undef DECLARE_STRING_FN
}

// ============================================================================
// Auto-Registration
// ============================================================================

/**
 * Static registration of String runtime with the central registry.
 *
 * This macro creates a static object that registers the String runtime
 * callbacks during C++ static initialization (before main()).
 *
 * The registry will later invoke these callbacks at appropriate times:
 * - JIT callback: During HoocJIT construction
 * - LLVM callback: During code generation for each module
 */
HOOC_REGISTER_RUNTIME(
    String,
    hoo_string_register_with_jit,
    hoo_string_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function (for linker compatibility)
// ============================================================================

/**
 * Explicitly register String runtime.
 * This function exists to ensure the static object is linked in.
 * It can be called from LLVMCodeGenerator to force registration.
 */
namespace hooc {

void _hoo_string_ensure_registration() {
    // This function forces the linker to include hoo_string_registration.cpp
    // The static object initialization in this file will run when this is called.
}

} // namespace hooc
