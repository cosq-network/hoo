#include "HoocJIT.h"
#include "ProcessIsolatedParser.h"
#include "SimpleASTBuilder.h"
#include "LLVMCodeGenerator.h"
#include "ast/AST.h"
#include "hoo_string.h"
#include "runtime/RuntimeRegistry.h"
#include <iostream>
#include <memory>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::orc;
using namespace hooc;

// Namespace aliases to avoid conflicts between hooc::Module and llvm::Module
namespace {
    using LLVMModule = llvm::Module;
    using HoocModule = hooc::Module;
}

HoocJIT::HoocJIT() {
    // Initialize LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // Create JIT
    auto JITExpected = LLJITBuilder().create();
    if (!JITExpected) {
        errs() << "Failed to create JIT: " << toString(JITExpected.takeError()) << "\n";
        exit(1);
    }
    JIT = std::move(*JITExpected);

    // ========================================================================
    // Register runtime classes with JIT via central registry
    // ========================================================================
    // All runtime libraries (String, Array, etc.) self-register via the
    // central RuntimeRegistry. Their callbacks are invoked here to register
    // functions as JIT symbols.

    auto& registry = runtime::RuntimeRegistry::getInstance();
    auto& mainJD = JIT->getMainJITDylib();

    registry.registerAllWithJIT(*JIT, mainJD);

    // Initialize parser, AST builder, and code generator
    parser_ = std::make_unique<ProcessIsolatedParser>();
    astBuilder_ = std::make_unique<SimpleASTBuilder>();
    codeGenerator_ = std::make_unique<LLVMCodeGenerator>(Context);

    std::cout << "HoocJIT initialized successfully!\n";
}

HoocJIT::~HoocJIT() {}

bool HoocJIT::compileHoocCode(const std::string& code) {
    std::cout << "\n=== Compiling Hooc Code ===\n";
    std::cout << "Source: \"" << code << "\"\n";
    
    // Step 1: Parse the code
    if (!parser_->parse(code)) {
        std::cout << "Parse failed: " << parser_->getLastError() << "\n";
        return false;
    }
    
    std::cout << "✅ Parsing successful\n";
    
    // TODO: Step 2: Build AST from parse tree
    // For now, we'll create a dummy compilation unit
    // In a complete implementation, you would:
    // 1. Get the parse tree from ProcessIsolatedParser
    // 2. Use ASTBuilder to convert parse tree to AST
    // 3. Generate LLVM IR from AST
    
    std::cout << "⚠️  AST building not yet integrated with ProcessIsolatedParser\n";
    std::cout << "⚠️  This requires connecting ANTLR4 parse tree to ASTBuilder\n";
    
    return true;
}

bool HoocJIT::executeFunction(const std::string& functionName) {
    std::cout << "\n=== Executing Function: " << functionName << " ===\n";
    
    // Look up the function in the JIT
    auto functionSymbol = JIT->lookup(functionName);
    if (!functionSymbol) {
        std::cout << "Function '" << functionName << "' not found: " 
                 << toString(functionSymbol.takeError()) << "\n";
        return false;
    }
    
    // For now, assume it's a void function with no parameters
    auto functionPtr = (void(*)())functionSymbol->getValue();
    
    std::cout << "Executing " << functionName << "()...\n";
    functionPtr();
    std::cout << "✅ Function executed successfully\n";
    
    return true;
}