#ifndef HOO_COMPILER_H
#define HOO_COMPILER_H

#include <string>
#include <memory>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

// ============================================================================
// Forward Declarations
// ============================================================================

class ProcessIsolatedParser;
class SimpleASTBuilder;
class CodeGenerator;

// ============================================================================
// HooCompiler - Main Compilation Interface
// ============================================================================

class HooCompiler {

public:

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    HooCompiler();
    ~HooCompiler();

    // ========================================================================
    // Compilation API
    // ========================================================================

    std::unique_ptr<llvm::Module> compile(const std::string& moduleName,
                                         const std::string& sourceCode);

    const std::string& getLastError() const {
        return lastError_;
    }

    bool wasLastCompilationSuccessful() const {
        return lastCompilationSuccessful_;
    }

private:

    // ========================================================================
    // Compilation Pipeline Components
    // ========================================================================

    std::unique_ptr<ProcessIsolatedParser> parser_;
    std::unique_ptr<SimpleASTBuilder>      astBuilder_;
    std::unique_ptr<CodeGenerator>         codeGenerator_;
    std::unique_ptr<llvm::LLVMContext>     context_;

    // ========================================================================
    // Error State
    // ========================================================================

    std::string lastError_;
    bool        lastCompilationSuccessful_;

    // ========================================================================
    // Non-Copyable
    // ========================================================================

    HooCompiler(const HooCompiler&)            = delete;
    HooCompiler& operator=(const HooCompiler&) = delete;
};

}  // namespace hooc

#endif  // HOO_COMPILER_H
