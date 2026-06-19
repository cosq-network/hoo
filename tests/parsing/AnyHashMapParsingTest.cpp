#include <gtest/gtest.h>

#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;
using namespace hooc::ast;

class AnyHashMapParsingTest : public ::testing::Test {
protected:
    HooParserWrapper parser;
    SimpleASTBuilder builder;

    std::unique_ptr<CompilationUnit> parseAst(const std::string& code) {
        auto* tree = parser.parseForAST(code);
        auto* cu = dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
        if (!cu) return nullptr;
        try {
            return builder.buildAST(cu);
        } catch (const std::exception&) {
            return nullptr;
        }
    }
};

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsVariableType) {
    // 'any' is NOT allowed as a standalone type for variables
    auto ast = parseAst(R"(
        var item: any = 42;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsModuleLevelVariable) {
    auto ast = parseAst(R"(
        var item: any;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsClassField) {
    auto ast = parseAst(R"(
        class Foo {
            var field: any;
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsConstant) {
    auto ast = parseAst(R"(
        const ITEM: any = 42;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsParameterType) {
    auto ast = parseAst(R"(
        func test(x: any) {}
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeAcceptedAsReturnType) {
    // 'any' IS allowed as a function return type
    auto ast = parseAst(R"(
        func:any test() { return 42; }
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_NE(dynamic_cast<const AnyType*>(funcDecl->getReturnType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedAsConstructorParameter) {
    auto ast = parseAst(R"(
        class Foo {
            constructor(x: any) {}
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyTypeRejectedInCatchClause) {
    auto ast = parseAst(R"(
        func test() {
            try {} catch (e: any) {}
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, ParsesAnyArrayType) {
    auto ast = parseAst(R"(
        var values: AnyArray;
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* arrayDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(arrayDecl, nullptr);
    EXPECT_NE(dynamic_cast<const AnyArrayType*>(arrayDecl->getType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, ParsesHashMapTypeAndConstructor) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: HashMap<int64, any> = new HashMap<int64, any>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
}

TEST_F(AnyHashMapParsingTest, ParsesAnyArrayLiteralSuffix) {
    auto ast = parseAst(R"(
        var values = [1, "two", 3.0]any;
    )");
    ASSERT_NE(ast, nullptr);
    auto* decl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(decl, nullptr);
    auto* primary = dynamic_cast<const PrimaryExpression*>(decl->getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* literal = dynamic_cast<const ArrayLiteral*>(&primary->getPrimary());
    ASSERT_NE(literal, nullptr);
    EXPECT_TRUE(literal->isAnyArray());
}

TEST_F(AnyHashMapParsingTest, HashMapWithInt64ValueType) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: HashMap<int64, int64> = new HashMap<int64, int64>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, HashMapWithStringValueType) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: HashMap<int8, string> = new HashMap<int8, string>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, HashMapWithAnyAsModuleLevelVar) {
    auto ast = parseAst(R"(
        var m: HashMap<int64, any>;
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_NE(dynamic_cast<const HashMapType*>(varDecl->getType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyArrayAsParameterType) {
    auto ast = parseAst(R"(
        func test(values: AnyArray) {}
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyArrayAsReturnType) {
    auto ast = parseAst(R"(
        func:AnyArray test() {
            var values = new AnyArray();
            return values;
        }
    )");
    ASSERT_NE(ast, nullptr);
    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_NE(dynamic_cast<const AnyArrayType*>(funcDecl->getReturnType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, NewAnyArrayConstructor) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var values = new AnyArray();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyInMapValueType) {
    auto ast = parseAst(R"(
        var m: map<byte, any>;
    )");
    ASSERT_NE(ast, nullptr);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    auto* mapType = dynamic_cast<const MapType*>(varDecl->getType());
    ASSERT_NE(mapType, nullptr);
    EXPECT_EQ(mapType->getKeyType(), MapKeyType::BYTE);
    EXPECT_NE(dynamic_cast<const AnyType*>(&mapType->getValueType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyInHashMapValueType) {
    auto ast = parseAst(R"(
        var m: HashMap<int64, any>;
    )");
    ASSERT_NE(ast, nullptr);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    auto* hmType = dynamic_cast<const HashMapType*>(varDecl->getType());
    ASSERT_NE(hmType, nullptr);
    EXPECT_EQ(hmType->getKeyType(), HashMapKeyType::INT64);
    EXPECT_NE(dynamic_cast<const AnyType*>(&hmType->getValueType()), nullptr);
}

TEST_F(AnyHashMapParsingTest, AnyRejectedAsMapKeyType) {
    auto ast = parseAst(R"(
        var m: map<any, int64>;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, HashMapInt64KeyAllValueTypes) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m1: HashMap<int64, int64>;
            var m2: HashMap<int64, string>;
            var m3: HashMap<int64, any>;
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(AnyHashMapParsingTest, MapWithCharKeyAndAnyValue) {
    auto ast = parseAst(R"(
        var m: map<char, any>;
    )");
    ASSERT_NE(ast, nullptr);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    auto* mapType = dynamic_cast<const MapType*>(varDecl->getType());
    ASSERT_NE(mapType, nullptr);
    EXPECT_EQ(mapType->getKeyType(), MapKeyType::CHAR);
}
