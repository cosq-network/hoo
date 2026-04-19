#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include "src/jit/HoocJIT.h"

using namespace hooc;

class IntegerTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        jit = std::make_unique<HoocJIT>();
    }

    std::unique_ptr<HoocJIT> jit;
};

// ============================================================================
// INT64 TESTS
// ============================================================================

TEST_F(IntegerTypesTest, Int64_ReturnConstant) {
    std::string code = R"(
        func:int64 test() { return 42; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_VariableAssignment) {
    std::string code = R"(
        func:int64 test() { var x: int64 = 100; return x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 100);
}

TEST_F(IntegerTypesTest, Int64_Addition) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 10; var b: int64 = 32; return a + b; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_Subtraction) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 100; var b: int64 = 58; return a - b; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_Multiplication) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 6; var b: int64 = 7; return a * b; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_Division) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 210; var b: int64 = 5; return a / b; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_Modulo) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 100; var b: int64 = 58; return a % b; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_CompoundAssignment) {
    std::string code = R"(
        func:int64 test() { var x: int64 = 10; x = x + 32; return x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Int64_ComparisonEquals) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 42; var b: int64 = 42; if (a == b) { return 1; } return 0; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 1);
}

TEST_F(IntegerTypesTest, Int64_ComparisonNotEquals) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 42; var b: int64 = 43; if (a != b) { return 1; } return 0; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 1);
}

TEST_F(IntegerTypesTest, Int64_ComparisonLessThan) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 10; var b: int64 = 42; if (a < b) { return 1; } return 0; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 1);
}

TEST_F(IntegerTypesTest, Int64_ComparisonGreaterThan) {
    std::string code = R"(
        func:int64 test() { var a: int64 = 100; var b: int64 = 42; if (a > b) { return 1; } return 0; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 1);
}

TEST_F(IntegerTypesTest, Int64_Negation) {
    std::string code = R"(
        func:int64 test() { var x: int64 = -42; return x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, -42);
}

TEST_F(IntegerTypesTest, Int64_UnaryMinus) {
    std::string code = R"(
        func:int64 test() { var x: int64 = 42; return -x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, -42);
}

TEST_F(IntegerTypesTest, Int64_ComplexExpression) {
    std::string code = R"(
        func:int64 test() { var x: int64 = 10; var y: int64 = 5; return (x + y) * (x - y); }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 75);
}

// ============================================================================
// TYPE CONVERSION TESTS
// ============================================================================

TEST_F(IntegerTypesTest, Convert_ByteToInt64) {
    std::string code = R"(
        func:int64 test() { var b: byte = 42; var x: int64 = b; return x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}

TEST_F(IntegerTypesTest, Convert_Int8ToInt64) {
    std::string code = R"(
        func:int64 test() { var b: int8 = 42; var x: int64 = b; return x; }
    )";

    auto result = jit->compile("test", code);
    ASSERT_TRUE(result.success) << result.error;

    auto execResult = jit->executeFunction<int64_t>("test");
    ASSERT_TRUE(execResult.success) << execResult.error;
    EXPECT_EQ(execResult.value, 42);
}