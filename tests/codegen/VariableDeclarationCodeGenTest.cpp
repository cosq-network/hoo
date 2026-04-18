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

class VariableDeclarationCodeGenTest : public ::testing::Test {
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

TEST_F(VariableDeclarationCodeGenTest, GenerateByteVariables) {
    std::string code = R"(
        func test() {
            var b = 255;
            var typed: byte = 128;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("store") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, GenerateFloatVariables) {
    std::string code = R"(
        func test() {
            var f = 3.14;
            var typed: float = 2.71;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("store") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, GenerateBoolVariables) {
    std::string code = R"(
        func test() {
            var flag = true;
            var explicit: bool = false;
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("store") != std::string::npos);
    EXPECT_TRUE(irString.find("i1 true") != std::string::npos);
    EXPECT_TRUE(irString.find("i1 false") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, GenerateCharVariables) {
    std::string code = R"(
        func test() {
            var ch = 'a';
            var explicit: char = 'Z';
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("store") != std::string::npos);
    EXPECT_TRUE(irString.find("i32 97") != std::string::npos); // ASCII 'a'
    EXPECT_TRUE(irString.find("i32 90") != std::string::npos); // ASCII 'Z'
}

TEST_F(VariableDeclarationCodeGenTest, GenerateArrayVariables) {
    std::string code = R"(
        func test() {
            var numbers = [1, 2, 3, 4, 5];
            var chars = ['a', 'b', 'c', 'd', 'e'];
            return;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    // Array literals create global constants, not local allocations
    EXPECT_TRUE(irString.find("@") != std::string::npos); // Global array constants
    EXPECT_TRUE(irString.find("alloca") != std::string::npos); // Variable allocations
}

TEST_F(VariableDeclarationCodeGenTest, Int64VariableWithTypeInference) {
    std::string code = R"(
        func:int64 test() {
            var x = 42;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("store i64 42") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, Int64VariableWithExplicitType) {
    std::string code = R"(
        func:int64 test() {
            var x: int64 = 100;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("store i64 100") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, DoubleVariableWithTypeInference) {
    std::string code = R"(
        func:double test() {
            var x = 3.14;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca double") != std::string::npos);
    EXPECT_TRUE(irString.find("store double") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, DoubleVariableWithExplicitType) {
    std::string code = R"(
        func:double test() {
            var x: double = 2.718;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca double") != std::string::npos);
    EXPECT_TRUE(irString.find("store double") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, BoolVariableWithTypeInference) {
    std::string code = R"(
        func:bool test() {
            var x = true;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i1") != std::string::npos);
    EXPECT_TRUE(irString.find("store i1") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, BoolVariableWithExplicitType) {
    std::string code = R"(
        func:bool test() {
            var x: bool = false;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i1") != std::string::npos);
    EXPECT_TRUE(irString.find("store i1 false") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, CharVariableWithTypeInference) {
    std::string code = R"(
        func:char test() {
            var x = 'A';
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    // char is i32 in hooc
    EXPECT_TRUE(irString.find("alloca i32") != std::string::npos);
    EXPECT_TRUE(irString.find("store i32") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, CharVariableWithExplicitType) {
    std::string code = R"(
        func:char test() {
            var x: char = 'Z';
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    // char is i32 in hooc
    EXPECT_TRUE(irString.find("alloca i32") != std::string::npos);
    EXPECT_TRUE(irString.find("store i32") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, ByteVariableWithExplicitType) {
    std::string code = R"(
        func:byte test() {
            var x: byte = 255;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i8") != std::string::npos);
    EXPECT_TRUE(irString.find("store") != std::string::npos);
    EXPECT_TRUE(irString.find("ret i8") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, MultipleVariablesInOneFunction) {
    std::string code = R"(
        func:int64 test() {
            var a = 10;
            var b: int64 = 20;
            var c = 30;
            return a + b + c;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    // Verify multiple allocations
    std::string irString = getModuleString(module.get());
    size_t count = 0;
    size_t pos = 0;
    while ((pos = irString.find("alloca i64", pos)) != std::string::npos) {
        count++;
        pos++;
    }
    EXPECT_GE(count, 3); // At least 3 allocations for a, b, c
}

TEST_F(VariableDeclarationCodeGenTest, VariableWithExpressionInitializer) {
    std::string code = R"(
        func:int64 test() {
            var x = 10 + 20;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("store i64") != std::string::npos);
    // Note: LLVM may constant-fold "10 + 20" to "30", so we don't check for "add i64"
}

TEST_F(VariableDeclarationCodeGenTest, VariableWithComplexExpressionInitializer) {
    std::string code = R"(
        func:int64 test() {
            var x = (10 + 20) * 3 - 5;
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("store i64") != std::string::npos);
    // Note: LLVM may constant-fold the entire expression, so we don't check for individual operations
}

TEST_F(VariableDeclarationCodeGenTest, VariableInitializedWithFunctionCall) {
    std::string code = R"(
        func:int64 getValue() {
            return 42;
        }

        func:int64 test() {
            var x = getValue();
            return x;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("call i64 @getValue") != std::string::npos);
    EXPECT_TRUE(irString.find("store i64") != std::string::npos);
}

TEST_F(VariableDeclarationCodeGenTest, MixedTypeVariables) {
    std::string code = R"(
        func:int64 test() {
            var intVar: int64 = 42;
            var doubleVar: double = 3.14;
            var boolVar: bool = true;
            var charVar: char = 'X';
            var byteVar: byte = 128;
            return intVar;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);

    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca i64") != std::string::npos);
    EXPECT_TRUE(irString.find("alloca double") != std::string::npos);
    EXPECT_TRUE(irString.find("alloca i1") != std::string::npos);
    EXPECT_TRUE(irString.find("alloca i8") != std::string::npos);
}

