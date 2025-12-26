#include <gtest/gtest.h>
#include "SimpleASTBuilder.h"
#include "ProcessIsolatedParser.h"
#include "ast/AST.h"
#include <memory>

using namespace hooc;
using namespace hooc::ast;

// Helper class to parse and build AST for testing
class ArrayLiteralParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;

    std::unique_ptr<CompilationUnit> parseCode(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) {
            return nullptr;
        }

        return astBuilder->buildAST(parseTree);
    }

    // Helper to get the first function from compilation unit
    const FunctionDeclaration* getFirstFunction(const CompilationUnit* unit) {
        if (!unit || unit->getDeclarations().empty()) {
            return nullptr;
        }
        return dynamic_cast<const FunctionDeclaration*>(unit->getDeclarations()[0].get());
    }

    // Helper to get first statement from function body
    const Statement* getFirstStatement(const FunctionDeclaration* func) {
        if (!func || func->getBody().getStatements().empty()) {
            return nullptr;
        }
        return func->getBody().getStatements()[0].get();
    }
};

// Test 1: Simple integer array literal with type inference
TEST_F(ArrayLiteralParsingTest, SimpleIntegerArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var numbers = [1, 2, 3, 4, 5];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "test");

    // Get the variable declaration statement
    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();
    EXPECT_EQ(varDecl.getName(), "numbers");

    // Check that it has an initializer (array literal)
    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    // Should be an ArrayLiteral, wrapped in PrimaryExpression
    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    // Check elements
    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(5));
}

// Test 2: Array literal with explicit type annotation
TEST_F(ArrayLiteralParsingTest, ExplicitTypeArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var data: int64[] = [10, 20, 30];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();
    EXPECT_EQ(varDecl.getName(), "data");

    // Check explicit type
    auto* type = varDecl.getType();
    ASSERT_NE(type, nullptr);

    auto* arrayType = dynamic_cast<const ArrayType*>(type);
    ASSERT_NE(arrayType, nullptr);

    // Check that it's a 1D array (one dimension)
    EXPECT_EQ(arrayType->getDimensions().size(), static_cast<size_t>(1));

    // Check array literal
    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(3));
}

// Test 3: Empty array literal (requires explicit type)
TEST_F(ArrayLiteralParsingTest, EmptyArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var empty: int64[] = [];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    // Check array literal
    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExpr, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(0));  // Empty array
}

// Test 4: Floating-point array literal
TEST_F(ArrayLiteralParsingTest, FloatArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var floats = [1.0, 2.5, 3.14, 4.2];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(4));

    // Check that first element is a floating point literal
    auto* firstExpr = elements->getExpressions()[0].get();
    ASSERT_NE(firstExpr, nullptr);

    auto* primaryExpr = dynamic_cast<const PrimaryExpression*>(firstExpr);
    ASSERT_NE(primaryExpr, nullptr);

    auto* floatLit = dynamic_cast<const FloatingLiteral*>(&primaryExpr->getPrimary());
    ASSERT_NE(floatLit, nullptr);
}

// Test 5: Boolean array literal
TEST_F(ArrayLiteralParsingTest, BooleanArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var flags = [true, false, true, true, false];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(5));
}

// Test 6: Multi-dimensional array (nested array literals)
TEST_F(ArrayLiteralParsingTest, TwoDimensionalArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();
    EXPECT_EQ(varDecl.getName(), "matrix");

    // Outer array literal
    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* outerArray = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(outerArray, nullptr);

    auto* outerElements = outerArray->getElements();
    ASSERT_NE(outerElements, nullptr);
    EXPECT_EQ(outerElements->getExpressions().size(), static_cast<size_t>(3));  // 3 rows

    // Check first inner array
    auto* firstRowPrimary = dynamic_cast<const PrimaryExpression*>(outerElements->getExpressions()[0].get());
    ASSERT_NE(firstRowPrimary, nullptr);
    auto* firstRow = dynamic_cast<const ArrayLiteral*>(&firstRowPrimary->getPrimary());
    ASSERT_NE(firstRow, nullptr);

    auto* firstRowElements = firstRow->getElements();
    ASSERT_NE(firstRowElements, nullptr);
    EXPECT_EQ(firstRowElements->getExpressions().size(), static_cast<size_t>(3));  // 3 columns
}

// Test 7: Three-dimensional array
TEST_F(ArrayLiteralParsingTest, ThreeDimensionalArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* cubeArray = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(cubeArray, nullptr);

    auto* cubeElements = cubeArray->getElements();
    ASSERT_NE(cubeElements, nullptr);
    EXPECT_EQ(cubeElements->getExpressions().size(), static_cast<size_t>(2));
}

// Test 8: Array literal with expressions (not just literals)
TEST_F(ArrayLiteralParsingTest, ArrayLiteralWithExpressions) {
    std::string code = R"(
        func test() -> int64 {
            var computed = [1 + 2, 3 * 4, 5 - 1];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(3));

    // First element should be an additive expression (1 + 2)
    auto* firstExpr = elements->getExpressions()[0].get();
    ASSERT_NE(firstExpr, nullptr);

    auto* additiveExpr = dynamic_cast<const AdditiveExpression*>(firstExpr);
    ASSERT_NE(additiveExpr, nullptr);
}

// Test 9: Multiple array literals in same function
TEST_F(ArrayLiteralParsingTest, MultipleArrayLiterals) {
    std::string code = R"(
        func test() -> int64 {
            var arr1 = [1, 2, 3];
            var arr2 = [4, 5, 6];
            var arr3 = [7, 8, 9];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    const auto& body = func->getBody();

    // Should have 4 statements (3 var decls + 1 return)
    EXPECT_EQ(body.getStatements().size(), static_cast<size_t>(4));

    // Check each array declaration
    for (int i = 0; i < 3; i++) {
        auto* stmt = body.getStatements()[i].get();
        ASSERT_NE(stmt, nullptr);

        auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
        ASSERT_NE(varDeclStmt, nullptr);

        const auto& varDecl = varDeclStmt->getDeclaration();

        auto* initializer = varDecl.getInitializer();
        ASSERT_NE(initializer, nullptr);

        auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
        ASSERT_NE(primaryExprOuter, nullptr);

        auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
        ASSERT_NE(arrayLit, nullptr);

        auto* elements = arrayLit->getElements();
        ASSERT_NE(elements, nullptr);
        EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(3));
    }
}

// Test 10: Char array literal
TEST_F(ArrayLiteralParsingTest, CharArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var chars = ['a', 'b', 'c'];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(3));
}

// Test 11: Array type in function parameter (slice syntax)
TEST_F(ArrayLiteralParsingTest, ArrayTypeInFunctionParameter) {
    std::string code = R"(
        func process(int64[] arr) -> void {
            return;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "process");

    // Check parameter
    const auto& params = func->getParameters();
    ASSERT_EQ(params.size(), static_cast<size_t>(1));

    auto* param = params[0].get();
    ASSERT_NE(param, nullptr);
    EXPECT_EQ(param->getName(), "arr");

    // Parameter type should be ArrayType
    const auto& paramType = param->getType();

    auto* arrayType = dynamic_cast<const ArrayType*>(&paramType);
    ASSERT_NE(arrayType, nullptr);

    // Should be 1D array (slice syntax)
    EXPECT_EQ(arrayType->getDimensions().size(), static_cast<size_t>(1));

    // Dimension should be nullptr (unsized/slice)
    EXPECT_EQ(arrayType->getDimensions()[0], nullptr);
}

// Test 12: Large array literal
TEST_F(ArrayLiteralParsingTest, LargeArrayLiteral) {
    std::string code = R"(
        func test() -> int64 {
            var large = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
            return 0;
        }
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr);

    auto* func = getFirstFunction(ast.get());
    ASSERT_NE(func, nullptr);

    auto* stmt = getFirstStatement(func);
    ASSERT_NE(stmt, nullptr);

    auto* varDeclStmt = dynamic_cast<const VariableDeclarationStatement*>(stmt);
    ASSERT_NE(varDeclStmt, nullptr);

    const auto& varDecl = varDeclStmt->getDeclaration();

    auto* initializer = varDecl.getInitializer();
    ASSERT_NE(initializer, nullptr);

    auto* primaryExprOuter = dynamic_cast<const PrimaryExpression*>(initializer);
    ASSERT_NE(primaryExprOuter, nullptr);

    auto* arrayLit = dynamic_cast<const ArrayLiteral*>(&primaryExprOuter->getPrimary());
    ASSERT_NE(arrayLit, nullptr);

    auto* elements = arrayLit->getElements();
    ASSERT_NE(elements, nullptr);
    EXPECT_EQ(elements->getExpressions().size(), static_cast<size_t>(15));
}