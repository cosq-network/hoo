#include <gtest/gtest.h>
#include <memory>
#include "../src/HooCompiler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace hooc;
using namespace llvm;

class HooCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler;
};

TEST_F(HooCompilerTest, CompileEmptySource) {
    auto module = compiler->compile("test_module", "");
    // Empty source might be valid depending on grammar
    if (module) {
        EXPECT_EQ(module->getName(), "hooc_module");
    } else {
        // If empty source is invalid, that's also acceptable
        EXPECT_FALSE(compiler->getLastError().empty());
    }
}

TEST_F(HooCompilerTest, CompileSingleFunction) {
    std::string code = "func test() -> void { return; }";
    auto module = compiler->compile("test_module", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    EXPECT_TRUE(compiler->getLastError().empty());
    
    // Verify function exists
    Function* testFunc = module->getFunction("test");
    ASSERT_NE(testFunc, nullptr);
    EXPECT_TRUE(testFunc->getReturnType()->isVoidTy());
}

TEST_F(HooCompilerTest, CompileMultipleFunctions) {
    std::string code = R"(
        func first() -> void { return; }
        func second() -> void { return; }
    )";
    auto module = compiler->compile("multi_func", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    // Verify both functions exist
    Function* firstFunc = module->getFunction("first");
    Function* secondFunc = module->getFunction("second");
    ASSERT_NE(firstFunc, nullptr);
    ASSERT_NE(secondFunc, nullptr);
}

TEST_F(HooCompilerTest, CompileInvalidSyntax) {
    std::string code = "invalid";
    auto module = compiler->compile("invalid", code);
    
    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->wasLastCompilationSuccessful());
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, CompileComplexFunction) {
    std::string code = R"(
        func calculate() -> void {
            var x = 42;
            if (x > 0) {
                return;
            }
        }
    )";
    auto module = compiler->compile("complex", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* calcFunc = module->getFunction("calculate");
    ASSERT_NE(calcFunc, nullptr);
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileWithParameters) {
    std::string code = "func add(int64 a, int64 b) -> int64 { return a + b; }";
    auto module = compiler->compile("params", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* addFunc = module->getFunction("add");
    ASSERT_NE(addFunc, nullptr);
}

TEST_F(HooCompilerTest, MultipleCompilations) {
    // Test that compiler can be reused
    std::string code1 = "func first() -> void { return; }";
    std::string code2 = "func second() -> void { return; }";
    
    auto module1 = compiler->compile("mod1", code1);
    ASSERT_NE(module1, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    auto module2 = compiler->compile("mod2", code2);
    ASSERT_NE(module2, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    // Verify each module has the correct function
    EXPECT_NE(module1->getFunction("first"), nullptr);
    EXPECT_NE(module2->getFunction("second"), nullptr);
}

TEST_F(HooCompilerTest, ErrorAfterSuccessfulCompilation) {
    // First successful compilation
    std::string validCode = "func valid() -> void { return; }";
    auto validModule = compiler->compile("valid", validCode);
    ASSERT_NE(validModule, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    // Then failed compilation
    std::string invalidCode = "invalid syntax";
    auto invalidModule = compiler->compile("invalid", invalidCode);
    EXPECT_EQ(invalidModule, nullptr);
    EXPECT_FALSE(compiler->wasLastCompilationSuccessful());
    EXPECT_FALSE(compiler->getLastError().empty());
}