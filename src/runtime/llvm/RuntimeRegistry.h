#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {
namespace runtime {

// ============================================================================
// Runtime Registration Callback Types
// ============================================================================

/**
 * JIT Symbol Registration Callback
 *
 * Called during HoocJIT initialization to register runtime functions as JIT symbols.
 * Runtime developers have full control over symbol registration.
 *
 * @param jit Reference to the LLJIT instance
 * @param mainDylib Reference to the main JIT dynamic library
 */
using RuntimeJITRegistrationCallback = void(*)(
    llvm::orc::LLJIT& jit,
    llvm::orc::JITDylib& mainDylib
);

/**
 * LLVM Function Declaration Callback
 *
 * Called during code generation to declare runtime functions in LLVM modules.
 * Runtime developers declare LLVM function prototypes and store references
 * in the userData storage for later use during code generation.
 *
 * @param module LLVM module to declare functions in
 * @param context LLVM context for creating types
 * @param userData Opaque pointer to LLVMCodeGenerator's RuntimeFunctionStorage
 *                 Cast to access storage for your runtime type
 */
using RuntimeLLVMDeclarationCallback = void(*)(
    llvm::Module& module,
    llvm::LLVMContext& context,
    void* userData
);

// ============================================================================
// Runtime Registration Entry
// ============================================================================

/**
 * Entry point for a runtime library registration.
 *
 * Each runtime library (String, Array, Dict, etc.) provides two callbacks:
 * 1. JIT callback for symbol registration
 * 2. LLVM callback for function declarations
 */
struct RuntimeRegistrationEntry {
    const char* runtimeName;                          ///< Name of the runtime (e.g., "String")
    RuntimeJITRegistrationCallback jitCallback;       ///< JIT symbol registration
    RuntimeLLVMDeclarationCallback llvmCallback;      ///< LLVM function declaration
};

// ============================================================================
// Central Runtime Registry
// ============================================================================

/**
 * Central registry for all runtime libraries.
 *
 * This singleton class collects registration callbacks from all runtime libraries
 * (via static initialization) and provides batch invocation methods for:
 * - HoocJIT: to register JIT symbols
 * - LLVMCodeGenerator: to declare LLVM functions
 *
 * Runtime libraries self-register using the HOOC_REGISTER_RUNTIME macro,
 * which creates a static RuntimeAutoRegister object that registers the callbacks
 * during C++ static initialization (before main()).
 *
 * Example usage in a runtime library (e.g., src/rt/hoo_string_registration.cpp):
 *
 *   static void hoo_string_register_jit(LLJIT& jit, JITDylib& mainDylib) {
 *       // Register all String functions with JIT
 *       SymbolMap symbols;
 *       symbols[jit.mangleAndIntern("hoo_string_concat")] =
 *           ExecutorSymbolDef(ExecutorAddr::fromPtr(&hoo_string_concat), ...);
 *       mainDylib.define(absoluteSymbols(symbols));
 *   }
 *
 *   static void hoo_string_declare_llvm(Module& mod, LLVMContext& ctx, void* userData) {
 *       // Declare all String LLVM functions
 *       auto storage = static_cast<StringFunctionStorage*>(userData);
 *       // ... create LLVM function prototypes and store in storage ...
 *   }
 *
 *   HOOC_REGISTER_RUNTIME(String, hoo_string_register_jit, hoo_string_declare_llvm)
 */
class RuntimeRegistry {
public:
    /**
     * Get singleton instance of the registry.
     * Safe to call from static initialization contexts.
     */
    static RuntimeRegistry& getInstance();

    /**
     * Register a runtime library.
     * Typically called via HOOC_REGISTER_RUNTIME macro during static init.
     *
     * @param entry Registration entry with runtime name and callbacks
     */
    void registerRuntime(const RuntimeRegistrationEntry& entry);

    /**
     * Invoke all registered JIT callbacks.
     * Called by HoocJIT constructor to register symbols with the JIT.
     *
     * @param jit Reference to LLJIT instance
     * @param mainDylib Reference to main dynamic library
     */
    void registerAllWithJIT(
        llvm::orc::LLJIT& jit,
        llvm::orc::JITDylib& mainDylib);

    /**
     * Invoke all registered LLVM callbacks.
     * Called by LLVMCodeGenerator to declare functions in the module.
     *
     * @param module LLVM module to declare functions in
     * @param context LLVM context for type creation
     * @param userData Opaque pointer passed to all callbacks
     *                 Typically points to LLVMCodeGenerator's RuntimeFunctionStorage
     */
    void declareAllFunctions(
        llvm::Module& module,
        llvm::LLVMContext& context,
        void* userData);

    /**
     * Get list of all registered runtimes.
     * Useful for debugging and validation.
     */
    const std::vector<RuntimeRegistrationEntry>& getRegisteredRuntimes() const;

private:
    RuntimeRegistry() = default;
    std::vector<RuntimeRegistrationEntry> runtimes_;
};

// ============================================================================
// Auto-Registration Helper
// ============================================================================

/**
 * RAII helper for automatic registration during static initialization.
 *
 * This class is instantiated as a static object, causing its constructor
 * to run during C++ static initialization, which registers the callbacks
 * with the central RuntimeRegistry.
 */
class RuntimeAutoRegister {
public:
    /**
     * Register a runtime library.
     * Creates a registry entry and registers it immediately.
     *
     * @param entry Registration entry with runtime name and callbacks
     */
    RuntimeAutoRegister(const RuntimeRegistrationEntry& entry) {
        RuntimeRegistry::getInstance().registerRuntime(entry);
    }
};

} // namespace runtime
} // namespace hooc

// ============================================================================
// Registration Macro
// ============================================================================

/**
 * Convenience macro for registering a runtime library.
 *
 * Place this macro in your runtime's registration file (e.g., hoo_string_registration.cpp)
 * after defining your JIT and LLVM callbacks.
 *
 * Example:
 *   HOOC_REGISTER_RUNTIME(String, hoo_string_register_jit, hoo_string_declare_llvm)
 *
 * This expands to:
 *   static ::hooc::runtime::RuntimeAutoRegister
 *       __hooc_runtime_auto_register_String({
 *           "String",
 *           hoo_string_register_jit,
 *           hoo_string_declare_llvm
 *       });
 *
 * The static initialization will run automatically before main(), registering
 * the callbacks with the central RuntimeRegistry.
 */
#define HOOC_REGISTER_RUNTIME(Name, JitFn, LlvmFn) \
    static ::hooc::runtime::RuntimeAutoRegister \
        __hooc_runtime_auto_register_##Name({ \
            #Name, \
            JitFn, \
            LlvmFn \
        });
