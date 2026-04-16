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
// HoocJIT - JIT Compilation and Execution Engine
// ============================================================================

class HoocJIT {

public:

    // ========================================================================
    // Type Aliases
    // ========================================================================

    using Symbol = llvm::orc::ExecutorSymbolDef;

    struct CompileResult {
        bool        success;
        std::string error;
        std::string ir;
    };

    struct ExecutionResult {
        bool        success;
        std::string error;
    };

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    HoocJIT();
    ~HoocJIT();

    HoocJIT(const HoocJIT&)            = delete;
    HoocJIT& operator=(const HoocJIT&) = delete;

    HoocJIT(HoocJIT&& other) noexcept;
    HoocJIT& operator=(HoocJIT&& other) noexcept;

    // ========================================================================
    // Compilation API
    // ========================================================================

    CompileResult compile(const std::string& moduleName,
                         const std::string& sourceCode);

    // ========================================================================
    // Execution API
    // ========================================================================

    std::optional<ExecutionResult> execute(const std::string& functionName) {
        auto result = executeRaw(functionName);
        if (result.success) {
            return result;
        }
        return std::nullopt;
    }

    template<typename T>
    std::optional<ExecutionResult> executeTyped(const std::string& functionName) {
        auto symbolOrError = jit_->lookup(functionName);

        if (!symbolOrError) {
            std::ostringstream oss;
            oss << "Function '" << functionName << "' not found: "
                << llvm::toString(symbolOrError.takeError());
            return ExecutionResult{false, oss.str()};
        }

        auto addr = symbolOrError->getValue();
        using FuncPtr = T(*)();
        auto funcPtr = reinterpret_cast<FuncPtr>(static_cast<uintptr_t>(addr));

        funcPtr();
        return ExecutionResult{true, {}};
    }

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
    // Private Methods
    // ========================================================================

    ExecutionResult executeRaw(const std::string& functionName);
    bool initializeJIT();
    bool addModule(std::unique_ptr<llvm::Module> module);

    // ========================================================================
    // Member Variables
    // ========================================================================

    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<HooCompiler>       compiler_;
    std::string                       lastError_;
};

}  // namespace hooc
