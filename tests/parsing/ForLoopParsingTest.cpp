#include <gtest/gtest.h>
#include <memory>
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

class ForLoopParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
};

// For-range loop tests
TEST_F(ForLoopParsingTest, SimpleForRangeLoop) {
    std::string code = R"(
        func test() {
            for i in 0 .. 10 {
                var x = i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    auto& stmts = func->getBody().getStatements();
    auto* forStmt = dynamic_cast<const ForRangeStatement*>(stmts[0].get());
    
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->getVariable(), "i");
    EXPECT_FALSE(forStmt->hasStep());
}

TEST_F(ForLoopParsingTest, ForRangeLoopWithStep) {
    std::string code = R"(
        func test() {
            for i in 0 .. 10 by 2 {
                var x = i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    auto& stmts = func->getBody().getStatements();
    auto* forStmt = dynamic_cast<const ForRangeStatement*>(stmts[0].get());
    
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->getVariable(), "i");
    EXPECT_TRUE(forStmt->hasStep());
    
    // Verify step is 2
    auto* stepExpr = dynamic_cast<const PrimaryExpression*>(forStmt->getStep());
    auto* stepLit = dynamic_cast<const IntegerLiteral*>(&stepExpr->getPrimary());
    EXPECT_EQ(stepLit->getValue(), 2);
}

TEST_F(ForLoopParsingTest, ForRangeLoopWithNegativeStep) {
    std::string code = R"(
        func test() {
            for i in 10 .. 0 by -1 {
                var x = i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    auto& stmts = func->getBody().getStatements();
    auto* forStmt = dynamic_cast<const ForRangeStatement*>(stmts[0].get());
    
    ASSERT_NE(forStmt, nullptr);
    EXPECT_TRUE(forStmt->hasStep());
}

TEST_F(ForLoopParsingTest, ForRangeLoopWithVariableBounds) {
    std::string code = R"(
        func test(start: int64, end: int64, step: int64) {
            for i in start .. end by step {
                var x = i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    auto& stmts = func->getBody().getStatements();
    auto* forStmt = dynamic_cast<const ForRangeStatement*>(stmts[0].get());
    
    ASSERT_NE(forStmt, nullptr);
    EXPECT_TRUE(forStmt->hasStep());
}

// For-in loop tests
TEST_F(ForLoopParsingTest, SimpleForInLoop) {
    std::string code = R"(
        func test() {
            var arr = [1, 2, 3];
            for item in arr {
                var x = item;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    auto& stmts = func->getBody().getStatements();
    auto* forInStmt = dynamic_cast<const ForInStatement*>(stmts[1].get());
    
    ASSERT_NE(forInStmt, nullptr);
    EXPECT_EQ(forInStmt->getVariable(), "item");
}

TEST_F(ForLoopParsingTest, ForInLoopWithExpression) {
    std::string code = R"(
        func:int64[] getArray() { return [1]; }
        func test() {
            for x in getArray() {
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
}
