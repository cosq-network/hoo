#include "gtest/gtest.h"

// Project-specific headers
#include "src/ast/SimpleASTBuilder.h"
#include "src/ast/AST.h" // Includes CompilationUnit.h, Declaration.h, VariableDeclaration.h, Expression.h, Type.h, etc.
#include "src/ast/Statement.h" // Includes VariableDeclarationStatement.h
#include "src/ast/Primary.h" // Includes Identifier.h

// ANTLR4 runtime headers for parsing
#include "antlr4-runtime.h"
#include "HoocLexer.h"
#include "HoocParser.h"

#include <string>
#include <vector>

// Helper function to parse a code string into an AST CompilationUnit
std::unique_ptr<hooc::ast::CompilationUnit> parseCode(const std::string& code) {
    using namespace hooc; // Add using namespace hooc here
    antlr4::ANTLRInputStream input(code);
    HoocLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    HoocParser parser(&tokens);

    // Clear default error listeners to avoid cluttering test output unless debugging
    // lexer.removeErrorListeners();
    // parser.removeErrorListeners();

    // Get the root context for parsing
    HoocParser::CompilationUnitContext* compilationUnitCtx = parser.compilationUnit();

    // Build the AST using SimpleASTBuilder
    SimpleASTBuilder builder;
    return builder.buildAST(compilationUnitCtx);
}

// Test fixture for module-level variable parsing tests
class ModuleLevelVariableParsingTest : public ::testing::Test {
protected:
    // You can add setup/teardown logic here if needed
};

// Test case for a simple module-level variable declaration with type inference
TEST_F(ModuleLevelVariableParsingTest, SimpleDeclarationInferredType) {
    std::string code = R"(
        import std; // Example import to ensure it's handled
        var module_var = 10
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    
    // Check for imports (optional, but good to ensure parsing works)
    ASSERT_EQ(ast->getImports().size(), 1);
    const hooc::ast::BasicImport* basicImport = dynamic_cast<const hooc::ast::BasicImport*>(ast->getImports()[0].get());
    ASSERT_NE(basicImport, nullptr) << "Expected BasicImport type.";
    ASSERT_NE(basicImport->getModule(), nullptr) << "Import module path should not be null.";
    ASSERT_EQ(basicImport->getModule()->getComponents().size(), 1);
    EXPECT_EQ(basicImport->getModule()->getComponents()[0], "std");

    // Check for declarations
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    // Verify the declaration is a VariableDeclaration
    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    // Verify variable details
    EXPECT_EQ(varDecl->getName(), "module_var");
    EXPECT_TRUE(varDecl->hasTypeInference()) << "Variable should have type inference enabled.";
    EXPECT_EQ(varDecl->getType(), nullptr) << "Type should be null for inferred types.";
    ASSERT_NE(varDecl->getInitializer(), nullptr) << "Variable should have an initializer.";

    // Verify initializer is an integer literal
    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::IntegerLiteral* intLiteral = dynamic_cast<const hooc::ast::IntegerLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 10);
}

// Test case for a module-level variable declaration with explicit type
TEST_F(ModuleLevelVariableParsingTest, ExplicitTypeDeclaration) {
    std::string code = R"(
        var count: int64 = 123
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "count");
    EXPECT_FALSE(varDecl->hasTypeInference()) << "Variable should have explicit type.";
    ASSERT_NE(varDecl->getType(), nullptr) << "Variable should have a type specified.";
    // Basic check for type name (requires Type::toString() to be implemented or direct comparison)
    // For now, assuming simple string check is sufficient or add a way to check type details.
    // For a robust check, we'd need to access the underlying BaseType and its name.
    // Example: EXPECT_EQ(varDecl->getType()->toString(), "int64"); 
    // For now, we'll just check it's not null.

    ASSERT_NE(varDecl->getInitializer(), nullptr) << "Variable should have an initializer.";
    // Verify initializer is an integer literal
    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::IntegerLiteral* intLiteral = dynamic_cast<const hooc::ast::IntegerLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(intLiteral, nullptr);
    EXPECT_EQ(intLiteral->getValue(), 123);
}

// Test case for a module-level variable declaration with explicit type and no initializer
TEST_F(ModuleLevelVariableParsingTest, ExplicitTypeNoInitializer) {
    std::string code = R"(
        var active: bool
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "active");
    EXPECT_FALSE(varDecl->hasTypeInference()) << "Variable should have explicit type.";
    ASSERT_NE(varDecl->getType(), nullptr) << "Variable should have a type specified.";
    
    ASSERT_EQ(varDecl->getInitializer(), nullptr) << "Variable should not have an initializer.";
}

// Test case for multiple module-level variable declarations
TEST_F(ModuleLevelVariableParsingTest, MultipleDeclarations) {
    std::string code = R"(
        var module_pi = 3.14
        var message: string = "hello"
        var flag: bool
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 3) << "Expected three top-level declarations.";

    // First declaration: module_pi = 3.14
    const auto& decl1 = ast->getDeclarations()[0];
    ASSERT_NE(decl1, nullptr);
    const hooc::ast::VariableDeclaration* varDecl1 = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl1.get());
    ASSERT_NE(varDecl1, nullptr);
    EXPECT_EQ(varDecl1->getName(), "module_pi");
    EXPECT_TRUE(varDecl1->hasTypeInference());
    ASSERT_NE(varDecl1->getInitializer(), nullptr);
    const hooc::ast::FloatingLiteral* floatLiteral = dynamic_cast<const hooc::ast::FloatingLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl1->getInitializer())->getPrimary());
    ASSERT_NE(floatLiteral, nullptr);
    EXPECT_DOUBLE_EQ(floatLiteral->getValue(), 3.14);

    // Second declaration: message: string = "hello"
    const auto& decl2 = ast->getDeclarations()[1];
    ASSERT_NE(decl2, nullptr);
    const hooc::ast::VariableDeclaration* varDecl2 = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl2.get());
    ASSERT_NE(varDecl2, nullptr);
    EXPECT_EQ(varDecl2->getName(), "message");
    EXPECT_FALSE(varDecl2->hasTypeInference());
    ASSERT_NE(varDecl2->getType(), nullptr);
    ASSERT_NE(varDecl2->getInitializer(), nullptr);
    const hooc::ast::StringLiteral* stringLiteral = dynamic_cast<const hooc::ast::StringLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl2->getInitializer())->getPrimary());
    ASSERT_NE(stringLiteral, nullptr);
    EXPECT_EQ(stringLiteral->getValue(), "hello");

    // Third declaration: flag: bool;
    const auto& decl3 = ast->getDeclarations()[2];
    ASSERT_NE(decl3, nullptr);
    const hooc::ast::VariableDeclaration* varDecl3 = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl3.get());
    ASSERT_NE(varDecl3, nullptr);
    EXPECT_EQ(varDecl3->getName(), "flag");
    EXPECT_FALSE(varDecl3->hasTypeInference());
    ASSERT_NE(varDecl3->getType(), nullptr);
    ASSERT_EQ(varDecl3->getInitializer(), nullptr);
}

// Test case for module-level variable declaration mixed with a function
TEST_F(ModuleLevelVariableParsingTest, MixedDeclarations) {
    std::string code = R"(
        var global_value = 100

        func:int64 add(a: int64, b: int64) {
            return a + b;
        }

        var another_global: float = 2.718
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 3) << "Expected three top-level declarations.";

    // Verify first declaration is module_value
    const auto& decl1 = ast->getDeclarations()[0];
    ASSERT_NE(decl1, nullptr);
    ASSERT_NE(dynamic_cast<const hooc::ast::VariableDeclaration*>(decl1.get()), nullptr);
    const auto& varDecl1 = static_cast<const hooc::ast::VariableDeclaration&>(*decl1);
    EXPECT_EQ(varDecl1.getName(), "global_value");
    EXPECT_TRUE(varDecl1.hasTypeInference());
    ASSERT_NE(varDecl1.getInitializer(), nullptr);

    // Verify second declaration is a function
    const auto& decl2 = ast->getDeclarations()[1];
    ASSERT_NE(decl2, nullptr);
    ASSERT_NE(dynamic_cast<const hooc::ast::FunctionDeclaration*>(decl2.get()), nullptr);
    const auto& funcDecl = static_cast<const hooc::ast::FunctionDeclaration&>(*decl2);
    EXPECT_EQ(funcDecl.getName(), "add");

    // Verify third declaration is another module_value
    const auto& decl3 = ast->getDeclarations()[2];
    ASSERT_NE(decl3, nullptr);
    ASSERT_NE(dynamic_cast<const hooc::ast::VariableDeclaration*>(decl3.get()), nullptr);
    const auto& varDecl3 = static_cast<const hooc::ast::VariableDeclaration*>(decl3.get());
    EXPECT_EQ(varDecl3->getName(), "another_global");
    EXPECT_FALSE(varDecl3->hasTypeInference());
    ASSERT_NE(varDecl3->getType(), nullptr);
    ASSERT_NE(varDecl3->getInitializer(), nullptr);
}

// Test case for module-level array literal with inferred type
TEST_F(ModuleLevelVariableParsingTest, ArrayLiteralInferredType) {
    std::string code = R"(
        var intArray = [1, 2, 3]
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "intArray");
    EXPECT_TRUE(varDecl->hasTypeInference());
    ASSERT_NE(varDecl->getInitializer(), nullptr);

    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::ArrayLiteral* arrayLiteral = dynamic_cast<const hooc::ast::ArrayLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(arrayLiteral, nullptr);

    ASSERT_NE(arrayLiteral->getElements(), nullptr);
    ASSERT_EQ(arrayLiteral->getElements()->getExpressions().size(), 3);

    const hooc::ast::IntegerLiteral* elem1 = dynamic_cast<const hooc::ast::IntegerLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(arrayLiteral->getElements()->getExpressions()[0].get())->getPrimary());
    ASSERT_NE(elem1, nullptr);
    EXPECT_EQ(elem1->getValue(), 1);
}

// Test case for module-level array literal with explicit type
TEST_F(ModuleLevelVariableParsingTest, ArrayLiteralExplicitType) {
    std::string code = R"(
        var stringArray: string[] = ["hello", "world"]
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "stringArray");
    EXPECT_FALSE(varDecl->hasTypeInference());
    ASSERT_NE(varDecl->getType(), nullptr);
    const hooc::ast::ArrayType* arrayType = dynamic_cast<const hooc::ast::ArrayType*>(varDecl->getType());
    ASSERT_NE(arrayType, nullptr);
    const hooc::ast::BaseType* baseType = dynamic_cast<const hooc::ast::BaseType*>(&arrayType->getBaseType());
    ASSERT_NE(baseType, nullptr);
    ASSERT_TRUE(baseType->isPrimitive());
    EXPECT_EQ(baseType->getPrimitiveType()->getKind(), hooc::ast::PrimitiveTypeKind::STRING);
    ASSERT_EQ(arrayType->getDimensions().size(), 1); // 1D array

    ASSERT_NE(varDecl->getInitializer(), nullptr);
    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::ArrayLiteral* arrayLiteral = dynamic_cast<const hooc::ast::ArrayLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(arrayLiteral, nullptr);

    ASSERT_NE(arrayLiteral->getElements(), nullptr);
    ASSERT_EQ(arrayLiteral->getElements()->getExpressions().size(), 2);

    const hooc::ast::StringLiteral* elem1 = dynamic_cast<const hooc::ast::StringLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(arrayLiteral->getElements()->getExpressions()[0].get())->getPrimary());
    ASSERT_NE(elem1, nullptr);
    EXPECT_EQ(elem1->getValue(), "hello");
}

// Test case for module-level multi-dimensional array literal
TEST_F(ModuleLevelVariableParsingTest, MultiDimensionalArrayLiteral) {
    std::string code = R"(
        var matrix = [[1, 2], [3, 4]]
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "matrix");
    EXPECT_TRUE(varDecl->hasTypeInference());
    ASSERT_NE(varDecl->getInitializer(), nullptr);

    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::ArrayLiteral* arrayLiteral = dynamic_cast<const hooc::ast::ArrayLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(arrayLiteral, nullptr);

    ASSERT_NE(arrayLiteral->getElements(), nullptr);
    ASSERT_EQ(arrayLiteral->getElements()->getExpressions().size(), 2); // Two inner arrays

    // Check first inner array
    const hooc::ast::ArrayLiteral* innerArray1 = dynamic_cast<const hooc::ast::ArrayLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(arrayLiteral->getElements()->getExpressions()[0].get())->getPrimary());
    ASSERT_NE(innerArray1, nullptr);
    ASSERT_NE(innerArray1->getElements(), nullptr);
    ASSERT_EQ(innerArray1->getElements()->getExpressions().size(), 2);
    const hooc::ast::IntegerLiteral* elem1_1 = dynamic_cast<const hooc::ast::IntegerLiteral*>(&dynamic_cast<const hooc::ast::PrimaryExpression*>(innerArray1->getElements()->getExpressions()[0].get())->getPrimary());
    ASSERT_NE(elem1_1, nullptr);
    EXPECT_EQ(elem1_1->getValue(), 1);
}

// Test case for module-level array literal with expressions as elements
TEST_F(ModuleLevelVariableParsingTest, ArrayLiteralWithExpressions) {
    std::string code = R"(
        var exprArray = [1 + 1, 2 * 2]
    )";

    auto ast = parseCode(code);
    ASSERT_NE(ast, nullptr) << "AST should not be null.";
    ASSERT_EQ(ast->getDeclarations().size(), 1) << "Expected one top-level declaration.";

    const auto& decl = ast->getDeclarations()[0];
    ASSERT_NE(decl, nullptr);
    const hooc::ast::VariableDeclaration* varDecl = dynamic_cast<const hooc::ast::VariableDeclaration*>(decl.get());
    ASSERT_NE(varDecl, nullptr) << "Declaration should be a VariableDeclaration.";

    EXPECT_EQ(varDecl->getName(), "exprArray");
    EXPECT_TRUE(varDecl->hasTypeInference());
    ASSERT_NE(varDecl->getInitializer(), nullptr);

    const hooc::ast::PrimaryExpression* initializerPrimary = dynamic_cast<const hooc::ast::PrimaryExpression*>(varDecl->getInitializer());
    ASSERT_NE(initializerPrimary, nullptr);
    const hooc::ast::ArrayLiteral* arrayLiteral = dynamic_cast<const hooc::ast::ArrayLiteral*>(&initializerPrimary->getPrimary());
    ASSERT_NE(arrayLiteral, nullptr);

    ASSERT_NE(arrayLiteral->getElements(), nullptr);
    ASSERT_EQ(arrayLiteral->getElements()->getExpressions().size(), 2);

    // Check first element (1 + 1)
    const hooc::ast::AdditiveExpression* addExpr = dynamic_cast<const hooc::ast::AdditiveExpression*>(arrayLiteral->getElements()->getExpressions()[0].get());
    ASSERT_NE(addExpr, nullptr);
    EXPECT_EQ(addExpr->getOperator(), hooc::ast::BinaryOperator::PLUS);

    // Check second element (2 * 2)
    const hooc::ast::MultiplicativeExpression* mulExpr = dynamic_cast<const hooc::ast::MultiplicativeExpression*>(arrayLiteral->getElements()->getExpressions()[1].get());
    ASSERT_NE(mulExpr, nullptr);
    EXPECT_EQ(mulExpr->getOperator(), hooc::ast::BinaryOperator::MULTIPLY);
}