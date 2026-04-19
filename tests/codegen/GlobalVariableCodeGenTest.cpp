#include <gtest/gtest.h>
#include "src/core/HooCompiler.h"
#include "src/codegen/LLVMCodeGenerator.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class GlobalVariableCodeGenTest : public ::testing::Test {
protected:
    HooCompiler compiler;

    std::unique_ptr<llvm::Module> compileToModule(const std::string& code) {
        return compiler.compile("test_module", code);
    }
};

TEST_F(GlobalVariableCodeGenTest, SimpleGlobalInt) {
    std::string code = "var x: int64 = 42;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* x = module->getGlobalVariable("x");
    ASSERT_NE(x, nullptr);
    EXPECT_FALSE(x->isConstant());
    
    auto* init = dyn_cast<ConstantInt>(x->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 42);
}

TEST_F(GlobalVariableCodeGenTest, InferredGlobalFloat) {
    std::string code = "var pi = 3.14;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* pi = module->getGlobalVariable("pi");
    ASSERT_NE(pi, nullptr);
    
    auto* init = dyn_cast<ConstantFP>(pi->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_DOUBLE_EQ(init->getValueAPF().convertToDouble(), 3.14);
}

TEST_F(GlobalVariableCodeGenTest, GlobalBool) {
    std::string code = "var flag: bool = true;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* flag = module->getGlobalVariable("flag");
    ASSERT_NE(flag, nullptr);
    
    auto* init = dyn_cast<ConstantInt>(flag->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getZExtValue(), 1);
}

TEST_F(GlobalVariableCodeGenTest, GlobalUninitialized) {
    std::string code = "var count: int64;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* count = module->getGlobalVariable("count");
    ASSERT_NE(count, nullptr);
    
    auto* init = dyn_cast<ConstantInt>(count->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 0);
}

TEST_F(GlobalVariableCodeGenTest, GlobalString) {
    std::string code = "var msg: string = \"hello\";";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* msg = module->getGlobalVariable("msg");
    ASSERT_NE(msg, nullptr);
    
    auto* initFunc = module->getFunction("__hoo_init");
    ASSERT_NE(initFunc, nullptr);
}

TEST_F(GlobalVariableCodeGenTest, GlobalNonConstantWithExplicitType) {
    std::string code = R"(
        func:int64 getVal() { return 10; }
        var x: int64 = getVal();
    )";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* x = module->getGlobalVariable("x");
    ASSERT_NE(x, nullptr);
}

TEST_F(GlobalVariableCodeGenTest, RejectsNonConstantWithInference) {
    // This still fails because we can't infer type from a function call yet
    std::string code = R"(
        func:int64 getVal() { return 10; }
        var x = getVal();
    )";
    auto module = compileToModule(code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler.getLastError().find("prevents type inference") != std::string::npos);
}
