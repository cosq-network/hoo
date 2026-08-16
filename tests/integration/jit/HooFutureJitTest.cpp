#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/future/hoo_future.h"
#include "runtime/lib/event_loop/hoo_event_loop.h"

using namespace hooc;

class HooFutureJitTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
        hoo_event_loop_init();
    }

    void TearDown() override {
        hoo_event_loop_destroy();
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

// ============================================================================
// FUTURE BASIC TESTS
// ============================================================================

TEST_F(HooFutureJitTest, FutureCreation) {
    std::string code = R"(
        import hoo;
        
        func :bool test() {
            var f: Future<int64> = getVal();
            return true;
        }

        async func:Future<int64> getVal() {
            return 42;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
}

TEST_F(HooFutureJitTest, FutureImmediateResolution) {
    // Test that a future created from a synchronous return is immediately ready
    std::string code = R"(
        import hoo;
        
        async func:Future<int64> getVal() {
            return 42;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    
    // Create a future programmatically to test the runtime
    HooFuture fut = hoo_future_new(1); // type ID 1 = int64
    ASSERT_NE(fut, nullptr);
    
    // Initially not ready
    EXPECT_EQ(hoo_future_is_ready(fut), 0);
    EXPECT_EQ(hoo_future_has_error(fut), 0);
    
    // Set value
    hoo_future_set_value(fut, nullptr); // void future for simplicity
    
    // Now ready
    EXPECT_EQ(hoo_future_is_ready(fut), 1);
    EXPECT_EQ(hoo_future_has_error(fut), 0);
    
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, FutureErrorHandling) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    // Set error
    hoo_future_set_error(fut, "Test error");
    
    // Should be ready with error
    EXPECT_EQ(hoo_future_is_ready(fut), 1);
    EXPECT_EQ(hoo_future_has_error(fut), 1);
    
    const char* error = hoo_future_get_error(fut);
    ASSERT_NE(error, nullptr);
    EXPECT_STREQ(error, "Test error");
    
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, FutureContinuation) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    bool continuationCalled = false;
    auto continuation = [](void* arg) {
        *static_cast<bool*>(arg) = true;
    };
    
    // Set continuation before resolution
    hoo_future_set_continuation(fut, continuation, &continuationCalled);
    
    // Verify not called yet
    EXPECT_FALSE(continuationCalled);
    
    // Resolve the future
    hoo_future_set_value(fut, nullptr);
    
    // Continuation should have been called
    EXPECT_TRUE(continuationCalled);
    
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, FutureContinuationAfterResolution) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    bool continuationCalled = false;
    auto continuation = [](void* arg) {
        *static_cast<bool*>(arg) = true;
    };
    
    // Resolve first
    hoo_future_set_value(fut, nullptr);
    
    // Set continuation after resolution
    hoo_future_set_continuation(fut, continuation, &continuationCalled);
    
    // Continuation should be called immediately
    EXPECT_TRUE(continuationCalled);
    
    hoo_future_release(fut);
}

// ============================================================================
// EVENT LOOP INTEGRATION TESTS (ISSUE-058)
// ============================================================================

TEST_F(HooFutureJitTest, FutureWaitWithEventLoop) {
    // Test that waiting on a future yields to the event loop
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    // Simulate async resolution after a delay
    std::thread resolver([fut]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hoo_future_set_value(fut, nullptr);
    });
    
    // Wait for the future with event loop integration
    void* result = hoo_future_get_value(fut);
    
    resolver.join();
    
    // Should have completed
    EXPECT_EQ(hoo_future_is_ready(fut), 1);
    
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, AwaitUnwrapWithEventLoop) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    // Set a value immediately
    hoo_future_set_value(fut, nullptr);
    
    // await_unwrap should return without error
    void* result = _F_hoo_future_await_unwrap_p_p(fut);
    // Result can be nullptr for void futures
    
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, PrimitiveValueRoundTrip) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);

    hoo_future_set_value(fut, reinterpret_cast<void*>(static_cast<uintptr_t>(42)));
    EXPECT_EQ(reinterpret_cast<intptr_t>(hoo_future_get_value(fut)), 42);
    EXPECT_EQ(reinterpret_cast<intptr_t>(_F_hoo_future_await_unwrap_p_p(fut)), 42);

    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, MultipleContinuationsAreAllInvoked) {
    HooFuture fut = hoo_future_new(4);
    ASSERT_NE(fut, nullptr);
    int callbacks = 0;
    auto continuation = [](void* arg) {
        ++*static_cast<int*>(arg);
    };

    hoo_future_set_continuation(fut, continuation, &callbacks);
    hoo_future_set_continuation(fut, continuation, &callbacks);
    hoo_future_set_value(fut, nullptr);

    EXPECT_EQ(callbacks, 2);
    hoo_future_release(fut);
}

TEST_F(HooFutureJitTest, AwaitRejectedFutureIsHandled) {
    // A rejected Future awaited inside an async function with a try/catch
    // must route the rejection into the catch clause instead of aborting.
    std::string code = R"(
        import hoo;

        async func:Future<int64> probe(f: Future<int64>) {
            try {
                var v = await(f);
                return v;
            } catch (e: Exception) {
                return 7;
            }
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();

    HooFuture rejected = hoo_future_new(1);
    ASSERT_NE(rejected, nullptr);
    hoo_future_set_error(rejected, "boom");

    void* tramp = jit->createInboundTrampoline("test", "_F_M_test_E_probe_p_p", 1);
    ASSERT_NE(tramp, nullptr) << jit->getLastError();
    auto fn = reinterpret_cast<uint64_t (*)(uint64_t)>(tramp);
    uint64_t futPtr = fn(reinterpret_cast<uint64_t>(rejected));
    HooFuture result = reinterpret_cast<HooFuture>(futPtr);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(hoo_future_is_ready(result), 1) << "async function returned a pending Future";
    ASSERT_EQ(hoo_future_has_error(result), 0);
    EXPECT_EQ(reinterpret_cast<int64_t>(hoo_future_get_value(result)), 7);

    hoo_future_release(result);
    hoo_future_release(rejected);
}

TEST_F(HooFutureJitTest, AwaitResolvedFutureValuePassedThrough) {
    // The happy path through the await bridge: a resolved value must flow
    // out of await unchanged, and the error flag must stay clear.
    std::string code = R"(
        import hoo;

        async func:Future<int64> probe(f: Future<int64>) {
            return await(f) + 1;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();

    HooFuture resolved = hoo_future_new(1);
    ASSERT_NE(resolved, nullptr);
    hoo_future_set_value(resolved, reinterpret_cast<void*>(static_cast<uintptr_t>(41)));

    void* tramp = jit->createInboundTrampoline("test", "_F_M_test_E_probe_p_p", 1);
    ASSERT_NE(tramp, nullptr) << jit->getLastError();
    auto fn = reinterpret_cast<uint64_t (*)(uint64_t)>(tramp);
    uint64_t futPtr = fn(reinterpret_cast<uint64_t>(resolved));
    HooFuture result = reinterpret_cast<HooFuture>(futPtr);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(hoo_future_is_ready(result), 1);
    ASSERT_EQ(hoo_future_has_error(result), 0);
    EXPECT_EQ(reinterpret_cast<int64_t>(hoo_future_get_value(result)), 42);

    hoo_future_release(result);
    hoo_future_release(resolved);
}

// ============================================================================
// CONCURRENT FUTURE TESTS
// ============================================================================

TEST_F(HooFutureJitTest, MultipleFutures) {
    // Test multiple futures being resolved concurrently
    const int numFutures = 5;
    HooFuture futures[numFutures];
    bool resolved[numFutures] = {false};
    
    for (int i = 0; i < numFutures; ++i) {
        futures[i] = hoo_future_new(1);
        ASSERT_NE(futures[i], nullptr);
    }
    
    // Launch resolvers
    std::vector<std::thread> resolvers;
    for (int i = 0; i < numFutures; ++i) {
        resolvers.emplace_back([this, i, futures, resolved]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5 * (i + 1)));
            hoo_future_set_value(futures[i], nullptr);
        });
    }
    
    // Wait for all futures
    for (int i = 0; i < numFutures; ++i) {
        void* result = hoo_future_get_value(futures[i]);
        EXPECT_EQ(hoo_future_is_ready(futures[i]), 1);
    }
    
    // Join all resolvers
    for (auto& t : resolvers) {
        t.join();
    }
    
    // Release all futures
    for (int i = 0; i < numFutures; ++i) {
        hoo_future_release(futures[i]);
    }
}

TEST_F(HooFutureJitTest, FutureRetention) {
    HooFuture fut = hoo_future_new(1);
    ASSERT_NE(fut, nullptr);
    
    // Retain multiple times
    HooFuture retained1 = hoo_future_retain(fut);
    HooFuture retained2 = hoo_future_retain(fut);
    
    // All should be the same pointer
    EXPECT_EQ(fut, retained1);
    EXPECT_EQ(fut, retained2);
    
    // Release in reverse order
    hoo_future_release(retained2);
    hoo_future_release(retained1);
    
    // Original should still be valid
    EXPECT_NE(fut, nullptr);
    
    hoo_future_release(fut);
}

// ============================================================================
// NULL SAFETY TESTS
// ============================================================================

TEST_F(HooFutureJitTest, NullFutureOperations) {
    // All operations should handle null gracefully
    EXPECT_EQ(hoo_future_get_elem_type_id(nullptr), 0);
    EXPECT_EQ(hoo_future_is_ready(nullptr), 0);
    EXPECT_EQ(hoo_future_has_error(nullptr), 0);
    EXPECT_EQ(hoo_future_get_error(nullptr), nullptr);
    
    // These should not crash
    hoo_future_set_value(nullptr, nullptr);
    hoo_future_set_error(nullptr, "error");
    hoo_future_set_continuation(nullptr, [](void*){}, nullptr);
    hoo_future_release(nullptr);
    hoo_future_retain(nullptr);
}
