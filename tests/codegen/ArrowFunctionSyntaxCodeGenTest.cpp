#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "src/parsing/ProcessIsolatedParser.h"
#include "src/ast/SimpleASTBuilder.h"
#include "src/codegen/LLVMCodeGenerator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class ArrowFunctionSyntaxCodeGenTest : public ::testing::Test {
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
};

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowSyntaxCompilationFails) {
    std::string code = "func test() -> int64 { return 42; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithVoidReturnCompilationFails) {
    std::string code = "func test() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithParametersCompilationFails) {
    std::string code = "func add(a: int64, b: int64) -> int64 { return a + b; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithDoubleReturnCompilationFails) {
    std::string code = "func compute() -> double { return 3.14; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithStringReturnCompilationFails) {
    std::string code = "func getName() -> string { return \"test\"; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithBoolReturnCompilationFails) {
    std::string code = "func check() -> bool { return true; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithArrayReturnCompilationFails) {
    std::string code = "func getArray() -> int64[] { return [1, 2, 3]; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithNullableReturnCompilationFails) {
    std::string code = "func getValue() -> int64? { return null; }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}

TEST_F(ArrowFunctionSyntaxCodeGenTest, ArrowWithMultipleReturnCompilationFails) {
    std::string code = "func:(int64, int64) divmod(a: int64, b: int64) -> (int64, int64) { return (a / b, a % b); }";
    auto ast = parseAndBuildAST(code);
    
    EXPECT_EQ(ast, nullptr);
    EXPECT_FALSE(parser->getLastError().empty());
}