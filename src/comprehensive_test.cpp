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
 * Comprehensive test demonstrating all CodeGenerator features
 */
int main() {
    std::cout << "=== Comprehensive CodeGenerator Test ===\n\n";

    LLVMContext context;
    CodeGenerator codeGen(context);

    // Test 1: Function with parameters and arithmetic
    std::cout << "Test 1: Function with binary operations (a + b)\n";
    {
        // func add(int64 a, int64 b) -> int64 { return a + b; }
        std::vector<std::unique_ptr<Parameter>> params;

        auto typeA = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
        params.push_back(std::make_unique<Parameter>(std::move(typeA), "a"));

        auto typeB = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
        params.push_back(std::make_unique<Parameter>(std::move(typeB), "b"));

        auto returnType = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));

        // Create a + b expression
        auto identA = std::make_unique<Identifier>("a");
        auto exprA = std::make_unique<PrimaryExpression>(std::move(identA));

        auto identB = std::make_unique<Identifier>("b");
        auto exprB = std::make_unique<PrimaryExpression>(std::move(identB));

        auto addExpr = std::make_unique<AdditiveExpression>(
            std::move(exprA), hooc::ast::BinaryOperator::PLUS, std::move(exprB));

        auto returnStmt = std::make_unique<ReturnStatement>(std::move(addExpr));

        std::vector<std::unique_ptr<Statement>> stmts;
        stmts.push_back(std::move(returnStmt));
        auto body = std::make_unique<Block>(std::move(stmts));

        auto funcDecl = std::make_unique<FunctionDeclaration>(
            "add", std::move(params), std::move(returnType), std::move(body));

        std::vector<std::unique_ptr<ImportStatement>> imports;
        std::vector<std::unique_ptr<Declaration>> decls;
        decls.push_back(std::move(funcDecl));

        CompilationUnit cu(std::move(imports), std::move(decls));
        auto module = codeGen.generateModule(cu);

        if (module) {
            std::cout << "✅ Test 1 PASSED\n";
            module->print(outs(), nullptr);
            std::cout << "\n";
        } else {
            std::cout << "❌ Test 1 FAILED\n";
            return 1;
        }
    }

    // Test 2: Function with if statement
    std::cout << "\nTest 2: If statement\n";
    {
        LLVMContext ctx2;
        CodeGenerator cg2(ctx2);

        // func max(int64 a, int64 b) -> int64 { if (a > b) { return a; } else { return b; } }
        std::vector<std::unique_ptr<Parameter>> params;

        auto typeA = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
        params.push_back(std::make_unique<Parameter>(std::move(typeA), "a"));

        auto typeB = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));
        params.push_back(std::make_unique<Parameter>(std::move(typeB), "b"));

        auto returnType = std::make_unique<BaseType>(std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64));

        // Condition: a > b
        auto identA1 = std::make_unique<Identifier>("a");
        auto exprA1 = std::make_unique<PrimaryExpression>(std::move(identA1));

        auto identB1 = std::make_unique<Identifier>("b");
        auto exprB1 = std::make_unique<PrimaryExpression>(std::move(identB1));

        auto condExpr = std::make_unique<RelationalExpression>(
            std::move(exprA1), hooc::ast::BinaryOperator::GREATER, std::move(exprB1));

        // Then block: return a
        auto identA2 = std::make_unique<Identifier>("a");
        auto exprA2 = std::make_unique<PrimaryExpression>(std::move(identA2));
        auto retA = std::make_unique<ReturnStatement>(std::move(exprA2));
        std::vector<std::unique_ptr<Statement>> thenStmts;
        thenStmts.push_back(std::move(retA));
        auto thenBlock = std::make_unique<Block>(std::move(thenStmts));

        // Else block: return b
        auto identB2 = std::make_unique<Identifier>("b");
        auto exprB2 = std::make_unique<PrimaryExpression>(std::move(identB2));
        auto retB = std::make_unique<ReturnStatement>(std::move(exprB2));
        std::vector<std::unique_ptr<Statement>> elseStmts;
        elseStmts.push_back(std::move(retB));
        auto elseBlock = std::make_unique<Block>(std::move(elseStmts));

        auto ifStmt = std::make_unique<IfStatement>(
            std::move(condExpr), std::move(thenBlock), std::move(elseBlock));

        std::vector<std::unique_ptr<Statement>> bodyStmts;
        bodyStmts.push_back(std::move(ifStmt));
        auto body = std::make_unique<Block>(std::move(bodyStmts));

        auto funcDecl = std::make_unique<FunctionDeclaration>(
            "max", std::move(params), std::move(returnType), std::move(body));

        std::vector<std::unique_ptr<ImportStatement>> imports;
        std::vector<std::unique_ptr<Declaration>> decls;
        decls.push_back(std::move(funcDecl));

        CompilationUnit cu(std::move(imports), std::move(decls));
        auto module = cg2.generateModule(cu);

        if (module) {
            std::cout << "✅ Test 2 PASSED\n";
            module->print(outs(), nullptr);
            std::cout << "\n";
        } else {
            std::cout << "❌ Test 2 FAILED\n";
            return 1;
        }
    }

    std::cout << "\n🎉 All comprehensive tests passed!\n";
    return 0;
}
