#include <gtest/gtest.h>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "ast/AST.h"

using namespace hooc;
using namespace hooc::ast;

static std::unique_ptr<CompilationUnit> parseTensorCode(const std::string& code) {
    HooParserWrapper parser;
    SimpleASTBuilder builder;
    auto* parseTree = parser.parseForAST(code);
    if (!parseTree) return nullptr;
    return builder.buildAST(parseTree);
}

class TensorTypeParsingTest : public ::testing::Test {
};

static PrimitiveTypeKind tensorElementKind(const TensorType* type) {
    EXPECT_NE(type, nullptr);
    EXPECT_TRUE(type->getElementType().isPrimitive());
    return type->getElementType().getPrimitiveType()->getKind();
}

TEST_F(TensorTypeParsingTest, ParsesTensorTypeInFunctionSurface) {
    const std::string code = R"(
        func:tensor<f8>[2, 2] scale(input:tensor<f8>[2, 2], mask:tensor<bit>[2, 2]) {
            return input;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    auto* returnType = dynamic_cast<const TensorType*>(func->getReturnType());
    ASSERT_NE(returnType, nullptr);
    EXPECT_EQ(tensorElementKind(returnType), PrimitiveTypeKind::F8);
    EXPECT_EQ(returnType->getRank(), 2u);

    ASSERT_EQ(func->getParameters().size(), 2u);
    auto* inputType = dynamic_cast<const TensorType*>(&func->getParameters()[0]->getType());
    auto* maskType = dynamic_cast<const TensorType*>(&func->getParameters()[1]->getType());
    ASSERT_NE(inputType, nullptr);
    ASSERT_NE(maskType, nullptr);
    EXPECT_EQ(tensorElementKind(inputType), PrimitiveTypeKind::F8);
    EXPECT_EQ(tensorElementKind(maskType), PrimitiveTypeKind::BIT);
    EXPECT_EQ(maskType->getRank(), 2u);
}

TEST_F(TensorTypeParsingTest, Parses1DTensorType) {
    const std::string code = R"(
        func:int64 test(x:tensor<int64>[5]) {
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getParameters().size(), 1u);
    auto* paramType = dynamic_cast<const TensorType*>(&func->getParameters()[0]->getType());
    ASSERT_NE(paramType, nullptr);
    EXPECT_EQ(tensorElementKind(paramType), PrimitiveTypeKind::INT64);
    EXPECT_EQ(paramType->getRank(), 1u);
}

TEST_F(TensorTypeParsingTest, Parses3DTensorType) {
    const std::string code = R"(
        func:int64 test(x:tensor<int8>[2, 3, 4]) {
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getParameters().size(), 1u);
    auto* paramType = dynamic_cast<const TensorType*>(&func->getParameters()[0]->getType());
    ASSERT_NE(paramType, nullptr);
    EXPECT_EQ(tensorElementKind(paramType), PrimitiveTypeKind::INT8);
    EXPECT_EQ(paramType->getRank(), 3u);
}

TEST_F(TensorTypeParsingTest, ParsesAllElementTypes) {
    auto testElem = [](const std::string& elem, PrimitiveTypeKind kind) {
        std::string code = "func:int64 test(x:tensor<" + elem + ">[2]) { return 0; }";
        auto ast = parseTensorCode(code);
        ASSERT_NE(ast, nullptr);
        auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
        ASSERT_NE(func, nullptr);
        auto* paramType = dynamic_cast<const TensorType*>(&func->getParameters()[0]->getType());
        ASSERT_NE(paramType, nullptr) << "for element type " << elem;
        EXPECT_EQ(tensorElementKind(paramType), kind) << "for element type " << elem;
    };

    testElem("int64", PrimitiveTypeKind::INT64);
    testElem("int8", PrimitiveTypeKind::INT8);
    testElem("bit", PrimitiveTypeKind::BIT);
    testElem("f64", PrimitiveTypeKind::F64);
    testElem("f8", PrimitiveTypeKind::F8);
}

TEST_F(TensorTypeParsingTest, BuildsTensorLiteralNode) {
    const std::string code = R"(
        func:int64 test() {
            var m = [[1, 2], [3, 4]]t;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_FALSE(func->getBody().getStatements().empty());

    auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* primary = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* tensor = dynamic_cast<const TensorLiteral*>(&primary->getPrimary());
    ASSERT_NE(tensor, nullptr);
    ASSERT_NE(tensor->getElements(), nullptr);
    EXPECT_EQ(tensor->getElements()->getExpressions().size(), 2u);
}

TEST_F(TensorTypeParsingTest, Builds1DTensorLiteral) {
    const std::string code = R"(
        func:int64 test() {
            var v = [10, 20, 30]t;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* primary = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* tensor = dynamic_cast<const TensorLiteral*>(&primary->getPrimary());
    ASSERT_NE(tensor, nullptr);
    ASSERT_NE(tensor->getElements(), nullptr);
    EXPECT_EQ(tensor->getElements()->getExpressions().size(), 3u);
}

TEST_F(TensorTypeParsingTest, BuildsEmptyTensorLiteral) {
    const std::string code = R"(
        func:int64 test() {
            var v = []t;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* primary = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* tensor = dynamic_cast<const TensorLiteral*>(&primary->getPrimary());
    ASSERT_NE(tensor, nullptr);
    ASSERT_NE(tensor->getElements(), nullptr);
    EXPECT_EQ(tensor->getElements()->getExpressions().size(), 0u);
}

TEST_F(TensorTypeParsingTest, Builds3DTensorLiteral) {
    const std::string code = R"(
        func:int64 test() {
            var v = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]t;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);
    auto* primary = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* tensor = dynamic_cast<const TensorLiteral*>(&primary->getPrimary());
    ASSERT_NE(tensor, nullptr);
    ASSERT_NE(tensor->getElements(), nullptr);
    EXPECT_EQ(tensor->getElements()->getExpressions().size(), 2u);
}

TEST_F(TensorTypeParsingTest, TensorTypeAnnotationAndLiteral) {
    const std::string code = R"(
        func:int64 test() {
            var v: tensor<int64>[3] = [1, 2, 3]t;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
    auto* func = dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    auto* stmt = dynamic_cast<const VariableDeclarationStatement*>(func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* declType = dynamic_cast<const TensorType*>(stmt->getDeclaration().getType());
    ASSERT_NE(declType, nullptr);
    EXPECT_EQ(tensorElementKind(declType), PrimitiveTypeKind::INT64);
    EXPECT_EQ(declType->getRank(), 1u);

    auto* primary = dynamic_cast<const PrimaryExpression*>(stmt->getDeclaration().getInitializer());
    ASSERT_NE(primary, nullptr);
    auto* tensor = dynamic_cast<const TensorLiteral*>(&primary->getPrimary());
    ASSERT_NE(tensor, nullptr);
    EXPECT_EQ(tensor->getElements()->getExpressions().size(), 3u);
}

TEST_F(TensorTypeParsingTest, ParsesTensorElementwiseOperators) {
    const std::string code = R"(
        func:int64 test() {
            var a = [1, 2]t;
            var b = [3, 4]t;
            var c = a .* b;
            var d = b ./ a;
            return 0;
        }
    )";

    auto ast = parseTensorCode(code);
    ASSERT_NE(ast, nullptr);
}
