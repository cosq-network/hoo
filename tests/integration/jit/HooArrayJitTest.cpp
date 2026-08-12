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
            a.pushDouble(3.14);
            return a.getDouble(0);
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
            a.pushInt64(42);
            return a.getInt64(0);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooArrayJitTest, StaticArrayMethodsRejected) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            Array.pushInt64(a, 42);
            return 0;
        }
    )";
    EXPECT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_NE(jit.getLastError().find("not supported as a static method"), std::string::npos);
}

TEST_F(HooArrayJitTest, ArrayLength) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
return a.length();
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
            a.pushInt64(10);
            a.clear();
            return a.length();
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
            return a.empty();
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
            a.pushString("hello");
            var s: string = a.getString(0);
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
            a.pushBool(1);
            return a.getBool(0);
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
            a.pushInt64(5);
            a.pushInt64(3);
            a.pushInt64(9);
            a.pushInt64(1);
            a.sort();
            var r = 1;
            if (a.getInt64(0) != 1) { r = 0; }
            if (a.getInt64(1) != 3) { r = 0; }
            if (a.getInt64(2) != 5) { r = 0; }
            if (a.getInt64(3) != 9) { r = 0; }
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
            a.sort();
            return a.length();
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
            a.pushInt64(42);
            a.sort();
            return a.getInt64(0);
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
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            a.sort();
            a.reverse();
            var r = 1;
            if (a.getInt64(0) != 30) { r = 0; }
            if (a.getInt64(1) != 20) { r = 0; }
            if (a.getInt64(2) != 10) { r = 0; }
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
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.reverse();
            var r = 1;
            if (a.getInt64(0) != 3) { r = 0; }
            if (a.getInt64(1) != 2) { r = 0; }
            if (a.getInt64(2) != 1) { r = 0; }
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
            a.reverse();
            return a.length();
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
            a.pushInt64(99);
            a.reverse();
            return a.getInt64(0);
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
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.shuffle();
            return a.length();
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
            a.pushInt64(5);
            a.pushInt64(4);
            a.pushInt64(3);
            a.pushInt64(2);
            a.pushInt64(1);
            a.sortRange(1, 4);
            var r = 1;
            if (a.getInt64(0) != 5) { r = 0; }
            if (a.getInt64(1) != 2) { r = 0; }
            if (a.getInt64(2) != 3) { r = 0; }
            if (a.getInt64(3) != 4) { r = 0; }
            if (a.getInt64(4) != 1) { r = 0; }
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
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            a.pushInt64(40);
            var idx1 = a.binarySearch(20);
            var idx2 = a.binarySearch(25);
            if (idx1 == 1 && idx2 == -1) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}
