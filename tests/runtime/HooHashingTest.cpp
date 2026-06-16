#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "runtime/lib/hoo_hashing.h"

class HooHashingTest : public ::testing::Test {
};

TEST_F(HooHashingTest, Sha256) {
    char* hash = hoo_hashing_sha256((const uint8_t*)"hello", 5);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Sha256Empty) {
    char* hash = hoo_hashing_sha256((const uint8_t*)"", 0);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Sha256File) {
#ifdef _WIN32
    char tmp_dir[MAX_PATH + 1] = {0};
    GetTempPathA(MAX_PATH, tmp_dir);
    char path[MAX_PATH + 1] = {0};
    snprintf(path, MAX_PATH, "%s\\hoo_hash_test.tmp", tmp_dir);
    FILE* f = fopen(path, "w");
    ASSERT_NE(f, nullptr);
    fclose(f);
#else
    const char* path = "/dev/null";
#endif
    char* hash = hoo_hashing_sha256_file(path);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    hoo_hashing_free_string(hash);
#ifdef _WIN32
    std::remove(path);
#endif
}

TEST_F(HooHashingTest, Sha256FileNotFound) {
#ifdef _WIN32
    const char* path = "Z:\\nonexistent\\path\\file";
#else
    const char* path = "/nonexistent/path";
#endif
    char* hash = hoo_hashing_sha256_file(path);
    EXPECT_EQ(hash, nullptr);
}

TEST_F(HooHashingTest, Sha1) {
    char* hash = hoo_hashing_sha1((const uint8_t*)"hello", 5);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Sha1Empty) {
    char* hash = hoo_hashing_sha1((const uint8_t*)"", 0);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Md5) {
    char* hash = hoo_hashing_md5((const uint8_t*)"hello", 5);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "5d41402abc4b2a76b9719d911017c592");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Md5Empty) {
    char* hash = hoo_hashing_md5((const uint8_t*)"", 0);
    ASSERT_NE(hash, nullptr);
    EXPECT_STREQ(hash, "d41d8cd98f00b204e9800998ecf8427e");
    hoo_hashing_free_string(hash);
}

TEST_F(HooHashingTest, Crc32) {
    uint64_t crc = hoo_hashing_crc32((const uint8_t*)"hello", 5);
    EXPECT_EQ(crc, 0x3610a686ULL);
}

TEST_F(HooHashingTest, Crc32Empty) {
    uint64_t crc = hoo_hashing_crc32((const uint8_t*)"", 0);
    EXPECT_EQ(crc, 0x0ULL);
}

TEST_F(HooHashingTest, Crc32Known) {
    uint64_t crc = hoo_hashing_crc32((const uint8_t*)"123456789", 9);
    EXPECT_EQ(crc, 0xcbf43926ULL);
}

TEST_F(HooHashingTest, HmacSha256) {
    char* mac = hoo_hashing_hmac_sha256(
        (const uint8_t*)"key", 3,
        (const uint8_t*)"data", 4);
    ASSERT_NE(mac, nullptr);
    EXPECT_STREQ(mac, "5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0");
    hoo_hashing_free_string(mac);
}

TEST_F(HooHashingTest, HmacSha256Empty) {
    char* mac = hoo_hashing_hmac_sha256(
        (const uint8_t*)"", 0,
        (const uint8_t*)"", 0);
    ASSERT_NE(mac, nullptr);
    hoo_hashing_free_string(mac);
}

TEST_F(HooHashingTest, NullInputs) {
    EXPECT_EQ(hoo_hashing_sha256(nullptr, 5), nullptr);
    EXPECT_EQ(hoo_hashing_sha256_file(nullptr), nullptr);
    EXPECT_EQ(hoo_hashing_sha1(nullptr, 5), nullptr);
    EXPECT_EQ(hoo_hashing_md5(nullptr, 5), nullptr);
    EXPECT_EQ(hoo_hashing_crc32(nullptr, 5), 0ULL);
    EXPECT_EQ(hoo_hashing_hmac_sha256(nullptr, 0, nullptr, 0), nullptr);
}
