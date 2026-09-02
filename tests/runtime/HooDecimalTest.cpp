#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include "runtime/lib/data/hoo_decimal.h"
#include "runtime/lib/core/hoo_runtime.h"

class HooDecimalTest : public ::testing::Test {
protected:
    static std::string toStr(HooDecimal d) {
        HooString s = hoo_decimal_to_string(d);
        std::string out(hoo_string_data(s));
        hoo_release(s);
        return out;
    }
};

TEST_F(HooDecimalTest, ToStringFraction) {
    HooDecimal a = hoo_decimal_from_literal("0.05", 38, 2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("0.05", toStr(a));
    hoo_decimal_release(a);

    HooDecimal b = hoo_decimal_from_literal("0.5", 38, 1);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ("0.5", toStr(b));
    hoo_decimal_release(b);

    // Trailing zero is normalized away like other scale-0 results.
    HooDecimal c = hoo_decimal_from_literal("0.50", 38, 2);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ("0.5", toStr(c));
    hoo_decimal_release(c);
}

TEST_F(HooDecimalTest, ToStringNegativeFraction) {
    HooDecimal a = hoo_decimal_from_literal("-0.05", 38, 2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("-0.05", toStr(a));
    hoo_decimal_release(a);
}

TEST_F(HooDecimalTest, ToStringMixed) {
    HooDecimal a = hoo_decimal_from_literal("123.45", 38, 2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("123.45", toStr(a));
    hoo_decimal_release(a);

    HooDecimal b = hoo_decimal_from_literal("19.99", 38, 2);
    ASSERT_NE(b, nullptr);
    HooDecimal neg = hoo_decimal_neg(b);
    EXPECT_EQ("-19.99", toStr(neg));
    hoo_decimal_release(neg);
    hoo_decimal_release(b);
}

TEST_F(HooDecimalTest, ToStringInteger) {
    HooDecimal a = hoo_decimal_from_literal("100", 38, 2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("100", toStr(a));
    hoo_decimal_release(a);
}

TEST_F(HooDecimalTest, ToStringZero) {
    HooDecimal a = hoo_decimal_from_literal("0.00", 38, 2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("0", toStr(a));
    hoo_decimal_release(a);
}

TEST_F(HooDecimalTest, ToStringInt64Min) {
    // Regression: negating INT64_MIN in int64 is UB; must format correctly.
    HooDecimal a = hoo_decimal_new(INT64_MIN, 19, 0);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("-9223372036854775808", toStr(a));
    hoo_decimal_release(a);
}

TEST_F(HooDecimalTest, ToStringInt64Max) {
    HooDecimal a = hoo_decimal_new(INT64_MAX, 19, 0);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ("9223372036854775807", toStr(a));
    hoo_decimal_release(a);
}

TEST_F(HooDecimalTest, ParseOverflowThrows) {
    // A 39-digit literal cannot fit in an int64 mantissa at scale 2.
    EXPECT_THROW(hoo_decimal_from_literal(
                     "99999999999999999999999999999999999999", 38, 2),
                 std::exception);
}

TEST_F(HooDecimalTest, AddOverflowThrows) {
    HooDecimal a = hoo_decimal_new(INT64_MAX, 19, 0);
    HooDecimal b = hoo_decimal_new(1, 19, 0);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_THROW(hoo_decimal_add(a, b), std::exception);
    hoo_decimal_release(a);
    hoo_decimal_release(b);
}

TEST_F(HooDecimalTest, DivByZeroThrows) {
    HooDecimal a = hoo_decimal_from_literal("5.00", 38, 2);
    HooDecimal b = hoo_decimal_from_literal("0.00", 38, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_THROW(hoo_decimal_div(a, b), std::exception);
    hoo_decimal_release(a);
    hoo_decimal_release(b);
}

TEST_F(HooDecimalTest, DivOneThird) {
    HooDecimal a = hoo_decimal_from_literal("1.00", 38, 2);
    HooDecimal b = hoo_decimal_from_literal("3.00", 38, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    HooDecimal q = hoo_decimal_div(a, b);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ("0.33333333", toStr(q));
    hoo_decimal_release(q);
    hoo_decimal_release(a);
    hoo_decimal_release(b);
}

TEST_F(HooDecimalTest, DivLowPrecisionDoesNotOverflow) {
    // Decimal<4,2>: 1/7 previously overflowed because the fixed +8 target
    // scale (8 digits) exceeded the 4-digit precision. Now the target scale
    // is clamped to the precision and the division succeeds.
    HooDecimal a = hoo_decimal_from_literal("1.00", 4, 2);
    HooDecimal b = hoo_decimal_from_literal("7.00", 4, 2);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    HooDecimal q = nullptr;
    EXPECT_NO_THROW(q = hoo_decimal_div(a, b));
    ASSERT_NE(q, nullptr);
    EXPECT_EQ("0.1428", toStr(q));
    hoo_decimal_release(q);
    hoo_decimal_release(a);
    hoo_decimal_release(b);
}

TEST_F(HooDecimalTest, CompareAlignedDoesNotOverflow) {
    // Aligning these mantissas requires multiplying one by 10 past INT64_MAX;
    // this must not wrap (previously int64 UB made A < B).
    HooDecimal a = hoo_decimal_new(INT64_MAX, 19, 0);
    HooDecimal b = hoo_decimal_new(INT64_C(1000000000000000000), 19, 1);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(1, hoo_decimal_gt(a, b));
    EXPECT_EQ(0, hoo_decimal_lt(a, b));
    hoo_decimal_release(a);
    hoo_decimal_release(b);
}