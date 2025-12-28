#include <gtest/gtest.h>
#include "../src/ProcessIsolatedParser.h"
#include "../src/SimpleASTBuilder.h"
#include "../src/ast/AST.h"

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
        func test() -> void {
            var x = new std.String();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithArguments) {
    std::string code = R"(
        func test() -> void {
            var x = new std.String("hello");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithMultipleArguments) {
    std::string code = R"(
        func test() -> void {
            var x = new std.Point(10, 20);
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, NestedQualifiedNewExpression) {
    std::string code = R"(
        func test() -> void {
            var x = new std.io.File("/path");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithGenericArguments) {
    std::string code = R"(
        func test() -> void {
            var arr = new std.Array<int64>();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionWithNestedGenerics) {
    std::string code = R"(
        func test() -> void {
            var map = new std.collections.Map<std.String, int64>();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInAssignment) {
    std::string code = R"(
        func test() -> void {
            var x: std.String = new std.String("test");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInFunctionCall) {
    std::string code = R"(
        func process(s: std.String) -> void {}
        func test() -> void {
            process(new std.String("hello"));
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, QualifiedNewExpressionInReturnStatement) {
    std::string code = R"(
        func createString() -> std.String {
            return new std.String("created");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, MultipleQualifiedNewExpressions) {
    std::string code = R"(
        func test() -> void {
            var s1 = new std.String("first");
            var s2 = new std.String("second");
            var arr = new std.Array<int64>();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

// Test backward compatibility - simple new expressions should still work
TEST_F(QualifiedNewExpressionParsingTest, SimpleNewExpressionStillWorks) {
    std::string code = R"(
        func test() -> void {
            var x = new String();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, SimpleNewExpressionWithArguments) {
    std::string code = R"(
        func test() -> void {
            var x = new String("hello");
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, SimpleNewExpressionWithGenerics) {
    std::string code = R"(
        func test() -> void {
            var arr = new Array<int64>();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}

TEST_F(QualifiedNewExpressionParsingTest, MixedQualifiedAndSimpleNewExpressions) {
    std::string code = R"(
        func test() -> void {
            var s = new std.String("test");
            var p = new Point(10, 20);
            var arr = new std.Array<int64>();
        }
    )";
    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_GT(ast->getDeclarations().size(), 0);
}
