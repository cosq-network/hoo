#include "HoocJIT.h"
#include "ProcessIsolatedParser.h"
#include "SimpleASTBuilder.h"
#include "LLVMCodeGenerator.h"
#include "ast/AST.h"
#include "../runtime/hoo_string.h"
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

// ============================================================================
// Auto-Generated Runtime Functions Registration
// ============================================================================
// These functions are auto-generated from the RUNTIME_CLASSES registry
// using the X-Macro pattern. Each runtime class gets a registerXxxFunctions()
// implementation that registers all its symbols with the JIT.
//
// This implementation uses a simplified macro pattern that avoids nested
// #define directives which can cause preprocessor complications.

#define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
    void HoocJIT::register##ClassName##Functions() { \
        auto& mainJD = JIT->getMainJITDylib(); \
        llvm::orc::SymbolMap symbols;

#define BEGIN_RUNTIME_FUNCTIONS \
        std::cout << "Registering String runtime functions...\n";

#define RUNTIME_FUNCTION(FuncName, RetType, LLVMRetType, ...) \
    symbols[JIT->mangleAndIntern("hoo_string_" #FuncName)] = \
        llvm::orc::ExecutorSymbolDef( \
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_##FuncName), \
            JITSymbolFlags::Exported \
        );

#define END_RUNTIME_FUNCTIONS \
        auto Err = mainJD.define(absoluteSymbols(symbols)); \
        if (Err) { \
            errs() << "ERROR: Failed to register string functions with JIT: " \
                   << toString(std::move(Err)) << "\n"; \
            exit(1); \
        } \
        std::cout << "Successfully registered string functions with HoocJIT\n"; \
    }

#define BEGIN_RUNTIME_OPERATORS
#define END_RUNTIME_OPERATORS
#define RUNTIME_OPERATOR(Op, FuncName)

// Expand RUNTIME_CLASSES to generate registration functions
RUNTIME_CLASSES

// Undefine macros to prevent pollution
#undef DEFINE_RUNTIME_CLASS
#undef BEGIN_RUNTIME_FUNCTIONS
#undef END_RUNTIME_FUNCTIONS
#undef RUNTIME_FUNCTION
#undef BEGIN_RUNTIME_OPERATORS
#undef END_RUNTIME_OPERATORS
#undef RUNTIME_OPERATOR

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
    // Auto-register all runtime classes
    // ========================================================================
    // These calls are auto-generated from the RUNTIME_CLASSES registry.
    // Each runtime class gets a registerXxxFunctions() call here.

    #define DEFINE_RUNTIME_CLASS(ClassName, HandleType, DetectionPredicate) \
        register##ClassName##Functions();
    #define BEGIN_RUNTIME_FUNCTIONS
    #define END_RUNTIME_FUNCTIONS
    #define BEGIN_RUNTIME_OPERATORS
    #define END_RUNTIME_OPERATORS
    #define RUNTIME_FUNCTION(...)
    #define RUNTIME_OPERATOR(...)

    RUNTIME_CLASSES  // Expands to: registerStringFunctions();

    #undef DEFINE_RUNTIME_CLASS
    #undef BEGIN_RUNTIME_FUNCTIONS
    #undef END_RUNTIME_FUNCTIONS
    #undef BEGIN_RUNTIME_OPERATORS
    #undef END_RUNTIME_OPERATORS
    #undef RUNTIME_FUNCTION
    #undef RUNTIME_OPERATOR

    // Initialize parser, AST builder, and code generator
    parser_ = std::make_unique<ProcessIsolatedParser>();
    astBuilder_ = std::make_unique<SimpleASTBuilder>();
    codeGenerator_ = std::make_unique<LLVMCodeGenerator>(Context);

    std::cout << "HoocJIT initialized successfully!\n";
}

HoocJIT::~HoocJIT() {}

void HoocJIT::createSimpleFunction() {
    // Create a simple module with a function that returns 42
    auto M = std::make_unique<Module>("hooc_module", Context);
    
    // Create function: int add(int a, int b)
    FunctionType *FT = FunctionType::get(Type::getInt32Ty(Context),
                                        {Type::getInt32Ty(Context), Type::getInt32Ty(Context)},
                                        false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, "add", M.get());
    
    // Create basic block
    BasicBlock *BB = BasicBlock::Create(Context, "entry", F);
    IRBuilder<> Builder(BB);
    
    // Get function arguments
    auto ArgIt = F->arg_begin();
    Value *A = &*ArgIt++;
    Value *B = &*ArgIt;
    A->setName("a");
    B->setName("b");
    
    // Create add instruction and return
    Value *Sum = Builder.CreateAdd(A, B, "sum");
    Builder.CreateRet(Sum);
    
    // Verify function
    if (verifyFunction(*F, &errs())) {
        errs() << "Error: Function verification failed!\n";
        return;
    }
    
    // Print the generated IR
    std::cout << "Generated LLVM IR:\n";
    M->print(outs(), nullptr);
    
    // Add module to JIT
    auto TSM = ThreadSafeModule(std::move(M), std::make_unique<LLVMContext>());
    auto Err = JIT->addIRModule(std::move(TSM));
    if (Err) {
        errs() << "Failed to add module: " << toString(std::move(Err)) << "\n";
        return;
    }
    
    std::cout << "Module added to JIT successfully!\n";
}

void HoocJIT::executeFunction() {
    // Look up the function
    auto AddSymbol = JIT->lookup("add");
    if (!AddSymbol) {
        errs() << "Failed to lookup function: " << toString(AddSymbol.takeError()) << "\n";
        return;
    }
    
    // Cast to function pointer and call
    auto AddFn = (int(*)(int, int))AddSymbol->getValue();
    int result = AddFn(15, 27);
    
    std::cout << "Result: add(15, 27) = " << result << "\n";
}

void HoocJIT::parseHoocCode(const std::string& code) {
    std::cout << "\nParsing Hooc code: \"" << code << "\"\n";
    
    if (parser_->parse(code)) {
        std::cout << "Parse successful!\n";
        std::cout << "Parse tree children: " << parser_->getParseTreeChildCount() << "\n";
        std::cout << "Parse tree: " << parser_->getParseTreeString() << "\n";
    } else {
        std::cout << "Parse failed: " << parser_->getLastError() << "\n";
    }
}

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

std::unique_ptr<llvm::Module> HoocJIT::generateModuleFromAST(const ast::CompilationUnit& ast) {
    std::cout << "\n=== Generating LLVM IR from AST ===\n";

    // Since we know we're using LLVM backend, use the LLVM-specific API
    auto* llvmCodeGen = static_cast<LLVMCodeGenerator*>(codeGenerator_.get());
    auto module = llvmCodeGen->generateLLVMModule(ast);

    if (module) {
        std::cout << "✅ LLVM IR generation successful\n";

        // Print the generated IR
        std::cout << "Generated LLVM IR:\n";
        module->print(outs(), nullptr);
    } else {
        std::cout << "❌ LLVM IR generation failed\n";
    }

    return module;
}