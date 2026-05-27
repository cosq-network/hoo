#include <gtest/gtest.h>
#include <memory>
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"
#include "src/ast/Declaration.h"
#include "src/ast/ClassDeclaration.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for class declaration parsing in SimpleASTBuilder.
 *
 * This suite tests:
 * - Class declarations with various modifiers
 * - Primary constructors
 * - Base class inheritance (extends)
 * - Class members (functions, events)
 */
class ClassDeclarationParsingTest : public ::testing::Test {
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

    // Helper to extract first declaration from compilation unit
    const Declaration* getFirstDeclaration(const CompilationUnit& ast) {
        auto& decls = ast.getDeclarations();
        if (decls.empty()) return nullptr;
        return decls[0].get();
    }
};

// Test 1: Simple class with no modifiers or members
TEST_F(ClassDeclarationParsingTest, SimpleClassDeclaration) {
    std::string code = R"(
        class MyClass {
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

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(decl);
    ASSERT_NE(classDecl, nullptr) << "Declaration should be a ClassDeclaration";

    // Verify class name
    EXPECT_EQ(classDecl->getName(), "MyClass");

    // Verify no modifiers
    EXPECT_TRUE(classDecl->getModifiers().empty());

    // Verify no base class
    EXPECT_FALSE(classDecl->hasBaseClass());

    // Verify empty body (no constructor or other members)
    EXPECT_TRUE(classDecl->getBody().getMembers().empty());
}

// Test 2: Class with single modifier (SINGLETON)
TEST_F(ClassDeclarationParsingTest, ClassWithSingletonModifier) {
    std::string code = R"(
        singleton class Config {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify class name
    EXPECT_EQ(classDecl->getName(), "Config");

    // Verify singleton modifier
    EXPECT_EQ(classDecl->getModifiers().size(), 1);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SINGLETON));
}

// Test 3: Class with multiple modifiers
TEST_F(ClassDeclarationParsingTest, ClassWithMultipleModifiers) {
    std::string code = R"(
        immutable final class DataModel {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify modifiers
    EXPECT_EQ(classDecl->getModifiers().size(), 2);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
}

// Test 4: Class with empty constructor
TEST_F(ClassDeclarationParsingTest, ClassWithEmptyConstructor) {
    std::string code = R"(
        class Point {
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

    // Verify constructor exists with no parameters in the class body
    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);
    EXPECT_TRUE(members[0]->isConstructor());

    auto* constructor = members[0]->getConstructor();
    ASSERT_NE(constructor, nullptr);
    EXPECT_TRUE(constructor->getParameters().empty());
}

// Test 5: Class with constructor with parameters
TEST_F(ClassDeclarationParsingTest, ClassWithConstructorParameters) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify constructor parameters in the class body
    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);
    EXPECT_TRUE(members[0]->isConstructor());

    auto* constructor = members[0]->getConstructor();
    ASSERT_NE(constructor, nullptr);
    EXPECT_EQ(constructor->getParameters().size(), 2);
    EXPECT_EQ(constructor->getParameters()[0]->getName(), "x");
    EXPECT_EQ(constructor->getParameters()[1]->getName(), "y");
}

// Test 6: Class with base class (extends)
TEST_F(ClassDeclarationParsingTest, ClassWithBaseClass) {
    std::string code = R"(
        class Child extends Parent {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify base class
    EXPECT_TRUE(classDecl->hasBaseClass());
    EXPECT_EQ(classDecl->getBaseClass(), "Parent");
}

// Test 9: Class with function member
TEST_F(ClassDeclarationParsingTest, ClassWithFunctionMember) {
    std::string code = R"(
        class Calculator {
            func:int64 add(a: int64, b: int64) {
                return a + b;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify function member
    auto& members = classDecl->getBody().getMembers();
    EXPECT_EQ(members.size(), 1);

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_EQ(funcDecl->getName(), "add");
}

// Tests for class modifiers

// Test 11: Class with mixed members (functions only)

// Test 16: Class with all modifiers
TEST_F(ClassDeclarationParsingTest, ClassWithAllModifiers) {
    std::string code = R"(
        singleton immutable service final class AllModifiers {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify all modifiers
    EXPECT_EQ(classDecl->getModifiers().size(), 4);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SINGLETON));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERVICE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
}

// Test 17: Complex class with all features
TEST_F(ClassDeclarationParsingTest, ComplexClassWithAllFeatures) {
    std::string code = R"(
        immutable class ComplexClass extends BaseClass {
            constructor(id: int64, name: int64) {
            }
            func method1() {
            }
            func:int64 method2(param: int64) {
                return param;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify class name
    EXPECT_EQ(classDecl->getName(), "ComplexClass");

    // Verify modifier
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));

    // Verify base class
    EXPECT_TRUE(classDecl->hasBaseClass());
    EXPECT_EQ(classDecl->getBaseClass(), "BaseClass");

    // Verify members (constructor + 2 methods = 3 members)
    auto& members = classDecl->getBody().getMembers();
    EXPECT_EQ(members.size(), 3);
    EXPECT_TRUE(members[0]->isConstructor());  // constructor

    // Verify constructor parameters
    auto* constructor = members[0]->getConstructor();
    ASSERT_NE(constructor, nullptr);
    EXPECT_EQ(constructor->getParameters().size(), 2);
}

// Tests to verify that multiple constructors are NOT allowed

TEST_F(ClassDeclarationParsingTest, ClassWithMultipleConstructorsShouldFail) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {
            }
            constructor(x: int64) {
            }
        }
    )";

    // This should fail when building AST
    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

TEST_F(ClassDeclarationParsingTest, ClassWithTwoEmptyConstructorsShouldFail) {
    std::string code = R"(
        class Point {
            constructor() {
            }
            constructor() {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

TEST_F(ClassDeclarationParsingTest, ClassWithThreeConstructorsShouldFail) {
    std::string code = R"(
        class Shape {
            constructor() {
            }
            constructor(x: int64) {
            }
            constructor(x: int64, y: int64) {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

TEST_F(ClassDeclarationParsingTest, ClassWithConstructorAndMethodsWithMultipleConstructorsShouldFail) {
    std::string code = R"(
        class User {
            constructor(name: int64) {
            }
            func greet() {
            }
            constructor(name: int64, age: int64) {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

TEST_F(ClassDeclarationParsingTest, SingletonClassWithMultipleConstructorsShouldFail) {
    std::string code = R"(
        singleton class Database {
            constructor(url: int64) {
            }
            constructor() {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

TEST_F(ClassDeclarationParsingTest, ImmutableClassWithMultipleConstructorsShouldFail) {
    std::string code = R"(
        immutable class Point {
            constructor(x: int64) {
            }
            constructor(x: int64, y: int64) {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    EXPECT_THROW({
        astBuilder->buildAST(ctx);
    }, std::runtime_error);
}

// Positive test: verify single constructor is allowed
TEST_F(ClassDeclarationParsingTest, ClassWithSingleConstructorShouldSucceed) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {
            }
            func:int64 getX() {
                return x;
            }
            func:int64 getY() {
                return y;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);

    auto* classDecl = dynamic_cast<ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);

    // Verify class has exactly one constructor
    auto& members = classDecl->getBody().getMembers();
    int constructorCount = 0;
    for (const auto& member : members) {
        if (member->isConstructor()) {
            constructorCount++;
        }
    }
    EXPECT_EQ(constructorCount, 1);
}

TEST_F(ClassDeclarationParsingTest, ClassWithNoConstructorShouldSucceed) {
    std::string code = R"(
        class Util {
            func:int64 helper() {
                return 42;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);
    auto* ctx = getCompilationUnit(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = astBuilder->buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);

    auto* classDecl = dynamic_cast<ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);

    // Verify class has no constructors
    auto& members = classDecl->getBody().getMembers();
    int constructorCount = 0;
    for (const auto& member : members) {
        if (member->isConstructor()) {
            constructorCount++;
        }
    }
    EXPECT_EQ(constructorCount, 0);
}

// Test 17: Class with public field
TEST_F(ClassDeclarationParsingTest, ClassWithPublicField) {
    std::string code = R"(
        class MyClass {
            public var x: int64;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);

    auto* declMember = members[0]->getDeclaration();
    ASSERT_NE(declMember, nullptr);

    auto* varDecl = dynamic_cast<const VariableDeclaration*>(declMember);
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->getName(), "x");
    EXPECT_TRUE(varDecl->isPublic());
    EXPECT_FALSE(varDecl->isPrivate());
}

// Test 18: Class with private field
TEST_F(ClassDeclarationParsingTest, ClassWithPrivateField) {
    std::string code = R"(
        class MyClass {
            private var x: int64;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);

    auto* declMember = members[0]->getDeclaration();
    ASSERT_NE(declMember, nullptr);

    auto* varDecl = dynamic_cast<const VariableDeclaration*>(declMember);
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->getName(), "x");
    EXPECT_FALSE(varDecl->isPublic());
    EXPECT_TRUE(varDecl->isPrivate());
}

// Test 19: Class with var field (no modifier)
TEST_F(ClassDeclarationParsingTest, ClassWithDefaultField) {
    std::string code = R"(
        class MyClass {
            var x: int64;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);

    auto* declMember = members[0]->getDeclaration();
    ASSERT_NE(declMember, nullptr);

    auto* varDecl = dynamic_cast<const VariableDeclaration*>(declMember);
    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->getName(), "x");
    EXPECT_FALSE(varDecl->isPublic());
    EXPECT_FALSE(varDecl->isPrivate());
}

// Test 20: Class with mixed private and public fields
TEST_F(ClassDeclarationParsingTest, ClassWithMixedFieldAccess) {
    std::string code = R"(
        class MyClass {
            public var name: string;
            private var ssn: int64;
            var email: string;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 3);

    auto* var0 = dynamic_cast<const VariableDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(var0, nullptr);
    EXPECT_EQ(var0->getName(), "name");
    EXPECT_TRUE(var0->isPublic());
    EXPECT_FALSE(var0->isPrivate());

    auto* var1 = dynamic_cast<const VariableDeclaration*>(members[1]->getDeclaration());
    ASSERT_NE(var1, nullptr);
    EXPECT_EQ(var1->getName(), "ssn");
    EXPECT_FALSE(var1->isPublic());
    EXPECT_TRUE(var1->isPrivate());

    auto* var2 = dynamic_cast<const VariableDeclaration*>(members[2]->getDeclaration());
    ASSERT_NE(var2, nullptr);
    EXPECT_EQ(var2->getName(), "email");
    EXPECT_FALSE(var2->isPublic());
    EXPECT_FALSE(var2->isPrivate());
}

// Test 21: Class with private inferred field
TEST_F(ClassDeclarationParsingTest, ClassWithPrivateInferredField) {
    std::string code = R"(
        class MyClass {
            private var data = 42;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 1);

    auto* varDecl = dynamic_cast<const VariableDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_TRUE(varDecl->isPrivate());
}
