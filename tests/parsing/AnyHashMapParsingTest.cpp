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
        return builder.buildAST(cu);
    }
};

TEST_F(AnyHashMapParsingTest, ParsesAnyAndAnyArrayTypes) {
    auto ast = parseAst(R"(
        var item: any = 42;
        var values: AnyArray;
    )");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 2u);

    auto* anyDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[0].get());
    auto* arrayDecl = dynamic_cast<const VariableDeclaration*>(ast->getDeclarations()[1].get());
    ASSERT_NE(anyDecl, nullptr);
    ASSERT_NE(arrayDecl, nullptr);
    EXPECT_NE(dynamic_cast<const AnyType*>(anyDecl->getType()), nullptr);
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
