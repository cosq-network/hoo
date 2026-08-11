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

TEST_F(ClassDeclarationParsingTest, EachClassModifierParsesToExpectedEnum) {
    struct ModifierCase {
        const char* sourceName;
        ClassModifier expectedModifier;
    };

    const ModifierCase cases[] = {
        {"singleton", ClassModifier::SINGLETON},
        {"immutable", ClassModifier::IMMUTABLE},
        {"service", ClassModifier::SERVICE},
        {"final", ClassModifier::FINAL},
        {"serializable", ClassModifier::SERIALIZABLE},
    };

    for (const auto& testCase : cases) {
        SCOPED_TRACE(testCase.sourceName);
        std::string code = std::string(testCase.sourceName) + R"( class ModifierTest {
        })";

        auto* parseTree = parseCode(code);
        ASSERT_NE(parseTree, nullptr);

        auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
        ASSERT_NE(ast, nullptr);

        auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
        ASSERT_NE(classDecl, nullptr);

        ASSERT_EQ(classDecl->getModifiers().size(), 1);
        EXPECT_EQ(classDecl->getModifiers()[0], testCase.expectedModifier);
        EXPECT_TRUE(classDecl->hasModifier(testCase.expectedModifier));
    }
}

TEST_F(ClassDeclarationParsingTest, ClassModifierSourceOrderIsPreserved) {
    std::string code = R"(
        service serializable singleton final immutable class OrderedModifiers {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    ASSERT_EQ(classDecl->getModifiers().size(), 5);
    EXPECT_EQ(classDecl->getModifiers()[0], ClassModifier::SERVICE);
    EXPECT_EQ(classDecl->getModifiers()[1], ClassModifier::SERIALIZABLE);
    EXPECT_EQ(classDecl->getModifiers()[2], ClassModifier::SINGLETON);
    EXPECT_EQ(classDecl->getModifiers()[3], ClassModifier::FINAL);
    EXPECT_EQ(classDecl->getModifiers()[4], ClassModifier::IMMUTABLE);
}

TEST_F(ClassDeclarationParsingTest, ClassModifiersPreservedWithInheritanceConstructorAndMembers) {
    std::string code = R"(
        final serializable class DerivedConfig extends BaseConfig {
            public var name: string;
            constructor() {
            }
            public func:string serialize() {
                return name;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "DerivedConfig");
    EXPECT_TRUE(classDecl->hasBaseClass());
    EXPECT_EQ(classDecl->getBaseClass(), "BaseConfig");

    ASSERT_EQ(classDecl->getModifiers().size(), 2);
    EXPECT_EQ(classDecl->getModifiers()[0], ClassModifier::FINAL);
    EXPECT_EQ(classDecl->getModifiers()[1], ClassModifier::SERIALIZABLE);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));

    const auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 3);
    EXPECT_FALSE(members[0]->isConstructor());
    EXPECT_TRUE(members[1]->isConstructor());
    EXPECT_FALSE(members[2]->isConstructor());
}

// Test 11: Class with mixed members (functions only)

// Test 16: Class with all modifiers (including SERIALIZABLE)
TEST_F(ClassDeclarationParsingTest, ClassWithAllFiveModifiers) {
    std::string code = R"(
        singleton immutable service final serializable class AllModifiers {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify all five modifiers
    EXPECT_EQ(classDecl->getModifiers().size(), 5);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SINGLETON));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERVICE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
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

// Test 22: Class with SERIALIZABLE modifier
TEST_F(ClassDeclarationParsingTest, ClassWithSerializableModifier) {
    std::string code = R"(
        serializable class UserConfig {
            public var name: string;
            public var version: int64;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "UserConfig");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
    EXPECT_FALSE(classDecl->hasModifier(ClassModifier::SINGLETON));
}

// Test 23: Class with combined SERIALIZABLE and final modifiers
TEST_F(ClassDeclarationParsingTest, ClassWithSerializableAndFinal) {
    std::string code = R"(
        final serializable class FinalConfig {
            public var name: string;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "FinalConfig");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
}

// Test 24: Serializable class toString includes modifier
TEST_F(ClassDeclarationParsingTest, SerializableModifierToString) {
    EXPECT_EQ(classModifierToString(ClassModifier::SERIALIZABLE), "serializable");
}

// Test 25: All class modifiers toString
TEST_F(ClassDeclarationParsingTest, AllClassModifiersToString) {
    EXPECT_EQ(classModifierToString(ClassModifier::SINGLETON), "singleton");
    EXPECT_EQ(classModifierToString(ClassModifier::IMMUTABLE), "immutable");
    EXPECT_EQ(classModifierToString(ClassModifier::SERVICE), "service");
    EXPECT_EQ(classModifierToString(ClassModifier::FINAL), "final");
    EXPECT_EQ(classModifierToString(ClassModifier::SERIALIZABLE), "serializable");
}

// Test 26: Serializable class with HashMap field
TEST_F(ClassDeclarationParsingTest, SerializableClassWithHashMapField) {
    std::string code = R"(
        serializable class Config {
            public var labels: HashMap<int64, string>;
            public var version: int64;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Config");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));

    auto& members = classDecl->getBody().getMembers();
    ASSERT_GE(members.size(), 2);

    auto* field1 = dynamic_cast<const VariableDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(field1, nullptr);
    EXPECT_EQ(field1->getName(), "labels");
    EXPECT_TRUE(field1->isPublic());
}

// Test 27: Serializable class with AnyArray field
TEST_F(ClassDeclarationParsingTest, SerializableClassWithAnyArrayField) {
    std::string code = R"(
        serializable class Container {
            public var items: AnyArray;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Container");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
}

// Test 28: Serializable class with tensor field
TEST_F(ClassDeclarationParsingTest, SerializableClassWithTensorField) {
    std::string code = R"(
        serializable class TensorHolder {
            public var mat: tensor<double>[4, 4];
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "TensorHolder");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
}

// Test 29: Serializable class with buffer and bit fields
TEST_F(ClassDeclarationParsingTest, SerializableClassWithBufferAndBitFields) {
    std::string code = R"(
        serializable class Flags {
            public var active: bit;
            public var raw: buffer;
            public var name: string;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Flags");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));

    auto& members = classDecl->getBody().getMembers();
    ASSERT_GE(members.size(), 3);
}

// Test 30: Serializable class with constructor having parameters (parsing OK, codegen rejects)
TEST_F(ClassDeclarationParsingTest, SerializableClassWithParameterizedConstructorParsing) {
    std::string code = R"(
        serializable class ParamCtor {
            public var x: int64;
            constructor(x: int64) { this.x = x; }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "ParamCtor");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));

    // Parser should accept it; validation happens in codegen
    auto& members = classDecl->getBody().getMembers();
    // The class has 2 members: the var decl + the constructor
    ASSERT_EQ(members.size(), 2);
    EXPECT_TRUE(members[1]->isConstructor());
    auto* ctor = members[1]->getConstructor();
    ASSERT_NE(ctor, nullptr);
    EXPECT_EQ(ctor->getParameters().size(), 1);
}

// Test 31: Serializable class with only private field
TEST_F(ClassDeclarationParsingTest, SerializableClassWithOnlyPrivateFieldParsing) {
    std::string code = R"(
        serializable class PrivateOnly {
            private var x: int64;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));

    // Parser accepts it; codegen rejects "no public field"
    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 2);
    ASSERT_TRUE(members[0]->getDeclaration() != nullptr);
    auto* varDecl = dynamic_cast<const VariableDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_FALSE(varDecl->isPublic());
}

// NOTE: This test is disabled due to a pre-existing ANTLR4 runtime / libc++ std::unordered_map
// hash table issue (__next_prime overflow) during prediction/ATN evaluation for nested class field types.
TEST_F(ClassDeclarationParsingTest, SerializableClassWithNestedSerializableField) {
    std::string code = R"(
        serializable class Point {
            public var x: f64;
            public var y: f64;
            constructor() { x = 0.0; y = 0.0; }
        }

        serializable class Line {
            public var start: Point;
            public var end: Point;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    ASSERT_EQ(ast->getDeclarations().size(), 2);

    auto* pointDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(pointDecl, nullptr);
    EXPECT_EQ(pointDecl->getName(), "Point");
    EXPECT_TRUE(pointDecl->hasModifier(ClassModifier::SERIALIZABLE));

    auto* lineDecl = dynamic_cast<const ClassDeclaration*>(ast->getDeclarations()[1].get());
    ASSERT_NE(lineDecl, nullptr);
    EXPECT_EQ(lineDecl->getName(), "Line");
    EXPECT_TRUE(lineDecl->hasModifier(ClassModifier::SERIALIZABLE));
}

// Test 33: Class with SERIALIZABLE in different positions
TEST_F(ClassDeclarationParsingTest, SerializableModifierVariousPositions) {
    std::string code = R"(
        serializable final immutable class Multi {
            public var x: int64;
            constructor() {}
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    EXPECT_EQ(classDecl->getName(), "Multi");
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERIALIZABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));
    EXPECT_EQ(classDecl->getModifiers().size(), 3);
}

// Factory constructors
TEST_F(ClassDeclarationParsingTest, ClassWithFactoryConstructor) {
    std::string code = R"(
        class Point {
            constructor(x: f64, y: f64) {}
            factory origin() {
                return 1;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 2);

    // First member: generative constructor
    EXPECT_TRUE(members[0]->isConstructor());
    EXPECT_FALSE(members[0]->getConstructor()->isFactory());
    EXPECT_TRUE(members[0]->getConstructor()->getName().empty());

    // Second member: factory constructor
    EXPECT_TRUE(members[1]->isConstructor());
    auto* factory = members[1]->getConstructor();
    ASSERT_NE(factory, nullptr);
    EXPECT_TRUE(factory->isFactory());
    EXPECT_EQ(factory->getName(), "origin");
    EXPECT_TRUE(factory->getParameters().empty());
}

TEST_F(ClassDeclarationParsingTest, FactoryConstructorWithParameters) {
    std::string code = R"(
        class Point {
            constructor(x: f64) {}
            factory unitCircle(angle: f64) {
                return 1;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 2);

    auto* factory = members[1]->getConstructor();
    ASSERT_NE(factory, nullptr);
    EXPECT_TRUE(factory->isFactory());
    EXPECT_EQ(factory->getName(), "unitCircle");
    EXPECT_EQ(factory->getParameters().size(), 1);
    EXPECT_EQ(factory->getParameters()[0]->getName(), "angle");
}

TEST_F(ClassDeclarationParsingTest, MultipleFactoriesAllowed) {
    std::string code = R"(
        class Point {
            var x: f64;
            constructor(x: f64) {}
            factory origin() {
                return 1;
            }
            factory unitCircle(angle: f64) {
                return 1;
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    auto& members = classDecl->getBody().getMembers();
    ASSERT_EQ(members.size(), 4);
    EXPECT_FALSE(members[1]->getConstructor()->isFactory()); // generative constructor
    EXPECT_TRUE(members[2]->getConstructor()->isFactory());
    EXPECT_TRUE(members[3]->getConstructor()->isFactory());
}

TEST_F(ClassDeclarationParsingTest, DuplicateFactoryNamesShouldFail) {
    std::string code = R"(
        class Point {
            var x: f64;
            constructor(x: f64) {}
            factory origin() {
                return 1;
            }
            factory origin() {
                return 1;
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

TEST_F(ClassDeclarationParsingTest, MultipleGenerativeConstructorsWithFactoryShouldFail) {
    std::string code = R"(
        class Point {
            var x: f64;
            constructor(x: f64) {}
            constructor() {}
            factory origin() {
                return 1;
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
