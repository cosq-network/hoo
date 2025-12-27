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
// String Functions Registration
// ============================================================================

void HoocJIT::registerStringFunctions() {
    auto& mainJD = JIT->getMainJITDylib();

    llvm::orc::SymbolMap symbols;

    // ========================================================================
    // Creation Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_from_cstr")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_from_cstr),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_new")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_new),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_bytes")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_from_bytes),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_repeat")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_repeat),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Manipulation Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_concat")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_concat),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_substring")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_substring),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_upper")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_to_upper),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_lower")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_to_lower),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_trim")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_trim),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_replace")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_replace),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Query Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_length")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_length),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_data")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_data),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_byte_at")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_byte_at),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_is_empty")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_is_empty),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_index_of")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_index_of),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_last_index_of")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_last_index_of),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_contains")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_contains),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_starts_with")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_starts_with),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_ends_with")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_ends_with),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Comparison Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_compare")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_compare),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_equals")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_equals),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_equals_ignore_case")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_equals_ignore_case),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Reference Counting Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_retain")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_retain),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_release")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_release),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_refcount")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_refcount),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Conversion Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_from_int64")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_from_int64),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_double")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_from_double),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_from_bool")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_from_bool),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_int64")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_to_int64),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_to_double")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_to_double),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_format")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_format),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Debugging Functions
    // ========================================================================

    symbols[JIT->mangleAndIntern("hoo_string_print")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_print),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_println")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_println),
            JITSymbolFlags::Exported
        );

    symbols[JIT->mangleAndIntern("hoo_string_debug")] =
        llvm::orc::ExecutorSymbolDef(
            llvm::orc::ExecutorAddr::fromPtr(&hoo_string_debug),
            JITSymbolFlags::Exported
        );

    // ========================================================================
    // Register All Symbols with JIT
    // ========================================================================

    auto Err = mainJD.define(absoluteSymbols(symbols));
    if (Err) {
        errs() << "ERROR: Failed to register string functions with JIT: "
               << toString(std::move(Err)) << "\n";
        exit(1);
    }

    std::cout << "✅ Registered 30 string functions with HoocJIT\n";
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

    // Register string functions with JIT
    registerStringFunctions();

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