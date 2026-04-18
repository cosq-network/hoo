#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <iostream>
#include "src/jit/HoocJIT.h"

using namespace hooc;

class IOFunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

TEST_F(IOFunctionTest, PrintlnHelloWorld) {
    std::string code = R"(
        func main() -> void {
            println("Hello, World!");
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}

TEST_F(IOFunctionTest, PrintWithoutNewline) {
    std::string code = R"(
        func main() -> void {
            print("Test");
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}

TEST_F(IOFunctionTest, PrintlnWithVariable) {
    std::string code = R"(
        func main() -> void {
            var message: string = "Test message";
            println(message);
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}

TEST_F(IOFunctionTest, PrintlnEmptyString) {
    std::string code = R"(
        func main() -> void {
            println("");
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}

TEST_F(IOFunctionTest, MultiplePrintln) {
    std::string code = R"(
        func main() -> void {
            println("Line 1");
            println("Line 2");
            println("Line 3");
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}

TEST_F(IOFunctionTest, PrintlnWithStringConcat) {
    std::string code = R"(
        func main() -> void {
            var name: string = "World";
            println("Hello, " + name + "!");
        }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->execute("main");
    ASSERT_TRUE(execResult.has_value()) << jit->getLastError();
}