#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "src/ast/Expression.h"
#include "src/ast/Primary.h"
#include "src/ast/Statement.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for variable declaration parsing in SimpleASTBuilder.
 *
 * This suite focuses on testing variable declarations with primitive types:
 * - byte, int64, double, bool, char
 * - Explicit type annotations
 * - Type inference
 * - Initialization expressions
 */
class VariableDeclarationParseTest : public ::testing::Test {
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

    // Helper to extract first statement from a function body
    const Statement* getFirstStatement(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;

        auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decls[0].get());
        if (!funcDecl) return nullptr;

        const Block& block = funcDecl->getBody();
        auto& stmts = block.getStatements();
        if (stmts.empty()) return nullptr;

        return stmts[0].get();
    }

    // Helper to extract variable declaration from a statement
    const VariableDeclaration* getVariableDecl(const Statement* stmt) {
        auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
        if (!varDeclStmt) return nullptr;
        return &varDeclStmt->getDeclaration();
    }
};

// Test 1: Variable declaration with explicit int64 type
TEST_F(VariableDeclarationParseTest, ExplicitInt64Declaration) {
    std::string code = R"(
        func test() -> void {
            var x: int64 = 42;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr) << "Statement should be a VariableDeclaration";

    // Verify variable name
    EXPECT_EQ(varDecl->getName(), "x");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::INT64);

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 42);
}

// Test 2: Variable declaration with type inference (int64)
TEST_F(VariableDeclarationParseTest, Int64TypeInference) {
    std::string code = R"(
        func test() -> void {
            var num = 100;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "num");

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 100);
}

// Test 3: Variable declaration with explicit double type
TEST_F(VariableDeclarationParseTest, ExplicitDoubleDeclaration) {
    std::string code = R"(
        func test() -> void {
            var pi: double = 3.14159;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "pi");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::DOUBLE);

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* floatLiteral = dynamic_cast<const FloatingLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(floatLiteral, nullptr);
    EXPECT_DOUBLE_EQ(floatLiteral->getValue(), 3.14159);
}

// Test 4: Variable declaration with type inference (double)
TEST_F(VariableDeclarationParseTest, DoubleTypeInference) {
    std::string code = R"(
        func test() -> void {
            var value = 2.718;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "value");

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* floatLiteral = dynamic_cast<const FloatingLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(floatLiteral, nullptr);
    EXPECT_DOUBLE_EQ(floatLiteral->getValue(), 2.718);
}

// Test 5: Variable declaration with explicit bool type
TEST_F(VariableDeclarationParseTest, ExplicitBoolDeclaration) {
    std::string code = R"(
        func test() -> void {
            var flag: bool = true;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "flag");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::BOOL);

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* boolLiteral = dynamic_cast<const BooleanLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(boolLiteral, nullptr);
    EXPECT_TRUE(boolLiteral->getValue());
}

// Test 6: Variable declaration with type inference (bool)
TEST_F(VariableDeclarationParseTest, BoolTypeInference) {
    std::string code = R"(
        func test() -> void {
            var isReady = false;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "isReady");

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* boolLiteral = dynamic_cast<const BooleanLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(boolLiteral, nullptr);
    EXPECT_FALSE(boolLiteral->getValue());
}

// Test 7: Variable declaration with explicit char type
TEST_F(VariableDeclarationParseTest, ExplicitCharDeclaration) {
    std::string code = R"(
        func test() -> void {
            var ch: char = 'A';
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "ch");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::CHAR);

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* charLiteral = dynamic_cast<const CharacterLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(charLiteral, nullptr);
    EXPECT_EQ(charLiteral->getValue(), 'A');
}

// Test 8: Variable declaration with type inference (char)
TEST_F(VariableDeclarationParseTest, CharTypeInference) {
    std::string code = R"(
        func test() -> void {
            var letter = 'z';
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "letter");

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* charLiteral = dynamic_cast<const CharacterLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(charLiteral, nullptr);
    EXPECT_EQ(charLiteral->getValue(), 'z');
}

// Test 9: Variable declaration with explicit byte type
TEST_F(VariableDeclarationParseTest, ExplicitByteDeclaration) {
    std::string code = R"(
        func test() -> void {
            var b: byte = 255;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "b");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::BYTE);

    // Verify initializer
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 255);
}

// Test 10: Variable declaration with expression initializer
TEST_F(VariableDeclarationParseTest, VariableWithExpressionInitializer) {
    std::string code = R"(
        func test() -> void {
            var sum: int64 = 10 + 20;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "sum");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::INT64);

    // Verify initializer is an additive expression
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* additiveExpr = dynamic_cast<const AdditiveExpression*>(initializer);
    ASSERT_NE(additiveExpr, nullptr) << "Initializer should be an AdditiveExpression";
}

// Test 11: Multiple variable declarations in sequence
TEST_F(VariableDeclarationParseTest, MultipleVariableDeclarations) {
    std::string code = R"(
        func test() -> void {
            var x: int64 = 10;
            var y: int64 = 20;
            var z: int64 = 30;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto& decls = ast->getDeclarations();
    ASSERT_FALSE(decls.empty());

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decls[0].get());
    ASSERT_NE(funcDecl, nullptr);

    const Block& block = funcDecl->getBody();
    auto& stmts = block.getStatements();
    ASSERT_EQ(stmts.size(), 3) << "Should have 3 variable declarations";

    // Verify each variable declaration
    const char* expectedNames[] = {"x", "y", "z"};
    int expectedValues[] = {10, 20, 30};

    for (size_t i = 0; i < 3; i++) {
        auto* varDecl = getVariableDecl(stmts[i].get());
        ASSERT_NE(varDecl, nullptr) << "Statement " << i << " should be a VariableDeclaration";
        EXPECT_EQ(varDecl->getName(), expectedNames[i]) << "Variable " << i << " name mismatch";

        auto* initializer = varDecl->getInitializer();
        ASSERT_NE(initializer, nullptr);
        auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
        ASSERT_NE(primaryExpr, nullptr);
        auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&primaryExpr->getPrimary());
        ASSERT_NE(intLiteral, nullptr);
        EXPECT_EQ(intLiteral->getValue(), expectedValues[i]) << "Variable " << i << " value mismatch";
    }
}

// Test 12: Variable declaration with complex expression
TEST_F(VariableDeclarationParseTest, VariableWithComplexExpression) {
    std::string code = R"(
        func test() -> void {
            var result: int64 = (5 + 3) * 2;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "result");

    // Verify initializer is a multiplicative expression
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* multExpr = dynamic_cast<const MultiplicativeExpression*>(initializer);
    ASSERT_NE(multExpr, nullptr) << "Initializer should be a MultiplicativeExpression";
}

// Test 13: Variable declaration with boolean expression
TEST_F(VariableDeclarationParseTest, VariableWithBooleanExpression) {
    std::string code = R"(
        func test() -> void {
            var isValid: bool = true && false;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "isValid");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::BOOL);

    // Verify initializer is a logical AND expression
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* logicalAnd = dynamic_cast<const LogicalAnd*>(initializer);
    ASSERT_NE(logicalAnd, nullptr) << "Initializer should be a LogicalAnd expression";
}

// Test 14: Variable declaration with comparison expression
TEST_F(VariableDeclarationParseTest, VariableWithComparisonExpression) {
    std::string code = R"(
        func test() -> void {
            var isGreater = 10 > 5;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "isGreater");

    // Verify initializer is a relational expression
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* relationalExpr = dynamic_cast<const RelationalExpression*>(initializer);
    ASSERT_NE(relationalExpr, nullptr) << "Initializer should be a RelationalExpression";
}

// Test 15: Variable declaration with negative number
TEST_F(VariableDeclarationParseTest, VariableWithNegativeNumber) {
    std::string code = R"(
        func test() -> void {
            var negative: int64 = -42;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* stmt = getFirstStatement(*ast);
    ASSERT_NE(stmt, nullptr);

    auto* varDecl = getVariableDecl(stmt);
    ASSERT_NE(varDecl, nullptr);

    EXPECT_EQ(varDecl->getName(), "negative");

    // Verify type
    auto* baseType = dynamic_cast<const BaseType*>(varDecl->getType());
    ASSERT_NE(baseType, nullptr);
    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::INT64);

    // Verify initializer is a unary minus expression
    auto* initializer = varDecl->getInitializer();
    ASSERT_NE(initializer, nullptr);
    auto* unaryMinus = dynamic_cast<const UnaryMinus*>(initializer);
    ASSERT_NE(unaryMinus, nullptr) << "Initializer should be a UnaryMinus expression";
}
