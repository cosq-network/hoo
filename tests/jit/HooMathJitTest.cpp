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
        import hoo.math;
        func :int64 test() { return Math.abs(-42); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooMathJitTest, MinMax) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.min(10, 20); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 10);
}

TEST_F(HooMathJitTest, MaxInt64) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.max(10, 20); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 20);
}

TEST_F(HooMathJitTest, SignPositive) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.sign(7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, SignNegative) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.sign(-3); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1);
}

TEST_F(HooMathJitTest, SignZero) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.sign(0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMathJitTest, Gcd) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.gcd(12, 18); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 6);
}

TEST_F(HooMathJitTest, Factorial) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.factorial(5); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 120);
}

TEST_F(HooMathJitTest, Fibonacci) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.fibonacci(10); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 55);
}

TEST_F(HooMathJitTest, IsEven) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.isEven(4); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsOdd) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.isOdd(5); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsPrime) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.isPrime(7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, IsNotPrime) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.isPrime(8); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooMathJitTest, Lcm) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() { return Math.lcm(4, 6); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 12);
}

TEST_F(HooMathJitTest, Sqrt) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.sqrt(9.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.0, 0.0001);
}

TEST_F(HooMathJitTest, Pi) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.getPi(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.14159, 0.0001);
}

TEST_F(HooMathJitTest, Pow) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.pow(2.0, 3.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 8.0, 0.0001);
}

TEST_F(HooMathJitTest, Floor) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.floor(3.7); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.0, 0.0001);
}

TEST_F(HooMathJitTest, Ceil) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.ceil(3.2); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 4.0, 0.0001);
}

TEST_F(HooMathJitTest, Sin) {
    const std::string source = R"(
        import hoo.math;
        func :double test() { return Math.sin(0.0); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 0.0, 0.0001);
}

TEST_F(HooMathJitTest, DocumentedDoubleMathFunctions) {
    const std::string source = R"(
        import hoo.math;
        func :double test() {
            return Math.getE() + Math.cos(0.0) + Math.log(Math.getE()) + Math.round(3.5);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 2.718281828459045 + 1.0 + 1.0 + 4.0, 0.0001);
}

TEST_F(HooMathJitTest, DoubleOverloads) {
    const std::string source = R"(
        import hoo.math;
        func :double test() {
            return Math.abs(-3.5) + Math.min(8.0, 2.25) + Math.max(1.5, 4.0) + Math.sign(-2.0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.5 + 2.25 + 4.0 - 1.0, 0.0001);
}

TEST_F(HooMathJitTest, RandomSeededConstructorAndNextIntMax) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() {
            var a = new Random(12345);
            var b = new Random(12345);
            var av = a.nextIntMax(1000000);
            var bv = b.nextIntMax(1000000);
            a.release();
            b.release();
            return av == bv;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooMathJitTest, RandomNextDouble) {
    const std::string source = R"(
        import hoo.math;
        func :double test() {
            var rng = new Random(42);
            var value = rng.nextDouble();
            rng.release();
            return value;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_GE(val, 0.0);
    EXPECT_LT(val, 1.0);
}

TEST_F(HooMathJitTest, RandomNextBoolCanDriveBranch) {
    const std::string source = R"(
        import hoo.math;
        func :int64 test() {
            var rng = new Random(42);
            var flag = rng.nextBool();
            rng.release();
            if (flag) {
                return 1;
            }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_i8");
    EXPECT_TRUE(result == 0 || result == 1);
}
