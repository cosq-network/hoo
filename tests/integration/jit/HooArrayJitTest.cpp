#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/generic_array/hoo_generic_array.h"
#include "runtime/lib/string/hoo_string.h"
#include <cmath>
#include <cstring>

using namespace hooc;

class HooArrayJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooArrayJitTest, NewArray) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return new Array(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooArray a = (HooArray)r;
    EXPECT_EQ(0, hoo_array_length(a));
    hoo_array_release(a);
}

TEST_F(HooArrayJitTest, PushGetDouble) {
    const std::string source = R"(
        import hoo;
        func :double test() {
            var a = new Array();
            Array.pushDouble(a, 3.14);
            return Array.getDouble(a, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_d");
    double val;
    std::memcpy(&val, &result, sizeof(double));
    EXPECT_NEAR(val, 3.14, 0.001);
}

TEST_F(HooArrayJitTest, PushGetInt64) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 42);
            return Array.getInt64(a, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooArrayJitTest, ArrayLength) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 10);
            Array.pushInt64(a, 20);
return Array.length(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}

TEST_F(HooArrayJitTest, ArrayClear) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 10);
            Array.clear(a);
            return Array.length(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArrayJitTest, ArrayEmpty) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            return Array.empty(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, PushGetString) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushString(a, "hello");
            var s = Array.getString(a, 0);
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooArrayJitTest, PushGetBool) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushBool(a, 1);
            return Array.getBool(a, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArraySortInt64) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 5);
            Array.pushInt64(a, 3);
            Array.pushInt64(a, 9);
            Array.pushInt64(a, 1);
            Array.sort(a);
            var r = 1;
            if (Array.getInt64(a, 0) != 1) { r = 0; }
            if (Array.getInt64(a, 1) != 3) { r = 0; }
            if (Array.getInt64(a, 2) != 5) { r = 0; }
            if (Array.getInt64(a, 3) != 9) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArraySortEmpty) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.sort(a);
            return Array.length(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArrayJitTest, ArraySortSingle) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 42);
            Array.sort(a);
            return Array.getInt64(a, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooArrayJitTest, ArraySortReverse) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 10);
            Array.pushInt64(a, 20);
            Array.pushInt64(a, 30);
            Array.sort(a);
            Array.reverse(a);
            var r = 1;
            if (Array.getInt64(a, 0) != 30) { r = 0; }
            if (Array.getInt64(a, 1) != 20) { r = 0; }
            if (Array.getInt64(a, 2) != 10) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArrayReverseInt64) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 1);
            Array.pushInt64(a, 2);
            Array.pushInt64(a, 3);
            Array.reverse(a);
            var r = 1;
            if (Array.getInt64(a, 0) != 3) { r = 0; }
            if (Array.getInt64(a, 1) != 2) { r = 0; }
            if (Array.getInt64(a, 2) != 1) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArrayReverseEmpty) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.reverse(a);
            return Array.length(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooArrayJitTest, ArrayReverseSingle) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 99);
            Array.reverse(a);
            return Array.getInt64(a, 0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 99);
}

TEST_F(HooArrayJitTest, ArrayShuffle) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 1);
            Array.pushInt64(a, 2);
            Array.pushInt64(a, 3);
            Array.shuffle(a);
            return Array.length(a);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 3);
}

TEST_F(HooArrayJitTest, ArraySortRange) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 5);
            Array.pushInt64(a, 4);
            Array.pushInt64(a, 3);
            Array.pushInt64(a, 2);
            Array.pushInt64(a, 1);
            Array.sortRange(a, 1, 4);
            var r = 1;
            if (Array.getInt64(a, 0) != 5) { r = 0; }
            if (Array.getInt64(a, 1) != 2) { r = 0; }
            if (Array.getInt64(a, 2) != 3) { r = 0; }
            if (Array.getInt64(a, 3) != 4) { r = 0; }
            if (Array.getInt64(a, 4) != 1) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArrayBinarySearch) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 10);
            Array.pushInt64(a, 20);
            Array.pushInt64(a, 30);
            Array.pushInt64(a, 40);
            var idx1 = Array.binarySearch(a, 20);
            var idx2 = Array.binarySearch(a, 25);
            if (idx1 == 1 && idx2 == -1) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
