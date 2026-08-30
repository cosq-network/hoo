#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooDecimalJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};

    void expectTrue(const std::string& source) {
        ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
        EXPECT_EQ(jit.run("_F_M_test_E_test_b"), 1) << jit.getLastError();
    }

    void expectException(const std::string& source) {
        ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
        EXPECT_THROW(jit.run("_F_M_test_E_test_v"), std::exception);
    }
};

TEST_F(HooDecimalJitTest, CreatesFractionalLiteral) {
    expectTrue(R"(
        func :bool test() {
            var amount: Decimal<38,2> = 19.99m;
            return amount == 19.99m;
        }
    )");
}

TEST_F(HooDecimalJitTest, CreatesIntegerLiteralAtDeclaredScale) {
    expectTrue(R"(
        func :bool test() {
            var amount: Decimal<38,2> = 100m;
            return amount == 100.00m;
        }
    )");
}

TEST_F(HooDecimalJitTest, Adds) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 8.01m;
            return a + b == 28.00m;
        }
    )");
}

TEST_F(HooDecimalJitTest, Subtracts) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 100.00m;
            var b: Decimal<38,2> = 25.50m;
            return a - b == 74.50m;
        }
    )");
}

TEST_F(HooDecimalJitTest, Multiplies) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 3.00m;
            return a * b == 59.97m;
        }
    )");
}

TEST_F(HooDecimalJitTest, Divides) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 100.00m;
            var b: Decimal<38,2> = 4.00m;
            return a / b == 25.00m;
        }
    )");
}

TEST_F(HooDecimalJitTest, ComputesModulo) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 10.50m;
            var b: Decimal<38,2> = 4.00m;
            return a % b == 2.50m;
        }
    )");
}

TEST_F(HooDecimalJitTest, ComparesEqual) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,3> = 19.990m;
            return a == b;
        }
    )");
}

TEST_F(HooDecimalJitTest, ComparesNotEqual) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            return a != 20.00m;
        }
    )");
}

TEST_F(HooDecimalJitTest, ComparesLessThanAndLessThanOrEqual) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = 20.00m;
            var c: Decimal<38,3> = 19.990m;
            return a < b && a <= c;
        }
    )");
}

TEST_F(HooDecimalJitTest, ComparesGreaterThanAndGreaterThanOrEqual) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 20.00m;
            var b: Decimal<38,2> = 19.99m;
            var c: Decimal<38,3> = 20.000m;
            return a > b && a >= c;
        }
    )");
}

TEST_F(HooDecimalJitTest, NegatesValue) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = -a;
            return b == (0m - 19.99m);
        }
    )");
}

TEST_F(HooDecimalJitTest, DoubleNegation) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var b: Decimal<38,2> = -a;
            return -b == 19.99m;
        }
    )");
}

TEST_F(HooDecimalJitTest, NegationInExpression) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 10.00m;
            var b: Decimal<38,2> = 9.99m;
            return -(a + b) == (0m - 19.99m);
        }
    )");
}

TEST_F(HooDecimalJitTest, NegatesZero) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 0.00m;
            return -a == 0.00m;
        }
    )");
}

TEST_F(HooDecimalJitTest, ToString) {
    expectTrue(R"(
        func :bool test() {
            var a: Decimal<38,2> = 19.99m;
            var s: string = a.toString();
            return s.length() == 5;
        }
    )");
}

#ifndef _WIN32
TEST_F(HooDecimalJitTest, ThrowsOnOverflowAdd) {
    expectException(R"(
        func :void test() {
            var a: Decimal<38,2> = 99999999999999999m;
            var b: Decimal<38,2> = 99999999999999999m;
            var c: Decimal<38,2> = a + b;
        }
    )");
}

TEST_F(HooDecimalJitTest, ThrowsOnOverflowMul) {
    expectException(R"(
        func :void test() {
            var a: Decimal<38,2> = 9999999999m;
            var b: Decimal<38,2> = 9999999999m;
            var c: Decimal<38,2> = a * b;
        }
    )");
}

TEST_F(HooDecimalJitTest, ThrowsOnDivZero) {
    expectException(R"(
        func :void test() {
            var a: Decimal<38,2> = 10.00m;
            var b: Decimal<38,2> = 0.00m;
            var c: Decimal<38,2> = a / b;
        }
    )");
}

TEST_F(HooDecimalJitTest, ThrowsOnModZero) {
    expectException(R"(
        func :void test() {
            var a: Decimal<38,2> = 10.00m;
            var b: Decimal<38,2> = 0.00m;
            var c: Decimal<38,2> = a % b;
        }
    )");
}
#endif
