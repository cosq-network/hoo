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
    std::string code = "func add(a: int64, b: int64) -> int64 { return a + b; }";
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

TEST_F(HooCompilerTest, CompileByteFunction) {
    std::string code = "func process(data: byte) -> byte { return data; }";
    auto module = compiler->compile("byte_test", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Verify byte type is properly handled (i8 in LLVM)
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(8));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(8));
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileByteArithmetic) {
    std::string code = R"(
        func calculate(a: byte, b: byte) -> byte {
            var sum = a + b;
            var diff = a - b;
            return sum;
        }
    )";
    auto module = compiler->compile("byte_arithmetic", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
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

TEST_F(HooCompilerTest, CompileFloatFunction) {
    std::string code = "func process(data: float) -> float { return data; }";
    auto module = compiler->compile("float_test", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Verify float type is properly handled (float in LLVM)
    EXPECT_TRUE(func->getReturnType()->isFloatTy());
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isFloatTy());
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileFloatArithmetic) {
    std::string code = R"(
        func calculate(a: float, b: float) -> float {
            var sum = a + b;
            var diff = a - b;
            var product = a * b;
            var quotient = a / b;
            return sum;
        }
    )";
    auto module = compiler->compile("float_arithmetic", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileBoolFunction) {
    std::string code = "func process(flag: bool) -> bool { return flag; }";
    auto module = compiler->compile("bool_test", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Verify bool type is properly handled (i1 in LLVM)
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(1));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(1));
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileBoolLogic) {
    std::string code = R"(
        func logic(a: bool, b: bool) -> bool {
            var and_result = a && b;
            var or_result = a || b;
            var not_result = !a;
            if (and_result) {
                return true;
            } else {
                return false;
            }
        }
    )";
    auto module = compiler->compile("bool_logic", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("logic");
    ASSERT_NE(func, nullptr);
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileCharFunction) {
    std::string code = "func process(ch: char) -> char { return ch; }";
    auto module = compiler->compile("char_test", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Verify char type is properly handled (i32 in LLVM for Unicode scalar)
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(32));
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(32));
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileCharLiterals) {
    std::string code = R"(
        func test_chars() -> char {
            var letter = 'A';
            var digit = '9';
            var symbol = '@';
            if (letter == 'A') {
                return digit;
            } else {
                return symbol;
            }
        }
    )";
    auto module = compiler->compile("char_literals", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("test_chars");
    ASSERT_NE(func, nullptr);
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileArrayFunction) {
    std::string code = "func process(data: int64) -> void { var arr: int64[5]; return; }";
    auto module = compiler->compile("array_test", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("process");
    ASSERT_NE(func, nullptr);
    
    // Verify basic function signature (parameter: int64, not array since array parameters aren't fully supported)
    EXPECT_TRUE(func->getReturnType()->isVoidTy());
    EXPECT_EQ(func->arg_size(), 1);
    EXPECT_TRUE(func->getArg(0)->getType()->isIntegerTy(64));
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}

TEST_F(HooCompilerTest, CompileArrayDeclarations) {
    std::string code = R"(
        func array_demo() -> int64 {
            var numbers: int64[10];
            var matrix: char[3][4];
            var bytes: byte[256];
            return 42;
        }
    )";
    auto module = compiler->compile("array_declarations", code);
    
    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    
    Function* func = module->getFunction("array_demo");
    ASSERT_NE(func, nullptr);
    
    // Verify return type
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
    
    // Verify module is valid LLVM IR
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    EXPECT_FALSE(verifyModule(*module, &errorStream));
}