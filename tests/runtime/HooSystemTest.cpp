#include <gtest/gtest.h>
#include <cstring>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "runtime/lib/hoo_system.h"

class HooSystemTest : public ::testing::Test {
};

TEST_F(HooSystemTest, GetEnv) {
    char* val = hoo_system_get_env("PATH");
    ASSERT_NE(val, nullptr);
    EXPECT_GT(strlen(val), 0);
    hoo_system_free_string(val);
}

TEST_F(HooSystemTest, SetEnv) {
    const char* key = "HOO_TEST_VAR";
    const char* expected = "test_value_42";
    int64_t rc = hoo_system_set_env(key, expected);
    EXPECT_EQ(rc, 0);
    char* val = hoo_system_get_env(key);
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, expected);
    hoo_system_free_string(val);
    hoo_system_unset_env(key);
}

TEST_F(HooSystemTest, UnsetEnv) {
    const char* key = "HOO_UNSET_TEST";
    hoo_system_set_env(key, "temp");
    hoo_system_unset_env(key);
    char* val = hoo_system_get_env(key);
    EXPECT_EQ(val, nullptr);
    hoo_system_free_string(val);
}

TEST_F(HooSystemTest, Hostname) {
    char* name = hoo_system_hostname();
    ASSERT_NE(name, nullptr);
    EXPECT_GT(strlen(name), 0);
    hoo_system_free_string(name);
}

TEST_F(HooSystemTest, OsName) {
    char* os = hoo_system_os_name();
    ASSERT_NE(os, nullptr);
    EXPECT_GT(strlen(os), 0);
    hoo_system_free_string(os);
}

TEST_F(HooSystemTest, CpuCount) {
    int64_t count = hoo_system_cpu_count();
    EXPECT_GT(count, 0);
    EXPECT_LE(count, 1024);
}

TEST_F(HooSystemTest, ProcessId) {
    int64_t pid = hoo_system_process_id();
    EXPECT_GT(pid, 0);
}

TEST_F(HooSystemTest, UserHome) {
    char* home = hoo_system_user_home();
    ASSERT_NE(home, nullptr);
    EXPECT_GT(strlen(home), 0);
#ifdef _WIN32
    EXPECT_TRUE(isalpha((unsigned char)home[0]) && home[1] == ':');
#else
    EXPECT_EQ(home[0], '/');
#endif
    hoo_system_free_string(home);
}

TEST_F(HooSystemTest, UserName) {
    char* name = hoo_system_user_name();
    ASSERT_NE(name, nullptr);
    EXPECT_GT(strlen(name), 0);
    hoo_system_free_string(name);
}

TEST_F(HooSystemTest, CurrentDir) {
    char* dir = hoo_system_current_dir();
    ASSERT_NE(dir, nullptr);
    EXPECT_GT(strlen(dir), 0);
    hoo_system_free_string(dir);
}

TEST_F(HooSystemTest, SetCurrentDir) {
    char* saved = hoo_system_current_dir();
    ASSERT_NE(saved, nullptr);
#ifdef _WIN32
    char tmp_dir[MAX_PATH + 1] = {0};
    GetTempPathA(MAX_PATH, tmp_dir);
    tmp_dir[strcspn(tmp_dir, "\\")] = '\0';
    int64_t rc = hoo_system_set_current_dir(tmp_dir);
#else
    int64_t rc = hoo_system_set_current_dir("/tmp");
#endif
    EXPECT_EQ(rc, 0);
    char* updated = hoo_system_current_dir();
    ASSERT_NE(updated, nullptr);
    EXPECT_GT(strlen(updated), 0);
    hoo_system_free_string(updated);
    hoo_system_set_current_dir(saved);
    hoo_system_free_string(saved);
}

TEST_F(HooSystemTest, FreeString) {
    hoo_system_free_string(nullptr);
}
