#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/hoo_encoding.h"

class HooEncodingTest : public ::testing::Test {
};

TEST_F(HooEncodingTest, Base64EncodeDecode) {
    uint8_t* out;
    int64_t  len;

    char* enc = hoo_encoding_base64_encode((const uint8_t*)"hello", 5);
    ASSERT_NE(enc, nullptr);
    EXPECT_STREQ(enc, "aGVsbG8=");
    hoo_encoding_free_string(enc);

    len = hoo_encoding_base64_decode("aGVsbG8=", &out);
    ASSERT_EQ(len, 5);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(std::memcmp(out, "hello", 5), 0);
    hoo_encoding_free_bytes(out);

    uint8_t bin[] = {0x00, 0x01, 0x02, 0xFF, 0xFE};
    enc = hoo_encoding_base64_encode(bin, 5);
    ASSERT_NE(enc, nullptr);
    len = hoo_encoding_base64_decode(enc, &out);
    ASSERT_EQ(len, 5);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(std::memcmp(out, bin, 5), 0);
    hoo_encoding_free_string(enc);
    hoo_encoding_free_bytes(out);

    len = hoo_encoding_base64_decode("!!!invalid!!!", &out);
    EXPECT_EQ(len, -1);
}

TEST_F(HooEncodingTest, Base64Empty) {
    char* enc = hoo_encoding_base64_encode((const uint8_t*)"", 0);
    ASSERT_NE(enc, nullptr);
    EXPECT_STREQ(enc, "");
    hoo_encoding_free_string(enc);
}

TEST_F(HooEncodingTest, HexEncodeDecode) {
    uint8_t* out;
    int64_t  len;

    char* hex = hoo_encoding_hex_encode((const uint8_t*)"hello", 5);
    ASSERT_NE(hex, nullptr);
    EXPECT_STREQ(hex, "68656c6c6f");
    hoo_encoding_free_string(hex);

    len = hoo_encoding_hex_decode("68656c6c6f", &out);
    ASSERT_EQ(len, 5);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(std::memcmp(out, "hello", 5), 0);
    hoo_encoding_free_bytes(out);

    uint8_t bin[] = {0x00, 0x01, 0x02, 0xFF, 0xFE};
    hex = hoo_encoding_hex_encode(bin, 5);
    ASSERT_NE(hex, nullptr);
    len = hoo_encoding_hex_decode(hex, &out);
    ASSERT_EQ(len, 5);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(std::memcmp(out, bin, 5), 0);
    hoo_encoding_free_string(hex);
    hoo_encoding_free_bytes(out);

    len = hoo_encoding_hex_decode("abc", &out);
    EXPECT_EQ(len, -1);
}

TEST_F(HooEncodingTest, UrlEncodeDecode) {
    char* enc;
    char* dec;

    enc = hoo_encoding_url_encode("hello world");
    ASSERT_NE(enc, nullptr);
    EXPECT_STREQ(enc, "hello%20world");
    dec = hoo_encoding_url_decode(enc);
    ASSERT_NE(dec, nullptr);
    EXPECT_STREQ(dec, "hello world");
    hoo_encoding_free_string(enc);
    hoo_encoding_free_string(dec);

    enc = hoo_encoding_url_encode("a=b&c=d");
    ASSERT_NE(enc, nullptr);
    EXPECT_STREQ(enc, "a%3Db%26c%3Dd");
    dec = hoo_encoding_url_decode(enc);
    ASSERT_NE(dec, nullptr);
    EXPECT_STREQ(dec, "a=b&c=d");
    hoo_encoding_free_string(enc);
    hoo_encoding_free_string(dec);
}

TEST_F(HooEncodingTest, FreeNull) {
    hoo_encoding_free_string(nullptr);
    hoo_encoding_free_bytes(nullptr);
}
