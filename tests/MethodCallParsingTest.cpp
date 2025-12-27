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

class MethodCallParsingTest : public ::testing::Test {
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

// Test 1: Simple method call with no arguments
TEST_F(MethodCallParsingTest, SimpleMethodCallNoArgs) {
    std::string code = R"(
        class Counter {
            var count: int64;
            constructor() {}

            func getValue() -> int64 {
                return 42;
            }
        }

        func test() {
            var c: Counter = new Counter();
            var result = c.getValue();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 2: Method call with single argument
TEST_F(MethodCallParsingTest, MethodCallSingleArg) {
    std::string code = R"(
        class Adder {
            constructor() {}

            func add(x: int64) -> int64 {
                return x + 10;
            }
        }

        func test() {
            var a: Adder = new Adder();
            var result = a.add(5);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 3: Method call with multiple arguments
TEST_F(MethodCallParsingTest, MethodCallMultipleArgs) {
    std::string code = R"(
        class Calculator {
            constructor() {}

            func multiply(a: int64, b: int64) -> int64 {
                return a * b;
            }
        }

        func test() {
            var calc: Calculator = new Calculator();
            var result = calc.multiply(3, 4);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 4: Method call with void return type
TEST_F(MethodCallParsingTest, MethodCallVoidReturn) {
    std::string code = R"(
        class Printer {
            constructor() {}

            func print() {
                var x = 5;
            }
        }

        func test() {
            var p: Printer = new Printer();
            p.print();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 5: Multiple method calls on same object
TEST_F(MethodCallParsingTest, MultipleMethodCalls) {
    std::string code = R"(
        class Operations {
            constructor() {}

            func getX() -> int64 {
                return 10;
            }

            func getY() -> int64 {
                return 20;
            }
        }

        func test() {
            var ops: Operations = new Operations();
            var x = ops.getX();
            var y = ops.getY();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 6: Method call in expression
TEST_F(MethodCallParsingTest, MethodCallInExpression) {
    std::string code = R"(
        class Math {
            constructor() {}

            func getValue() -> int64 {
                return 5;
            }
        }

        func test() {
            var m: Math = new Math();
            var result = m.getValue() + 10;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 7: Method call as function argument
TEST_F(MethodCallParsingTest, MethodCallAsArgument) {
    std::string code = R"(
        class Value {
            constructor() {}

            func getValue() -> int64 {
                return 42;
            }
        }

        func process(x: int64) -> int64 {
            return x * 2;
        }

        func test() {
            var v: Value = new Value();
            var result = process(v.getValue());
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 3);
}

// Test 8: Method call in return statement
TEST_F(MethodCallParsingTest, MethodCallInReturn) {
    std::string code = R"(
        class Data {
            constructor() {}

            func getData() -> int64 {
                return 100;
            }
        }

        func getResult() -> int64 {
            var d: Data = new Data();
            return d.getData();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 9: Method call in if condition
TEST_F(MethodCallParsingTest, MethodCallInIfCondition) {
    std::string code = R"(
        class Checker {
            constructor() {}

            func check() -> bool {
                return true;
            }
        }

        func test() {
            var c: Checker = new Checker();
            if (c.check()) {
                var x = 5;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 10: Method call with expression arguments
TEST_F(MethodCallParsingTest, MethodCallWithExpressionArgs) {
    std::string code = R"(
        class Calculator {
            constructor() {}

            func add(a: int64, b: int64) -> int64 {
                return a + b;
            }
        }

        func test() {
            var calc: Calculator = new Calculator();
            var x = 5;
            var y = 10;
            var result = calc.add(x + 2, y * 3);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 11: Chained method calls (method returning object)
TEST_F(MethodCallParsingTest, ChainedMethodCalls) {
    std::string code = R"(
        class Handler {
            constructor() {}

            func process() -> int64 {
                return 42;
            }
        }

        func test() {
            var h: Handler = new Handler();
            var result = h.process();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 12: Method call in while loop
TEST_F(MethodCallParsingTest, MethodCallInWhileLoop) {
    std::string code = R"(
        class Iterator {
            constructor() {}

            func hasNext() -> bool {
                return false;
            }
        }

        func test() {
            var iter: Iterator = new Iterator();
            while (iter.hasNext()) {
                var x = 1;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 13: Method call with multiple different types
TEST_F(MethodCallParsingTest, MethodCallMixedTypes) {
    std::string code = R"(
        class Process {
            constructor() {}

            func process(i: int64, f: float, b: bool) -> double {
                return 3.14;
            }
        }

        func test() {
            var p: Process = new Process();
            var result = p.process(42, 2.5, true);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 14: Multiple objects with method calls
TEST_F(MethodCallParsingTest, MultipleObjectMethodCalls) {
    std::string code = R"(
        class A {
            constructor() {}
            func methodA() -> int64 { return 1; }
        }

        class B {
            constructor() {}
            func methodB() -> int64 { return 2; }
        }

        func test() {
            var a: A = new A();
            var b: B = new B();
            var x = a.methodA();
            var y = b.methodB();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 3);
}

// Test 15: Method call with no explicit arguments (empty parens)
TEST_F(MethodCallParsingTest, MethodCallEmptyParens) {
    std::string code = R"(
        class Simple {
            constructor() {}

            func getValue() -> int64 {
                return 99;
            }
        }

        func test() {
            var s: Simple = new Simple();
            var result = s.getValue();
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

} // namespace tests
} // namespace hooc
