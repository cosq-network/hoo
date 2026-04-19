#include <gtest/gtest.h>
#include "src/core/HooCompiler.h"
#include "src/codegen/LLVMCodeGenerator.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>

using namespace hooc;
using namespace hooc::ast;
using namespace llvm;

class ConstantCodeGenTest : public ::testing::Test {
protected:
    HooCompiler compiler;

    std::unique_ptr<llvm::Module> compileToModule(const std::string& code) {
        return compiler.compile("test_module", code);
    }
};

TEST_F(ConstantCodeGenTest, SimpleConstantInt) {
    std::string code = "const MAX_COUNT: int64 = 100;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* maxCount = module->getGlobalVariable("MAX_COUNT");
    ASSERT_NE(maxCount, nullptr);
    EXPECT_TRUE(maxCount->isConstant());
    
    auto* init = dyn_cast<ConstantInt>(maxCount->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getSExtValue(), 100);
}

TEST_F(ConstantCodeGenTest, InferredConstantFloat) {
    std::string code = "const PI = 3.14159;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* pi = module->getGlobalVariable("PI");
    ASSERT_NE(pi, nullptr);
    EXPECT_TRUE(pi->isConstant());
    
    auto* init = dyn_cast<ConstantFP>(pi->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_DOUBLE_EQ(init->getValueAPF().convertToDouble(), 3.14159);
}

TEST_F(ConstantCodeGenTest, ConstantBool) {
    std::string code = "const DEBUG_MODE: bool = false;";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* debugMode = module->getGlobalVariable("DEBUG_MODE");
    ASSERT_NE(debugMode, nullptr);
    EXPECT_TRUE(debugMode->isConstant());
    
    auto* init = dyn_cast<ConstantInt>(debugMode->getInitializer());
    ASSERT_NE(init, nullptr);
    EXPECT_EQ(init->getZExtValue(), 0);
}

TEST_F(ConstantCodeGenTest, RejectsConstantWithoutInitializer) {
    // This should be a syntax error in the grammar or a semantic error
    // In our grammar it's a syntax error because we require ASSIGN expression
    std::string code = "const ERR;";
    auto module = compileToModule(code);
    EXPECT_EQ(module, nullptr);
}

TEST_F(ConstantCodeGenTest, ConstantNonConstantWithExplicitType) {
    std::string code = R"(
        func:int64 getVal() { return 10; }
        const X: int64 = getVal();
    )";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();

    auto* x = module->getGlobalVariable("X");
    ASSERT_NE(x, nullptr);
    EXPECT_TRUE(x->isConstant());
}

TEST_F(ConstantCodeGenTest, RejectsNonConstantWithInference) {
    std::string code = R"(
        func:int64 getVal() { return 10; }
        const X = getVal();
    )";
    auto module = compileToModule(code);
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(compiler.getLastError().find("prevents type inference") != std::string::npos);
}

TEST_F(ConstantCodeGenTest, ConstantArray) {
    std::string code = "const PRIMES: int64[] = [2, 3, 5, 7];";
    auto module = compileToModule(code);
    ASSERT_NE(module, nullptr) << compiler.getLastError();
    
    auto* primes = module->getGlobalVariable("PRIMES");
    ASSERT_NE(primes, nullptr);
    EXPECT_TRUE(primes->isConstant());

    // Check for module initializer
    auto* initFunc = module->getFunction("__hoo_init");
    ASSERT_NE(initFunc, nullptr);
}
