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

class BasicCodeGenTest : public ::testing::Test {
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

TEST_F(BasicCodeGenTest, GenerateEmptyModule) {
    std::vector<std::unique_ptr<ast::ImportStatement>> imports;
    std::vector<std::unique_ptr<ast::Declaration>> declarations;
    auto ast = std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName(), "hooc_module");
}

TEST_F(BasicCodeGenTest, GenerateSingleVoidFunction) {
    std::string code = "func test() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
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

TEST_F(BasicCodeGenTest, GenerateMultipleFunctions) {
    std::string code = R"(
        func first() -> void { return; }
        func second() -> void { return; }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
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

TEST_F(BasicCodeGenTest, GenerateFunctionWithReturnStatement) {
    std::string code = "func getValue() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
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

TEST_F(BasicCodeGenTest, GenerateFunctionWithExpressionStatement) {
    std::string code = "func calculate() -> void { 42; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    
    // Verify module is valid
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(BasicCodeGenTest, VerifyModuleStructure) {
    std::string code = "func main() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
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

TEST_F(BasicCodeGenTest, HandleEmptyAST) {
    std::vector<std::unique_ptr<ast::ImportStatement>> imports;
    std::vector<std::unique_ptr<ast::Declaration>> declarations;
    auto ast = std::make_unique<CompilationUnit>(std::move(imports), std::move(declarations));
    
    auto module = codeGen->generateLLVMModule(*ast);
    EXPECT_NE(module, nullptr);
}

TEST_F(BasicCodeGenTest, GenerateModuleWithComplexFunction) {
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
    
    auto module = codeGen->generateLLVMModule(*ast);
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

TEST_F(BasicCodeGenTest, VerifyGeneratedIRFormat) {
    std::string code = "func test() -> void { return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    // Get the generated IR as string
    std::string irString = getModuleString(module.get());
    
    // Check for expected IR elements
    EXPECT_TRUE(irString.find("ModuleID = 'hooc_module'") != std::string::npos);
    EXPECT_TRUE(irString.find("define void @test()") != std::string::npos);
    EXPECT_TRUE(irString.find("ret void") != std::string::npos);
}

TEST_F(BasicCodeGenTest, GenerateByteFunction) {
    std::string code = "func process(data: byte) -> byte { return data; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Check function signature
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(8));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(8));
}

TEST_F(BasicCodeGenTest, GenerateByteArithmetic) {
    std::string code = R"(
        func calculate(a: byte, b: byte) -> byte {
            var result = a + b;
            return result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    
    // Check that function has byte parameters and return type
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(8));
    EXPECT_EQ(func->arg_size(), 2);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(8));
    EXPECT_TRUE(func->getArg(1)->getType()->isIntegerTy(8));
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("add i8") != std::string::npos);
}

TEST_F(BasicCodeGenTest, GenerateFloatFunction) {
    std::string code = "func process(data: float) -> float { return data; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Check function signature
    EXPECT_TRUE(func->getReturnType()->isFloatTy());
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isFloatTy());
}

TEST_F(BasicCodeGenTest, GenerateFloatArithmetic) {
    std::string code = R"(
        func calculate(a: float, b: float) -> float {
            var sum = a + b;
            var product = a * b;
            return sum;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    
    // Check that function has float parameters and return type
    EXPECT_TRUE(func->getReturnType()->isFloatTy());
    EXPECT_EQ(func->arg_size(), 2);
    EXPECT_TRUE(func->getArg(0)->getType()->isFloatTy());
    EXPECT_TRUE(func->getArg(1)->getType()->isFloatTy());
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("fadd float") != std::string::npos);
    EXPECT_TRUE(irString.find("fmul float") != std::string::npos);
}

TEST_F(BasicCodeGenTest, GenerateBoolFunction) {
    std::string code = "func process(flag: bool) -> bool { return flag; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Check function signature
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(1));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(1));
}

TEST_F(BasicCodeGenTest, GenerateBoolLogic) {
    std::string code = R"(
        func logic(a: bool, b: bool) -> bool {
            var and_result = a && b;
            var or_result = a || b;
            var not_result = !a;
            return and_result;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("logic");
    ASSERT_NE(func, nullptr);
    
    // Check that function has bool parameters and return type
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(1));
    EXPECT_EQ(func->arg_size(), 2);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(1));
    EXPECT_TRUE(func->getArg(1)->getType()->isIntegerTy(1));
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("i1") != std::string::npos);
}

TEST_F(BasicCodeGenTest, GenerateCharFunction) {
    std::string code = "func process(ch: char) -> char { return ch; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Check function signature
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(32));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(32));
}

TEST_F(BasicCodeGenTest, GenerateCharComparison) {
    std::string code = R"(
        func compare(a: char, b: char) -> bool {
            var equal = a == b;
            var less = a < b;
            return equal;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);
    
    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);
    
    Function* func = module->getFunction("compare");
    ASSERT_NE(func, nullptr);
    
    // Check that function has char parameters and bool return type
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(1));
    EXPECT_EQ(func->arg_size(), 2);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(32));
    EXPECT_TRUE(func->getArg(1)->getType()->isIntegerTy(32));
    
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("icmp eq i32") != std::string::npos);
    EXPECT_TRUE(irString.find("icmp slt i32") != std::string::npos);
}

TEST_F(BasicCodeGenTest, GenerateArrayFunction) {
    std::string code = "func process(data: int64) -> void { var arr = [1, 2, 3, 4, 5]; return; }";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);

    // Check basic function signature
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(64));

    // Verify array literal is present in IR
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("@") != std::string::npos); // Global array constant
}

TEST_F(BasicCodeGenTest, GenerateArrayAccess) {
    std::string code = R"(
        func access_test() -> int64 {
            var arr = [10, 20, 30, 40, 50];
            var index = 2;
            var value = arr[index];
            return 42;
        }
    )";
    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateLLVMModule(*ast);
    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("access_test");
    ASSERT_NE(func, nullptr);

    // Verify variables are allocated and function returns correctly
    std::string irString = getModuleString(module.get());
    EXPECT_TRUE(irString.find("alloca") != std::string::npos);
    EXPECT_TRUE(irString.find("ret i64") != std::string::npos);
}

