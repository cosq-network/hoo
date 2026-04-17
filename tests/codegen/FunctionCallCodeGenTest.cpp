#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "src/codegen/LLVMCodeGenerator.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class FunctionCallCodeGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<LLVMContext>();
        codeGen = std::make_unique<LLVMCodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<LLVMCodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
    
    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
    
    std::string getModuleString(llvm::Module* module) {
        std::string str;
        raw_string_ostream rso(str);
        module->print(rso, nullptr);
        return str;
    }
};

TEST_F(FunctionCallCodeGenTest, SimpleFunctionCall) {
    std::string code = R"(
        func helper() -> int64 {
            return 42;
        }

        func test() -> int64 {
            var result = helper();
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify both functions exist
    Function* helperFunc = module->getFunction("helper");
    Function* testFunc = module->getFunction("test");
    ASSERT_NE(helperFunc, nullptr);
    ASSERT_NE(testFunc, nullptr);

    // Verify function call instruction exists
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @helper()") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, FunctionCallWithParameters) {
    std::string code = R"(
        func add(a: int64, b: int64) -> int64 {
            return a + b;
        }

        func test() -> int64 {
            var result = add(10, 20);
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* addFunc = module->getFunction("add");
    Function* testFunc = module->getFunction("test");
    ASSERT_NE(addFunc, nullptr);
    ASSERT_NE(testFunc, nullptr);

    // Verify function has correct parameters
    EXPECT_EQ(addFunc->arg_size(), 2);

    // Verify function call with arguments exists
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @add") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, FunctionCallWithMultipleParameters) {
    std::string code = R"(
        func multiply(x: int64, y: int64, z: int64) -> int64 {
            return x * y * z;
        }

        func test() -> int64 {
            var result = multiply(2, 3, 4);
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* multiplyFunc = module->getFunction("multiply");
    ASSERT_NE(multiplyFunc, nullptr);
    EXPECT_EQ(multiplyFunc->arg_size(), 3);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @multiply") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, NestedFunctionCalls) {
    std::string code = R"(
        func add(a: int64, b: int64) -> int64 {
            return a + b;
        }

        func test() -> int64 {
            var result = add(add(1, 2), add(3, 4));
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    // Verify multiple calls to add function
    std::string irString = getModuleString(module.get());
    size_t count = 0;
    size_t pos = 0;
    while ((pos = irString.find("call i64 @add", pos)) != std::string::npos) {
        count++;
        pos++;
    }
    EXPECT_GE(count, 3); // At least 3 calls to add
}

TEST_F(FunctionCallCodeGenTest, RecursiveFunctionCall) {
    std::string code = R"(
        func factorial(n: int64) -> int64 {
            if n <= 1 {
                return 1;
            } else {
                var prev = factorial(n - 1);
                return n * prev;
            }
        }

        func test() -> int64 {
            var result = factorial(5);
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* factorialFunc = module->getFunction("factorial");
    ASSERT_NE(factorialFunc, nullptr);

    // Verify recursive call exists
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @factorial") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, FunctionCallInReturnStatement) {
    std::string code = R"(
        func getValue() -> int64 {
            return 100;
        }

        func test() -> int64 {
            return getValue();
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @getValue") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, FunctionCallInExpression) {
    std::string code = R"(
        func getValue() -> int64 {
            return 10;
        }

        func test() -> int64 {
            var result = getValue() + 5;
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @getValue") != std::string::npos);
    EXPECT_TRUE(irString.find("add i64") != std::string::npos);
}

TEST_F(FunctionCallCodeGenTest, MultipleFunctionCallsInOneFunction) {
    std::string code = R"(
        func getA() -> int64 { return 10; }
        func getB() -> int64 { return 20; }
        func getC() -> int64 { return 30; }

        func test() -> int64 {
            var a = getA();
            var b = getB();
            var c = getC();
            return a + b + c;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify all helper functions exist
    ASSERT_NE(module->getFunction("getA"), nullptr);
    ASSERT_NE(module->getFunction("getB"), nullptr);
    ASSERT_NE(module->getFunction("getC"), nullptr);
    ASSERT_NE(module->getFunction("test"), nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("call i64 @getA") != std::string::npos);
    EXPECT_TRUE(irString.find("call i64 @getB") != std::string::npos);
    EXPECT_TRUE(irString.find("call i64 @getC") != std::string::npos);
}

