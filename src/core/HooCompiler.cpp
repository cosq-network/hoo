#include "HooCompiler.h"
#include "parsing/ProcessIsolatedParser.h"
#include "ast/SimpleASTBuilder.h"
#include "codegen/HVMCodeGenerator.h"
#include "HoocParser.h"

namespace hooc {

// ============================================================================
// Construction / Destruction
// ============================================================================

HooCompiler::HooCompiler()
    : lastCompilationSuccessful_(false)
    , parser_(std::make_unique<ProcessIsolatedParser>())
    , astBuilder_(std::make_unique<SimpleASTBuilder>())
    , codeGenerator_(std::make_unique<HVMCodeGenerator>(moduleRegistry_)) {
}

HooCompiler::~HooCompiler() = default;

// ============================================================================
// Compilation API
// ============================================================================

std::unique_ptr<hvm::HOModule> HooCompiler::compile(
    const std::string& moduleName,
    const std::string& sourceCode) {

    lastCompilationSuccessful_ = false;
    lastError_.clear();

    // 1. Parse
    auto* parseTree = parser_->parseForAST(sourceCode);
    if (!parseTree) {
        lastError_ = "Parse error: " + parser_->getLastError();
        return nullptr;
    }

    // 2. Build AST
    std::unique_ptr<ast::CompilationUnit> ast;
    try {
        ast = astBuilder_->buildAST(parseTree);
    } catch (const std::exception& e) {
        lastError_ = std::string("AST building failed: ") + e.what();
        return nullptr;
    }

    if (!ast) {
        lastError_ = "AST building failed: unknown error";
        return nullptr;
    }

    // 3. Generate HVM Bytecode
    auto* hvmCodeGen = static_cast<HVMCodeGenerator*>(codeGenerator_.get());
    auto generatedModule = hvmCodeGen->generateModule(*ast);

    if (hvmCodeGen->hasErrors()) {
        lastError_ = "HVM Code generation failed: " + hvmCodeGen->getErrors().front();
        return nullptr;
    }

    if (!generatedModule) {
        lastError_ = "HVM generation failed";
        return nullptr;
    }

    auto hvmModule = static_cast<HVMGeneratedModule*>(generatedModule.get())->takeModule();
    hvmModule->setName(moduleName);

    lastCompilationSuccessful_ = true;
    return hvmModule;
}

}  // namespace hooc
