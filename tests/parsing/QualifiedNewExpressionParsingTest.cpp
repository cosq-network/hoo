#include <gtest/gtest.h>
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h"

using namespace hooc;

class QualifiedNewExpressionParsingTest : public ::testing::Test {
protected:
    ProcessIsolatedParser parser;
    SimpleASTBuilder builder;

    std::unique_ptr<ast::CompilationUnit> parseCode(const std::string& code) {
        auto parseTree = parser.parseForAST(code);
        if (!parseTree) return nullptr;
        return builder.buildAST(parseTree);
    }
};

// Test qualified identifiers in new expressions
TEST_F(QualifiedNewExpressionParsingTest, SimpleQualifiedNewExpression) {
    std::string code = R"(
        func test() {
            var x = new hoo.String();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithArguments) {
    std::string code = R"(
        func test() {
            var x = new hoo.String("hello");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithMultipleArguments) {
    std::string code = R"(
        func test() {
            var x = new hoo.Point(10, 20);
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, NestedQualifiedNewExpression) {
    std::string code = R"(
        func test() {
            var x = new hoo.io.File("/path");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}



TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInAssignment) {
    std::string code = R"(
        func test() {
            var x: hoo.String = new hoo.String("test");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInFunctionCall) {
    std::string code = R"(
        func process(s: hoo.String) {}
        func test() {
            process(new hoo.String("hello"));
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInReturnStatement) {
    std::string code = R"(
        func:hoo.String createString() {
            return new hoo.String("created");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}



// Test backward compatibility - simple new expressions should still work
TEST_F(QualifiedNewExpressionParsingTest, SimpleNewExpressionStillWorks) {
    std::string code = R"(
        func test() {
            var x = new String();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, SimpleNewExpressionWithArguments) {
    std::string code = R"(
        func test() {
            var x = new String("hello");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}


