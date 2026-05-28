#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "runtime/lib/hoo_compression.h"

class HooCompressionTest : public ::testing::Test {
};

TEST_F(HooCompressionTest, GzipRoundTrip) {
    const uint8_t* input = (const uint8_t*)"hello world";
    int64_t input_len = 11;
    uint8_t* compressed = nullptr;
    int64_t compressed_len = 0;
    int64_t ret = hoo_compression_gzip_compress(input, input_len, &compressed, &compressed_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(compressed, nullptr);
    EXPECT_GT(compressed_len, 0);

    uint8_t* decompressed = nullptr;
    int64_t decompressed_len = 0;
    ret = hoo_compression_gzip_decompress(compressed, compressed_len, &decompressed, &decompressed_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(decompressed, nullptr);
    EXPECT_EQ(decompressed_len, input_len);
    EXPECT_EQ(std::memcmp(decompressed, input, input_len), 0);

    hoo_compression_free_bytes(compressed);
    hoo_compression_free_bytes(decompressed);
}

TEST_F(HooCompressionTest, DeflateRoundTrip) {
    const uint8_t* input = (const uint8_t*)"hello world";
    int64_t input_len = 11;
    uint8_t* compressed = nullptr;
    int64_t compressed_len = 0;
    int64_t ret = hoo_compression_deflate_compress(input, input_len, &compressed, &compressed_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(compressed, nullptr);
    EXPECT_GT(compressed_len, 0);

    uint8_t* decompressed = nullptr;
    int64_t decompressed_len = 0;
    ret = hoo_compression_deflate_decompress(compressed, compressed_len, &decompressed, &decompressed_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(decompressed, nullptr);
    EXPECT_EQ(decompressed_len, input_len);
    EXPECT_EQ(std::memcmp(decompressed, input, input_len), 0);

    hoo_compression_free_bytes(compressed);
    hoo_compression_free_bytes(decompressed);
}

TEST_F(HooCompressionTest, GzipEmptyInput) {
    uint8_t* compressed = nullptr;
    int64_t compressed_len = 0;
    int64_t ret = hoo_compression_gzip_compress((const uint8_t*)"", 0, &compressed, &compressed_len);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(compressed, nullptr);
    uint8_t* decompressed = nullptr;
    int64_t decompressed_len = 0;
    ret = hoo_compression_gzip_decompress(compressed, compressed_len, &decompressed, &decompressed_len);
    ASSERT_EQ(ret, 0);
    EXPECT_EQ(decompressed_len, 0);
    hoo_compression_free_bytes(compressed);
    hoo_compression_free_bytes(decompressed);
}

TEST_F(HooCompressionTest, NullInputs) {
    uint8_t* out = nullptr;
    int64_t out_len = 0;
    EXPECT_EQ(hoo_compression_gzip_compress(nullptr, 0, &out, &out_len), -1);
    EXPECT_EQ(hoo_compression_gzip_decompress(nullptr, 0, &out, &out_len), -1);
    EXPECT_EQ(hoo_compression_deflate_compress(nullptr, 0, &out, &out_len), -1);
    EXPECT_EQ(hoo_compression_deflate_decompress(nullptr, 0, &out, &out_len), -1);
}

TEST_F(HooCompressionTest, GzipInvalidDecompress) {
    uint8_t* out = nullptr;
    int64_t out_len = 0;
    int64_t ret = hoo_compression_gzip_decompress((const uint8_t*)"invalid", 7, &out, &out_len);
    EXPECT_EQ(ret, -1);
}
