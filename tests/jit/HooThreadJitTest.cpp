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
        import hoo.thread;
        func:int64 test() {
            return Thread.self();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexCreateDestroy) {
    const std::string source = R"(
        import hoo.thread;
        func:int64 test() {
            var m = Thread.mutexCreate();
            var ok = Thread.mutexDestroy(m);
            return ok;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexLockUnlock) {
    const std::string source = R"(
        import hoo.thread;
        func:int64 test() {
            var m = Thread.mutexCreate();
            var l = Thread.mutexLock(m);
            var u = Thread.mutexUnlock(m);
            var d = Thread.mutexDestroy(m);
            return l + u + d;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooThreadJitTest, MutexNullLockUnlock) {
    const std::string source = R"(
        import hoo.thread;
        func:int64 test() {
            return Thread.mutexLock(null) + Thread.mutexUnlock(null) + Thread.mutexDestroy(null);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -3);
}
