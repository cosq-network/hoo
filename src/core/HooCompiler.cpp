#include "HooCompiler.h"
#include "../parsing/ProcessIsolatedParser.h"
#include "../ast/SimpleASTBuilder.h"
#include "../codegen/LLVMCodeGenerator.h"
#include "../codegen/HVMCodeGenerator.h"
#include "HoocParser.h"

namespace hooc {

// ============================================================================
// Construction / Destruction
// ============================================================================

HooCompiler::HooCompiler(llvm::LLVMContext* context)
    : lastCompilationSuccessful_(false)
    , backend_(Backend::LLVM) {

    parser_      = std::make_unique<ProcessIsolatedParser>();
    astBuilder_  = std::make_unique<SimpleASTBuilder>();
    
    if (context) {
        context_ = context;
    } else {
        ownedContext_ = std::make_unique<llvm::LLVMContext>();
        context_ = ownedContext_.get();
    }
}

HooCompiler::~HooCompiler() = default;

void HooCompiler::setBackend(Backend backend) {
    if (backend_ == backend) return;
    backend_ = backend;
    codeGenerator_.reset();
}

// ============================================================================
// Compilation API
// ============================================================================

std::unique_ptr<llvm::Module> HooCompiler::compile(
    const std::string& moduleName,
    const std::string& sourceCode) {

    lastCompilationSuccessful_ = false;
    lastError_.clear();

    if (!codeGenerator_ || backend_ != Backend::LLVM) {
        backend_ = Backend::LLVM;
        codeGenerator_ = std::make_unique<LLVMCodeGenerator>(*context_);
    }

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

    // 3. Generate LLVM IR
    auto* llvmCodeGen = static_cast<LLVMCodeGenerator*>(codeGenerator_.get());
    llvmCodeGen->clearErrors();
    auto module = llvmCodeGen->generateLLVMModule(*ast);

    if (llvmCodeGen->hasErrors()) {
        lastError_ = "Code generation failed: " + llvmCodeGen->getLastError();
        return nullptr;
    }

    if (!module) {
        lastError_ = "LLVM IR generation failed";
        return nullptr;
    }

    module->setModuleIdentifier(moduleName);

    lastCompilationSuccessful_ = true;
    return module;
}

std::unique_ptr<hvm::HoModule> HooCompiler::compileToHVM(
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
    // Temporarily switch backend if needed
    Backend originalBackend = backend_;
    if (!codeGenerator_ || backend_ != Backend::HVM) {
        backend_ = Backend::HVM;
        codeGenerator_ = std::make_unique<HVMCodeGenerator>(moduleRegistry_);
    }
    
    auto* hvmCodeGen = static_cast<HVMCodeGenerator*>(codeGenerator_.get());
    auto generatedModule = hvmCodeGen->generateModule(*ast);

    if (hvmCodeGen->hasErrors()) {
        lastError_ = "HVM Code generation failed: " + hvmCodeGen->getErrors().front();
        setBackend(originalBackend);
        return nullptr;
    }

    if (!generatedModule) {
        lastError_ = "HVM generation failed";
        setBackend(originalBackend);
        return nullptr;
    }

    auto hvmModule = static_cast<HVMGeneratedModule*>(generatedModule.get())->takeModule();
    hvmModule->setName(moduleName);

    setBackend(originalBackend);
    lastCompilationSuccessful_ = true;
    return hvmModule;
}

}  // namespace hooc
