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
        func :int64 test() { return Process.selfPid(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooProcessJitTest, Capture) {
#ifdef _WIN32
    const std::string source = R"(
        import hoo.process;
        func :int64 test() {
            var out = Process.capture("cmd.exe /c echo hello");
            return out.length();
        }
    )";
#else
    const std::string source = R"(
        import hoo.process;
        func :int64 test() {
            var out = Process.capture("echo hello");
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
            var pid = Process.selfPid();
            return Process.kill(pid, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // kill with signal 0 on self should succeed
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}
