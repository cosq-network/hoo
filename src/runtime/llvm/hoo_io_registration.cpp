#include "../lib/hoo_io.h"
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
 * Register I/O runtime functions with HoocJIT.
 */
void hoo_io_register_with_jit(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib)
{
    SymbolMap symbols;

    // Helper macro for registering a function
    #define REGISTER_IO_FUNC(name) \
        symbols[jit.mangleAndIntern("hoo_io_" #name)] = \
            ExecutorSymbolDef( \
                ExecutorAddr::fromPtr(&hoo_##name), \
                JITSymbolFlags::Exported \
            );

    // I/O functions
    REGISTER_IO_FUNC(print)
    REGISTER_IO_FUNC(println)
    REGISTER_IO_FUNC(readline)
    REGISTER_IO_FUNC(readchar)

    #undef REGISTER_IO_FUNC

    // Also register as "hoo" module functions (without prefix)
    // This allows hooc code to call print(), readline() directly
    SymbolMap hooSymbols;
    hooSymbols[jit.mangleAndIntern("hoo.print")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_print), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.println")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_println), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.readline")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_readline), JITSymbolFlags::Exported);
    hooSymbols[jit.mangleAndIntern("hoo.readchar")] =
        ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_readchar), JITSymbolFlags::Exported);

    // Define all symbols in the JIT
    auto err1 = mainDylib.define(absoluteSymbols(symbols));
    if (err1) {
        llvm::errs() << "ERROR: Failed to register I/O functions with JIT: "
                     << toString(std::move(err1)) << "\n";
        exit(1);
    }

    auto err2 = mainDylib.define(absoluteSymbols(hooSymbols));
    if (err2) {
        llvm::errs() << "ERROR: Failed to register hoo.* I/O functions with JIT: "
                     << toString(std::move(err2)) << "\n";
        exit(1);
    }
}

// ============================================================================
// LLVM Function Declaration Callback
// ============================================================================

/**
 * Declare I/O runtime functions in LLVM module.
 */
void hoo_io_declare_llvm_functions(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData)
{
    if (!userData) {
        std::cerr << "ERROR: I/O registration callback received null userData\n";
        return;
    }

    // I/O functions don't need storage in RuntimeFunctionStorage
    // They are directly declared and called

    // Create LLVM types
    auto ptrTy = PointerType::get(context, 0);  // void*
    auto i64Ty = Type::getInt64Ty(context);     // int64_t
    auto voidTy = Type::getVoidTy(context);     // void

    // hoo_print(void* str) -> void
    {
        std::vector<Type*> params = { ptrTy };
        auto funcType = FunctionType::get(voidTy, params, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo_print", &module);
    }

    // hoo_println(void* str) -> void
    {
        std::vector<Type*> params = { ptrTy };
        auto funcType = FunctionType::get(voidTy, params, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo_println", &module);
    }

    // hoo_readline() -> void*
    {
        auto funcType = FunctionType::get(ptrTy, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo_readline", &module);
    }

    // hoo_readchar() -> int64_t
    {
        auto funcType = FunctionType::get(i64Ty, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo_readchar", &module);
    }

    // Also declare hoo.* prefixed versions for direct module-level function calls
    // hoo.print(void* str) -> void
    {
        std::vector<Type*> params = { ptrTy };
        auto funcType = FunctionType::get(voidTy, params, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo.print", &module);
    }

    // hoo.println(void* str) -> void
    {
        std::vector<Type*> params = { ptrTy };
        auto funcType = FunctionType::get(voidTy, params, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo.println", &module);
    }

    // hoo.readline() -> void*
    {
        auto funcType = FunctionType::get(ptrTy, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo.readline", &module);
    }

    // hoo.readchar() -> int64_t
    {
        auto funcType = FunctionType::get(i64Ty, false);
        Function::Create(funcType, Function::ExternalLinkage, "hoo.readchar", &module);
    }
}

// ============================================================================
// Auto-Registration
// ============================================================================

/**
 * Static registration of IO runtime with the central registry.
 *
 * This macro creates a static object that registers the IO runtime
 * callbacks during C++ static initialization (before main()).
 */
HOOC_REGISTER_RUNTIME(
    IO,
    hoo_io_register_with_jit,
    hoo_io_declare_llvm_functions
)

// ============================================================================
// Explicit Initialization Function (for linker compatibility)
// ============================================================================

namespace hooc {

void _hoo_io_ensure_registration() {
    // Force the linker to include hoo_io_registration.cpp
    // The static RuntimeAutoRegister will run during this call
    auto& registry = runtime::RuntimeRegistry::getInstance();
    (void)registry.getRegisteredRuntimes();
}

} // namespace hooc