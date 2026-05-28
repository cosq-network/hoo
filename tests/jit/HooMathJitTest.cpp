#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include <cmath>
#include <cstring>

using namespace hooc;

class HooMathJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooMathJitTest, AbsInt64) {
    const std::string source = R"(
        func :int64 test() { return math_abs_int64(-42); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooMathJitTest, MinMax) {
    const std::string source = R"(
        func :int64 test() { return math_min_int64(10, 20); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 10);
}

TEST_F(HooMathJitTest, MaxInt64) {
    const std::string source = R"(
        func :int64 test() { return math_max_int64(10, 20); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 20);
}

TEST_F(HooMathJitTest, SignPositive) {
    const std::string source = R"(
        func :int64 test() { return math_sign_int64(7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, SignNegative) {
    const std::string source = R"(
        func :int64 test() { return math_sign_int64(-3); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1);
}

TEST_F(HooMathJitTest, SignZero) {
    const std::string source = R"(
        func :int64 test() { return math_sign_int64(0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMathJitTest, Gcd) {
    const std::string source = R"(
        func :int64 test() { return math_gcd(12, 18); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 6);
}

TEST_F(HooMathJitTest, Factorial) {
    const std::string source = R"(
        func :int64 test() { return math_factorial(5); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 120);
}

TEST_F(HooMathJitTest, Fibonacci) {
    const std::string source = R"(
        func :int64 test() { return math_fibonacci(10); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 55);
}

TEST_F(HooMathJitTest, IsEven) {
    const std::string source = R"(
        func :int64 test() { return math_is_even(4); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsOdd) {
    const std::string source = R"(
        func :int64 test() { return math_is_odd(5); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsPrime) {
    const std::string source = R"(
        func :int64 test() { return math_is_prime(7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsNotPrime) {
    const std::string source = R"(
        func :int64 test() { return math_is_prime(8); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMathJitTest, Lcm) {
    const std::string source = R"(
        func :int64 test() { return math_lcm(4, 6); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 12);
}

TEST_F(HooMathJitTest, Sqrt) {
    const std::string source = R"(
        func :double test() { return math_sqrt(9.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.0, 0.0001);
}

TEST_F(HooMathJitTest, Pi) {
    const std::string source = R"(
        func :double test() { return math_get_pi(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.14159, 0.0001);
}

TEST_F(HooMathJitTest, Pow) {
    const std::string source = R"(
        func :double test() { return math_pow(2.0, 3.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 8.0, 0.0001);
}

TEST_F(HooMathJitTest, Floor) {
    const std::string source = R"(
        func :double test() { return math_floor(3.7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.0, 0.0001);
}

TEST_F(HooMathJitTest, Ceil) {
    const std::string source = R"(
        func :double test() { return math_ceil(3.2); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 4.0, 0.0001);
}

TEST_F(HooMathJitTest, Sin) {
    const std::string source = R"(
        func :double test() { return math_sin(0.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 0.0, 0.0001);
}
