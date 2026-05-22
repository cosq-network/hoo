#include <gtest/gtest.h>
#include "runtime/lib/hoo_runtime.h"

class HooRuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        hoo_reset_memory_stats();
    }
};

TEST_F(HooRuntimeTest, BasicAllocation) {
    void* obj = hoo_alloc(64, 100);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_refcount(obj), 1);
    EXPECT_EQ(hoo_get_type_id(obj), 100);
    
    hoo_release(obj);
}

TEST_F(HooRuntimeTest, ReferenceCounting) {
    void* obj = hoo_alloc(32, 200);
    
    hoo_retain(obj);
    EXPECT_EQ(hoo_get_refcount(obj), 2);
    
    hoo_retain(obj);
    EXPECT_EQ(hoo_get_refcount(obj), 3);
    
    hoo_release(obj);
    EXPECT_EQ(hoo_get_refcount(obj), 2);
    
    hoo_release(obj);
    EXPECT_EQ(hoo_get_refcount(obj), 1);
    
    hoo_release(obj); // Should free here
}

TEST_F(HooRuntimeTest, NullSafety) {
    EXPECT_EQ(hoo_retain(nullptr), nullptr);
    hoo_release(nullptr); // Should not crash
    EXPECT_EQ(hoo_get_refcount(nullptr), 0);
}
