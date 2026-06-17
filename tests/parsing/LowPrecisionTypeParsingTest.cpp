#include <gtest/gtest.h>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

namespace {

std::unique_ptr<CompilationUnit> parseCode(const std::string& code) {
    HooParserWrapper parser;
    SimpleASTBuilder astBuilder;
    auto* parseTree = parser.parseForAST(code);
    if (!parseTree) return nullptr;
    return astBuilder.buildAST(parseTree);
}

PrimitiveTypeKind primitiveKind(const Type* type) {
    auto* base = dynamic_cast<const BaseType*>(type);
    EXPECT_NE(base, nullptr);
    EXPECT_TRUE(base->isPrimitive());
    return base->getPrimitiveType()->getKind();
}

PrimitiveTypeKind arrayElementKind(const Type* type) {
    auto* array = dynamic_cast<const ArrayType*>(type);
    EXPECT_NE(array, nullptr);
    EXPECT_TRUE(array->getBaseType().isPrimitive());
    return array->getBaseType().getPrimitiveType()->getKind();
}

} // namespace

TEST(LowPrecisionTypeParsingTest, ParsesF8AndBitTypesInFunctionSurface) {
    const std::string code = R"(
        func:f8 scale(value:f8, enabled:bit) {
            return value;
        }

        func:bit choose(left:bit, right:bit) {
            return left && right;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 2u);

    auto* scale = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(scale, nullptr);
    EXPECT_EQ(primitiveKind(scale->getReturnType()), PrimitiveTypeKind::F8);
    ASSERT_EQ(scale->getParameters().size(), 2u);
    EXPECT_EQ(primitiveKind(&scale->getParameters()[0]->getType()), PrimitiveTypeKind::F8);
    EXPECT_EQ(primitiveKind(&scale->getParameters()[1]->getType()), PrimitiveTypeKind::BIT);

    auto* choose = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[1].get());
    ASSERT_NE(choose, nullptr);
    EXPECT_EQ(primitiveKind(choose->getReturnType()), PrimitiveTypeKind::BIT);
    ASSERT_EQ(choose->getParameters().size(), 2u);
    EXPECT_EQ(primitiveKind(&choose->getParameters()[0]->getType()), PrimitiveTypeKind::BIT);
    EXPECT_EQ(primitiveKind(&choose->getParameters()[1]->getType()), PrimitiveTypeKind::BIT);
}

TEST(LowPrecisionTypeParsingTest, ParsesF8AndBitArraysAndLiterals) {
    const std::string code = R"(
        func:int64 test(values:f8[], flags:bit[]) {
            var localValues:f8[] = [1.0f8, 2.5f8];
            var localFlags:bit[] = [1b, 0b, 1b];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getParameters().size(), 2u);
    EXPECT_EQ(arrayElementKind(&func->getParameters()[0]->getType()), PrimitiveTypeKind::F8);
    EXPECT_EQ(arrayElementKind(&func->getParameters()[1]->getType()), PrimitiveTypeKind::BIT);

    ASSERT_GE(func->getBody().getStatements().size(), 2u);
    auto* f8DeclStmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    auto* bitDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[1].get());
    ASSERT_NE(f8DeclStmt, nullptr);
    ASSERT_NE(bitDeclStmt, nullptr);
    EXPECT_EQ(arrayElementKind(f8DeclStmt->getDeclaration().getType()), PrimitiveTypeKind::F8);
    EXPECT_EQ(arrayElementKind(bitDeclStmt->getDeclaration().getType()), PrimitiveTypeKind::BIT);
}

TEST(LowPrecisionTypeParsingTest, BuildsF8AndBitLiteralNodes) {
    const std::string code = R"(
        func:int64 test() {
            var one = 1b;
            var zero = 0b;
            var small = 3.25f8;
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_GE(func->getBody().getStatements().size(), 3u);

    auto literalAt = [&](size_t index) -> const ASTNode* {
        auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[index].get());
        EXPECT_NE(stmt, nullptr);
        auto* expr = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
        EXPECT_NE(expr, nullptr);
        return expr ? &expr->getPrimary() : nullptr;
    };

    auto* one = dynamic_cast<const BitLiteral*>(literalAt(0));
    auto* zero = dynamic_cast<const BitLiteral*>(literalAt(1));
    auto* small = dynamic_cast<const F8Literal*>(literalAt(2));

    ASSERT_NE(one, nullptr);
    ASSERT_NE(zero, nullptr);
    ASSERT_NE(small, nullptr);
    EXPECT_EQ(one->getValue(), 1);
    EXPECT_EQ(zero->getValue(), 0);
    EXPECT_DOUBLE_EQ(small->getValue(), 3.25);
}

TEST(LowPrecisionTypeParsingTest, ParsesLowPrecisionReturnLiterals) {
    const std::string code = R"(
        func:f8 tiny() {
            return 0.5f8;
        }

        func:bit enabled() {
            return 1b;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 2u);

    auto returnLiteral = [&](size_t functionIndex) -> const ASTNode* {
        auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[functionIndex].get());
        EXPECT_NE(func, nullptr);
        EXPECT_EQ(func->getBody().getStatements().size(), 1u);
        auto* ret = dynamic_cast<const ReturnStatement*>(func->getBody().getStatements()[0].get());
        EXPECT_NE(ret, nullptr);
        auto* expr = ret ? dynamic_cast<const PrimaryExpression*>(ret->getExpression()) : nullptr;
        EXPECT_NE(expr, nullptr);
        return expr ? &expr->getPrimary() : nullptr;
    };

    auto* f8 = dynamic_cast<const F8Literal*>(returnLiteral(0));
    auto* bit = dynamic_cast<const BitLiteral*>(returnLiteral(1));
    ASSERT_NE(f8, nullptr);
    ASSERT_NE(bit, nullptr);
    EXPECT_DOUBLE_EQ(f8->getValue(), 0.5);
    EXPECT_EQ(bit->getValue(), 1);
}
