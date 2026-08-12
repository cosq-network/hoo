#include <gtest/gtest.h>
#include "runtime/lib/runtime/hoo_runtime.h"
#include <atomic>
#include <thread>

namespace {
std::atomic<int> g_highTypeDestructorCalls{0};

void highTypeDestructor(void*) {
    g_highTypeDestructorCalls.fetch_add(1, std::memory_order_relaxed);
}
}

class HooRuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        hoo_reset_memory_stats();
        hoo_reset_tlab_stats();
        hoo_tlab_reset_thread_cache();
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

TEST_F(HooRuntimeTest, TLABSmallAllocationFastPath) {
    ASSERT_EQ(hoo_tlab_enabled(), 1);

    HooTLABStats before = hoo_get_tlab_stats();
    void* obj = hoo_alloc(64, 300);
    ASSERT_NE(obj, nullptr);
    hoo_release(obj);
    HooTLABStats after = hoo_get_tlab_stats();

    EXPECT_GT(after.tlab_hits, before.tlab_hits);
}

TEST_F(HooRuntimeTest, TLABLargeAllocationFallsBack) {
    HooTLABStats before = hoo_get_tlab_stats();
    void* obj = hoo_alloc(8192, 301);
    ASSERT_NE(obj, nullptr);
    hoo_release(obj);
    HooTLABStats after = hoo_get_tlab_stats();

    EXPECT_GT(after.tlab_misses, before.tlab_misses);
}

TEST_F(HooRuntimeTest, TLABThreadCacheResetIsSafe) {
    void* obj1 = hoo_alloc(128, 302);
    void* obj2 = hoo_alloc(128, 303);
    ASSERT_NE(obj1, nullptr);
    ASSERT_NE(obj2, nullptr);
    hoo_release(obj1);
    hoo_release(obj2);

    hoo_tlab_reset_thread_cache();

    void* obj3 = hoo_alloc(128, 304);
    ASSERT_NE(obj3, nullptr);
    EXPECT_EQ(hoo_get_refcount(obj3), 1);
    hoo_release(obj3);
}

TEST_F(HooRuntimeTest, TLABCrossThreadReleaseIsSafe) {
    void* obj = nullptr;
    std::thread allocator([&] { obj = hoo_alloc(64, 305); });
    allocator.join();
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_refcount(obj), 1);
    hoo_release(obj);
}

TEST_F(HooRuntimeTest, TLABObjectSurvivesAllocatorThreadExit) {
    void* obj = nullptr;
    std::thread allocator([&] {
        obj = hoo_alloc(64, 306);
        hoo_retain(obj);
    });
    allocator.join();
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(hoo_get_refcount(obj), 2);
    hoo_release(obj);
    hoo_release(obj);
}

TEST_F(HooRuntimeTest, DestructorRegistrySupportsTypeIdsBeyondLegacyLimit) {
    constexpr int64_t typeId = 1000000;
    g_highTypeDestructorCalls.store(0, std::memory_order_relaxed);
    hoo_register_destructor(typeId, highTypeDestructor);

    void* obj = hoo_alloc(16, typeId);
    ASSERT_NE(obj, nullptr);
    hoo_release(obj);

    EXPECT_EQ(g_highTypeDestructorCalls.load(std::memory_order_relaxed), 1);
    hoo_register_destructor(typeId, nullptr);
}
