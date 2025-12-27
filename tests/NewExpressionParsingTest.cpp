#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"
#include "../src/ast/Declaration.h"
#include "../src/ast/Expression.h"
#include "../src/ast/ClassDeclaration.h"
#include "antlr4-runtime.h"

using namespace hooc::ast;

namespace hooc {
namespace tests {

/**
 * Test suite for new expression parsing in SimpleASTBuilder.
 *
 * This suite tests:
 * - Simple object creation with new keyword
 * - New expressions with constructor arguments
 * - New expressions with various argument types
 * - New expressions in different contexts (assignments, returns, etc.)
 * - Multiple new expressions
 * - New expressions with complex arguments
 */
class NewExpressionParsingTest : public ::testing::Test {
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

    // Helper to find a statement in a function body
    const Statement* findStatementInFunction(const CompilationUnit& ast, int funcIndex) {
        auto& decls = ast.getDeclarations();
        if (funcIndex >= decls.size()) return nullptr;

        auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decls[funcIndex].get());
        if (!funcDecl) return nullptr;

        auto& stmts = funcDecl->getBody().getStatements();
        if (stmts.empty()) return nullptr;

        return stmts[0].get();
    }
};

// Test 1: Simple new expression without arguments
TEST_F(NewExpressionParsingTest, SimpleNewExpressionNoArgs) {
    std::string code = R"(
        class Widget {
            constructor() {}
        }

        func test() {
            var w = new Widget();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 2: New expression with single argument
TEST_F(NewExpressionParsingTest, NewExpressionSingleArgument) {
    std::string code = R"(
        class Counter {
            constructor(initial: int64) {}
        }

        func test() {
            var c = new Counter(42);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 3: New expression with multiple arguments
TEST_F(NewExpressionParsingTest, NewExpressionMultipleArguments) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var p = new Point(10, 20);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 4: New expression with mixed argument types
TEST_F(NewExpressionParsingTest, NewExpressionMixedArgumentTypes) {
    std::string code = R"(
        class Data {
            constructor(id: int64, value: double, flag: bool) {}
        }

        func test() {
            var d = new Data(42, 3.14, true);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 5: New expression in return statement
TEST_F(NewExpressionParsingTest, NewExpressionInReturn) {
    std::string code = R"(
        class Box {
            constructor() {}
        }

        func createBox() {
            return new Box();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 6: Multiple new expressions in same function
TEST_F(NewExpressionParsingTest, MultipleNewExpressions) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() {
            var a = new Item();
            var b = new Item();
            var c = new Item();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 7: New expression with expression arguments
TEST_F(NewExpressionParsingTest, NewExpressionWithExpressionArguments) {
    std::string code = R"(
        class Sum {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var a = 10;
            var b = 20;
            var s = new Sum(a + b, a * 2);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 8: New expression with function call arguments
TEST_F(NewExpressionParsingTest, NewExpressionWithFunctionCallArguments) {
    std::string code = R"(
        func getValue() -> int64 {
            return 42;
        }

        class Value {
            constructor(val: int64) {}
        }

        func test() {
            var v = new Value(getValue());
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 3);
}

// Test 9: New expression with variable arguments
TEST_F(NewExpressionParsingTest, NewExpressionWithVariableArguments) {
    std::string code = R"(
        class Holder {
            constructor(x: int64, y: int64, z: int64) {}
        }

        func test() {
            var a = 1;
            var b = 2;
            var c = 3;
            var h = new Holder(a, b, c);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 10: New expression in if statement
TEST_F(NewExpressionParsingTest, NewExpressionInIfStatement) {
    std::string code = R"(
        class Obj {
            constructor() {}
        }

        func test() {
            if (true) {
                var o = new Obj();
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 11: New expression in while loop
TEST_F(NewExpressionParsingTest, NewExpressionInWhileLoop) {
    std::string code = R"(
        class Counter {
            constructor(init: int64) {}
        }

        func test() {
            var i = 0;
            while (i < 5) {
                var c = new Counter(i);
                i = i + 1;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 12: New expression with assignment
TEST_F(NewExpressionParsingTest, NewExpressionWithAssignment) {
    std::string code = R"(
        class Item {
            constructor() {}
        }

        func test() {
            var obj: Item = new Item();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 13: New expression with all primitive types as arguments
TEST_F(NewExpressionParsingTest, NewExpressionAllPrimitiveTypes) {
    std::string code = R"(
        class AllTypes {
            constructor(
                b: byte,
                i: int64,
                f: float,
                d: double,
                bo: bool,
                c: char
            ) {}
        }

        func test() {
            var a = new AllTypes(1, 2, 3.0, 4.0, true, 'x');
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 14: New expression in nested blocks
TEST_F(NewExpressionParsingTest, NewExpressionInNestedBlocks) {
    std::string code = R"(
        class Nested {
            constructor() {}
        }

        func test() {
            if (true) {
                if (true) {
                    var n = new Nested();
                }
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 15: New expression with complex expression arguments
TEST_F(NewExpressionParsingTest, NewExpressionComplexExpressions) {
    std::string code = R"(
        class Calculator {
            constructor(result: int64) {}
        }

        func test() {
            var a = 10;
            var b = 20;
            var c = new Calculator((a + b) * 2 - 5);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 16: New expression assigned to typed variable
TEST_F(NewExpressionParsingTest, NewExpressionWithTypeAnnotation) {
    std::string code = R"(
        class Widget {
            constructor() {}
        }

        func test() {
            var w: Widget = new Widget();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 17: Multiple different classes created
TEST_F(NewExpressionParsingTest, MultipleClassesNewExpressions) {
    std::string code = R"(
        class ClassA {
            constructor() {}
        }

        class ClassB {
            constructor(x: int64) {}
        }

        class ClassC {
            constructor(x: int64, y: int64) {}
        }

        func test() {
            var a = new ClassA();
            var b = new ClassB(42);
            var c = new ClassC(10, 20);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 4);
}

// Test 18: New expression with no spaces in arguments
TEST_F(NewExpressionParsingTest, NewExpressionCompactSyntax) {
    std::string code = R"(
        class Compact {
            constructor(a: int64, b: int64) {}
        }

        func test() {
            var c = new Compact(1,2);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 19: New expression with boolean arguments
TEST_F(NewExpressionParsingTest, NewExpressionBooleanArguments) {
    std::string code = R"(
        class Config {
            constructor(enabled: bool, debug: bool) {}
        }

        func test() {
            var cfg = new Config(true, false);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 20: New expression in complex expression
TEST_F(NewExpressionParsingTest, NewExpressionInComplexContext) {
    std::string code = R"(
        class Value {
            constructor(x: int64) {}
        }

        func getNewValue(n: int64) {
            var v = new Value(n + 10);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

} // namespace tests
} // namespace hooc
