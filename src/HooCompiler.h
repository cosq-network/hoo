#ifndef HOO_COMPILER_H
#define HOO_COMPILER_H

#include <string>
#include <memory>
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

namespace hooc {

// Forward declarations
class ProcessIsolatedParser;
class SimpleASTBuilder;
class LLVMCodeGenerator;

/**
 * HooCompiler - Main compilation interface
 * 
 * Provides a high-level interface for compiling hoo source code
 * to LLVM IR modules. Abstracts the entire compilation pipeline:
 * Source Code -> Parse -> AST -> LLVM IR Module
 */
class HooCompiler {
public:
    HooCompiler();
    ~HooCompiler();

    /**
     * Compile hoo source code to LLVM IR module
     * 
     * @param moduleName Name for the LLVM module
     * @param sourceCode The hoo source code to compile
     * @return Unique pointer to LLVM Module, or nullptr on failure
     */
    std::unique_ptr<llvm::Module> compile(const std::string& moduleName, 
                                        const std::string& sourceCode);

    /**
     * Get the last compilation error message
     * @return Error message from last failed compilation
     */
    const std::string& getLastError() const { return lastError_; }

    /**
     * Check if last compilation was successful
     * @return true if last compilation succeeded
     */
    bool wasLastCompilationSuccessful() const { return lastCompilationSuccessful_; }

private:
    std::unique_ptr<ProcessIsolatedParser> parser_;
    std::unique_ptr<SimpleASTBuilder> astBuilder_;
    std::unique_ptr<LLVMCodeGenerator> codeGenerator_;
    std::unique_ptr<llvm::LLVMContext> context_;
    
    std::string lastError_;
    bool lastCompilationSuccessful_;
    
    // Non-copyable
    HooCompiler(const HooCompiler&) = delete;
    HooCompiler& operator=(const HooCompiler&) = delete;
};

} // namespace hooc

#endif // HOO_COMPILER_H