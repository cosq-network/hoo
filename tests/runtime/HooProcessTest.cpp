#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/process/hoo_process.h"

class HooProcessTest : public ::testing::Test {
};

TEST_F(HooProcessTest, SelfPid) {
    int64_t pid = hoo_process_self_pid();
    EXPECT_GT(pid, 0);
}

TEST_F(HooProcessTest, CaptureEcho) {
#ifdef _WIN32
    char* out = hoo_process_capture("cmd.exe /c echo hello");
#else
    char* out = hoo_process_capture("echo hello");
#endif
    ASSERT_NE(out, nullptr);
    std::string result(out);
    EXPECT_EQ(result, "hello\n");
    hoo_process_free_string(out);
}

TEST_F(HooProcessTest, CaptureStatusSuccess) {
    char* output = nullptr;
    int64_t exit_code = -1;
#ifdef _WIN32
    int64_t ret = hoo_process_capture_status("cmd.exe /c echo success", &output, &exit_code);
#else
    int64_t ret = hoo_process_capture_status("echo success", &output, &exit_code);
#endif
    ASSERT_EQ(ret, 0);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(exit_code, 0);
    EXPECT_STREQ(output, "success\n");
    hoo_process_free_string(output);
}

TEST_F(HooProcessTest, CaptureStatusFailure) {
    char* output = nullptr;
    int64_t exit_code = -1;
#ifdef _WIN32
    int64_t ret = hoo_process_capture_status("cmd.exe /c exit 1", &output, &exit_code);
#else
    int64_t ret = hoo_process_capture_status("false", &output, &exit_code);
#endif
    ASSERT_EQ(ret, 0);
    EXPECT_NE(exit_code, 0);
    hoo_process_free_string(output);
}

TEST_F(HooProcessTest, SpawnAndWait) {
    int64_t pid = 0;
#ifdef _WIN32
    const char* argv[] = {"cmd.exe", "/c", "echo hi", nullptr};
    int64_t ret = hoo_process_spawn("cmd.exe", argv, &pid);
#else
    const char* argv[] = {"echo", "hi", nullptr};
    int64_t ret = hoo_process_spawn("echo", argv, &pid);
#endif
    ASSERT_EQ(ret, 0);
    EXPECT_GT(pid, 0);
    int64_t exit_code = -1;
    ret = hoo_process_wait(pid, &exit_code);
    ASSERT_EQ(ret, 0);
    EXPECT_EQ(exit_code, 0);
}

TEST_F(HooProcessTest, Kill) {
    int64_t pid = 0;
#ifdef _WIN32
    const char* argv[] = {"cmd.exe", "/c", "timeout /t 10 /nobreak", nullptr};
    int64_t ret = hoo_process_spawn("cmd.exe", argv, &pid);
#else
    const char* argv[] = {"sleep", "10", nullptr};
    int64_t ret = hoo_process_spawn("sleep", argv, &pid);
#endif
    ASSERT_EQ(ret, 0);
    EXPECT_GT(pid, 0);
    ret = hoo_process_kill(pid, 9);
    EXPECT_EQ(ret, 0);
    int64_t exit_code = -1;
    ret = hoo_process_wait(pid, &exit_code);
    ASSERT_EQ(ret, 0);
}

TEST_F(HooProcessTest, CaptureNonExistentCommand) {
    char* out = hoo_process_capture("nonexistent_cmd_xyzzy");
    ASSERT_NE(out, nullptr);
    EXPECT_STREQ(out, "");
    hoo_process_free_string(out);
}
