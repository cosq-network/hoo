#include "HooCompiler.h"
#include "parsing/HooParserWrapper.h"
#include "ast/SimpleASTBuilder.h"
#include "codegen/HVMCodeGenerator.h"
#include "HoocParser.h"
#include <numeric>

namespace hooc {

// ============================================================================
// Construction / Destruction
// ============================================================================

HooCompiler::HooCompiler()
    : lastCompilationSuccessful_(false)
    , parser_(std::make_unique<HooParserWrapper>())
    , astBuilder_(std::make_unique<SimpleASTBuilder>())
    , codeGenerator_(std::make_unique<HVMCodeGenerator>()) {
}

HooCompiler::~HooCompiler() = default;

void HooCompiler::setExternalFunctionImports(
    const std::unordered_map<std::string, std::pair<std::string, std::string>>& functions) {
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) {
        hvmCodeGen->setExternalFunctionImports(functions);
    }
}

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
    // We can cast here as long as we only support HVMCodeGenerator in this method,
    // but we use the virtual interface where possible.
    auto* hvmCodeGen = dynamic_cast<HVMCodeGenerator*>(codeGenerator_.get());
    if (hvmCodeGen) {
        hvmCodeGen->setModuleContext(moduleName);
    }
    
    auto generatedModule = codeGenerator_->generateModule(*ast);

    if (hvmCodeGen && hvmCodeGen->hasErrors()) {
        const auto& errors = hvmCodeGen->getErrors();
        if (errors.size() == 1) {
            lastError_ = "HVM Code generation failed: " + errors.front();
        } else {
            lastError_ = "HVM Code generation failed with " + std::to_string(errors.size()) + " errors:\n";
            for (const auto& err : errors) {
                lastError_ += "  - " + err + "\n";
            }
        }
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
