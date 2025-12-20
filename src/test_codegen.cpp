#include "CodeGenerator.h"
#include "ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

/**
 * Standalone test to verify CodeGenerator functionality with manually created AST
 */
int main() {
    std::cout << "=== CodeGenerator Test ===\n";
    
    LLVMContext context;
    CodeGenerator codeGen(context);
    
    // Create a simple AST manually for testing
    // func add() { return; }
    
    // Create function declaration
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<ast::Type> returnType = nullptr; // void return
    
    // Create return statement
    std::unique_ptr<Statement> returnStmt = std::make_unique<ReturnStatement>();
    
    // Create block with return statement
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::move(returnStmt));
    std::unique_ptr<Block> body = std::make_unique<Block>(std::move(statements));
    
    // Create function declaration
    std::unique_ptr<Declaration> funcDecl = std::make_unique<FunctionDeclaration>(
        "testFunc", std::move(parameters), std::move(returnType), std::move(body));
    
    // Create compilation unit
    std::vector<std::unique_ptr<ImportStatement>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    declarations.push_back(std::move(funcDecl));
    
    CompilationUnit compilationUnit(std::move(imports), std::move(declarations));
    
    // Generate LLVM module
    auto module = codeGen.generateModule(compilationUnit);
    
    if (module) {
        std::cout << "✅ Successfully generated LLVM IR:\n";
        module->print(outs(), nullptr);
        std::cout << "\n🎉 CodeGenerator test passed!\n";
        return 0;
    } else {
        std::cout << "❌ Failed to generate LLVM IR\n";
        return 1;
    }
}