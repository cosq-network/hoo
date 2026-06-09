#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "src/core/HooCompiler.h"
#include "hvm/HOModule.h"

using namespace hooc;

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
    EXPECT_EQ(module->getName(), "my_custom_module");
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
        func first() { return; }
        func second() { return; }
        func third() { return; }
    )";
    auto module = compiler->compile("test", code);

    ASSERT_NE(module, nullptr);

    auto hasSymbol = [&](const std::string& base) {
        for (const auto& sym : module->getSymbols()) {
            if (sym.name.find(base) != std::string::npos) return true;
        }
        return false;
    };

    if (!hasSymbol("first")) {
        std::cout << "Available symbols: ";
        for (const auto& sym : module->getSymbols()) std::cout << sym.name << " ";
        std::cout << std::endl;
    }

    EXPECT_TRUE(hasSymbol("first"));
    EXPECT_TRUE(hasSymbol("second"));
    EXPECT_TRUE(hasSymbol("third"));
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

TEST_F(HooCompilerTest, EmptySourceReturnsValidModule) {
    auto module = compiler->compile("test", "");
    EXPECT_NE(module, nullptr);
    EXPECT_TRUE(compiler->wasLastCompilationSuccessful());
}
