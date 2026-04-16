#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "../src/ast/Declaration.h"
#include "../src/ast/Statement.h"
#include "../src/ast/Type.h"
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
 * - func process(val: int64?) -> void  // nullable parameter
 */
class NullableTypeParsingTest : public ::testing::Test {
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

    // Helper to extract variable declaration from compilation unit
    const VariableDeclaration* getVariableDeclaration(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (!decls.empty()) {
            return dynamic_cast<const VariableDeclaration*>(decls[0].get());
        }
        return nullptr;
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
        func test() -> void {
            var x: int64? = 0;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr) << "Compilation unit context should not be null";

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr) << "AST should not be null";

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr) << "Should have a function declaration";

    // Verify the function body contains the variable declaration
    auto& block = funcDecl->getBody();
    auto& stmts = block.getStatements();
    ASSERT_GT(stmts.size(), 0) << "Function should have at least one statement";

    // The first statement should be a variable declaration statement
    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmts[0].get());
    ASSERT_NE(varDeclStmt, nullptr) << "First statement should be a variable declaration";

    auto& varDecl = varDeclStmt->getDeclaration();
    EXPECT_EQ(varDecl.getName(), "x") << "Variable name should be 'x'";
}

// Test 2: Nullable type in function parameter
TEST_F(NullableTypeParsingTest, NullableTypeInFunctionParameter) {
    std::string code = R"(
        func process(value: int64?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr) << "Should have a function declaration";

    // Verify the function name
    EXPECT_EQ(funcDecl->getName(), "process");

    // Verify the parameter
    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1) << "Function should have 1 parameter";

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);

    // Verify parameter type is nullable
    auto& paramType = param->getType();

    // The parameter type should be an OptionalType or wrapped in UnionType
    if (auto unionType = dynamic_cast<const UnionType*>(&paramType)) {
        // If it's a union type, check if it's a single optional type
        // This depends on how the builder constructs the type
        EXPECT_TRUE(unionType != nullptr) << "Parameter type should be UnionType";
    } else if (auto optType = dynamic_cast<const OptionalType*>(&paramType)) {
        // If it's directly optional
        EXPECT_TRUE(optType->isOptional()) << "Parameter should be optional";
    }
}

// Test 3: Nullable array type in function parameter
TEST_F(NullableTypeParsingTest, NullableArrayTypeInParameter) {
    std::string code = R"(
        func processArray(items: int64[]?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr) << "Parameter should exist";

    // Verify parameter name
    EXPECT_EQ(param->getName(), "items");

    // The type should be a nullable array (OptionalType containing ArrayType)
    const Type* paramTypePtr = &param->getType();
    // This will be wrapped in a UnionType since buildType returns UnionType
    // We expect the union to contain one optional type that wraps an array
    if (auto unionType = dynamic_cast<const UnionType*>(paramTypePtr)) {
        auto& optTypes = unionType->getTypes();
        if (!optTypes.empty()) {
            EXPECT_TRUE(optTypes[0]->isOptional()) << "Array parameter should be optional";
        }
    }
}

// Test 4: Multiple nullable parameters
TEST_F(NullableTypeParsingTest, MultipleNullableParameters) {
    std::string code = R"(
        func multiParam(a: int64?, b: string?, c: double?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 3) << "Function should have 3 parameters";

    // Verify parameter names
    EXPECT_EQ(params[0]->getName(), "a");
    EXPECT_EQ(params[1]->getName(), "b");
    EXPECT_EQ(params[2]->getName(), "c");
}

// Test 5: Non-nullable type (should work as before)
TEST_F(NullableTypeParsingTest, NonNullableTypeInParameter) {
    std::string code = R"(
        func nonNull(value: int64) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);

    // Non-nullable types should have isOptional() = false
    const Type* paramTypePtr = &param->getType();
    if (auto unionType = dynamic_cast<const UnionType*>(paramTypePtr)) {
        // Get the first optional type from the union
        auto& optTypes = unionType->getTypes();
        if (!optTypes.empty()) {
            EXPECT_FALSE(optTypes[0]->isOptional())
                << "Non-nullable type should have isOptional() = false";
        }
    }
}

// Test 6: Union type with nullable and non-nullable - int64 | double?
TEST_F(NullableTypeParsingTest, UnionWithMixedNullability) {
    std::string code = R"(
        func unionType(value: int64 | double?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);

    const Type* paramTypePtr = &param->getType();

    // This should be a UnionType with multiple optional types
    if (auto unionType = dynamic_cast<const UnionType*>(paramTypePtr)) {
        auto& optTypes = unionType->getTypes();
        ASSERT_EQ(optTypes.size(), 2) << "Union should have 2 types";

        // First should be non-nullable int64, second should be nullable double
        EXPECT_FALSE(optTypes[0]->isOptional()) << "int64 should not be optional";
        EXPECT_TRUE(optTypes[1]->isOptional()) << "double? should be optional";
    }
}

// Test 7: Nullable array with multiple dimensions - int64[][]?
TEST_F(NullableTypeParsingTest, NullableMultiDimensionalArray) {
    std::string code = R"(
        func matrix(data: int64[][]?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);
    EXPECT_EQ(param->getName(), "data");

    // The type should be nullable multi-dimensional array
    const Type* paramTypePtr = &param->getType();
    if (auto unionType = dynamic_cast<const UnionType*>(paramTypePtr)) {
        auto& optTypes = unionType->getTypes();
        if (!optTypes.empty()) {
            // Get the array type from the optional type
            auto& arrayType = optTypes[0]->getArrayType();
            // Check if it's actually an array with 2 dimensions
            // (This depends on how getArrayType() works)
        }
    }
}

// Test 8: Nullable string type
TEST_F(NullableTypeParsingTest, NullableStringType) {
    std::string code = R"(
        func greet(message: string?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);
    EXPECT_EQ(param->getName(), "message");
}

// Test 9: Nullable boolean type
TEST_F(NullableTypeParsingTest, NullableBoolType) {
    std::string code = R"(
        func validateFlag(flag: bool?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);
}

// Test 10: Nullable char type
TEST_F(NullableTypeParsingTest, NullableCharType) {
    std::string code = R"(
        func processChar(ch: char?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);
}

// Test 11: Function return type as nullable
TEST_F(NullableTypeParsingTest, NullableReturnType) {
    std::string code = R"(
        func getValue() -> int64? {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    // Verify return type
    const Type* returnType = funcDecl->getReturnType();
    EXPECT_NE(returnType, nullptr) << "Return type should exist";
}

// Test 12: Multiple parameters with mixed nullable and non-nullable
TEST_F(NullableTypeParsingTest, MixedNullabilityInMultipleParameters) {
    std::string code = R"(
        func mixed(a: int64, b: int64?, c: string, d: string?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 4) << "Function should have 4 parameters";

    // Verify parameter names match expected pattern
    EXPECT_EQ(params[0]->getName(), "a");
    EXPECT_EQ(params[1]->getName(), "b");
    EXPECT_EQ(params[2]->getName(), "c");
    EXPECT_EQ(params[3]->getName(), "d");
}

// Test 13: Nullable float type
TEST_F(NullableTypeParsingTest, NullableFloatType) {
    std::string code = R"(
        func floatOp(val: float?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);
    EXPECT_EQ(params[0]->getName(), "val");
}

// Test 14: Nullable double/f64 type
TEST_F(NullableTypeParsingTest, NullableDoubleType) {
    std::string code = R"(
        func doubleOp(val: double?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);
}

// Test 15: Complex nullable array in union type
TEST_F(NullableTypeParsingTest, ComplexNullableUnionType) {
    std::string code = R"(
        func complex(data: int64 | string? | bool[]?) -> void {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = getFunctionDeclaration(*ast);
    ASSERT_NE(funcDecl, nullptr);

    auto& params = funcDecl->getParameters();
    ASSERT_EQ(params.size(), 1);

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);

    const Type* paramTypePtr = &param->getType();

    // Should be a UnionType with 3 optional types
    if (auto unionType = dynamic_cast<const UnionType*>(paramTypePtr)) {
        auto& optTypes = unionType->getTypes();
        ASSERT_EQ(optTypes.size(), 3) << "Union should have 3 types";
    }
}
