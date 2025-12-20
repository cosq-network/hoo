#include "HoocJIT.h"
#include "HoocJIT.h"
#include "ProcessIsolatedParser.h"
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
    
    // Initialize parser
    parser_ = std::make_unique<ProcessIsolatedParser>();
    
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