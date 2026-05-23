#include <gtest/gtest.h>
#include "ast/AST.h"
#include "ast/Primary.h"
#include "ast/Expression.h"

using namespace hooc::ast;

class ASTExpressionTest : public ::testing::Test {};

static auto intLit(int64_t v) {
    return std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(v));
}

TEST_F(ASTExpressionTest, PrimaryExpression) {
    PrimaryExpression pe(std::make_unique<IntegerLiteral>(42));
    EXPECT_EQ(pe.toString(), "PrimaryExpression(IntegerLiteral(42))");
}

TEST_F(ASTExpressionTest, MemberAccess) {
    auto obj = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("p"));
    MemberAccess ma(std::move(obj), "x");
    EXPECT_EQ(ma.getMember(), "x");
    EXPECT_EQ(ma.toString(), "MemberAccess .x");
}

TEST_F(ASTExpressionTest, ArrayAccess) {
    auto arr = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("a"));
    auto idx = intLit(0);
    ArrayAccess aa(std::move(arr), std::move(idx));
    EXPECT_EQ(aa.toString(), "ArrayAccess");
}

TEST_F(ASTExpressionTest, ArgumentList) {
    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(intLit(1));
    args.push_back(intLit(2));
    ArgumentList al(std::move(args));
    EXPECT_EQ(al.getArguments().size(), 2);
}

TEST_F(ASTExpressionTest, FunctionCall) {
    auto fn = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("foo"));
    auto args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>{});
    FunctionCall fc(std::move(fn), std::move(args));
    EXPECT_EQ(fc.toString(), "FunctionCall");
}

TEST_F(ASTExpressionTest, NewObjectExpressionSimple) {
    auto args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>{});
    NewObjectExpression noe("Point", std::move(args));
    EXPECT_EQ(noe.getClassName(), "Point");
    EXPECT_EQ(noe.getQualifiedClassName()->getFullName(), "Point");
    EXPECT_NE(noe.getArguments(), nullptr);
    EXPECT_EQ(noe.toString(), "NewObjectExpression Point");
}

TEST_F(ASTExpressionTest, NewObjectExpressionQualified) {
    auto qi = std::make_unique<QualifiedIdentifier>(std::vector<std::string>{"std", "String"});
    auto args = std::make_unique<ArgumentList>(std::vector<std::unique_ptr<Expression>>{});
    NewObjectExpression noe(std::move(qi), std::move(args));
    EXPECT_EQ(noe.getClassName(), "String");
    EXPECT_EQ(noe.getQualifiedClassName()->getFullName(), "std.String");
}

TEST_F(ASTExpressionTest, UnaryMinus) {
    auto op = intLit(5);
    UnaryMinus um(std::move(op));
    EXPECT_EQ(um.toString(), "UnaryMinus");
}

TEST_F(ASTExpressionTest, LogicalNot) {
    auto op = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    LogicalNot ln(std::move(op));
    EXPECT_EQ(ln.toString(), "LogicalNot");
}

TEST_F(ASTExpressionTest, MultiplicativeExpression) {
    MultiplicativeExpression me(intLit(3), BinaryOperator::MULTIPLY, intLit(4));
    EXPECT_EQ(me.getLeft().toString(), "PrimaryExpression(IntegerLiteral(3))");
    EXPECT_EQ(me.getOperator(), BinaryOperator::MULTIPLY);
    EXPECT_EQ(me.getRight().toString(), "PrimaryExpression(IntegerLiteral(4))");
    EXPECT_EQ(me.toString(), "MultiplicativeExpression");
}

TEST_F(ASTExpressionTest, AdditiveExpression) {
    AdditiveExpression ae(intLit(1), BinaryOperator::PLUS, intLit(2));
    EXPECT_EQ(ae.getOperator(), BinaryOperator::PLUS);
    EXPECT_EQ(ae.toString(), "AdditiveExpression");
}

TEST_F(ASTExpressionTest, RelationalExpression) {
    RelationalExpression re(intLit(5), BinaryOperator::GREATER, intLit(3));
    EXPECT_EQ(re.getOperator(), BinaryOperator::GREATER);
    EXPECT_EQ(re.toString(), "RelationalExpression");
}

TEST_F(ASTExpressionTest, LogicalAnd) {
    auto t = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    auto f = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(false));
    LogicalAnd la(std::move(t), std::move(f));
    EXPECT_EQ(la.toString(), "LogicalAnd");
}

TEST_F(ASTExpressionTest, LogicalOr) {
    auto t = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    auto f = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(false));
    LogicalOr lo(std::move(t), std::move(f));
    EXPECT_EQ(lo.toString(), "LogicalOr");
}

TEST_F(ASTExpressionTest, AssignmentExpression) {
    auto lhs = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("x"));
    AssignmentExpression ae(std::move(lhs), intLit(42));
    EXPECT_EQ(ae.toString(), "AssignmentExpression");
}

TEST_F(ASTExpressionTest, CompoundAssignmentExpression) {
    auto lhs = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("x"));
    CompoundAssignmentExpression cae(std::move(lhs), CompoundAssignmentOperator::PLUS_ASSIGN, intLit(1));
    EXPECT_EQ(cae.getOperator(), CompoundAssignmentOperator::PLUS_ASSIGN);
    EXPECT_EQ(cae.toString(), "CompoundAssignmentExpression");
}

TEST_F(ASTExpressionTest, IncrementDecrementPrefix) {
    auto op = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("i"));
    IncrementDecrementExpression ide(std::move(op), IncrementDecrementOperator::INCREMENT, true);
    EXPECT_EQ(ide.getOperator(), IncrementDecrementOperator::INCREMENT);
    EXPECT_TRUE(ide.isPrefix());
}

TEST_F(ASTExpressionTest, IncrementDecrementPostfix) {
    auto op = std::make_unique<PrimaryExpression>(std::make_unique<Identifier>("i"));
    IncrementDecrementExpression ide(std::move(op), IncrementDecrementOperator::DECREMENT, false);
    EXPECT_TRUE(!ide.isPrefix());
    EXPECT_EQ(ide.toString(), "IncrementDecrementExpression");
}

TEST_F(ASTExpressionTest, ExpressionList) {
    std::vector<std::unique_ptr<Expression>> exprs;
    exprs.push_back(intLit(1));
    exprs.push_back(intLit(2));
    exprs.push_back(intLit(3));
    ExpressionList el(std::move(exprs));
    EXPECT_EQ(el.getExpressions().size(), 3);
}

TEST_F(ASTExpressionTest, ArrayLiteral) {
    std::vector<std::unique_ptr<Expression>> elems;
    elems.push_back(intLit(1));
    elems.push_back(intLit(2));
    auto el = std::make_unique<ExpressionList>(std::move(elems));
    ArrayLiteral al(std::move(el));
    EXPECT_NE(al.getElements(), nullptr);
    EXPECT_EQ(al.getElements()->getExpressions().size(), 2);
}
