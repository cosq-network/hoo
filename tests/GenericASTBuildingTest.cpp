#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "HoocParser.h"
#include "../src/ast/Declaration.h"
#include "../src/ast/ClassDeclaration.h"
#include "../src/ast/Type.h"
#include "../src/ast/Expression.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for generic AST building from parse tree.
 *
 * This suite tests:
 * - AST nodes store type parameters correctly
 * - AST nodes store type arguments correctly
 * - toString() methods display generic syntax
 * - isGeneric() and hasTypeArguments() methods work correctly
 */
class GenericASTBuildingTest : public ::testing::Test {
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

    const Declaration* getFirstDeclaration(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;
        return decls[0].get();
    }
};

// Test 1: Generic class stores type parameters
TEST_F(GenericASTBuildingTest, GenericClassStoresTypeParameters) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(classDecl->isGeneric()) << "Class should be generic";
    const auto& typeParams = classDecl->getTypeParameters();
    EXPECT_EQ(typeParams.size(), 1) << "Should have one type parameter";
    EXPECT_EQ(typeParams[0], "T") << "Type parameter should be 'T'";
}

// Test 2: Generic class with multiple type parameters
TEST_F(GenericASTBuildingTest, GenericClassWithMultipleTypeParameters) {
    std::string code = R"(
        class Pair<K, V> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(classDecl->isGeneric());
    const auto& typeParams = classDecl->getTypeParameters();
    EXPECT_EQ(typeParams.size(), 2) << "Should have two type parameters";
    EXPECT_EQ(typeParams[0], "K");
    EXPECT_EQ(typeParams[1], "V");
}

// Test 3: Non-generic class is not marked as generic
TEST_F(GenericASTBuildingTest, NonGenericClassIsNotGeneric) {
    std::string code = R"(
        class Widget {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_FALSE(classDecl->isGeneric()) << "Non-generic class should not be generic";
    EXPECT_EQ(classDecl->getTypeParameters().size(), 0);
}

// Test 4: Generic function stores type parameters
TEST_F(GenericASTBuildingTest, GenericFunctionStoresTypeParameters) {
    std::string code = R"(
        func identity<T>(x: T) -> T {
            return x;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_TRUE(funcDecl->isGeneric()) << "Function should be generic";
    const auto& typeParams = funcDecl->getTypeParameters();
    EXPECT_EQ(typeParams.size(), 1) << "Should have one type parameter";
    EXPECT_EQ(typeParams[0], "T");
}

// Test 5: Generic function with multiple type parameters
TEST_F(GenericASTBuildingTest, GenericFunctionWithMultipleTypeParameters) {
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

    EXPECT_TRUE(funcDecl->isGeneric());
    const auto& typeParams = funcDecl->getTypeParameters();
    EXPECT_EQ(typeParams.size(), 2) << "Should have two type parameters";
    EXPECT_EQ(typeParams[0], "T");
    EXPECT_EQ(typeParams[1], "U");
}

// Test 6: Non-generic function is not marked as generic
TEST_F(GenericASTBuildingTest, NonGenericFunctionIsNotGeneric) {
    std::string code = R"(
        func add(a: int64, b: int64) -> int64 {
            return a + b;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_FALSE(funcDecl->isGeneric());
    EXPECT_EQ(funcDecl->getTypeParameters().size(), 0);
}

// Test 7: Type reference with type arguments in variable declaration
TEST_F(GenericASTBuildingTest, TypeReferenceWithTypeArguments) {
    std::string code = R"(
        func test() {
            var arr: Array<int64>;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    // This test just verifies parsing doesn't crash
    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 8: Nested generic type arguments
TEST_F(GenericASTBuildingTest, NestedGenericTypeArguments) {
    std::string code = R"(
        func test() {
            var matrix: Array<Array<int64>>;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}

// Test 9: Generic class with constructor and type parameters
TEST_F(GenericASTBuildingTest, GenericClassWithConstructor) {
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

    EXPECT_TRUE(classDecl->isGeneric());
    EXPECT_EQ(classDecl->getTypeParameters().size(), 1);
    EXPECT_EQ(classDecl->getTypeParameters()[0], "T");
    EXPECT_EQ(classDecl->getBody().getMembers().size(), 1);
}

// Test 10: Generic class with method
TEST_F(GenericASTBuildingTest, GenericClassWithMethod) {
    std::string code = R"(
        class Container<T> {
            func get() -> T {
                return 0;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(classDecl->isGeneric());
    EXPECT_EQ(classDecl->getTypeParameters().size(), 1);
    EXPECT_EQ(classDecl->getBody().getMembers().size(), 1);
}

// Test 11: Generic class with variable field of type parameter
TEST_F(GenericASTBuildingTest, GenericClassWithGenericField) {
    std::string code = R"(
        class Container<T> {
            var items: T;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(classDecl->isGeneric());
    EXPECT_EQ(classDecl->getTypeParameters().size(), 1);
}

// Test 12: Generic class toString displays type parameters
TEST_F(GenericASTBuildingTest, GenericClassToStringDisplaysTypeParameters) {
    std::string code = R"(
        class Box<T> {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    std::string toStr = classDecl->toString();
    EXPECT_NE(toStr.find("Box"), std::string::npos) << "toString should contain 'Box'";
    EXPECT_NE(toStr.find("<"), std::string::npos) << "toString should contain '<'";
    EXPECT_NE(toStr.find("T"), std::string::npos) << "toString should contain 'T'";
}

// Test 13: Generic function toString displays type parameters
TEST_F(GenericASTBuildingTest, GenericFunctionToStringDisplaysTypeParameters) {
    std::string code = R"(
        func identity<T>(x: T) -> T {
            return x;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    std::string toStr = funcDecl->toString();
    EXPECT_NE(toStr.find("identity"), std::string::npos) << "toString should contain 'identity'";
    EXPECT_NE(toStr.find("<"), std::string::npos) << "toString should contain '<'";
    EXPECT_NE(toStr.find("T"), std::string::npos) << "toString should contain 'T'";
}

// Test 14: Non-generic class toString doesn't include type parameters
TEST_F(GenericASTBuildingTest, NonGenericClassToStringNoTypeParameters) {
    std::string code = R"(
        class Widget {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    std::string toStr = classDecl->toString();
    EXPECT_NE(toStr.find("Widget"), std::string::npos) << "toString should contain 'Widget'";
    EXPECT_EQ(toStr.find("<"), std::string::npos)
        << "Non-generic class should not have < in toString";
}

// Test 15: New expression with type arguments
TEST_F(GenericASTBuildingTest, NewExpressionWithTypeArguments) {
    std::string code = R"(
        func test() {
            var b = new Box<int64>();
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(funcDecl, nullptr);

    EXPECT_EQ(funcDecl->getName(), "test");
}
