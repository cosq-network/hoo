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

} // namespace

TEST(DecimalTypeParsingTest, ParsesDecimalTypeDeclaration) {
    const std::string code = R"(
        func:int64 test() {
            var price:Decimal<38,2>;
            return 0;
        }
    )";

    auto ast = parseCode(code);

    ASSERT_NE(ast, nullptr);
}
TEST(DecimalTypeParsingTest, ParsesPrecisionAndScale) {
    const std::string code = R"(
        func:void test(price: Decimal<38, 2>) {
            return;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getParameters().size(), 1u);

    const Type& type = func->getParameters()[0]->getType();

    auto* decimalType = dynamic_cast<const DecimalType*>(&type);
    ASSERT_NE(decimalType, nullptr);

    EXPECT_EQ(decimalType->getPrecision(), 38);
    EXPECT_EQ(decimalType->getScale(), 2);
}
TEST(DecimalTypeParsingTest, RejectsMissingScale) {
    const std::string code = R"(
        func:void test(price: Decimal<38>) {
            return;
        }
    )";

    auto ast = parseCode(code);

    EXPECT_EQ(ast, nullptr);
}
TEST(DecimalTypeParsingTest, RejectsScaleGreaterThanPrecision) {
    const std::string code = R"(
        func:void test(price: Decimal<10,12>) {
            return;
        }
    )";

    EXPECT_THROW({
        auto ast = parseCode(code);
    }, std::runtime_error);
}
TEST(DecimalTypeParsingTest, RejectsZeroPrecision) {
    const std::string code = R"(
        func:void test(price: Decimal<0,0>) {
            return;
        }
    )";

    EXPECT_THROW(
        parseCode(code),
        std::runtime_error
    );
}
TEST(DecimalLiteralParsingTest, ParsesSuffixLiteral) {
    const std::string code = R"(
        func:void test() {
            var price = 19.99m;
            return;
        }
    )";

    auto ast = parseCode(code);

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);
}
TEST(DecimalLiteralParsingTest, RejectsUnsignedFloatLiteralAsDecimal) {
    const std::string code = R"(
        func:void test() {
            var x = 19.99;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());

    ASSERT_NE(func, nullptr);

    auto* stmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[0].get());

    ASSERT_NE(stmt, nullptr);

    auto* expr =
        dynamic_cast<const PrimaryExpression*>(
            stmt->getDeclaration().getInitializer());

    ASSERT_NE(expr, nullptr);

    EXPECT_EQ(
        dynamic_cast<const DecimalLiteral*>(&expr->getPrimary()),
        nullptr);

    EXPECT_NE(
        dynamic_cast<const FloatingLiteral*>(&expr->getPrimary()),
        nullptr);
}
TEST(DecimalLiteralParsingTest, KeepsFloatAndF8LiteralsDistinct) {
    const std::string code = R"(
        func:void test() {
            var d = 19.99m;
            var f = 19.99;
            var h = 19.99f8;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getBody().getStatements().size(), 3u);

  auto getPrimary = [&](size_t index) -> const ASTNode& {
    auto* stmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[index].get());

    EXPECT_NE(stmt, nullptr);

    auto* expr =
        dynamic_cast<const PrimaryExpression*>(
            stmt->getDeclaration().getInitializer());

    EXPECT_NE(expr, nullptr);

    return expr->getPrimary();
};

   EXPECT_NE(
    dynamic_cast<const DecimalLiteral*>(&getPrimary(0)),
    nullptr);

EXPECT_NE(
    dynamic_cast<const FloatingLiteral*>(&getPrimary(1)),
    nullptr);

EXPECT_NE(
    dynamic_cast<const F8Literal*>(&getPrimary(2)),
    nullptr);
}
TEST(SimpleASTBuilderTest, BuildsDecimalTypeNode) {
    const std::string code = R"(
        func:void test(price: Decimal<38,2>) {
            return;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getParameters().size(), 1u);

    const Type& type = func->getParameters()[0]->getType();

    auto* decimalType =
        dynamic_cast<const DecimalType*>(&type);

    ASSERT_NE(decimalType, nullptr);

    EXPECT_EQ(decimalType->getPrecision(), 38);
    EXPECT_EQ(decimalType->getScale(), 2);
}
TEST(SimpleASTBuilderTest, BuildsDecimalLiteralNode) {
    const std::string code = R"(
        func:void test() {
            var amount = 19.99m;
        }
    )";

    auto ast = parseCode(code);

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1u);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);

    ASSERT_EQ(func->getBody().getStatements().size(), 1u);

    auto* stmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);

    auto* expr =
        dynamic_cast<const PrimaryExpression*>(
            stmt->getDeclaration().getInitializer());
    ASSERT_NE(expr, nullptr);

    auto* decimal =
        dynamic_cast<const DecimalLiteral*>(
            &expr->getPrimary());

    ASSERT_NE(decimal, nullptr);

    EXPECT_EQ(decimal->getValue(), "19.99m");
}
TEST(DecimalTypeResolutionTest, ResolvesDecimalDeclaredType) {
    const std::string code = R"(
        func:void test() {
            var amount: Decimal<38,2>;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);

    auto* stmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);

    const Type* type = stmt->getDeclaration().getType();

    auto* decimalType =
        dynamic_cast<const DecimalType*>(type);

    ASSERT_NE(decimalType, nullptr);
    EXPECT_EQ(decimalType->getPrecision(), 38);
    EXPECT_EQ(decimalType->getScale(), 2);
}
TEST(DecimalTypeResolutionTest, KeepsDecimalDistinctFromDouble) {
    const std::string code = R"(
        func:void test() {
            var amount: Decimal<38,2>;
            var rate: double;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());
    ASSERT_NE(func, nullptr);

    ASSERT_EQ(func->getBody().getStatements().size(), 2u);

    auto* amountStmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[0].get());

    auto* rateStmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[1].get());

    ASSERT_NE(amountStmt, nullptr);
    ASSERT_NE(rateStmt, nullptr);

    const Type* amountType = amountStmt->getDeclaration().getType();
    const Type* rateType = rateStmt->getDeclaration().getType();

    EXPECT_NE(dynamic_cast<const DecimalType*>(amountType), nullptr);

    auto* doubleType =
        dynamic_cast<const BaseType*>(rateType);

    ASSERT_NE(doubleType, nullptr);
    EXPECT_TRUE(doubleType->isPrimitive());
    EXPECT_EQ(
        doubleType->getPrimitiveType()->getKind(),
        PrimitiveTypeKind::DOUBLE);
}
TEST(DecimalTypeResolutionTest, KeepsDecimalDistinctFromF8) {
    const std::string code = R"(
        func:void test() {
            var amount: Decimal<38,2>;
            var value: f8;
        }
    )";

    auto ast = parseCode(code);

    ASSERT_NE(ast, nullptr);

    auto* func =
        dynamic_cast<const FunctionDeclaration*>(
            ast->getDeclarations()[0].get());

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getBody().getStatements().size(), 2u);

    auto* amountStmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[0].get());

    auto* valueStmt =
        dynamic_cast<const VariableDeclarationStatement*>(
            func->getBody().getStatements()[1].get());

    ASSERT_NE(amountStmt, nullptr);
    ASSERT_NE(valueStmt, nullptr);

    const Type* amountType = amountStmt->getDeclaration().getType();
    const Type* valueType = valueStmt->getDeclaration().getType();

    
    EXPECT_NE(
        dynamic_cast<const DecimalType*>(amountType),
        nullptr);

    
    EXPECT_EQ(
        dynamic_cast<const DecimalType*>(valueType),
        nullptr);

    
    EXPECT_NE(typeid(*amountType), typeid(*valueType));
}