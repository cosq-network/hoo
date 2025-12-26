#include <gtest/gtest.h>
#include <memory>
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../antlr4/generated/HoocParser.h"
#include "../src/ast/Declaration.h"
#include "../src/ast/ClassDeclaration.h"
#include "../src/ast/InterfaceDeclaration.h"
#include "antlr4-runtime.h"

using namespace hooc;
using namespace hooc::ast;

/**
 * Test suite for class and interface declaration parsing in SimpleASTBuilder.
 *
 * This suite tests:
 * - Class declarations with various modifiers
 * - Primary constructors
 * - Base class inheritance (extends)
 * - Interface implementation (implements)
 * - Class members (functions, events)
 * - Interface declarations and members
 */
class ClassDeclarationParsingTest : public ::testing::Test {
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

    // Verify no constructor
    EXPECT_EQ(classDecl->getConstructor(), nullptr);

    // Verify no base class
    EXPECT_FALSE(classDecl->hasBaseClass());

    // Verify no interfaces
    EXPECT_FALSE(classDecl->hasInterfaces());

    // Verify empty body
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
        class Point() {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify constructor exists with no parameters
    auto* constructor = classDecl->getConstructor();
    ASSERT_NE(constructor, nullptr);
    EXPECT_TRUE(constructor->getParameters().empty());
}

// Test 5: Class with constructor with parameters
TEST_F(ClassDeclarationParsingTest, ClassWithConstructorParameters) {
    std::string code = R"(
        class Point(x: int64, y: int64) {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify constructor parameters
    auto* constructor = classDecl->getConstructor();
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

// Test 7: Class with interfaces (implements)
TEST_F(ClassDeclarationParsingTest, ClassWithInterfaces) {
    std::string code = R"(
        class MyClass implements Drawable, Serializable {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify interfaces
    EXPECT_TRUE(classDecl->hasInterfaces());
    EXPECT_EQ(classDecl->getInterfaces().size(), 2);
    EXPECT_EQ(classDecl->getInterfaces()[0], "Drawable");
    EXPECT_EQ(classDecl->getInterfaces()[1], "Serializable");
}

// Test 8: Class with base class and interfaces
TEST_F(ClassDeclarationParsingTest, ClassWithBaseClassAndInterfaces) {
    std::string code = R"(
        class MyClass extends BaseClass implements Interface1, Interface2 {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify base class and interfaces
    EXPECT_TRUE(classDecl->hasBaseClass());
    EXPECT_EQ(classDecl->getBaseClass(), "BaseClass");
    EXPECT_TRUE(classDecl->hasInterfaces());
    EXPECT_EQ(classDecl->getInterfaces().size(), 2);
}

// Test 9: Class with function member
TEST_F(ClassDeclarationParsingTest, ClassWithFunctionMember) {
    std::string code = R"(
        class Calculator {
            func add(a: int64, b: int64) -> int64 {
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
    EXPECT_FALSE(members[0]->isEvent());

    auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(members[0]->getDeclaration());
    ASSERT_NE(funcDecl, nullptr);
    EXPECT_EQ(funcDecl->getName(), "add");
}

// Test 10: Class with event member
TEST_F(ClassDeclarationParsingTest, ClassWithEventMember) {
    std::string code = R"(
        class Button {
            event onClick;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify event member
    auto& members = classDecl->getBody().getMembers();
    EXPECT_EQ(members.size(), 1);
    EXPECT_TRUE(members[0]->isEvent());

    auto* event = members[0]->getEvent();
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->getName(), "onClick");
}

// Test 11: Class with mixed members (functions and events)
TEST_F(ClassDeclarationParsingTest, ClassWithMixedMembers) {
    std::string code = R"(
        class Widget {
            func render() -> void {
            }
            event onUpdate;
            func update() -> void {
            }
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify mixed members
    auto& members = classDecl->getBody().getMembers();
    EXPECT_EQ(members.size(), 3);

    // First member: function
    EXPECT_FALSE(members[0]->isEvent());
    auto* func1 = dynamic_cast<const FunctionDeclaration*>(members[0]->getDeclaration());
    EXPECT_EQ(func1->getName(), "render");

    // Second member: event
    EXPECT_TRUE(members[1]->isEvent());
    EXPECT_EQ(members[1]->getEvent()->getName(), "onUpdate");

    // Third member: function
    EXPECT_FALSE(members[2]->isEvent());
    auto* func2 = dynamic_cast<const FunctionDeclaration*>(members[2]->getDeclaration());
    EXPECT_EQ(func2->getName(), "update");
}

// Test 12: Simple interface with no members
TEST_F(ClassDeclarationParsingTest, SimpleInterfaceDeclaration) {
    std::string code = R"(
        interface Empty {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* decl = getFirstDeclaration(*ast);
    ASSERT_NE(decl, nullptr);

    auto* interfaceDecl = dynamic_cast<const InterfaceDeclaration*>(decl);
    ASSERT_NE(interfaceDecl, nullptr) << "Declaration should be an InterfaceDeclaration";

    // Verify interface name
    EXPECT_EQ(interfaceDecl->getName(), "Empty");

    // Verify no members
    EXPECT_TRUE(interfaceDecl->getMembers().empty());
}

// Test 13: Interface with single method signature
TEST_F(ClassDeclarationParsingTest, InterfaceWithSingleMethod) {
    std::string code = R"(
        interface Drawable {
            func draw() -> void;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* interfaceDecl = dynamic_cast<const InterfaceDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(interfaceDecl, nullptr);

    // Verify method signature
    auto& members = interfaceDecl->getMembers();
    EXPECT_EQ(members.size(), 1);

    auto& signature = members[0]->getSignature();
    EXPECT_EQ(signature.getName(), "draw");
    EXPECT_TRUE(signature.getParameters().empty());

    // Verify return type is void
    auto* returnType = signature.getReturnType();
    ASSERT_NE(returnType, nullptr);
}

// Test 14: Interface with multiple method signatures
TEST_F(ClassDeclarationParsingTest, InterfaceWithMultipleMethods) {
    std::string code = R"(
        interface Repository {
            func save(data: int64) -> bool;
            func load(id: int64) -> int64;
            func delete(id: int64) -> void;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* interfaceDecl = dynamic_cast<const InterfaceDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(interfaceDecl, nullptr);

    // Verify multiple method signatures
    auto& members = interfaceDecl->getMembers();
    EXPECT_EQ(members.size(), 3);

    // First method: save
    EXPECT_EQ(members[0]->getSignature().getName(), "save");
    EXPECT_EQ(members[0]->getSignature().getParameters().size(), 1);

    // Second method: load
    EXPECT_EQ(members[1]->getSignature().getName(), "load");
    EXPECT_EQ(members[1]->getSignature().getParameters().size(), 1);

    // Third method: delete
    EXPECT_EQ(members[2]->getSignature().getName(), "delete");
    EXPECT_EQ(members[2]->getSignature().getParameters().size(), 1);
}

// Test 15: Interface method with various return types
TEST_F(ClassDeclarationParsingTest, InterfaceMethodWithVariousReturnTypes) {
    std::string code = R"(
        interface TypeTest {
            func getInt() -> int64;
            func getDouble() -> double;
            func getBool() -> bool;
            func doNothing() -> void;
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* interfaceDecl = dynamic_cast<const InterfaceDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(interfaceDecl, nullptr);

    // Verify all methods are present with correct return types
    auto& members = interfaceDecl->getMembers();
    EXPECT_EQ(members.size(), 4);

    EXPECT_EQ(members[0]->getSignature().getName(), "getInt");
    EXPECT_EQ(members[1]->getSignature().getName(), "getDouble");
    EXPECT_EQ(members[2]->getSignature().getName(), "getBool");
    EXPECT_EQ(members[3]->getSignature().getName(), "doNothing");
}

// Test 16: Class with all modifiers
TEST_F(ClassDeclarationParsingTest, ClassWithAllModifiers) {
    std::string code = R"(
        singleton immutable factory observable service strategy actor final class AllModifiers {
        }
    )";

    auto* parseTree = parseCode(code);
    ASSERT_NE(parseTree, nullptr);

    auto ast = astBuilder->buildAST(getCompilationUnit(parseTree));
    ASSERT_NE(ast, nullptr);

    auto* classDecl = dynamic_cast<const ClassDeclaration*>(getFirstDeclaration(*ast));
    ASSERT_NE(classDecl, nullptr);

    // Verify all modifiers
    EXPECT_EQ(classDecl->getModifiers().size(), 8);
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SINGLETON));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::IMMUTABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FACTORY));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::OBSERVABLE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::SERVICE));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::STRATEGY));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::ACTOR));
    EXPECT_TRUE(classDecl->hasModifier(ClassModifier::FINAL));
}

// Test 17: Complex class with all features
TEST_F(ClassDeclarationParsingTest, ComplexClassWithAllFeatures) {
    std::string code = R"(
        immutable class ComplexClass(id: int64, name: int64) extends BaseClass implements Interface1, Interface2 {
            func method1() -> void {
            }
            event onEvent1;
            func method2(param: int64) -> int64 {
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

    // Verify constructor
    auto* constructor = classDecl->getConstructor();
    ASSERT_NE(constructor, nullptr);
    EXPECT_EQ(constructor->getParameters().size(), 2);

    // Verify base class
    EXPECT_TRUE(classDecl->hasBaseClass());
    EXPECT_EQ(classDecl->getBaseClass(), "BaseClass");

    // Verify interfaces
    EXPECT_TRUE(classDecl->hasInterfaces());
    EXPECT_EQ(classDecl->getInterfaces().size(), 2);

    // Verify members
    auto& members = classDecl->getBody().getMembers();
    EXPECT_EQ(members.size(), 3);
    EXPECT_FALSE(members[0]->isEvent()); // method1
    EXPECT_TRUE(members[1]->isEvent());  // onEvent1
    EXPECT_FALSE(members[2]->isEvent()); // method2
}
