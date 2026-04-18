#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "src/ast/Declaration.h"
#include "src/ast/Expression.h"
#include "src/ast/ClassDeclaration.h"
#include "src/ast/Primary.h"
#include "antlr4-runtime.h"

using namespace hooc::ast;

namespace hooc {
namespace tests {

class ThisKeywordParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }

    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }

    std::unique_ptr<CompilationUnit> buildAST(const std::string& code) {
        auto* parseTree = parseCode(code);
        if (!parseTree) return nullptr;

        auto* ctx = getCompilationUnit(parseTree);
        if (!ctx) return nullptr;

        return astBuilder->buildAST(ctx);
    }
};

// Test 1: 'this' keyword in constructor for member assignment
TEST_F(ThisKeywordParsingTest, ThisInConstructor) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);
    
    auto* classDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);
    
    bool foundConstructor = false;
    for (const auto& member : classDecl->getBody().getMembers()) {
        if (member->isConstructor()) {
            foundConstructor = true;
            auto* ctor = member->getConstructor();
            ASSERT_EQ(ctor->getBody().getStatements().size(), 2);
            
            // Verify first statement is an assignment to this.x
            auto* stmt1 = dynamic_cast<const ExpressionStatement*>(ctor->getBody().getStatements()[0].get());
            ASSERT_NE(stmt1, nullptr);
            auto* assign = dynamic_cast<const AssignmentExpression*>(&stmt1->getExpression());
            ASSERT_NE(assign, nullptr);
            
            auto* memberAccess = dynamic_cast<const MemberAccess*>(&assign->getLeft());
            ASSERT_NE(memberAccess, nullptr);
            EXPECT_EQ(memberAccess->getMember(), "x");
            
            auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&memberAccess->getObject());
            ASSERT_NE(primaryExpr, nullptr);
            EXPECT_NE(dynamic_cast<const ThisLiteral*>(&primaryExpr->getPrimary()), nullptr);
        }
    }
    EXPECT_TRUE(foundConstructor);
}

// Test 2: 'this' keyword in a method
TEST_F(ThisKeywordParsingTest, ThisInMethod) {
    std::string code = R"(
        class Counter {
            var count: int64;
            func increment() {
                this.count = this.count + 1;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* classDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);
    
    bool foundMethod = false;
    for (const auto& member : classDecl->getBody().getMembers()) {
        if (auto* decl = member->getDeclaration()) {
            if (auto* func = dynamic_cast<const FunctionDeclaration*>(decl)) {
                if (func->getName() == "increment") {
                    foundMethod = true;
                    ASSERT_EQ(func->getBody().getStatements().size(), 1);
                    
                    auto* stmt = dynamic_cast<const ExpressionStatement*>(func->getBody().getStatements()[0].get());
                    ASSERT_NE(stmt, nullptr);
                    auto* assign = dynamic_cast<const AssignmentExpression*>(&stmt->getExpression());
                    ASSERT_NE(assign, nullptr);
                    
                    // Left side: this.count
                    auto* lhs = dynamic_cast<const MemberAccess*>(&assign->getLeft());
                    ASSERT_NE(lhs, nullptr);
                    auto* lhsObj = dynamic_cast<const PrimaryExpression*>(&lhs->getObject());
                    ASSERT_NE(lhsObj, nullptr);
                    EXPECT_NE(dynamic_cast<const ThisLiteral*>(&lhsObj->getPrimary()), nullptr);
                    
                    // Right side: this.count + 1
                    auto* rhs = dynamic_cast<const AdditiveExpression*>(&assign->getRight());
                    ASSERT_NE(rhs, nullptr);
                    auto* rhsLeft = dynamic_cast<const MemberAccess*>(&rhs->getLeft());
                    ASSERT_NE(rhsLeft, nullptr);
                    auto* rhsLeftObj = dynamic_cast<const PrimaryExpression*>(&rhsLeft->getObject());
                    ASSERT_NE(rhsLeftObj, nullptr);
                    EXPECT_NE(dynamic_cast<const ThisLiteral*>(&rhsLeftObj->getPrimary()), nullptr);
                }
            }
        }
    }
    EXPECT_TRUE(foundMethod);
}

// Test 3: Standalone 'this'
TEST_F(ThisKeywordParsingTest, StandaloneThis) {
    std::string code = R"(
        class Box {
            func:Box getSelf() {
                return this;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* classDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);
    
    bool foundMethod = false;
    for (const auto& member : classDecl->getBody().getMembers()) {
        if (auto* decl = member->getDeclaration()) {
            if (auto* func = dynamic_cast<const FunctionDeclaration*>(decl)) {
                if (func->getName() == "getSelf") {
                    foundMethod = true;
                    ASSERT_EQ(func->getBody().getStatements().size(), 1);
                    
                    auto* ret = dynamic_cast<const ReturnStatement*>(func->getBody().getStatements()[0].get());
                    ASSERT_NE(ret, nullptr);
                    ASSERT_TRUE(ret->hasExpression());
                    
                    auto* primary = dynamic_cast<const PrimaryExpression*>(ret->getExpression());
                    ASSERT_NE(primary, nullptr);
                    EXPECT_NE(dynamic_cast<const ThisLiteral*>(&primary->getPrimary()), nullptr);
                }
            }
        }
    }
    EXPECT_TRUE(foundMethod);
}

} // namespace tests
} // namespace hooc
