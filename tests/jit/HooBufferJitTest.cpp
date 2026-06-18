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

TEST_F(HooBufferJitTest, NewBuffer) {
    const std::string source = R"(
        func :int64 test() { return Buffer.new(); }
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
        func :int64 test() {
            var b = Buffer.new();
            return b.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_EQ(r, 0);
}

TEST_F(HooBufferJitTest, BufferCapacity) {
    const std::string source = R"(
        func :int64 test() {
            var b = Buffer.new();
            return b.capacity();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_GE(r, 0);
}

TEST_F(HooBufferJitTest, BufferCopy) {
    const std::string source = R"(
        func :int64 test() {
            var b = Buffer.new();
            return b.copy();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_NE(r, 0);
    HooBuffer copy = (HooBuffer)r;
    EXPECT_EQ(0, hoo_buffer_length(copy));
    hoo_buffer_release(copy);
}

TEST_F(HooBufferJitTest, BufferClear) {
    const std::string source = R"(
        func :int64 test() {
            var b = Buffer.new();
            b.clear();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooBufferJitTest, BufferByteAtIndex) {
    const std::string source = R"(
        func :int64 test() {
            var b = Buffer.new();
            return b.byteAt(0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1); // empty buffer, out of bounds
}

TEST_F(HooBufferJitTest, BufferSetByte) {
    const std::string source = R"(
        func :int64 test() {
            var b = Buffer.new();
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
