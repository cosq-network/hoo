#pragma once

#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

// ============================================================================
// Forward Declarations
// ============================================================================

class HooCompiler;

// ============================================================================
// Result Types
// ============================================================================

struct CompileResult {
    bool        success;
    std::string error;
    std::string ir;

    static CompileResult ok(std::string ir = {}) {
        return {true, {}, std::move(ir)};
    }

    static CompileResult fail(std::string error) {
        return {false, std::move(error), {}};
    }
};

struct ExecutionResult {
    bool        success;
    std::string error;

    static ExecutionResult ok() {
        return {true, {}};
    }

    static ExecutionResult fail(std::string error) {
        return {false, std::move(error)};
    }
};

// ============================================================================
// Symbol Handle
// ============================================================================

using Symbol = llvm::orc::ExecutorSymbolDef;

// ============================================================================
// HoocJIT - JIT Compilation and Execution Engine
// ============================================================================

class HoocJIT {

public:

    // ========================================================================
    // Lifecycle
    // ========================================================================

    HoocJIT();
    ~HoocJIT();

    HoocJIT(const HoocJIT&)            = delete;
    HoocJIT& operator=(const HoocJIT&) = delete;

    HoocJIT(HoocJIT&& other) noexcept;
    HoocJIT& operator=(HoocJIT&& other) noexcept;

    // ========================================================================
    // Compilation
    // ========================================================================

    CompileResult compile(const std::string& moduleName,
                         const std::string& sourceCode);

    // ========================================================================
    // Execution
    // ========================================================================

    std::optional<ExecutionResult> execute(const std::string& functionName);

    template<typename ReturnType>
    std::optional<ExecutionResult> executeFunction(const std::string& functionName);

    std::optional<Symbol> lookup(const std::string& symbolName);

    // ========================================================================
    // Error Handling
    // ========================================================================

    std::string getLastError() const {
        return lastError_;
    }

    bool hasError() const {
        return !lastError_.empty();
    }

    void clearError() {
        lastError_.clear();
    }

    // ========================================================================
    // Accessors
    // ========================================================================

    llvm::orc::LLJIT& getJIT() {
        return *jit_;
    }

    const llvm::orc::LLJIT& getJIT() const {
        return *jit_;
    }

private:

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    bool initialize();
    bool addModuleToJIT(std::unique_ptr<llvm::Module> module);
    bool verifyAndAddModule(std::unique_ptr<llvm::Module> module,
                            std::string& outIR);

    std::string getIRFromModule(const llvm::Module& module) const;

    ExecutionResult executeVoidFunction(const std::string& functionName);

    std::optional<ExecutionResult> executeTypedFunction(
        const std::string& functionName);

    void setError(std::string error) {
        lastError_ = std::move(error);
    }

    // ========================================================================
    // State
    // ========================================================================

    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<HooCompiler>       compiler_;
    std::string                       lastError_;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename ReturnType>
std::optional<ExecutionResult> HoocJIT::executeFunction(
    const std::string& functionName) {

    auto symbolOrError = jit_->lookup(functionName);

    if (!symbolOrError) {
        std::ostringstream oss;
        oss << "Function '" << functionName << "' not found: "
            << llvm::toString(symbolOrError.takeError());
        return ExecutionResult::fail(oss.str());
    }

    auto addr = symbolOrError->getValue();
    using FuncPtr = ReturnType(*)();
    auto funcPtr = reinterpret_cast<FuncPtr>(
        static_cast<uintptr_t>(addr)
    );

    funcPtr();
    return ExecutionResult::ok();
}

}  // namespace hooc
