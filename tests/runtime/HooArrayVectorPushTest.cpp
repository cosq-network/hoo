#include <gtest/gtest.h>
#include "runtime/lib/mem/hoo_generic_array.h"

using namespace hooc;

class HooArrayVectorPushTest : public ::testing::Test {};

TEST_F(HooArrayVectorPushTest, PushVectorInt64) {
    // Create an empty array
    HooArray arr = hoo_array_new();
    ASSERT_NE(arr, nullptr);

    // Prepare source data
    const int64_t src_vals[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    const size_t count = sizeof(src_vals) / sizeof(src_vals[0]);

    // Perform vector push
    arr = hoo_array_push_vector_int64(arr, src_vals, static_cast<int64_t>(count));
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(count));

    // Verify each element
    for (size_t i = 0; i < count; ++i) {
        int64_t retrieved = 0;
        int result = hoo_array_get_int64(arr, static_cast<int64_t>(i), &retrieved);
        EXPECT_EQ(result, 1);
        EXPECT_EQ(retrieved, src_vals[i]);
    }

    // Test pushing an empty vector (should leave array unchanged)
    arr = hoo_array_push_vector_int64(arr, nullptr, 0);
    EXPECT_EQ(hoo_array_length(arr), static_cast<int64_t>(count));

    hoo_array_release(arr);
}
