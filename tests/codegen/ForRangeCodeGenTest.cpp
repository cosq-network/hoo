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

class ForRangeCodeGenTest : public ::testing::Test {
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

TEST_F(ForRangeCodeGenTest, SimpleForRangeLoop) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 10 {
                sum = sum + i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    std::string ir = getModuleString(module.get());
    EXPECT_TRUE(ir.find("is_pos_step") != std::string::npos);
    EXPECT_TRUE(ir.find("pos_cond") != std::string::npos);
    EXPECT_TRUE(ir.find("neg_cond") != std::string::npos);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream)) << errorMsg;
}

TEST_F(ForRangeCodeGenTest, ForRangeLoopWithStep) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 0 .. 10 by 2 {
                sum = sum + i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream)) << errorMsg;
}

TEST_F(ForRangeCodeGenTest, ForRangeLoopWithNegativeStep) {
    std::string code = R"(
        func test() {
            var sum = 0;
            for i in 10 .. 0 by -1 {
                sum = sum + i;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream)) << errorMsg;
}

TEST_F(ForRangeCodeGenTest, ForRangeLoopFloatingPoint) {
    std::string code = R"(
        func test() {
            var sum = 0.0;
            for x in 0.0 .. 1.0 by 0.1 {
                sum = sum + x;
            }
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream)) << errorMsg;
}
