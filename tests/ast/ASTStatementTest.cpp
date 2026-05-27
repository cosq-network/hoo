#include <gtest/gtest.h>
#include "ast/AST.h"
#include "ast/Primary.h"
#include "ast/Expression.h"
#include "ast/Statement.h"

using namespace hooc::ast;

class ASTStatementTest : public ::testing::Test {};

static auto intLit(int64_t v) {
    return std::make_unique<PrimaryExpression>(std::make_unique<IntegerLiteral>(v));
}

static auto identExpr(const std::string& name) {
    return std::make_unique<PrimaryExpression>(std::make_unique<Identifier>(name));
}

static auto emptyBlock() {
    return std::make_unique<Block>(std::vector<std::unique_ptr<Statement>>{});
}

TEST_F(ASTStatementTest, BlockEmpty) {
    Block b({});
    EXPECT_TRUE(b.getStatements().empty());
    EXPECT_EQ(b.toString(), "Block");
}

TEST_F(ASTStatementTest, BlockWithStatements) {
    std::vector<std::unique_ptr<Statement>> stmts;
    stmts.push_back(std::make_unique<BreakStatement>());
    stmts.push_back(std::make_unique<ContinueStatement>());
    Block b(std::move(stmts));
    EXPECT_EQ(b.getStatements().size(), 2);
}

TEST_F(ASTStatementTest, ExpressionStatement) {
    auto expr = intLit(42);
    ExpressionStatement es(std::move(expr));
    EXPECT_EQ(es.toString(), "ExpressionStatement");
}

TEST_F(ASTStatementTest, IfStatementWithoutElse) {
    auto cond = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    IfStatement is(std::move(cond), emptyBlock());
    EXPECT_FALSE(is.hasElse());
    EXPECT_EQ(is.getElseBlock(), nullptr);
    EXPECT_EQ(is.toString(), "IfStatement");
}

TEST_F(ASTStatementTest, IfStatementWithElse) {
    auto cond = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    IfStatement is(std::move(cond), emptyBlock(), emptyBlock());
    EXPECT_TRUE(is.hasElse());
    EXPECT_NE(is.getElseBlock(), nullptr);
}

TEST_F(ASTStatementTest, ForInStatement) {
    auto iterable = intLit(5);
    ForInStatement fis("i", std::move(iterable), emptyBlock());
    EXPECT_EQ(fis.getVariable(), "i");
    EXPECT_EQ(fis.toString(), "ForInStatement i");
}

TEST_F(ASTStatementTest, ForRangeStatementWithoutStep) {
    auto start = intLit(0);
    auto end = intLit(10);
    ForRangeStatement frs("i", std::move(start), std::move(end), nullptr, emptyBlock());
    EXPECT_EQ(frs.getVariable(), "i");
    EXPECT_FALSE(frs.hasStep());
    EXPECT_EQ(frs.getStep(), nullptr);
    EXPECT_EQ(frs.toString(), "ForRangeStatement i");
}

TEST_F(ASTStatementTest, ForRangeStatementWithStep) {
    auto start = intLit(0);
    auto end = intLit(100);
    auto step = intLit(5);
    ForRangeStatement frs("x", std::move(start), std::move(end), std::move(step), emptyBlock());
    EXPECT_TRUE(frs.hasStep());
    EXPECT_NE(frs.getStep(), nullptr);
    EXPECT_EQ(frs.toString(), "ForRangeStatement x (with step)");
}

TEST_F(ASTStatementTest, WhileStatement) {
    auto cond = std::make_unique<PrimaryExpression>(std::make_unique<BooleanLiteral>(true));
    WhileStatement ws(std::move(cond), emptyBlock());
    EXPECT_EQ(ws.toString(), "WhileStatement");
}

TEST_F(ASTStatementTest, ReturnStatementWithoutExpression) {
    ReturnStatement rs;
    EXPECT_FALSE(rs.hasExpression());
    EXPECT_EQ(rs.getExpression(), nullptr);
}

TEST_F(ASTStatementTest, ReturnStatementWithExpression) {
    ReturnStatement rs(intLit(42));
    EXPECT_TRUE(rs.hasExpression());
    EXPECT_NE(rs.getExpression(), nullptr);
    EXPECT_EQ(rs.toString(), "ReturnStatement");
}

TEST_F(ASTStatementTest, VariableDeclarationStatement) {
    auto type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::INT64);
    auto vd = std::make_unique<VariableDeclaration>(std::move(type), "x");
    VariableDeclarationStatement vds(std::move(vd));
    EXPECT_EQ(vds.getDeclaration().getName(), "x");
}

TEST_F(ASTStatementTest, BreakStatement) {
    BreakStatement bs;
    EXPECT_EQ(bs.toString(), "BreakStatement");
}

TEST_F(ASTStatementTest, ContinueStatement) {
    ContinueStatement cs;
    EXPECT_EQ(cs.toString(), "ContinueStatement");
}

TEST_F(ASTStatementTest, TryCatchStatementTryOnly) {
    TryCatchStatement tcs(emptyBlock(), {}, nullptr);
    EXPECT_FALSE(tcs.hasCatch());
    EXPECT_FALSE(tcs.hasFinally());
    EXPECT_EQ(tcs.getFinallyBlock(), nullptr);
    EXPECT_TRUE(tcs.getCatchClauses().empty());
    EXPECT_EQ(tcs.toString(), "TryCatchStatement");
}

TEST_F(ASTStatementTest, TryCatchStatementWithFinally) {
    TryCatchStatement tcs(emptyBlock(), {}, emptyBlock());
    EXPECT_TRUE(tcs.hasFinally());
    EXPECT_NE(tcs.getFinallyBlock(), nullptr);
    EXPECT_EQ(tcs.toString(), "TryCatchStatement (with finally)");
}

TEST_F(ASTStatementTest, TryCatchStatementWithCatch) {
    TryCatchStatement::CatchClause clause;
    clause.variable = "e";
    clause.type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::STRING);
    clause.block = emptyBlock();
    std::vector<TryCatchStatement::CatchClause> clauses;
    clauses.push_back(std::move(clause));
    TryCatchStatement tcs(emptyBlock(), std::move(clauses), nullptr);
    EXPECT_TRUE(tcs.hasCatch());
    EXPECT_EQ(tcs.getCatchClauses().size(), 1);
    EXPECT_EQ(tcs.getCatchClauses()[0].variable, "e");
}

TEST_F(ASTStatementTest, ThrowStatement) {
    ThrowStatement ts(intLit(1));
    EXPECT_EQ(ts.getKind(), ThrowStatement::ThrowKind::THROW);
    EXPECT_FALSE(ts.isRethrow());
    EXPECT_NE(ts.getExpression(), nullptr);
    EXPECT_EQ(ts.toString(), "ThrowStatement");
}

TEST_F(ASTStatementTest, RethrowStatement) {
    ThrowStatement ts(ThrowStatement::ThrowKind::RETHROW);
    EXPECT_TRUE(ts.isRethrow());
    EXPECT_EQ(ts.getExpression(), nullptr);
    EXPECT_EQ(ts.toString(), "RethrowStatement");
}

TEST_F(ASTStatementTest, ThrowRequiresExpression) {
    EXPECT_THROW(ThrowStatement(nullptr), std::invalid_argument);
}

TEST_F(ASTStatementTest, ThrowKindRequiresExpression) {
    EXPECT_THROW(ThrowStatement(ThrowStatement::ThrowKind::THROW, nullptr), std::invalid_argument);
}

TEST_F(ASTStatementTest, RethrowRejectsExpression) {
    EXPECT_THROW(ThrowStatement(ThrowStatement::ThrowKind::RETHROW, intLit(1)), std::invalid_argument);
}
