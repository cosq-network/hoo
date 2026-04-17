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
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class QualifiedIdentifierCodeGenTest : public ::testing::Test {
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

    bool isModuleValid(llvm::Module* module) {
        std::string errorMsg;
        raw_string_ostream errorStream(errorMsg);
        return !verifyModule(*module, &errorStream);
    }
};

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringTypeDeclaration) {
    std::string code = R"(
        func test() -> void {
            var name: std.String = new std.String("hello");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should have string runtime functions
    EXPECT_TRUE(getModuleString(llvmModule.get()).find("hoo_string") != std::string::npos);
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringParameter) {
    std::string code = R"(
        func greet(msg: std.String) -> void {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Function should exist with pointer parameter
    Function* func = llvmModule->getFunction("greet");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->arg_size(), static_cast<size_t>(1));
    EXPECT_TRUE(func->getArg(0)->getType()->isPointerTy());
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringReturnType) {
    std::string code = R"(
        func getMessage() -> std.String {
            return new std.String("hello");
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("getMessage");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isPointerTy());
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringArrayDeclaration) {
    std::string code = R"(
        func test() -> void {
            var names: std.String[] = [new std.String("a"), new std.String("b")];
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Should have array runtime functions
    EXPECT_TRUE(getModuleString(llvmModule.get()).find("hoo_array") != std::string::npos);
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringNullableType) {
    std::string code = R"(
        func test() -> void {
            var name: std.String? = null;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(QualifiedIdentifierCodeGenTest, NestedQualifiedType) {
    std::string code = R"(
        func test() -> void {
            var path: std.io.File;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedTypeInClass) {
    std::string code = R"(
        class Person {
            var name: std.String;
            constructor() {}
        }

        func test() -> void {
            var p: Person = new Person();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(QualifiedIdentifierCodeGenTest, MultipleQualifiedParameters) {
    std::string code = R"(
        func process(first: std.String, second: std.String) -> void {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("process");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->arg_size(), static_cast<size_t>(2));
    EXPECT_TRUE(func->getArg(0)->getType()->isPointerTy());
    EXPECT_TRUE(func->getArg(1)->getType()->isPointerTy());
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringArrayWithLoop) {
    std::string code = R"(
        func test() -> void {
            var names: std.String[] = [new std.String("hello"), new std.String("world")];
            var i: int64 = 0;
            while i < 2 {
                i = i + 1;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));
}

TEST_F(QualifiedIdentifierCodeGenTest, QualifiedStringAsFunctionArgument) {
    std::string code = R"(
        func printMessage(msg: std.String) -> void {
            return;
        }

        func test() -> void {
            printMessage(new std.String("test"));
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Both functions should exist
    EXPECT_NE(llvmModule->getFunction("printMessage"), nullptr);
    EXPECT_NE(llvmModule->getFunction("test"), nullptr);
}
