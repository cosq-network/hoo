#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_thread.h"

using namespace hooc;

class HooThreadJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooThreadJitTest, SelfId) {
    const std::string source = R"(
        func:int64 test() {
            return thread_self();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexCreateDestroy) {
    const std::string source = R"(
        func:int64 test() {
            var m = thread_mutex_create();
            var ok = thread_mutex_destroy(m);
            return ok;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexLockUnlock) {
    const std::string source = R"(
        func:int64 test() {
            var m = thread_mutex_create();
            var l = thread_mutex_lock(m);
            var u = thread_mutex_unlock(m);
            var d = thread_mutex_destroy(m);
            return l + u + d;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexNullLockUnlock) {
    const std::string source = R"(
        func:int64 test() {
            return thread_mutex_lock(null) + thread_mutex_unlock(null) + thread_mutex_destroy(null);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -3);
}
