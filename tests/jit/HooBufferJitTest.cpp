#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_buffer.h"
#include "runtime/lib/hoo_string.h"
#include <cstring>

using namespace hooc;

class HooBufferJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

// NOTE: HooBufferJitTest cases are disabled due to a pre-existing ANTLR4 C++ runtime
// LL(*) prediction engine issue (__next_prime overflow) when parsing certain AST compilation units.

// NOTE: HooBufferJitTest cases are disabled due to a pre-existing ANTLR4 C++ runtime
// LL(*) prediction engine issue (__next_prime overflow) when parsing certain AST compilation units.

// NOTE: HooBufferJitTest cases are disabled due to a pre-existing ANTLR4 C++ runtime
// LL(*) prediction engine issue (__next_prime overflow) when parsing certain AST compilation units.

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
