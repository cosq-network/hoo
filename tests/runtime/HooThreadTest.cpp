#include <gtest/gtest.h>
#include <cstdlib>
#include <atomic>
#include "runtime/lib/concurrency/hoo_thread.h"
#include <thread>

class HooThreadTest : public ::testing::Test {
};

static int64_t test_thread_func(void* arg) {
    int64_t val = reinterpret_cast<int64_t>(arg);
    return val * 2;
}

TEST_F(HooThreadTest, SpawnAndJoin) {
    int64_t tid = hoo_thread_spawn(test_thread_func, reinterpret_cast<void*>(42));
    ASSERT_GT(tid, 0);

    int64_t result = hoo_thread_join(tid);
    EXPECT_EQ(result, 84);
}

TEST_F(HooThreadTest, SelfId) {
    int64_t self = hoo_thread_self();
    EXPECT_GT(self, 0);
}

TEST_F(HooThreadTest, MutexLockUnlock) {
    HooMutex mutex = hoo_thread_mutex_create();
    ASSERT_NE(mutex, nullptr);

    EXPECT_EQ(hoo_thread_mutex_lock(mutex), 0);
    EXPECT_EQ(hoo_thread_mutex_unlock(mutex), 0);
    EXPECT_EQ(hoo_thread_mutex_destroy(mutex), 0);
}

TEST_F(HooThreadTest, MutexNullHandling) {
    EXPECT_EQ(hoo_thread_mutex_lock(nullptr), -1);
    EXPECT_EQ(hoo_thread_mutex_unlock(nullptr), -1);
    EXPECT_EQ(hoo_thread_mutex_destroy(nullptr), -1);
}

static int64_t thread_with_mutex_func(void* arg) {
    HooMutex mutex = static_cast<HooMutex>(arg);
    EXPECT_EQ(hoo_thread_mutex_lock(mutex), 0);
    // Critical section
    EXPECT_EQ(hoo_thread_mutex_unlock(mutex), 0);
    return 0;
}

TEST_F(HooThreadTest, MutexCrossThread) {
    HooMutex mutex = hoo_thread_mutex_create();
    ASSERT_NE(mutex, nullptr);

    int64_t tid = hoo_thread_spawn(thread_with_mutex_func, mutex);
    ASSERT_GT(tid, 0);

    int64_t result = hoo_thread_join(tid);
    EXPECT_EQ(result, 0);

    hoo_thread_mutex_destroy(mutex);
}

static std::atomic<int64_t> gSharedCounter{0};
static const int64_t kCounterIterations = 10000;

static int64_t counter_with_mutex_func(void* arg) {
    HooMutex mutex = static_cast<HooMutex>(arg);
    for (int64_t i = 0; i < kCounterIterations; i++) {
        hoo_thread_mutex_lock(mutex);
        gSharedCounter++;
        hoo_thread_mutex_unlock(mutex);
    }
    return 0;
}

TEST_F(HooThreadTest, MutexProtectsSharedCounter) {
    gSharedCounter = 0;
    HooMutex mutex = hoo_thread_mutex_create();
    ASSERT_NE(mutex, nullptr);

    int64_t t1 = hoo_thread_spawn(counter_with_mutex_func, mutex);
    int64_t t2 = hoo_thread_spawn(counter_with_mutex_func, mutex);
    ASSERT_GT(t1, 0);
    ASSERT_GT(t2, 0);

    EXPECT_EQ(hoo_thread_join(t1), 0);
    EXPECT_EQ(hoo_thread_join(t2), 0);

    EXPECT_EQ(gSharedCounter, kCounterIterations * 2);

    hoo_thread_mutex_destroy(mutex);
}

TEST_F(HooThreadTest, ScopedLockReleasesOnScopeExit) {
    HooMutex mutex = hoo_thread_mutex_create();
    ASSERT_NE(mutex, nullptr);
    {
        hoo::thread::ScopedLock lock(mutex);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_EQ(hoo_thread_mutex_try_lock(mutex), 1);
    }
    EXPECT_EQ(hoo_thread_mutex_try_lock(mutex), 0);
    EXPECT_EQ(hoo_thread_mutex_unlock(mutex), 0);
    EXPECT_EQ(hoo_thread_mutex_destroy(mutex), 0);
}

struct ConditionState {
    HooMutex mutex;
    HooCondition condition;
    bool ready = false;
};

static int64_t condition_signal_thread(void* arg) {
    auto* state = static_cast<ConditionState*>(arg);
    hoo_thread_mutex_lock(state->mutex);
    state->ready = true;
    hoo_thread_condition_notify_one(state->condition);
    hoo_thread_mutex_unlock(state->mutex);
    return 0;
}

TEST_F(HooThreadTest, ConditionVariableWaitAndNotify) {
    ConditionState state{hoo_thread_mutex_create(), hoo_thread_condition_create(), false};
    ASSERT_NE(state.mutex, nullptr);
    ASSERT_NE(state.condition, nullptr);
    int64_t tid = hoo_thread_spawn(condition_signal_thread, &state);
    ASSERT_GT(tid, 0);

    hoo_thread_mutex_lock(state.mutex);
    while (!state.ready) {
        EXPECT_EQ(hoo_thread_condition_wait(state.condition, state.mutex), 0);
    }
    hoo_thread_mutex_unlock(state.mutex);
    EXPECT_EQ(hoo_thread_join(tid), 0);
    hoo_thread_condition_destroy(state.condition);
    hoo_thread_mutex_destroy(state.mutex);
}

TEST_F(HooThreadTest, SemaphoreTryWaitAndPost) {
    HooSemaphore semaphore = hoo_thread_semaphore_create(0);
    ASSERT_NE(semaphore, nullptr);
    EXPECT_EQ(hoo_thread_semaphore_try_wait(semaphore), 1);
    EXPECT_EQ(hoo_thread_semaphore_post(semaphore), 0);
    EXPECT_EQ(hoo_thread_semaphore_try_wait(semaphore), 0);
    EXPECT_EQ(hoo_thread_semaphore_destroy(semaphore), 0);
}
