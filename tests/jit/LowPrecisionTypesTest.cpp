#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include "src/hvm/HVMJIT.h"
#include "src/core/DefaultIOProvider.h"

using namespace hooc;

class LowPrecisionTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
    }

    double bitsToDouble(int64_t bits) {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(double));
        return value;
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

TEST_F(LowPrecisionTypesTest, BitLiteralVariableAndLogicalOperations) {
    const std::string code = R"(
        func :int64 test() {
            var a: bit = 1b;
            var b: bit = 0b;
            if (a && !b) {
                return a + b;
            }
            return 0;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, BitParameterReturnAndArrayAccess) {
    const std::string code = R"(
        func :bit id(x: bit) {
            return x;
        }

        func :int64 test() {
            var bits: bit[] = [1b, 0b, 1b];
            return id(bits[0]) + id(bits[2]);
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 2) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, BitReturnSymbolAndInferredLocals) {
    const std::string code = R"(
        func :bit one() {
            var inferred = 1b;
            return inferred;
        }

        func :int64 test() {
            var zero = 0b;
            return one() + zero;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_one_x"), 1) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, BitEqualityAndInequality) {
    const std::string code = R"(
        func :int64 test() {
            var one: bit = 1b;
            var zero: bit = 0b;
            if (one != zero && one == 1b) {
                return 1;
            }
            return 0;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, F8LiteralReturnUsesPromotedDoubleRegisterValue) {
    const std::string code = R"(
        func :f8 test() {
            return 1.5f8;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_NEAR(bitsToDouble(jit->run("_F_test_e")), 1.5, 0.00001) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, F8ArithmeticAndComparison) {
    const std::string code = R"(
        func :int64 test() {
            var a: f8 = 1.5f8;
            var b: f8 = 2.25f8;
            var c: f8 = a + b;
            if (c > 3.0f8) {
                return 1;
            }
            return 0;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, F8DivisionAndLessEqualsComparison) {
    const std::string code = R"(
        func :int64 test() {
            var dividend: f8 = 7.5f8;
            var divisor: f8 = 2.5f8;
            var result: f8 = dividend / divisor;
            if (result <= 3.0f8) {
                return 1;
            }
            return 0;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, F8PromotesInMixedDoubleExpression) {
    const std::string code = R"(
        func :double test() {
            var small: f8 = 1.25f8;
            var wide: double = 2.5;
            return small + wide;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_NEAR(bitsToDouble(jit->run("_F_test_d")), 3.75, 0.00001) << jit->getLastError();
}

TEST_F(LowPrecisionTypesTest, F8ParameterReturnAndArrayAccess) {
    const std::string code = R"(
        func :f8 id(x: f8) {
            return x;
        }

        func :int64 test() {
            var values: f8[] = [0.5f8, 1.5f8];
            var y: f8 = id(values[1]);
            if (y == 1.5f8) {
                return 1;
            }
            return 0;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}
