#include <gtest/gtest.h>
#include "src/core/HooCompiler.h"
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
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

class MethodCallCodeGenTest : public ::testing::Test {
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

// Test 1: Simple method call code generation
TEST_F(MethodCallCodeGenTest, SimpleMethodCallCodeGen) {
    std::string code = R"(
        class Counter {
            var count: int64;
            constructor() {}

            func:int64 getValue() {
                return 42;
            }
        }

        func test() {
            var c: Counter = new Counter();
            var result = c.getValue();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify test function exists
    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify Counter_getValue method exists
    auto* methodFunc = module->getFunction("Counter_getValue");
    ASSERT_NE(methodFunc, nullptr);

    // Method should have at least 1 parameter (this pointer)
    EXPECT_GE(methodFunc->arg_size(), 1);

    // Check for call instruction in test function
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("call") != std::string::npos);
}

// Test 2: Method call with arguments
TEST_F(MethodCallCodeGenTest, MethodCallWithArgs) {
    std::string code = R"(
        class Adder {
            constructor() {}

            func:int64 add(x: int64) {
                return x + 10;
            }
        }

        func test() {
            var a: Adder = new Adder();
            var result = a.add(5);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Adder_add");
    ASSERT_NE(methodFunc, nullptr);

    // Should have 2 parameters: this (void*) + x (int64)
    EXPECT_EQ(methodFunc->arg_size(), 2);
}

// Test 3: Method call with multiple arguments
TEST_F(MethodCallCodeGenTest, MethodCallMultipleArgs) {
    std::string code = R"(
        class Calculator {
            constructor() {}

            func:int64 multiply(a: int64, b: int64) {
                return a * b;
            }
        }

        func test() {
            var calc: Calculator = new Calculator();
            var result = calc.multiply(3, 4);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Calculator_multiply");
    ASSERT_NE(methodFunc, nullptr);

    // Should have 3 parameters: this + a + b
    EXPECT_EQ(methodFunc->arg_size(), 3);
}

// Test 4: Method call with void return
TEST_F(MethodCallCodeGenTest, MethodCallVoidReturn) {
    std::string code = R"(
        class Printer {
            constructor() {}

            func print() {
                var x = 5;
            }
        }

        func test() {
            var p: Printer = new Printer();
            p.print();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Printer_print");
    ASSERT_NE(methodFunc, nullptr);

    // Method should return void
    EXPECT_TRUE(methodFunc->getReturnType()->isVoidTy());
}

// Test 5: Method call in expression
TEST_F(MethodCallCodeGenTest, MethodCallInExpression) {
    std::string code = R"(
        class Math {
            constructor() {}

            func:int64 getValue() {
                return 5;
            }
        }

        func test() {
            var m: Math = new Math();
            var result = m.getValue() + 10;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Check for arithmetic operation
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("add") != std::string::npos);
}

// Test 6: Method call as argument
TEST_F(MethodCallCodeGenTest, MethodCallAsArgument) {
    std::string code = R"(
        class Value {
            constructor() {}

            func:int64 getValue() {
                return 42;
            }
        }

        func:int64 process(x: int64) {
            return x * 2;
        }

        func test() {
            var v: Value = new Value();
            var result = process(v.getValue());
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify both getValue and process are called
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    testFunc->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("call") != std::string::npos);
}

// Test 7: Multiple methods in same class
TEST_F(MethodCallCodeGenTest, MultipleMethodsInClass) {
    std::string code = R"(
        class Operations {
            constructor() {}

            func:int64 getX() {
                return 10;
            }

            func:int64 getY() {
                return 20;
            }
        }

        func test() {
            var ops: Operations = new Operations();
            var x = ops.getX();
            var y = ops.getY();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Both methods should exist
    auto* methodX = module->getFunction("Operations_getX");
    auto* methodY = module->getFunction("Operations_getY");
    ASSERT_NE(methodX, nullptr);
    ASSERT_NE(methodY, nullptr);

    // Both should return int64
    EXPECT_TRUE(methodX->getReturnType()->isIntegerTy(64));
    EXPECT_TRUE(methodY->getReturnType()->isIntegerTy(64));
}

// Test 8: Method call in return statement
TEST_F(MethodCallCodeGenTest, MethodCallInReturn) {
    std::string code = R"(
        class Data {
            constructor() {}

            func:int64 getData() {
                return 100;
            }
        }

        func:int64 getResult() {
            var d: Data = new Data();
            return d.getData();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("getResult");
    ASSERT_NE(func, nullptr);

    // Check for return statement
    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    func->print(irStream, nullptr);
    irStream.flush();

    EXPECT_TRUE(ir.find("ret") != std::string::npos);
}

// Test 9: Method on different object types
TEST_F(MethodCallCodeGenTest, MultipleClassMethods) {
    std::string code = R"(
        class ClassA {
            constructor() {}
            func:int64 methodA() { return 1; }
        }

        class ClassB {
            constructor() {}
            func:int64 methodB() { return 2; }
        }

        func test() {
            var a: ClassA = new ClassA();
            var b: ClassB = new ClassB();
            var x = a.methodA();
            var y = b.methodB();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify both methods exist
    auto* methodA = module->getFunction("ClassA_methodA");
    auto* methodB = module->getFunction("ClassB_methodB");
    ASSERT_NE(methodA, nullptr);
    ASSERT_NE(methodB, nullptr);
}

// Test 10: Method with mixed argument types
TEST_F(MethodCallCodeGenTest, MethodCallMixedTypes) {
    std::string code = R"(
        class Process {
            constructor() {}

            func:double process(i: int64, f: float) {
                return 3.14;
            }
        }

        func test() {
            var p: Process = new Process();
            var result = p.process(42, 2.5);
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    auto* methodFunc = module->getFunction("Process_process");
    ASSERT_NE(methodFunc, nullptr);

    // Should have 3 parameters: this + i (int64) + f (float)
    EXPECT_EQ(methodFunc->arg_size(), 3);

    // Return type should be double
    EXPECT_TRUE(methodFunc->getReturnType()->isDoubleTy());
}

} // namespace tests
} // namespace hooc
