#include "../lib/hoo_map.h"
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

void hoo_map_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    #define REGISTER_MAP_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_map_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_map_##name), \
                JITSymbolFlags::Exported \
            );

    // Creation functions
    REGISTER_MAP_FUNC(new)
    REGISTER_MAP_FUNC(from_pairs)

    // Basic operations
    REGISTER_MAP_FUNC(length)
    REGISTER_MAP_FUNC(clear)
    REGISTER_MAP_FUNC(empty)

    // Key-specific operations (int8 key)
    REGISTER_MAP_FUNC(contains_int8)
    REGISTER_MAP_FUNC(remove_int8)
    REGISTER_MAP_FUNC(set_int8_int64)
    REGISTER_MAP_FUNC(get_int8_int64)
    REGISTER_MAP_FUNC(set_int8_value)
    REGISTER_MAP_FUNC(get_int8_value)

    // Key-specific operations (int64 key)
    REGISTER_MAP_FUNC(contains_int64)
    REGISTER_MAP_FUNC(remove_int64)
    REGISTER_MAP_FUNC(set_int64_int64)
    REGISTER_MAP_FUNC(get_int64_int64)
    REGISTER_MAP_FUNC(set_int64_value)
    REGISTER_MAP_FUNC(get_int64_value)

    // Key-specific operations (char key)
    REGISTER_MAP_FUNC(contains_char)
    REGISTER_MAP_FUNC(remove_char)
    REGISTER_MAP_FUNC(set_char_double)
    REGISTER_MAP_FUNC(get_char_double)
    REGISTER_MAP_FUNC(set_char_value)
    REGISTER_MAP_FUNC(get_char_value)

    // Key-specific operations (string key)
    REGISTER_MAP_FUNC(contains_string)
    REGISTER_MAP_FUNC(remove_string)
    REGISTER_MAP_FUNC(set_string_int64)
    REGISTER_MAP_FUNC(get_string_int64)
    REGISTER_MAP_FUNC(set_string_double)
    REGISTER_MAP_FUNC(get_string_double)
    REGISTER_MAP_FUNC(set_string_string)
    REGISTER_MAP_FUNC(get_string_string)
    REGISTER_MAP_FUNC(set_string_object)
    REGISTER_MAP_FUNC(get_string_object)
    REGISTER_MAP_FUNC(set_string_value)
    REGISTER_MAP_FUNC(get_string_value)

    // Reference counting
    REGISTER_MAP_FUNC(retain)
    REGISTER_MAP_FUNC(release)
    REGISTER_MAP_FUNC(refcount)

    // Utility
    REGISTER_MAP_FUNC(key_type)

    #undef REGISTER_MAP_FUNC

    auto err = mainDylib.define(absoluteSymbols(symbols));
    if (err) {
        llvm::errs() << "ERROR: Failed to register Map functions with JIT: "
                     << toString(std::move(err)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

void hoo_map_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    if (!userData) {
        std::cerr << "ERROR: Map registration callback received null userData\n";
        return;
    }

    auto fullStorage = static_cast<RuntimeFunctionStorage*>(userData);
    auto& storage = fullStorage->maps;

    auto ptrTy = PointerType::get(context, 0);
    auto i64Ty = Type::getInt64Ty(context);
    auto i8Ty = Type::getInt8Ty(context);
    auto i32Ty = Type::getInt32Ty(context);
    auto voidTy = Type::getVoidTy(context);
    auto doubleTy = Type::getDoubleTy(context);
    auto floatTy = Type::getFloatTy(context);
    auto i1Ty = Type::getInt1Ty(context);

    #define DECLARE_MAP_FN(name, returnType, ...) \
    do { \
        std::vector<Type*> params = {__VA_ARGS__}; \
        auto funcType = FunctionType::get(returnType, params, false); \
        storage.hoo_map_##name##_func = Function::Create( \
            funcType, Function::ExternalLinkage, "hoo_map_" #name, &module); \
    } while(0);

    // Creation functions
    DECLARE_MAP_FN(new, ptrTy, i64Ty)
    DECLARE_MAP_FN(from_pairs, ptrTy, i64Ty, ptrTy, ptrTy, i64Ty)

    // Basic operations
    DECLARE_MAP_FN(length, i64Ty, ptrTy)
    DECLARE_MAP_FN(clear, voidTy, ptrTy)
    DECLARE_MAP_FN(empty, i64Ty, ptrTy)

    // int8 key operations
    DECLARE_MAP_FN(contains_int8, i64Ty, ptrTy, i8Ty)
    DECLARE_MAP_FN(remove_int8, i64Ty, ptrTy, i8Ty)
    DECLARE_MAP_FN(set_int8_int64, i64Ty, ptrTy, i8Ty, i64Ty)
    DECLARE_MAP_FN(get_int8_int64, i64Ty, ptrTy, i8Ty, ptrTy)
    DECLARE_MAP_FN(set_int8_value, i64Ty, ptrTy, i8Ty, ptrTy)
    DECLARE_MAP_FN(get_int8_value, i64Ty, ptrTy, i8Ty, ptrTy)

    // int64 key operations
    DECLARE_MAP_FN(contains_int64, i64Ty, ptrTy, i64Ty)
    DECLARE_MAP_FN(remove_int64, i64Ty, ptrTy, i64Ty)
    DECLARE_MAP_FN(set_int64_int64, i64Ty, ptrTy, i64Ty, i64Ty)
    DECLARE_MAP_FN(get_int64_int64, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_MAP_FN(set_int64_value, i64Ty, ptrTy, i64Ty, ptrTy)
    DECLARE_MAP_FN(get_int64_value, i64Ty, ptrTy, i64Ty, ptrTy)

    // char key operations
    DECLARE_MAP_FN(contains_char, i64Ty, ptrTy, i32Ty)
    DECLARE_MAP_FN(remove_char, i64Ty, ptrTy, i32Ty)
    DECLARE_MAP_FN(set_char_double, i64Ty, ptrTy, i32Ty, doubleTy)
    DECLARE_MAP_FN(get_char_double, i64Ty, ptrTy, i32Ty, ptrTy)
    DECLARE_MAP_FN(set_char_value, i64Ty, ptrTy, i32Ty, ptrTy)
    DECLARE_MAP_FN(get_char_value, i64Ty, ptrTy, i32Ty, ptrTy)

    // string key operations
    DECLARE_MAP_FN(contains_string, i64Ty, ptrTy, ptrTy)
    DECLARE_MAP_FN(remove_string, i64Ty, ptrTy, ptrTy)
    DECLARE_MAP_FN(set_string_int64, i64Ty, ptrTy, ptrTy, i64Ty)
    DECLARE_MAP_FN(get_string_int64, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(set_string_double, i64Ty, ptrTy, ptrTy, doubleTy)
    DECLARE_MAP_FN(get_string_double, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(set_string_string, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(get_string_string, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(set_string_object, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(get_string_object, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(set_string_value, i64Ty, ptrTy, ptrTy, ptrTy)
    DECLARE_MAP_FN(get_string_value, i64Ty, ptrTy, ptrTy, ptrTy)

    // Reference counting
    DECLARE_MAP_FN(retain, ptrTy, ptrTy)
    DECLARE_MAP_FN(release, voidTy, ptrTy)
    DECLARE_MAP_FN(refcount, i64Ty, ptrTy)

    // Utility
    DECLARE_MAP_FN(key_type, i64Ty, ptrTy)

    #undef DECLARE_MAP_FN
}

// ============================================================================
// Auto-Registration
// ============================================================================

HOOC_REGISTER_RUNTIME(
    Map,
    hoo_map_register_with_jit,
    hoo_map_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function
// ============================================================================

namespace hooc {

void _hoo_map_ensure_registration() {
}

} // namespace hooc