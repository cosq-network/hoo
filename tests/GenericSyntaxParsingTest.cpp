#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"
#include "../src/ast/Declaration.h"
#include "../src/ast/ClassDeclaration.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for generic syntax parsing in the grammar.
 *
 * This suite tests:
 * - Generic class declarations with type parameters
 * - Generic function declarations with type parameters
 * - Type arguments in type references
 * - Type arguments in new expressions
 * - Multiple type parameters
 * - Nested generics
 */
class GenericSyntaxParsingTest : public ::testing::Test {
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

// Test 1: Simple generic class with single type parameter
TEST_F(GenericSyntaxParsingTest, SimpleGenericClass) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr) << "Compilation unit context should not be null";

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr) << "Declaration should exist";

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(decl);
    ASSERT_NE(classDecl, nullptr) << "Declaration should be a ClassDeclaration";

    EXPECT_EQ(classDecl->getName(), "Box");
}

// Test 2: Generic class with multiple type parameters
TEST_F(GenericSyntaxParsingTest, GenericClassWithMultipleTypeParameters) {
    std::string code = R"(
        class Pair<K, V> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr) << "Compilation unit context should not be null";

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr) << "Declaration should exist";

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(decl);
    ASSERT_NE(classDecl, nullptr) << "Declaration should be a ClassDeclaration";

    EXPECT_EQ(classDecl->getName(), "Pair");
}

// Test 3: Generic class with constructor
TEST_F(GenericSyntaxParsingTest, GenericClassWithConstructor) {
    std::string code = R"(
        class Box<T> {
            constructor() {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Box");
    EXPECT_EQ(classDecl->getBody().getMembers().size(), 1) << "Should have one member (constructor)";
}

// Test 4: Generic function declaration
TEST_F(GenericSyntaxParsingTest, GenericFunctionDeclaration) {
    std::string code = R"(
        func identity<T>(x: T) -> T {
            return x;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr) << "Compilation unit context should not be null";

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr) << "Declaration should exist";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(decl);
    ASSERT_NE(funcDecl, nullptr) << "Declaration should be a FunctionDeclaration";

    EXPECT_EQ(funcDecl->getName(), "identity");
}

// Test 5: Generic function with multiple type parameters
TEST_F(GenericSyntaxParsingTest, GenericFunctionWithMultipleTypeParameters) {
    std::string code = R"(
        func swap<T, U>(a: T, b: U) -> U {
            return b;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "swap");
}

// Test 6: Type arguments in new expression
TEST_F(GenericSyntaxParsingTest, NewExpressionWithTypeArguments) {
    std::string code = R"(
        func test() {
            var b = new Box<int64>();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 7: Type arguments in variable declaration (type reference)
TEST_F(GenericSyntaxParsingTest, TypeArgumentsInVariableDeclaration) {
    std::string code = R"(
        func test() {
            var arr: Array<int64>;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 8: Nested generic type arguments
TEST_F(GenericSyntaxParsingTest, NestedGenericTypeArguments) {
    std::string code = R"(
        func test() {
            var matrix: Array<Array<int64>>;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 9: Multiple generic classes
TEST_F(GenericSyntaxParsingTest, MultipleGenericClasses) {
    std::string code = R"(
        class Box<T> {
        }

        class Pair<K, V> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto& decls = ast->getDeclarations();
    EXPECT_EQ(decls.size(), 2) << "Should have two declarations";

    auto* classDecl1 = dynamic_cast<const ClassDeclaration*>(decls[0].get());
    ASSERT_NE(classDecl1, nullptr);
    EXPECT_EQ(classDecl1->getName(), "Box");

    auto* classDecl2 = dynamic_cast<const ClassDeclaration*>(decls[1].get());
    ASSERT_NE(classDecl2, nullptr);
    EXPECT_EQ(classDecl2->getName(), "Pair");
}

// Test 10: Generic class instantiation with multiple type arguments
TEST_F(GenericSyntaxParsingTest, GenericClassInstantiationWithMultipleTypeArguments) {
    std::string code = R"(
        func test() {
            var p = new Pair<string, int64>();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 11: Generic class with inheritance using type arguments
TEST_F(GenericSyntaxParsingTest, GenericClassWithInheritance) {
    std::string code = R"(
        class ArrayList<T> extends List<T> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "ArrayList");
}

// Test 12: Generic method in class
TEST_F(GenericSyntaxParsingTest, GenericMethodInClass) {
    std::string code = R"(
        class Container<T> {
            func transform<U>(item: T) -> U {
                return item;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Container");
    EXPECT_EQ(classDecl->getBody().getMembers().size(), 1) << "Should have one method";
}

// Test 13: Generic function call syntax (postfix)
TEST_F(GenericSyntaxParsingTest, GenericFunctionCallSyntax) {
    std::string code = R"(
        func test() {
            var x = identity<int64>(42);
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 14: Generic class with member of generic type
TEST_F(GenericSyntaxParsingTest, GenericClassWithGenericMember) {
    std::string code = R"(
        class Container<T> {
            var items: Array<T>;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Container");
}

// Test 15: Complex nested generics with multiple instantiation levels
TEST_F(GenericSyntaxParsingTest, ComplexNestedGenerics) {
    std::string code = R"(
        func test() {
            var nested = new Container<Array<Dictionary<string, int64>>>();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr) << "Parse tree should not be null";

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr) << "AST should be built successfully";

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}
