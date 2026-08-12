#include <gtest/gtest.h>
#include "runtime/lib/math/hoo_math.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include <limits>

class HooMathTest : public ::testing::Test {
};

TEST_F(HooMathTest, Constants) {
    EXPECT_NEAR(hoo_math_get_pi(), 3.14159, 0.0001);
    EXPECT_NEAR(hoo_math_get_e(), 2.71828, 0.0001);
}

TEST_F(HooMathTest, BasicFunctions) {
    EXPECT_EQ(hoo_math_abs_int64(-10), 10);
    EXPECT_NEAR(hoo_math_abs_double(-5.5), 5.5, 0.0001);
    EXPECT_EQ(hoo_math_min_int64(10, 20), 10);
    EXPECT_EQ(hoo_math_max_int64(10, 20), 20);
    EXPECT_NEAR(hoo_math_clamp(15.0, 0.0, 10.0), 10.0, 0.0001);
}

TEST_F(HooMathTest, Random) {
    void* rng = hoo_math_random_new_with_seed(12345);
    int64_t v1 = hoo_math_random_next_int(rng);
    int64_t v2 = hoo_math_random_next_int(rng);
    EXPECT_NE(v1, v2);
    
    int64_t v3 = hoo_math_random_next_int_max(rng, 10);
    EXPECT_GE(v3, 0);
    EXPECT_LT(v3, 10);
    
    hoo_math_random_release(rng);
}

TEST_F(HooMathTest, Utilities) {
    EXPECT_EQ(hoo_math_is_even(4), 1);
    EXPECT_EQ(hoo_math_is_odd(5), 1);
    EXPECT_EQ(hoo_math_is_prime(7), 1);
    EXPECT_EQ(hoo_math_is_prime(8), 0);
    EXPECT_EQ(hoo_math_gcd(12, 18), 6);
    EXPECT_EQ(hoo_math_factorial(5), 120);
    EXPECT_EQ(hoo_math_fibonacci(10), 55);
}

TEST_F(HooMathTest, LargeIntegerUtilitiesAvoidOverflow) {
    EXPECT_EQ(hoo_math_is_prime(std::numeric_limits<int64_t>::max()), 0);
    EXPECT_EQ(hoo_math_lcm(std::numeric_limits<int64_t>::max(), 2), std::numeric_limits<int64_t>::max());
}

TEST_F(HooMathTest, RandomNextBytesFillsBufferData) {
    void* rng = hoo_math_random_new_with_seed(123);
    HooBuffer buf = hoo_buffer_new(8);

    EXPECT_EQ(hoo_math_random_next_bytes(rng, buf, 8), 8);
    EXPECT_EQ(hoo_buffer_length(buf), 8);
    EXPECT_GE(hoo_buffer_capacity(buf), 8);
    EXPECT_NE(hoo_buffer_byte_at(buf, 0), -1);
    EXPECT_NE(hoo_buffer_byte_at(buf, 7), -1);

    hoo_buffer_release(buf);
    hoo_math_random_release(rng);
}
