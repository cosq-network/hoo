#include "HooCompiler.h"
#include "ProcessIsolatedParser.h"
#include "SimpleASTBuilder.h"
#include "LLVMCodeGenerator.h"
#include "../antlr4/generated/HoocParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <iostream>

namespace hooc {

HooCompiler::HooCompiler() 
    : lastCompilationSuccessful_(false) {
    // Initialize compilation components
    parser_ = std::make_unique<ProcessIsolatedParser>();
    astBuilder_ = std::make_unique<SimpleASTBuilder>();
    context_ = std::make_unique<llvm::LLVMContext>();
    codeGenerator_ = std::make_unique<LLVMCodeGenerator>(*context_);
}

HooCompiler::~HooCompiler() = default;

std::unique_ptr<llvm::Module> HooCompiler::compile(const std::string& moduleName, 
                                                 const std::string& sourceCode) {
    lastCompilationSuccessful_ = false;
    lastError_.clear();
    
    try {
        // Step 1: Parse source code to get parse tree
        auto* parseTree = parser_->parseForAST(sourceCode);
        if (!parseTree) {
            lastError_ = "Parse error: " + parser_->getLastError();
            return nullptr;
        }
        
        // Step 2: Build AST from parse tree
        auto ast = astBuilder_->buildAST(parseTree);
        if (!ast) {
            lastError_ = "AST building failed";
            return nullptr;
        }
        
        // Step 3: Generate LLVM IR from AST
        // Since we know we're using LLVM backend, use the LLVM-specific API
        auto* llvmCodeGen = static_cast<LLVMCodeGenerator*>(codeGenerator_.get());
        auto module = llvmCodeGen->generateLLVMModule(*ast);
        if (!module) {
            lastError_ = "LLVM IR generation failed";
            return nullptr;
        }

        lastCompilationSuccessful_ = true;
        return module;
        
    } catch (const std::exception& e) {
        lastError_ = "Compilation exception: " + std::string(e.what());
        return nullptr;
    } catch (...) {
        lastError_ = "Unknown compilation error";
        return nullptr;
    }
}

} // namespace hooc