#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooProcessJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooProcessJitTest, SelfPid) {
    const std::string source = R"(
        import hoo.process;
        func :int64 test() { return process_self_pid(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooProcessJitTest, Capture) {
#ifdef _WIN32
    const std::string source = R"(
        import hoo.process;
        func :int64 test() {
            var out = process_capture("cmd.exe /c echo hello");
            return out.length();
        }
    )";
#else
    const std::string source = R"(
        import hoo.process;
        func :int64 test() {
            var out = process_capture("echo hello");
            return out.length();
        }
    )";
#endif
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // "echo hello" prints "hello\n" which is 6 bytes
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 6);
}

TEST_F(HooProcessJitTest, KillSignalZero) {
    // Signal 0 checks if the process exists without actually sending a signal
    const std::string source = R"(
        import hoo.process;
        func :int64 test() {
            var pid = process_self_pid();
            return process_kill(pid, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // kill with signal 0 on self should succeed
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooProcessJitTest, SpawnAndWait) {
#ifdef _WIN32
    const std::string source = R"(
        import hoo;
        import hoo.process;
        func :int64 test() {
            var args = new Array();
            args.pushString("/c");
            args.pushString("echo hi");
            var pid = process_spawn("cmd.exe", args);
            if (pid <= 0) {
                return -1;
            }
            return process_wait(pid);
        }
    )";
#else
    const std::string source = R"(
        import hoo;
        import hoo.process;
        func :int64 test() {
            var args = new Array();
            args.pushString("hi");
            var pid = process_spawn("echo", args);
            if (pid <= 0) {
                return -1;
            }
            return process_wait(pid);
        }
    )";
#endif
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}
