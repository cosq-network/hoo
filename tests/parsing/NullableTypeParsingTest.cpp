#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"
#include "src/ast/Declaration.h"
#include "src/ast/Statement.h"
#include "src/ast/Type.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for nullable type parsing in variable declarations.
 *
 * This test suite verifies that nullable types (with ? suffix) are properly parsed
 * and represented in the AST. It tests both variable declarations with explicit types
 * and function parameters with nullable types.
 *
 * Syntax examples:
 * - var x: int64? = null;           // nullable integer
 * - var name: string? = null;       // nullable string
 * - var items: int64[]? = null;     // nullable array
 * - func process(val: int64?)  // nullable parameter
 */
class NullableTypeParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<HooParserWrapper> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }

    HoocParser::CompilationUnitContext* getCompilationUnit(antlr4::tree::ParseTree* tree) {
        return dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
    }

    // Helper to extract function declaration from compilation unit
    const FunctionDeclaration* getFunctionDeclaration(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (!decls.empty()) {
            return dynamic_cast<const FunctionDeclaration*>(decls[0].get());
        }
        return nullptr;
    }
};

// Test 1: Simple nullable primitive type in function
TEST_F(NullableTypeParsingTest, SimpleNullableInt64Type) {
    std::string code = R"(
        func test() {
            var x: int64? = 0;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    auto& stmts = funcDecl->getBody().getStatements();
    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmts[0].get());
    auto& varDecl = varDeclStmt->getDeclaration();

    EXPECT_EQ(varDecl.getName(), "x");
    auto* type = dynamic_cast<const OptionalType*>(varDecl.getType());
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isOptional());
}

// Test 2: Nullable type in function parameter
TEST_F(NullableTypeParsingTest, NullableTypeInFunctionParameter) {
    std::string code = R"(
        func process(value: int64?) {
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto& params = funcDecl->getParameters();
    auto* type = dynamic_cast<const OptionalType*>(&params[0]->getType());
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isOptional());
}

// Test 3: Nullable array type in function parameter
TEST_F(NullableTypeParsingTest, NullableArrayTypeInParameter) {
    std::string code = R"(
        func processArray(items: int64[]?) {
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto& params = funcDecl->getParameters();
    auto* type = dynamic_cast<const OptionalType*>(&params[0]->getType());
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isOptional());
}

// Test 4: Multiple nullable parameters
TEST_F(NullableTypeParsingTest, MultipleNullableParameters) {
    std::string code = R"(
        func multiParam(a: int64?, b: string?, c: double?) {
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto& params = funcDecl->getParameters();
    EXPECT_TRUE(dynamic_cast<const OptionalType*>(&params[0]->getType())->isOptional());
    EXPECT_TRUE(dynamic_cast<const OptionalType*>(&params[1]->getType())->isOptional());
    EXPECT_TRUE(dynamic_cast<const OptionalType*>(&params[2]->getType())->isOptional());
}

// Test 5: Non-nullable type
TEST_F(NullableTypeParsingTest, NonNullableTypeInParameter) {
    std::string code = R"(
        func nonNull(value: int64) {
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto& params = funcDecl->getParameters();
    
    // In the new buildType implementation, non-optional types are NOT wrapped in OptionalType
    auto* optType = dynamic_cast<const OptionalType*>(&params[0]->getType());
    if (optType) {
        EXPECT_FALSE(optType->isOptional());
    } else {
        // It should be a BaseType
        auto* baseType = dynamic_cast<const BaseType*>(&params[0]->getType());
        ASSERT_NE(baseType, nullptr);
    }
}

// Test 6: Nullable array with multiple dimensions
TEST_F(NullableTypeParsingTest, NullableMultiDimensionalArray) {
    std::string code = R"(
        func matrix(data: int64[][]?) {
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto& params = funcDecl->getParameters();
    auto* type = dynamic_cast<const OptionalType*>(&params[0]->getType());
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isOptional());
    EXPECT_EQ(type->getArrayType().getDimensionCount(), 2);
}

// Test 7: Function return type as nullable
TEST_F(NullableTypeParsingTest, NullableReturnType) {
    std::string code = R"(
        func:int64? getValue() {
            return 42;
        }
    )";

    auto* parseTree = parseCode(code);
    auto* ctx = getCompilationUnit(parseTree);
    auto ast = astBuilder->buildAST(ctx);
    auto* funcDecl = getFunctionDeclaration(*ast);

    auto* type = dynamic_cast<const OptionalType*>(funcDecl->getReturnType());
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isOptional());
}
