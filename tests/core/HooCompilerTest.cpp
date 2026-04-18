#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "src/core/HooCompiler.h"

using namespace hooc;
using namespace llvm;

class HooCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler;
};

TEST_F(HooCompilerTest, DefaultConstruction) {
    EXPECT_NE(compiler, nullptr);
}

TEST_F(HooCompilerTest, CompilationSuccessFlagOnValidCode) {
    std::string code = "func test() { return; }";
    auto module = compiler->compile("test_module", code);

    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    EXPECT_TRUE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, CompilationSuccessFlagOnInvalidCode) {
    std::string code = "func test() { return; } invalid syntax";
    auto module = compiler->compile("test_module", code);

    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->wasLastCompilationSuccessful());
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, ModuleNameMatchesInput) {
    std::string code = "func test() { return; }";
    auto module = compiler->compile("my_custom_module", code);

    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->getName().str(), "my_custom_module");
}

TEST_F(HooCompilerTest, ModuleNotNullOnValidVoidFunction) {
    std::string code = "func test() { return; }";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnValidInt64Function) {
    std::string code = "func:int64 getValue() { return 42; }";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnValidDoubleFunction) {
    std::string code = "func:double getPi() { return 3.14; }";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnValidBoolFunction) {
    std::string code = "func:bool isValid() { return true; }";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnValidCharFunction) {
    std::string code = "func:char getChar() { return 'a'; }";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnFunctionWithParameters) {
    std::string code = R"(
        func:int64 add(a: int64, b: int64) {
            return a + b;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnFunctionWithArithmetic) {
    std::string code = R"(
        func:int64 calculate(x: int64) {
            var result = x * 2 + 10;
            return result;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnMultipleFunctions) {
    std::string code = R"(
        func:int64 first() { return 1; }
        func:int64 second() { return 2; }
        func:int64 third() { return 3; }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);

    Function* first = module->getFunction("first");
    Function* second = module->getFunction("second");
    Function* third = module->getFunction("third");

    EXPECT_NE(first, nullptr);
    EXPECT_NE(second, nullptr);
    EXPECT_NE(third, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnIfStatement) {
    std::string code = R"(
        func:int64 condTest(x: int64) {
            if x > 0 {
                return 1;
            }
            return 0;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnIfElseStatement) {
    std::string code = R"(
        func:int64 condTest(x: int64) {
            if x > 0 {
                return 1;
            } else {
                return 0;
            }
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnWhileLoop) {
    std::string code = R"(
        func:int64 loopTest() {
            var count = 0;
            while count < 10 {
                count = count + 1;
            }
            return count;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnVariableDeclaration) {
    std::string code = R"(
        func:int64 test() {
            var x = 10;
            var y = 20;
            return x + y;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ErrorMessageOnMissingParenthesis) {
    std::string code = "func test() { return;";
    auto module = compiler->compile("test", code);

    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, ErrorMessageOnInvalidToken) {
    std::string code = "func test() { return; } @@invalid";
    auto module = compiler->compile("test", code);

    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, ErrorMessageOnMismatchedBraces) {
    std::string code = "func test() { return; ";
    auto module = compiler->compile("test", code);

    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, ModuleContainsExpectedFunctionCount) {
    std::string code = R"(
        func:int64 one() { return 1; }
        func:int64 two() { return 2; }
    )";
    auto module = compiler->compile("test", code);

    ASSERT_NE(module, nullptr);
    EXPECT_GE(module->size(), 2);
}

TEST_F(HooCompilerTest, ErrorMessageOnMissingSemicolon) {
    std::string code = R"(
        func:int64 test() {
            var x = 10
            return x;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_EQ(module, nullptr);
    EXPECT_FALSE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, MultipleCompilationsResetErrorState) {
    std::string badCode = "invalid syntax here @#@";
    auto badModule = compiler->compile("test", badCode);

    EXPECT_EQ(badModule, nullptr);
    EXPECT_FALSE(compiler->wasLastCompilationSuccessful());

    std::string goodCode = "func test() { return; }";
    auto goodModule = compiler->compile("test", goodCode);

    EXPECT_NE(goodModule, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
    EXPECT_TRUE(compiler->getLastError().empty());
}

TEST_F(HooCompilerTest, MultipleCompilationsIndependent) {
    std::string code1 = "func:int64 func1() { return 1; }";
    std::string code2 = "func:int64 func2() { return 2; }";
    std::string code3 = "func:int64 func3() { return 3; }";

    auto module1 = compiler->compile("module1", code1);
    auto module2 = compiler->compile("module2", code2);
    auto module3 = compiler->compile("module3", code3);

    ASSERT_NE(module1, nullptr);
    ASSERT_NE(module2, nullptr);
    ASSERT_NE(module3, nullptr);

    EXPECT_EQ(module1->getName().str(), "module1");
    EXPECT_EQ(module2->getName().str(), "module2");
    EXPECT_EQ(module3->getName().str(), "module3");
}

TEST_F(HooCompilerTest, EachCompilationCreatesNewModule) {
    std::string code = "func:int64 test() { return 42; }";

    auto module1 = compiler->compile("test", code);
    auto module2 = compiler->compile("test", code);

    ASSERT_NE(module1, nullptr);
    ASSERT_NE(module2, nullptr);

    EXPECT_NE(module1.get(), module2.get());
}

TEST_F(HooCompilerTest, FunctionExistsInModule) {
    std::string code = "func:int64 myFunction() { return 100; }";
    auto module = compiler->compile("test", code);

    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("myFunction");
    EXPECT_NE(func, nullptr);
}

TEST_F(HooCompilerTest, FunctionReturnType) {
    std::string code = "func:int64 getFive() { return 5; }";
    auto module = compiler->compile("test", code);

    ASSERT_NE(module, nullptr);

    Function* func = module->getFunction("getFive");
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getReturnType()->isIntegerTy(64));
}

TEST_F(HooCompilerTest, EmptyModuleName) {
    std::string code = "func test() { return; }";
    auto module = compiler->compile("", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnLogicalOperations) {
    std::string code = R"(
        func:bool test(a: bool, b: bool) {
            return a && b;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ModuleNotNullOnComparisonOperations) {
    std::string code = R"(
        func:bool compare(a: int64, b: int64) {
            return a > b;
        }
    )";
    auto module = compiler->compile("test", code);

    EXPECT_NE(module, nullptr);
}

TEST_F(HooCompilerTest, ErrorClearedAfterSuccessfulCompilation) {
    std::string badCode = "invalid syntax";
    compiler->compile("test", badCode);
    ASSERT_FALSE(compiler->wasLastCompilationSuccessful());

    std::string goodCode = "func test() { return; }";
    auto module = compiler->compile("test", goodCode);

    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
}

TEST_F(HooCompilerTest, ErrorNotClearedAfterFailedCompilation) {
    std::string badCode1 = "invalid syntax 1";
    compiler->compile("test", badCode1);
    std::string error1 = compiler->getLastError();

    std::string badCode2 = "invalid syntax 2";
    compiler->compile("test", badCode2);
    std::string error2 = compiler->getLastError();

    EXPECT_FALSE(error1.empty());
    EXPECT_FALSE(error2.empty());
}

TEST_F(HooCompilerTest, EmptySourceReturnsValidModule) {
    auto module = compiler->compile("test", "");
    EXPECT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
}

TEST_F(HooCompilerTest, WhitespaceOnlySourceReturnsValidModule) {
    auto module = compiler->compile("test", "   \n\t  \n  ");
    EXPECT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
}

TEST_F(HooCompilerTest, CommentOnlySourceReturnsValidModule) {
    auto module = compiler->compile("test", "// this is a comment");
    EXPECT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
}
