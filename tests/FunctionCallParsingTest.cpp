#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"
#include "../src/ast/Expression.h"
#include "../src/ast/Primary.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite specifically for testing function call parsing in SimpleASTBuilder.
 *
 * This suite focuses on the buildPostfixExpression and buildArgumentList methods
 * in SimpleASTBuilder.cpp (lines 338-385), which handle:
 * - Function calls with zero, one, or multiple arguments
 * - Nested function calls
 * - Function calls with complex expressions as arguments
 * - Chained function calls
 */
class FunctionCallParsingTest : public ::testing::Test {
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

    // Helper to extract first statement's expression from a function body
    const Expression* getFirstStatementExpression(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;

        auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decls[0].get());
        if (!funcDecl) return nullptr;

        const Block& block = funcDecl->getBody();
        auto& stmts = block.getStatements();
        if (stmts.empty()) return nullptr;

        auto* exprStmt = dynamic_cast<const ExpressionStatement*>(stmts[0].get());
        if (!exprStmt) return nullptr;

        return &exprStmt->getExpression();
    }

    // Helper to extract return statement's expression from a function body
    const Expression* getReturnExpression(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;

        auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decls[0].get());
        if (!funcDecl) return nullptr;

        const Block& block = funcDecl->getBody();
        auto& stmts = block.getStatements();
        if (stmts.empty()) return nullptr;

        auto* retStmt = dynamic_cast<const ReturnStatement*>(stmts[0].get());
        if (!retStmt) return nullptr;

        return retStmt->getExpression();
    }
};

// Test 1: Simple function call with no arguments - foo()
TEST_F(FunctionCallParsingTest, SimpleFunctionCallNoArguments) {
    std::string code = R"(
        func test() -> void {
            foo();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    // Get the function call expression
    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    // Verify it's a FunctionCall
    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr) << "Expression should be a FunctionCall";

    // Verify the function being called is an identifier "foo"
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);

    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "foo");

    // Verify the argument list is empty
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    EXPECT_EQ(argList->getArguments().size(), 0) << "Function call should have 0 arguments";
}

// Test 2: Function call with one integer argument - bar(42)
TEST_F(FunctionCallParsingTest, FunctionCallWithOneIntegerArgument) {
    std::string code = R"(
        func test() -> void {
            bar(42);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    // Verify function name
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "bar");

    // Verify arguments
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 1) << "Function call should have 1 argument";

    // Verify the argument is the integer literal 42
    auto& arg = argList->getArguments()[0];
    auto* argPrimaryExpr = dynamic_cast<const PrimaryExpression*>(arg.get());
    ASSERT_NE(argPrimaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&argPrimaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 42);
}

// Test 3: Function call with multiple arguments - add(1, 2, 3)
TEST_F(FunctionCallParsingTest, FunctionCallWithMultipleArguments) {
    std::string code = R"(
        func test() -> void {
            add(10, 20, 30);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    // Verify function name
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "add");

    // Verify arguments
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 3) << "Function call should have 3 arguments";

    // Verify each argument value
    int expected_values[] = {10, 20, 30};
    for (size_t i = 0; i < 3; i++) {
        auto& arg = argList->getArguments()[i];
        auto* argPrimaryExpr = dynamic_cast<const PrimaryExpression*>(arg.get());
        ASSERT_NE(argPrimaryExpr, nullptr) << "Argument " << i << " should be a PrimaryExpression";
        auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&argPrimaryExpr->getPrimary());
        ASSERT_NE(intLiteral, nullptr) << "Argument " << i << " should be an IntegerLiteral";
        EXPECT_EQ(intLiteral->getValue(), expected_values[i]) << "Argument " << i << " value mismatch";
    }
}

// Test 4: Function call with expression as argument - calculate(x + y)
TEST_F(FunctionCallParsingTest, FunctionCallWithExpressionArgument) {
    std::string code = R"(
        func test() -> void {
            calculate(5 + 10);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    // Verify function name
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "calculate");

    // Verify argument is an expression (additive expression)
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 1);

    auto& arg = argList->getArguments()[0];
    auto* additiveExpr = dynamic_cast<const AdditiveExpression*>(arg.get());
    ASSERT_NE(additiveExpr, nullptr) << "Argument should be an AdditiveExpression";
}

// Test 5: Nested function call - outer(inner(42))
TEST_F(FunctionCallParsingTest, NestedFunctionCall) {
    std::string code = R"(
        func test() -> void {
            outer(inner(42));
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    // Verify outer function call
    auto* outerCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(outerCall, nullptr);

    auto& outerFuncExpr = outerCall->getFunction();
    auto* outerPrimaryExpr = dynamic_cast<const PrimaryExpression*>(&outerFuncExpr);
    ASSERT_NE(outerPrimaryExpr, nullptr);
    auto* outerIdentifier = dynamic_cast<const Identifier*>(&outerPrimaryExpr->getPrimary());
    ASSERT_NE(outerIdentifier, nullptr);
    EXPECT_EQ(outerIdentifier->getName(), "outer");

    // Verify the argument to outer is a function call
    auto* outerArgList = outerCall->getArguments();
    ASSERT_NE(outerArgList, nullptr);
    ASSERT_EQ(outerArgList->getArguments().size(), 1);

    auto& outerArg = outerArgList->getArguments()[0];
    auto* innerCall = dynamic_cast<const FunctionCall*>(outerArg.get());
    ASSERT_NE(innerCall, nullptr) << "Outer function's argument should be an inner function call";

    // Verify inner function call
    auto& innerFuncExpr = innerCall->getFunction();
    auto* innerPrimaryExpr = dynamic_cast<const PrimaryExpression*>(&innerFuncExpr);
    ASSERT_NE(innerPrimaryExpr, nullptr);
    auto* innerIdentifier = dynamic_cast<const Identifier*>(&innerPrimaryExpr->getPrimary());
    ASSERT_NE(innerIdentifier, nullptr);
    EXPECT_EQ(innerIdentifier->getName(), "inner");

    // Verify inner function's argument is 42
    auto* innerArgList = innerCall->getArguments();
    ASSERT_NE(innerArgList, nullptr);
    ASSERT_EQ(innerArgList->getArguments().size(), 1);

    auto& innerArg = innerArgList->getArguments()[0];
    auto* innerArgPrimaryExpr = dynamic_cast<const PrimaryExpression*>(innerArg.get());
    ASSERT_NE(innerArgPrimaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&innerArgPrimaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 42);
}

// Test 6: Chained function calls - getObject().getMethod()
TEST_F(FunctionCallParsingTest, ChainedFunctionCalls) {
    std::string code = R"(
        func test() -> void {
            foo()();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    // The outer call is calling the result of foo()
    auto* outerCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(outerCall, nullptr);

    // The function being called should be foo()
    auto& funcExpr = outerCall->getFunction();
    auto* innerCall = dynamic_cast<const FunctionCall*>(&funcExpr);
    ASSERT_NE(innerCall, nullptr) << "Chained call: outer call's function should be inner call";

    // Verify the inner call is foo
    auto& innerFuncExpr = innerCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&innerFuncExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "foo");
}

// Test 7: Function call with mixed argument types - process(42, 3.14, true, 'x')
TEST_F(FunctionCallParsingTest, FunctionCallWithMixedArgumentTypes) {
    std::string code = R"(
        func test() -> void {
            process(42, 3.14, true, 'x');
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 4) << "Function call should have 4 arguments";

    // Arg 0: Integer 42
    auto& arg0 = argList->getArguments()[0];
    auto* arg0Primary = dynamic_cast<const PrimaryExpression*>(arg0.get());
    ASSERT_NE(arg0Primary, nullptr);
    auto* intLit = dynamic_cast<const IntegerLiteral*>(&arg0Primary->getPrimary());
    ASSERT_NE(intLit, nullptr);
    EXPECT_EQ(intLit->getValue(), 42);

    // Arg 1: Float 3.14
    auto& arg1 = argList->getArguments()[1];
    auto* arg1Primary = dynamic_cast<const PrimaryExpression*>(arg1.get());
    ASSERT_NE(arg1Primary, nullptr);
    auto* floatLit = dynamic_cast<const FloatingLiteral*>(&arg1Primary->getPrimary());
    ASSERT_NE(floatLit, nullptr);
    EXPECT_DOUBLE_EQ(floatLit->getValue(), 3.14);

    // Arg 2: Boolean true
    auto& arg2 = argList->getArguments()[2];
    auto* arg2Primary = dynamic_cast<const PrimaryExpression*>(arg2.get());
    ASSERT_NE(arg2Primary, nullptr);
    auto* boolLit = dynamic_cast<const BooleanLiteral*>(&arg2Primary->getPrimary());
    ASSERT_NE(boolLit, nullptr);
    EXPECT_TRUE(boolLit->getValue());

    // Arg 3: Character 'x'
    auto& arg3 = argList->getArguments()[3];
    auto* arg3Primary = dynamic_cast<const PrimaryExpression*>(arg3.get());
    ASSERT_NE(arg3Primary, nullptr);
    auto* charLit = dynamic_cast<const CharacterLiteral*>(&arg3Primary->getPrimary());
    ASSERT_NE(charLit, nullptr);
    EXPECT_EQ(charLit->getValue(), 'x');
}

// Test 8: Function call with variable arguments - myFunc(x, y, z)
TEST_F(FunctionCallParsingTest, FunctionCallWithVariableArguments) {
    std::string code = R"(
        func test() -> void {
            myFunc(x, y, z);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 3);

    // Verify each argument is an identifier
    const char* expectedNames[] = {"x", "y", "z"};
    for (size_t i = 0; i < 3; i++) {
        auto& arg = argList->getArguments()[i];
        auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(arg.get());
        ASSERT_NE(primaryExpr, nullptr) << "Argument " << i << " should be a PrimaryExpression";
        auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
        ASSERT_NE(identifier, nullptr) << "Argument " << i << " should be an Identifier";
        EXPECT_EQ(identifier->getName(), expectedNames[i]) << "Argument " << i << " name mismatch";
    }
}

// Test 9: Function call with complex expressions - foo(a + b, c * d, e == f)
TEST_F(FunctionCallParsingTest, FunctionCallWithComplexExpressions) {
    std::string code = R"(
        func test() -> void {
            foo(a + b, c * d, e == f);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 3);

    // Arg 0: a + b (AdditiveExpression)
    auto& arg0 = argList->getArguments()[0];
    EXPECT_NE(dynamic_cast<const AdditiveExpression*>(arg0.get()), nullptr)
        << "First argument should be AdditiveExpression";

    // Arg 1: c * d (MultiplicativeExpression)
    auto& arg1 = argList->getArguments()[1];
    EXPECT_NE(dynamic_cast<const MultiplicativeExpression*>(arg1.get()), nullptr)
        << "Second argument should be MultiplicativeExpression";

    // Arg 2: e == f (RelationalExpression)
    auto& arg2 = argList->getArguments()[2];
    EXPECT_NE(dynamic_cast<const RelationalExpression*>(arg2.get()), nullptr)
        << "Third argument should be RelationalExpression";
}

// Test 10: Function call in return statement - return foo(42)
TEST_F(FunctionCallParsingTest, FunctionCallInReturnStatement) {
    std::string code = R"(
        func test() -> int64 {
            return getValue(100);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getReturnExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr) << "Return expression should be a FunctionCall";

    // Verify function name
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "getValue");

    // Verify argument
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 1);

    auto& arg = argList->getArguments()[0];
    auto* argPrimaryExpr = dynamic_cast<const PrimaryExpression*>(arg.get());
    ASSERT_NE(argPrimaryExpr, nullptr);
    auto* intLiteral = dynamic_cast<const IntegerLiteral*>(&argPrimaryExpr->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 100);
}

// Test 11: Multiple chained calls in sequence - a()()()
TEST_F(FunctionCallParsingTest, MultipleChainedCalls) {
    std::string code = R"(
        func test() -> void {
            a()()();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    // Outermost call: (a()())()
    auto* call3 = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(call3, nullptr);

    // Second call: (a())()
    auto& funcExpr2 = call3->getFunction();
    auto* call2 = dynamic_cast<const FunctionCall*>(&funcExpr2);
    ASSERT_NE(call2, nullptr) << "Second level should be a FunctionCall";

    // First call: a()
    auto& funcExpr1 = call2->getFunction();
    auto* call1 = dynamic_cast<const FunctionCall*>(&funcExpr1);
    ASSERT_NE(call1, nullptr) << "First level should be a FunctionCall";

    // Base function: a
    auto& baseFunc = call1->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&baseFunc);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "a");
}

// Test 12: Function call with string literal argument - print("hello")
TEST_F(FunctionCallParsingTest, FunctionCallWithStringLiteral) {
    std::string code = R"(
        func test() -> void {
            print("hello world");
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    // Verify function name
    auto& funcExpr = funcCall->getFunction();
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(&funcExpr);
    ASSERT_NE(primaryExpr, nullptr);
    auto* identifier = dynamic_cast<const Identifier*>(&primaryExpr->getPrimary());
    ASSERT_NE(identifier, nullptr);
    EXPECT_EQ(identifier->getName(), "print");

    // Verify string argument
    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 1);

    auto& arg = argList->getArguments()[0];
    auto* argPrimaryExpr = dynamic_cast<const PrimaryExpression*>(arg.get());
    ASSERT_NE(argPrimaryExpr, nullptr);
    auto* stringLiteral = dynamic_cast<const StringLiteral*>(&argPrimaryExpr->getPrimary());
    ASSERT_NE(stringLiteral, nullptr);
    EXPECT_EQ(stringLiteral->getValue(), "hello world");
}

// Test 13: Deeply nested function calls - a(b(c(d())))
TEST_F(FunctionCallParsingTest, DeeplyNestedFunctionCalls) {
    std::string code = R"(
        func test() -> void {
            a(b(c(d())));
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    // Level 1: a(...)
    auto* callA = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(callA, nullptr);
    auto* argListA = callA->getArguments();
    ASSERT_NE(argListA, nullptr);
    ASSERT_EQ(argListA->getArguments().size(), 1);

    // Level 2: b(...)
    auto& argA = argListA->getArguments()[0];
    auto* callB = dynamic_cast<const FunctionCall*>(argA.get());
    ASSERT_NE(callB, nullptr);
    auto* argListB = callB->getArguments();
    ASSERT_NE(argListB, nullptr);
    ASSERT_EQ(argListB->getArguments().size(), 1);

    // Level 3: c(...)
    auto& argB = argListB->getArguments()[0];
    auto* callC = dynamic_cast<const FunctionCall*>(argB.get());
    ASSERT_NE(callC, nullptr);
    auto* argListC = callC->getArguments();
    ASSERT_NE(argListC, nullptr);
    ASSERT_EQ(argListC->getArguments().size(), 1);

    // Level 4: d()
    auto& argC = argListC->getArguments()[0];
    auto* callD = dynamic_cast<const FunctionCall*>(argC.get());
    ASSERT_NE(callD, nullptr);
    auto* argListD = callD->getArguments();
    ASSERT_NE(argListD, nullptr);
    EXPECT_EQ(argListD->getArguments().size(), 0) << "Innermost call d() should have no arguments";
}

// Test 14: Function call with logical expressions - validate(a && b, c || d)
TEST_F(FunctionCallParsingTest, FunctionCallWithLogicalExpressions) {
    std::string code = R"(
        func test() -> void {
            validate(a && b, c || d);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 2);

    // Arg 0: a && b (LogicalAnd)
    auto& arg0 = argList->getArguments()[0];
    EXPECT_NE(dynamic_cast<const LogicalAnd*>(arg0.get()), nullptr)
        << "First argument should be LogicalAnd expression";

    // Arg 1: c || d (LogicalOr)
    auto& arg1 = argList->getArguments()[1];
    EXPECT_NE(dynamic_cast<const LogicalOr*>(arg1.get()), nullptr)
        << "Second argument should be LogicalOr expression";
}

// Test 15: Function call with unary expressions - process(-x, !flag)
TEST_F(FunctionCallParsingTest, FunctionCallWithUnaryExpressions) {
    std::string code = R"(
        func test() -> void {
            process(-x, !flag);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* expr = getFirstStatementExpression(*ast);
    ASSERT_NE(expr, nullptr);

    auto* funcCall = dynamic_cast<const FunctionCall*>(expr);
    ASSERT_NE(funcCall, nullptr);

    auto* argList = funcCall->getArguments();
    ASSERT_NE(argList, nullptr);
    ASSERT_EQ(argList->getArguments().size(), 2);

    // Arg 0: -x (UnaryMinus)
    auto& arg0 = argList->getArguments()[0];
    EXPECT_NE(dynamic_cast<const UnaryMinus*>(arg0.get()), nullptr)
        << "First argument should be UnaryMinus expression";

    // Arg 1: !flag (LogicalNot)
    auto& arg1 = argList->getArguments()[1];
    EXPECT_NE(dynamic_cast<const LogicalNot*>(arg1.get()), nullptr)
        << "Second argument should be LogicalNot expression";
}
