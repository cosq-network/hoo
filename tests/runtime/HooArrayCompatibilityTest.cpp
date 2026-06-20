#include <gtest/gtest.h>
#include "hoo_array.h"

// Verify that the array works correctly with homogeneous type after modifications.
TEST(HooArrayCompatibilityTest, HomogeneousInt64) {
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);
    // Push several int64 values
    for (int i = 0; i < 5; ++i) {
        arr = hoo_array_push_int64(arr, i * 10);
        ASSERT_NE(arr, nullptr);
    }
    EXPECT_EQ(hoo_array_length(arr), 5);
    // Retrieve and verify values
    for (int i = 0; i < 5; ++i) {
        int64_t val = 0;
        hoo_array_get_int64(arr, i, &val);
        EXPECT_EQ(val, i * 10);
    }
    hoo_array_release(arr);
}
