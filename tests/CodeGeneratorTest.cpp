#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "../src/CodeGenerator.h"
#include "../src/SimpleASTBuilder.h"
#include "../src/ProcessIsolatedParser.h"
#include "../src/ast/AST.h"
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

class CodeGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_unique<LLVMContext>();
        codeGen = std::make_unique<CodeGenerator>(*context);
        parser = std::make_unique<ProcessIsolatedParser>();
        astBuilder = std::make_unique<SimpleASTBuilder>();
    }

    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<CodeGenerator> codeGen;
    std::unique_ptr<ProcessIsolatedParser> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
    
    std::unique_ptr<CompilationUnit> parseAndBuildAST(const std::string& code) {
        auto* parseTree = parser->parseForAST(code);
        if (!parseTree) return nullptr;
        return astBuilder->buildAST(parseTree);
    }
    
    std::string getModuleString(Module* module) {
        std::string str;
        raw_string_ostream rso(str);
        module->print(rso, nullptr);
        return str;
    }
};

TEST_F(CodeGeneratorTest, GenerateEmptyModule) {
    std::vector<std::unique_ptr<ast::ImportStatement>> imports;
    std::vector<std::unique_ptr<ast::Declaration>> declarations;
    auto ast = std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "hooc_module");
}

TEST_F(CodeGeneratorTest, GenerateSingleVoidFunction) {
    std::string code = "func test() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    // Check that function exists
    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    EXPECT_TRUE(testFunc->getReturnType()->isVoidTy());
    EXPECT_EQ(testFunc->arg_size(), 0);
    
    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(CodeGeneratorTest, GenerateMultipleFunctions) {
    std::string code = R"(
        func first() -> void { return; }
        func second() -> void { return; }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    // Check that both functions exist
    Function* firstFunc = module->getFunction("first");
    Function* secondFunc = module->getFunction("second");
    ASSERT_NE(firstFunc, nullptr);
    ASSERT_NE(secondFunc, nullptr);
    
    // Verify both are void functions with no parameters
    EXPECT_TRUE(firstFunc->getReturnType()->isVoidTy());
    EXPECT_TRUE(secondFunc->getReturnType()->isVoidTy());
    EXPECT_EQ(firstFunc->arg_size(), 0);
    EXPECT_EQ(secondFunc->arg_size(), 0);
    
    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(CodeGeneratorTest, GenerateFunctionWithReturnStatement) {
    std::string code = "func getValue() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("getValue");
    ASSERT_NE(func, nullptr);
    
    // Check function has a basic block with return instruction
    ASSERT_FALSE(func->empty());
    BasicBlock& bb = func->getEntryBlock();
    EXPECT_FALSE(bb.empty());
    
    // Should have a return instruction
    auto* terminator = bb.getTerminator();
    ASSERT_NE(terminator, nullptr);
    EXPECT_TRUE(isa<ReturnInst>(terminator));
}

TEST_F(CodeGeneratorTest, GenerateFunctionWithExpressionStatement) {
    std::string code = "func calculate() -> void { 42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    
    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(CodeGeneratorTest, VerifyModuleStructure) {
    std::string code = "func main() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    // Check module properties
    EXPECT_EQ(module->getName(), "hooc_module");
    EXPECT_EQ(module->getSourceFileName(), "hooc_module");
    
    // Check function exists and is properly formed
    Function* mainFunc = module->getFunction("main");
    ASSERT_NE(mainFunc, nullptr);
    EXPECT_EQ(mainFunc->getName(), "main");
    EXPECT_TRUE(mainFunc->getReturnType()->isVoidTy());
    
    // Function should have exactly one basic block
    EXPECT_EQ(mainFunc->size(), 1);
}

TEST_F(CodeGeneratorTest, HandleEmptyAST) {
    std::vector<std::unique_ptr<ast::ImportStatement>> imports;
    std::vector<std::unique_ptr<ast::Declaration>> declarations;
    auto ast = std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
    
    auto module = codeGen->generateModule(*ast);
    EXPECT_NE(module, nullptr);
}

TEST_F(CodeGeneratorTest, GenerateModuleWithComplexFunction) {
    std::string code = R"(
        func complex() -> void {
            var x = 10;
            if (x > 5) {
                return;
            }
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("complex");
    ASSERT_NE(func, nullptr);
    
    // Verify the function exists and has basic structure
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    EXPECT_FALSE(func->empty());
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(CodeGeneratorTest, VerifyGeneratedIRFormat) {
    std::string code = "func test() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);
    
    // Get the generated IR as string
    std::string irString = getModuleString(module.get());
    
    // Check for expected IR elements
    EXPECT_TRUE(irString.find("ModuleID = 'hooc_module'") != std::string::npos);
    EXPECT_TRUE(irString.find("define void @test()") != std::string::npos);
    EXPECT_TRUE(irString.find("ret void") != std::string::npos);
}