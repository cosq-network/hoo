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

class ImportStatementCodeGenTest : public ::testing::Test {
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

TEST_F(ImportStatementCodeGenTest, BasicImportWithFunction) {
    std::string code = R"(
        import hoo.io;

        func test() {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Function should exist
    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, FromImportWithFunction) {
    std::string code = R"(
        from std import io;

        func test() {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, DottedModuleImport) {
    std::string code = R"(
        import hoo.collections;

        func test() {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, MultipleImports) {
    std::string code = R"(
        import hoo.io;
        import hoo.collections;

        func test() {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, ImportWithClassDefinition) {
    std::string code = R"(
        import hoo.io;

        class MyClass {
            var value: int64;
            constructor() {}
        }

        func test() {
            var obj: MyClass = new MyClass();
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    // Both function and constructor should exist
    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, FromImportWithMultipleNames) {
    std::string code = R"(
        from std import io, collections;

        func test() {
            return;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto llvmModule = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);
    EXPECT_TRUE(isModuleValid(llvmModule.get()));

    Function* func = llvmModule->getFunction("test");
    ASSERT_NE(func, nullptr);
}

TEST_F(ImportStatementCodeGenTest, ImportWithStringUsage) {
    std::string code = R"(
        import hoo.io;

        func test() {
            var msg: hoo.String = new hoo.String("hello");
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

TEST_F(ImportStatementCodeGenTest, ImportWithArrayUsage) {
    std::string code = R"(
        import hoo.io;

        func test() {
            var arr: int64[] = [1, 2, 3];
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
