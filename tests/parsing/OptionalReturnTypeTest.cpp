#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "src/ast/Declaration.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for optional return type functionality in function declarations.
 *
 * Functions without an explicit return type should default to void.
 */
class OptionalReturnTypeTest : public ::testing::Test {
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

    // Helper to extract first declaration from compilation unit
    const Declaration* getFirstDeclaration(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;
        return decls[0].get();
    }
};

// Test 1: Function without return type should default to void
TEST_F(OptionalReturnTypeTest, FunctionWithoutReturnType) {
    std::string code = R"(
        func doSomething() {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "doSomething");

    // Verify return type is void
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);
    EXPECT_TRUE(baseType->isPrimitive());

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::VOID);
}

// Test 2: Function with parameters but without return type
TEST_F(OptionalReturnTypeTest, FunctionWithParametersNoReturnType) {
    std::string code = R"(
        func processData(x: int64, y: int64) {
            var result = x + y;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "processData");
    EXPECT_EQ(funcDecl->getParameters().size(), 2);

    // Verify return type is void
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::VOID);
}

// Test 3: Function with explicit void return type
TEST_F(OptionalReturnTypeTest, FunctionWithExplicitVoidReturnType) {
    std::string code = R"(
        func doSomething() {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "doSomething");

    // Verify return type is void
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::VOID);
}

// Test 4: Function with explicit non-void return type
TEST_F(OptionalReturnTypeTest, FunctionWithExplicitReturnType) {
    std::string code = R"(
        func:int64 calculate() {
            return 42;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "calculate");

    // Verify return type is int64
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::INT64);
}

// Test 5: Multiple functions with mixed return type styles
TEST_F(OptionalReturnTypeTest, MultipleFunctionsWithMixedReturnTypes) {
    std::string code = R"(
        func noReturnType() {
        }

        func:int64 withReturnType() {
            return 10;
        }

        func explicitVoid() {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto& decls = ast->getDeclarations();
    EXPECT_EQ(decls.size(), 3);

    // First function - no return type (should be void)
    auto* func1 = dynamic_cast<const FunctionDeclaration*>(decls[0].get());
    ASSERT_NE(func1, nullptr);
    EXPECT_EQ(func1->getName(), "noReturnType");
    auto* returnType1 = func1->getReturnType();
    ASSERT_NE(returnType1, nullptr);
    auto* baseType1 = dynamic_cast<const BaseType*>(returnType1);
    ASSERT_NE(baseType1, nullptr);
    auto* primitiveType1 = baseType1->getPrimitiveType();
    ASSERT_NE(primitiveType1, nullptr);
    EXPECT_EQ(primitiveType1->getKind(), PrimitiveTypeKind::VOID);

    // Second function - explicit int64 return type
    auto* func2 = dynamic_cast<const FunctionDeclaration*>(decls[1].get());
    ASSERT_NE(func2, nullptr);
    EXPECT_EQ(func2->getName(), "withReturnType");
    auto* returnType2 = func2->getReturnType();
    ASSERT_NE(returnType2, nullptr);
    auto* baseType2 = dynamic_cast<const BaseType*>(returnType2);
    ASSERT_NE(baseType2, nullptr);
    auto* primitiveType2 = baseType2->getPrimitiveType();
    ASSERT_NE(primitiveType2, nullptr);
    EXPECT_EQ(primitiveType2->getKind(), PrimitiveTypeKind::INT64);

    // Third function - explicit void return type
    auto* func3 = dynamic_cast<const FunctionDeclaration*>(decls[2].get());
    ASSERT_NE(func3, nullptr);
    EXPECT_EQ(func3->getName(), "explicitVoid");
    auto* returnType3 = func3->getReturnType();
    ASSERT_NE(returnType3, nullptr);
    auto* baseType3 = dynamic_cast<const BaseType*>(returnType3);
    ASSERT_NE(baseType3, nullptr);
    auto* primitiveType3 = baseType3->getPrimitiveType();
    ASSERT_NE(primitiveType3, nullptr);
    EXPECT_EQ(primitiveType3->getKind(), PrimitiveTypeKind::VOID);
}

// Test 6: Function without return type with complex body
TEST_F(OptionalReturnTypeTest, FunctionWithoutReturnTypeComplexBody) {
    std::string code = R"(
        func complexFunction(x: int64) {
            var y = x + 10;
            if (y > 20) {
                y = 20;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "complexFunction");
    EXPECT_EQ(funcDecl->getParameters().size(), 1);

    // Verify return type is void
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::VOID);
}

// Test 7: Function without return type but with empty parameter list
TEST_F(OptionalReturnTypeTest, FunctionNoParamsNoReturnType) {
    std::string code = R"(
        func emptyFunction() {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "emptyFunction");
    EXPECT_EQ(funcDecl->getParameters().size(), 0);

    // Verify return type is void
    auto* returnType = funcDecl->getReturnType();
    ASSERT_NE(returnType, nullptr);
    auto* baseType = dynamic_cast<const BaseType*>(returnType);
    ASSERT_NE(baseType, nullptr);

    auto* primitiveType = baseType->getPrimitiveType();
    ASSERT_NE(primitiveType, nullptr);
    EXPECT_EQ(primitiveType->getKind(), PrimitiveTypeKind::VOID);
}
