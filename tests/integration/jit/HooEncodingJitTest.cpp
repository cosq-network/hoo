#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooEncodingJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooEncodingJitTest, Base64RoundTrip) {
    const std::string source = R"(
        import hoo.encoding;
        func:int64 test() {
            var str = "Hello World";
            var enc = encoding_base64_encode(str.data(), str.length());
            var dec = encoding_base64_decode(enc);
            return dec.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 11);
}

TEST_F(HooEncodingJitTest, HexRoundTrip) {
    const std::string source = R"(
        import hoo.encoding;
        func:int64 test() {
            var str = "Hello";
            var enc = encoding_hex_encode(str.data(), str.length());
            var dec = encoding_hex_decode(enc);
            return dec.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooEncodingJitTest, UrlRoundTrip) {
    const std::string source = R"(
        import hoo.encoding;
        func:int64 test() {
            var str = "hello world";
            var enc = encoding_url_encode(str);
            var dec = encoding_url_decode(enc);
            return dec.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 11);
}

TEST_F(HooEncodingJitTest, BufferOverloads) {
    const std::string source = R"(
        import hoo.encoding;
        import hoo.buffer;
        func:int64 test() {
            var buf = buffer_fromBytes("Hello", 5);
            var enc = encoding_base64_encode_buffer(buf);
            var dec = encoding_base64_decode_buffer(enc);
            var hex = encoding_hex_encode_buffer(dec);
            var dec2 = encoding_hex_decode_buffer(hex);
            return dec2.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooEncodingJitTest, ByteSliceJitWrappers) {
    const std::string source = R"(
        import hoo.encoding;
        import hoo.buffer;
        func:int64 test() {
            var buf = buffer_fromBytes("Hello", 5);
            var view = byte_slice_from_buffer(buf);
            var encoded = encoding_base64_encode_slice(view);
            byte_slice_release(view);
            return encoded.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 8) << jit.getLastError();
}
