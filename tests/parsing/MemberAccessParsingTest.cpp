#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "src/ast/Declaration.h"
#include "src/ast/Expression.h"
#include "src/ast/ClassDeclaration.h"
#include "antlr4-runtime.h"

using namespace hooc::ast;

namespace hooc {
namespace tests {

class MemberAccessParsingTest : public ::testing::Test {
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

// Test 1: Simple member access in variable declaration
TEST_F(MemberAccessParsingTest, SimpleMemberAccess) {
    std::string code = R"(
        class Person {
            var name: string;
            var age: int64;
            constructor() {}
        }

        func test() {
            var p: Person = new Person();
            var n = p.name;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 2: Member access with integer field
TEST_F(MemberAccessParsingTest, MemberAccessIntField) {
    std::string code = R"(
        class Counter {
            var count: int64;
            constructor() {}
        }

        func test() {
            var c: Counter = new Counter();
            var val = c.count;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 3: Member access in expression
TEST_F(MemberAccessParsingTest, MemberAccessInExpression) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            constructor() {}
        }

        func test() {
            var p: Point = new Point();
            var sum = p.x + p.y;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 4: Member access in function call
TEST_F(MemberAccessParsingTest, MemberAccessAsFunctionArg) {
    std::string code = R"(
        class Data {
            var value: int64;
            constructor() {}
        }

        func process(x: int64) {}

        func test() {
            var d: Data = new Data();
            process(d.value);
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 3);
}

// Test 5: Chained member access
TEST_F(MemberAccessParsingTest, ChainedMemberAccess) {
    std::string code = R"(
        class Address {
            var city: string;
            constructor() {}
        }

        class Person {
            var address: Address;
            constructor() {}
        }

        func test() {
            var p: Person = new Person();
            var city = p.address.city;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 3);
}

// Test 6: Member access with multiple fields
TEST_F(MemberAccessParsingTest, MultipleFieldAccess) {
    std::string code = R"(
        class Rectangle {
            var width: int64;
            var height: int64;
            var depth: int64;
            constructor() {}
        }

        func test() {
            var r: Rectangle = new Rectangle();
            var w = r.width;
            var h = r.height;
            var d = r.depth;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 7: Member access in if condition
TEST_F(MemberAccessParsingTest, MemberAccessInIfCondition) {
    std::string code = R"(
        class Box {
            var size: int64;
            constructor() {}
        }

        func test() {
            var b: Box = new Box();
            if (b.size > 10) {
                var x = 5;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 8: Member access in while loop
TEST_F(MemberAccessParsingTest, MemberAccessInWhileLoop) {
    std::string code = R"(
        class Counter {
            var value: int64;
            constructor() {}
        }

        func test() {
            var c: Counter = new Counter();
            while (c.value < 100) {
                var x = 1;
            }
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 9: Member access with different primitive types
TEST_F(MemberAccessParsingTest, MemberAccessDifferentTypes) {
    std::string code = R"(
        class Data {
            var intVal: int64;
            var floatVal: float;
            var boolVal: bool;
            var charVal: char;
            constructor() {}
        }

        func test() {
            var d: Data = new Data();
            var i = d.intVal;
            var f = d.floatVal;
            var b = d.boolVal;
            var c = d.charVal;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 10: Member access in return statement
TEST_F(MemberAccessParsingTest, MemberAccessInReturn) {
    std::string code = R"(
        class Value {
            var data: int64;
            constructor() {}
        }

        func:int64 getValue() {
            var v: Value = new Value();
            return v.data;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 11: Member access with array type
TEST_F(MemberAccessParsingTest, MemberAccessArrayType) {
    std::string code = R"(
        class Collection {
            var items: int64[];
            constructor() {}
        }

        func test() {
            var c: Collection = new Collection();
            var arr = c.items;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 12: Array element access on member
TEST_F(MemberAccessParsingTest, ArrayAccessOnMember) {
    std::string code = R"(
        class Collection {
            var items: int64[];
            constructor() {}
        }

        func test() {
            var c: Collection = new Collection();
            var first = c.items[0];
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 13: Member access in binary expression
TEST_F(MemberAccessParsingTest, MemberAccessBinaryExpression) {
    std::string code = R"(
        class Dimensions {
            var length: int64;
            var width: int64;
            constructor() {}
        }

        func test() {
            var d: Dimensions = new Dimensions();
            var area = d.length * d.width;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 14: Member access with nullable fields
TEST_F(MemberAccessParsingTest, MemberAccessNullableType) {
    std::string code = R"(
        class Optional {
            var value: int64?;
            constructor() {}
        }

        func test() {
            var o: Optional = new Optional();
            var v = o.value;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

// Test 15: Multiple member accesses in same statement
TEST_F(MemberAccessParsingTest, MultipleMemberAccessesSameStatement) {
    std::string code = R"(
        class Point {
            var x: int64;
            var y: int64;
            var z: int64;
            constructor() {}
        }

        func test() {
            var p: Point = new Point();
            var total = p.x + p.y + p.z;
        }
    )";

    auto ast = buildAST(code);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->getDeclarations().size(), 2);
}

} // namespace tests
} // namespace hooc
