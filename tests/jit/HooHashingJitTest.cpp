#include <gtest/gtest.h>
#include <unistd.h>
#include <cstring>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooHashingJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooHashingJitTest, Sha256) {
    const std::string source = R"(
        func :int64 test() {
            var data = "hello";
            var bytes = data.data();
            var len = data.length();
            var hash = Hashing.sha256(bytes, len);
            return hash.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // SHA-256 hex is 64 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 64);
}

TEST_F(HooHashingJitTest, Sha1) {
    const std::string source = R"(
        func :int64 test() {
            var data = "hello";
            var bytes = data.data();
            var len = data.length();
            var hash = Hashing.sha1(bytes, len);
            return hash.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // SHA-1 hex is 40 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 40);
}

TEST_F(HooHashingJitTest, Md5) {
    const std::string source = R"(
        func :int64 test() {
            var data = "hello";
            var bytes = data.data();
            var len = data.length();
            var hash = Hashing.md5(bytes, len);
            return hash.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // MD5 hex is 32 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 32);
}

TEST_F(HooHashingJitTest, Crc32) {
    const std::string source = R"(
        func :int64 test() {
            var data = "hello";
            var bytes = data.data();
            var len = data.length();
            return Hashing.crc32(bytes, len);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // CRC32 of "hello" is a known value
    EXPECT_NE(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooHashingJitTest, Sha256File) {
    // Create a temp file with known content
    char tmp_path[] = "/tmp/hoo_test_sha256_XXXXXX";
    int fd = mkstemp(tmp_path);
    ASSERT_GT(fd, -1);
    const char* content = "hello";
    ASSERT_EQ(write(fd, content, strlen(content)), static_cast<ssize_t>(strlen(content)));
    close(fd);

    std::string source = std::string(R"(
        func :int64 test() {
            var hash = Hashing.sha256_file(")") + tmp_path + R"(");
            return hash.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // SHA-256 hex is 64 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 64);
    unlink(tmp_path);
}
