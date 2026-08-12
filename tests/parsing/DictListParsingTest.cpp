#include <gtest/gtest.h>

#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;
using namespace hooc::ast;

class DictListParsingTest : public ::testing::Test {
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

TEST_F(DictListParsingTest, AnyTypeRejectedAsVariableType) {
    // 'any' is NOT allowed as a standalone type for variables
    auto ast = parseAst(R"(
        var item: any = 42;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeRejectedAsModuleLevelVariable) {
    auto ast = parseAst(R"(
        var item: any;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeRejectedAsClassField) {
    auto ast = parseAst(R"(
        class Foo {
            var field: any;
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeRejectedAsConstant) {
    auto ast = parseAst(R"(
        const ITEM: any = 42;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeRejectedAsParameterType) {
    auto ast = parseAst(R"(
        func test(x: any) {}
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeAcceptedAsReturnType) {
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

TEST_F(DictListParsingTest, AnyTypeRejectedAsConstructorParameter) {
    auto ast = parseAst(R"(
        class Foo {
            constructor(x: any) {}
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyTypeRejectedInCatchClause) {
    auto ast = parseAst(R"(
        func test() {
            try {} catch (e: any) {}
        }
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, ParsesListType) {
    auto ast = parseAst(R"(
        var values: List;
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* arrayDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(arrayDecl, nullptr);
    EXPECT_NE(dynamic_cast<const ListType*>(arrayDecl->getType()), nullptr);
}

TEST_F(DictListParsingTest, ParsesDictTypeAndConstructor) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: Dict<int64, any> = new Dict<int64, any>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
}

TEST_F(DictListParsingTest, ParsesListLiteralSuffix) {
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
    EXPECT_TRUE(literal->isList());
}

TEST_F(DictListParsingTest, DictWithInt64ValueType) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: Dict<int64, int64> = new Dict<int64, int64>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(DictListParsingTest, DictWithStringValueType) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m: Dict<int8, string> = new Dict<int8, string>();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(DictListParsingTest, DictWithAnyAsModuleLevelVar) {
    auto ast = parseAst(R"(
        var m: Dict<int64, any>;
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_NE(dynamic_cast<const DictType*>(varDecl->getType()), nullptr);
}

TEST_F(DictListParsingTest, ListAsParameterType) {
    auto ast = parseAst(R"(
        func test(values: List) {}
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(DictListParsingTest, ListAsReturnType) {
    auto ast = parseAst(R"(
        func:List test() {
            var values = new List();
            return values;
        }
    )");
    ASSERT_NE(ast, nullptr);
    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_NE(dynamic_cast<const ListType*>(funcDecl->getReturnType()), nullptr);
}

TEST_F(DictListParsingTest, NewListConstructor) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var values = new List();
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(DictListParsingTest, AnyInMapValueType) {
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

TEST_F(DictListParsingTest, AnyInHashMapValueType) {
    auto ast = parseAst(R"(
        var m: Dict<int64, any>;
    )");
    ASSERT_NE(ast, nullptr);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(varDecl, nullptr);
    auto* hmType = dynamic_cast<const DictType*>(varDecl->getType());
    ASSERT_NE(hmType, nullptr);
    EXPECT_EQ(hmType->getKeyType(), DictKeyType::INT64);
    EXPECT_NE(dynamic_cast<const AnyType*>(&hmType->getValueType()), nullptr);
}

TEST_F(DictListParsingTest, AnyRejectedAsMapKeyType) {
    auto ast = parseAst(R"(
        var m: map<any, int64>;
    )");
    ASSERT_EQ(ast, nullptr);
}

TEST_F(DictListParsingTest, DictInt64KeyAllValueTypes) {
    auto ast = parseAst(R"(
        func :int64 test() {
            var m1: Dict<int64, int64>;
            var m2: Dict<int64, string>;
            var m3: Dict<int64, any>;
            return 0;
        }
    )");
    ASSERT_NE(ast, nullptr);
}

TEST_F(DictListParsingTest, MapWithCharKeyAndAnyValue) {
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
