#pragma once

#include <memory>
#include <string>
#include <optional>
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

class HooCompiler;

class HoocJIT {
public:
    using Symbol = llvm::orc::ExecutorSymbolDef;

    HoocJIT();
    ~HoocJIT();

    HoocJIT(const HoocJIT&) = delete;
    HoocJIT& operator=(const HoocJIT&) = delete;
    HoocJIT(HoocJIT&&) noexcept;
    HoocJIT& operator=(HoocJIT&&) noexcept;

    struct CompileResult {
        bool success;
        std::string error;
        std::string ir;
    };

    struct ExecutionResult {
        bool success;
        std::string error;
    };

    CompileResult compile(const std::string& moduleName, const std::string& sourceCode);

    std::optional<ExecutionResult> execute(const std::string& functionName) {
        auto result = executeRaw(functionName);
        if (!result.success) {
            return result;
        }
        return std::nullopt;
    }

    std::optional<Symbol> lookup(const std::string& symbolName);

    std::string getLastError() const { return lastError_; }

    bool hasError() const { return !lastError_.empty(); }

    void clearError() { lastError_.clear(); }

    llvm::orc::LLJIT& getJIT() { return *jit_; }
    const llvm::orc::LLJIT& getJIT() const { return *jit_; }

private:
    ExecutionResult executeRaw(const std::string& functionName);

    bool initializeJIT();
    bool addModule(std::unique_ptr<llvm::Module> module);

    std::unique_ptr<llvm::orc::LLJIT> jit_;
    std::unique_ptr<HooCompiler> compiler_;
    std::string lastError_;
};

} // namespace hooc
