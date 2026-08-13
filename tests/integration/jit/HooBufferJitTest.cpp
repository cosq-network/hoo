#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/buffer/hoo_buffer.h"
#include "runtime/lib/string/hoo_string.h"
#include <cstring>

using namespace hooc;

class HooBufferJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooBufferJitTest, NewBuffer) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() { return new Buffer(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooBuffer buf = (HooBuffer)r;
    EXPECT_EQ(0, hoo_buffer_length(buf));
    hoo_buffer_release(buf);
}

TEST_F(HooBufferJitTest, BufferLength) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            return b.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_EQ(r, 0);
}

TEST_F(HooBufferJitTest, BufferCapacity) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            return b.capacity();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_GE(r, 0);
}

TEST_F(HooBufferJitTest, BufferFromBytesFreeFunction) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = buffer_fromBytes("Hello", 5);
            if (b.length() != 5) { return 0; }
            return b.byteAt(1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 101);
}

TEST_F(HooBufferJitTest, BufferCopy) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = buffer_fromBytes("abc", 3);
            return b.copy();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_NE(r, 0);
    HooBuffer copy = (HooBuffer)r;
    EXPECT_EQ(3, hoo_buffer_length(copy));
    hoo_buffer_release(copy);
}

TEST_F(HooBufferJitTest, BufferClear) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            b.clear();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooBufferJitTest, BufferByteAtIndex) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            return b.byteAt(0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1); // empty buffer, out of bounds
}

TEST_F(HooBufferJitTest, BufferSetByte) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = buffer_fromBytes("abc", 3);
            var old = b.setByte(0, 65);
            if (old != 97) { return 0; }
            return b.byteAt(0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 65);
}

TEST_F(HooBufferJitTest, ConstructorWithCapacityIsNotSupported) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer(64);
            return b.length();
        }
    )";
    EXPECT_FALSE(jit.loadSourceCode("test", source));
}

TEST_F(HooBufferJitTest, StaticFromBytesIsNotSupported) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = Buffer.fromBytes("abc", 3);
            return b.length();
        }
    )";
    EXPECT_FALSE(jit.loadSourceCode("test", source));
}

TEST_F(HooBufferJitTest, BufferAppend) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            b.append("Hello", 5);
            return b.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooBufferJitTest, BufferAppendBuffer) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b1 = new Buffer();
            b1.append("Hello", 5);
            var b2 = new Buffer();
            b2.append(" World", 6);
            b1.appendBuffer(b2);
            return b1.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 11);
}

TEST_F(HooBufferJitTest, BufferSubSliceAlias) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = new Buffer();
            b.append("HelloWorld", 10);
            var s = b.sub(0, 5);
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooBufferJitTest, BufferDataPointer) {
    const std::string source = R"(
        import hoo.buffer;
        func :int64 test() {
            var b = buffer_fromBytes("abc", 3);
            var ptr = b.data();
            if (ptr == 0) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooBufferJitTest, BufferWithEncodingBase64) {
    const std::string source = R"(
        import hoo.encoding;
        import hoo.buffer;
        func :int64 test() {
            var data = buffer_fromBytes("Hello", 5);
            var enc = encoding_base64_encode_buffer(data);
            if (enc.length() != 8) { return 0; }
            var dec = encoding_base64_decode_buffer(enc);
            if (dec.length() != 5) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooBufferJitTest, BufferWithHashingSha256) {
    const std::string source = R"(
        import hoo;
        import hoo.buffer;
        import hoo.hashing;
        func :int64 test() {
            var data = buffer_fromBytes("Hello", 5);
            var hash = hashing_sha256_buffer(data);
            if (hash.length() != 64) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
