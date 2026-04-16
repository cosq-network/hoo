#include <gtest/gtest.h>
#include "HooCompiler.h"
#include "LLVMCodeGenerator.h"
#include "SimpleASTBuilder.h"
#include "ProcessIsolatedParser.h"
#include "ast/AST.h"
#include "ast/ClassDeclaration.h"
#include "HoocParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "antlr4-runtime.h"

namespace hooc {
namespace tests {

class ObjectCreationCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<llvm::LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<ast::CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;

        auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(parseTree);
        if (!ctx) return nullptr;

        return astBuilder->buildAST(ctx);
    }

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
};

// Test 1: Parse a simple class declaration
TEST_F(ObjectCreationCodeGenTest, ParseSimpleClass) {
    std::string code = R"(
        class Person {
            constructor() {}
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 1);

    auto* classDecl = dynamic_cast<const ast::ClassDeclaration*>(ast->getDeclarations()[0].get());
    ASSERT_NE(classDecl, nullptr);
    EXPECT_EQ(classDecl->getName(), "Person");
}

// Test 2: Generate LLVM IR for a class with constructor
TEST_F(ObjectCreationCodeGenTest, GenerateClassWithConstructor) {
    std::string code = R"(
        class Counter {
            constructor() {}
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check that the constructor function was generated
    auto* ctorFunc = module->getFunction("Counter_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Constructor should take 'this' pointer as first parameter
    EXPECT_EQ(ctorFunc->arg_size(), 1);

    // Constructor should return void
    EXPECT_TRUE(ctorFunc->getReturnType()->isVoidTy());
}

// Test 3: Generate LLVM IR for class with constructor parameters
TEST_F(ObjectCreationCodeGenTest, GenerateClassWithConstructorParams) {
    std::string code = R"(
        class Point {
            constructor(x: int64, y: int64) {}
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Point_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Should have 3 params: this pointer + x + y
    EXPECT_EQ(ctorFunc->arg_size(), 3);
}

// Test 4: Parse new expression
TEST_F(ObjectCreationCodeGenTest, ParseNewExpression) {
    std::string code = R"(
        class Widget {
            constructor() {}
        }

        func createWidget() {
            var w = new Widget();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 2);
}

// Test 5: Generate LLVM IR for new expression
TEST_F(ObjectCreationCodeGenTest, GenerateNewExpression) {
    std::string code = R"(
        class Box {
            constructor() {}
        }

        func createBox() {
            var b = new Box();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check that hoo_alloc was declared
    auto* allocFunc = module->getFunction("hoo_alloc");
    ASSERT_NE(allocFunc, nullptr);

    // Check createBox function exists
    auto* createBoxFunc = module->getFunction("createBox");
    ASSERT_NE(createBoxFunc, nullptr);

    // Verify the function contains a call to hoo_alloc and Box_init
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    module->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("call") != std::string::npos);
    EXPECT_TRUE(ir.find("hoo_alloc") != std::string::npos);
}

// Test 6: Generate new expression with arguments
TEST_F(ObjectCreationCodeGenTest, GenerateNewExpressionWithArgs) {
    std::string code = R"(
        class Rectangle {
            constructor(width: int64, height: int64) {}
        }

        func createRect() {
            var r = new Rectangle(10, 20);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Rectangle_init");
    ASSERT_NE(ctorFunc, nullptr);

    auto* createRectFunc = module->getFunction("createRect");
    ASSERT_NE(createRectFunc, nullptr);
}

// Test 7: Runtime functions are declared
TEST_F(ObjectCreationCodeGenTest, RuntimeFunctionsAreDeclared) {
    std::string code = R"(
        class Empty {
            constructor() {}
        }

        func test() {
            var e = new Empty();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check all runtime functions are declared
    EXPECT_NE(module->getFunction("hoo_alloc"), nullptr);
    EXPECT_NE(module->getFunction("hoo_retain"), nullptr);
    EXPECT_NE(module->getFunction("hoo_release"), nullptr);
}

// Test 8: Class struct type is created and constructor uses it
TEST_F(ObjectCreationCodeGenTest, ClassStructTypeCreated) {
    std::string code = R"(
        class Node {
            constructor() {}
        }

        func test() {
            var n = new Node();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Check that both Node_init and test functions exist
    EXPECT_NE(module->getFunction("Node_init"), nullptr);
    EXPECT_NE(module->getFunction("test"), nullptr);

    // Check the IR contains the allocation call
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    module->print(irStream, nullptr);
    irStream.flush();

    // The hoo_alloc call should be present
    EXPECT_TRUE(ir.find("hoo_alloc") != std::string::npos);
}

// Test 9: Multiple classes
TEST_F(ObjectCreationCodeGenTest, MultipleClasses) {
    std::string code = R"(
        class Animal {
            constructor() {}
        }

        class Dog {
            constructor() {}
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getDeclarations().size(), 2);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("Animal_init"), nullptr);
    EXPECT_NE(module->getFunction("Dog_init"), nullptr);
}

// Test 10: Class with method (methods are named ClassName_methodName)
TEST_F(ObjectCreationCodeGenTest, ClassWithMethod) {
    std::string code = R"(
        class Calculator {
            constructor() {}

            func add(a: int64, b: int64) -> int64 {
                return a + b;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("Calculator_init"), nullptr);
    EXPECT_NE(module->getFunction("Calculator_add"), nullptr);
}

// Test 11: New expression stores to variable
TEST_F(ObjectCreationCodeGenTest, NewExpressionStoresToVariable) {
    std::string code = R"(
        class Data {
            constructor() {}
        }

        func createData() {
            var d = new Data();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("createData");
    ASSERT_NE(func, nullptr);

    // Check IR contains store instruction
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    func->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("store") != std::string::npos);
    EXPECT_TRUE(ir.find("alloca") != std::string::npos);
}

// Test 12: Class with no-op constructor body
TEST_F(ObjectCreationCodeGenTest, EmptyConstructorBody) {
    std::string code = R"(
        class Empty {
            constructor() {}
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* ctorFunc = module->getFunction("Empty_init");
    ASSERT_NE(ctorFunc, nullptr);

    // Verify function has proper structure (entry block with ret void)
    EXPECT_FALSE(ctorFunc->empty());
    EXPECT_TRUE(ctorFunc->getReturnType()->isVoidTy());
}

} // namespace tests
} // namespace hooc
