#include "HooCompiler.h"
#include "ProcessIsolatedParser.h"
#include "SimpleASTBuilder.h"
#include "LLVMCodeGenerator.h"
#include "HoocParser.h"

namespace hooc {

// ============================================================================
// Construction / Destruction
// ============================================================================

HooCompiler::HooCompiler()
    : lastCompilationSuccessful_(false) {

    parser_      = std::make_unique<ProcessIsolatedParser>();
    astBuilder_  = std::make_unique<SimpleASTBuilder>();
    context_     = std::make_unique<llvm::LLVMContext>();
    codeGenerator_ = std::make_unique<LLVMCodeGenerator>(*context_);
}

HooCompiler::~HooCompiler() = default;

// ============================================================================
// Compilation API
// ============================================================================

std::unique_ptr<llvm::Module> HooCompiler::compile(
    const std::string& moduleName,
    const std::string& sourceCode) {

    lastCompilationSuccessful_ = false;
    lastError_.clear();

    // ========================================================================
    // Step 1: Parse source code to get parse tree
    // ========================================================================

    auto* parseTree = parser_->parseForAST(sourceCode);

    if (!parseTree) {
        lastError_ = "Parse error: " + parser_->getLastError();
        return nullptr;
    }

    // ========================================================================
    // Step 2: Build AST from parse tree
    // ========================================================================

    auto ast = astBuilder_->buildAST(parseTree);

    if (!ast) {
        lastError_ = "AST building failed";
        return nullptr;
    }

    // ========================================================================
    // Step 3: Generate LLVM IR from AST
    // ========================================================================

    auto* llvmCodeGen = static_cast<LLVMCodeGenerator*>(codeGenerator_.get());
    auto module = llvmCodeGen->generateLLVMModule(*ast);

    if (!module) {
        lastError_ = "LLVM IR generation failed";
        return nullptr;
    }

    lastCompilationSuccessful_ = true;
    return module;
}

}  // namespace hooc
