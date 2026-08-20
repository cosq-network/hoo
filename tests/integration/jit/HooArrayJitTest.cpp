#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/generic_array/hoo_generic_array.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/runtime/hoo_runtime.h"
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

// ============================================================================
// Extended JIT Integration Tests: exhaustive coverage of the array module
// via HVMJIT compilation + C API verification.
// ============================================================================

TEST_F(HooArrayJitTest, PushChaining) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            var r = 1;
            var i: int64 = 0;
            while (i < 5) {
                if (a.getInt64(i) != i + 1) { r = 0; }
                i = i + 1;
            }
            if (a.length() != 5) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, DoubleChaining) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(1.1);
            a.pushDouble(2.2);
            a.pushDouble(3.3);
            var r = 1;
            if (a.length() != 3) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, StringChaining) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            a.pushString("!");
            if (a.length() != 3) { return 0; }
            var s1: string = a.getString(0);
            var s2: string = a.getString(1);
            var s3: string = a.getString(2);
            if (s1.length() != 5) { return 0; }
            if (s2.length() != 5) { return 0; }
            if (s3.length() != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BoolChaining) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushBool(1);
            a.pushBool(0);
            a.pushBool(1);
            if (a.length() != 3) { return 0; }
            if (a.getBool(0) != 1) { return 0; }
            if (a.getBool(1) != 0) { return 0; }
            if (a.getBool(2) != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, PushObjectGetClass) {
    const std::string source = R"(
        import hoo;
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
            func :int64 manhattan() {
                return this.x + this.y;
            }
        }
        func :int64 test() {
            var a = new Array();
            var p1 = new Point(3, 4);
            var p2 = new Point(10, 20);
            a.pushObject(p1);
            a.pushObject(p2);
            if (a.length() != 2) { return 0; }
            var r1: Point = a.getObject(0);
            var r2: Point = a.getObject(1);
            if (r1.manhattan() != 7) { return 0; }
            if (r2.manhattan() != 30) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, PushArrayNested) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var outer = new Array();
            var inner1 = new Array();
            inner1.pushInt64(10);
            inner1.pushInt64(20);
            var inner2 = new Array();
            inner2.pushInt64(30);
            inner2.pushInt64(40);
            outer.pushObject(inner1);
            outer.pushObject(inner2);
            if (outer.length() != 2) { return 0; }
            var a1: Array = outer.getObject(0);
            var a2: Array = outer.getObject(1);
            if (a1.length() != 2) { return 0; }
            if (a2.length() != 2) { return 0; }
            if (a1.getInt64(0) != 10) { return 0; }
            if (a1.getInt64(1) != 20) { return 0; }
            if (a2.getInt64(0) != 30) { return 0; }
            if (a2.getInt64(1) != 40) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, SortDouble) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(5.5);
            a.pushDouble(1.1);
            a.pushDouble(3.3);
            a.pushDouble(2.2);
            a.pushDouble(4.4);
            a.sort();
            var r = 1;
            if (a.getDouble(0) != 1.1) { r = 0; }
            if (a.getDouble(1) != 2.2) { r = 0; }
            if (a.getDouble(2) != 3.3) { r = 0; }
            if (a.getDouble(3) != 4.4) { r = 0; }
            if (a.getDouble(4) != 5.5) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, SortDoubleReverse) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(1.0);
            a.pushDouble(2.0);
            a.pushDouble(3.0);
            a.sort();
            a.reverse();
            var r = 1;
            if (a.getDouble(0) != 3.0) { r = 0; }
            if (a.getDouble(1) != 2.0) { r = 0; }
            if (a.getDouble(2) != 1.0) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BinarySearchFirstElement) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            var idx = a.binarySearch(10);
            if (idx != 0) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BinarySearchLastElement) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            var idx = a.binarySearch(30);
            if (idx != 2) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BinarySearchSingleElement) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(42);
            var idx = a.binarySearch(42);
            if (idx != 0) { return 0; }
            var idx2 = a.binarySearch(99);
            if (idx2 != -1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BinarySearchEmptyArray) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var idx = a.binarySearch(42);
            if (idx != -1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BinarySearchAfterSort) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(50);
            a.pushInt64(10);
            a.pushInt64(40);
            a.pushInt64(20);
            a.pushInt64(30);
            a.sort();
            var r = 1;
            if (a.binarySearch(10) != 0) { r = 0; }
            if (a.binarySearch(30) != 2) { r = 0; }
            if (a.binarySearch(50) != 4) { r = 0; }
            if (a.binarySearch(25) != -1) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, TypeMismatchPushInt64OnDoubleArray) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(1.0);
            a.pushInt64(42);
            if (a.length() != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1) << "Type mismatch should silently reject push";
}

TEST_F(HooArrayJitTest, TypeMismatchPushDoubleOnInt64Array) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushDouble(2.5);
            if (a.length() != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1) << "Type mismatch should silently reject push";
}

TEST_F(HooArrayJitTest, SortThenAccessAllElements) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var i: int64 = 99;
            while (i >= 0) {
                a.pushInt64(i);
                i = i - 1;
            }
            a.sort();
            var r = 1;
            i = 0;
            while (i < 100) {
                if (a.getInt64(i) != i) { r = 0; }
                i = i + 1;
            }
            if (a.length() != 100) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ReverseThenSort) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            a.pushInt64(4);
            a.pushInt64(5);
            a.reverse();
            a.sort();
            var r = 1;
            var i: int64 = 0;
            while (i < 5) {
                if (a.getInt64(i) != i + 1) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, SortRangePreservesOutside) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(100);
            a.pushInt64(90);
            a.pushInt64(80);
            a.pushInt64(70);
            a.pushInt64(60);
            a.sortRange(1, 4);
            var r = 1;
            if (a.getInt64(0) != 100) { r = 0; }
            if (a.getInt64(1) != 70) { r = 0; }
            if (a.getInt64(2) != 80) { r = 0; }
            if (a.getInt64(3) != 90) { r = 0; }
            if (a.getInt64(4) != 60) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, SortRangeFullArray) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(5);
            a.pushInt64(3);
            a.pushInt64(1);
            a.pushInt64(4);
            a.pushInt64(2);
            a.sortRange(0, 5);
            var r = 1;
            var i: int64 = 0;
            while (i < 5) {
                if (a.getInt64(i) != i + 1) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ShuffleSingleElement) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(42);
            a.shuffle();
            if (a.length() != 1) { return 0; }
            if (a.getInt64(0) != 42) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ShufflePreservesLength) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 50) {
                a.pushInt64(i);
                i = i + 1;
            }
            a.shuffle();
            if (a.length() != 50) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ShuffleThenSort) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 20) {
                a.pushInt64(i);
                i = i + 1;
            }
            a.shuffle();
            a.sort();
            var r = 1;
            i = 0;
            while (i < 20) {
                if (a.getInt64(i) != i) { r = 0; }
                i = i + 1;
            }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ClearThenReuse) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            a.pushInt64(3);
            if (a.length() != 3) { return 0; }
            a.clear();
            if (a.length() != 0) { return 0; }
            a.pushInt64(10);
            a.pushInt64(20);
            if (a.length() != 2) { return 0; }
            if (a.getInt64(0) != 10) { return 0; }
            if (a.getInt64(1) != 20) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, EmptyArrayEdgeCases) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            if (a.length() != 0) { return 0; }
            if (a.empty() != 1) { return 0; }
            a.sort();
            if (a.length() != 0) { return 0; }
            a.reverse();
            if (a.length() != 0) { return 0; }
            a.shuffle();
            if (a.length() != 0) { return 0; }
            a.sortRange(0, 0);
            if (a.length() != 0) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, SingleElementOperations) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(42);
            a.sort();
            if (a.getInt64(0) != 42) { return 0; }
            a.reverse();
            if (a.getInt64(0) != 42) { return 0; }
            a.shuffle();
            if (a.getInt64(0) != 42) { return 0; }
            a.sortRange(0, 1);
            if (a.getInt64(0) != 42) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, NegativeValuesInt64) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(-5);
            a.pushInt64(-1);
            a.pushInt64(-10);
            a.pushInt64(0);
            a.pushInt64(3);
            a.sort();
            var r = 1;
            if (a.getInt64(0) != -10) { r = 0; }
            if (a.getInt64(1) != -5) { r = 0; }
            if (a.getInt64(2) != -1) { r = 0; }
            if (a.getInt64(3) != 0) { r = 0; }
            if (a.getInt64(4) != 3) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, NegativeValuesDouble) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(-3.14);
            a.pushDouble(2.71);
            a.pushDouble(-1.0);
            a.pushDouble(0.0);
            a.sort();
            var r = 1;
            if (a.getDouble(0) != -3.14) { r = 0; }
            if (a.getDouble(1) != -1.0) { r = 0; }
            if (a.getDouble(2) != 0.0) { r = 0; }
            if (a.getDouble(3) != 2.71) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, MultipleClearAndReuse) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var round: int64 = 0;
            while (round < 5) {
                a.clear();
                var i: int64 = 0;
                while (i < 10) {
                    a.pushInt64(i + round * 10);
                    i = i + 1;
                }
                if (a.length() != 10) { return 0; }
                if (a.getInt64(0) != round * 10) { return 0; }
                if (a.getInt64(9) != round * 10 + 9) { return 0; }
                round = round + 1;
            }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, BoolArrayOperations) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushBool(1);
            a.pushBool(0);
            a.pushBool(1);
            a.pushBool(1);
            a.pushBool(0);
            if (a.length() != 5) { return 0; }
            a.reverse();
            if (a.getBool(0) != 0) { return 0; }
            if (a.getBool(1) != 1) { return 0; }
            if (a.getBool(2) != 1) { return 0; }
            if (a.getBool(3) != 0) { return 0; }
            if (a.getBool(4) != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, StringArraySortByLength) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushString("banana");
            a.pushString("apple");
            a.pushString("cherry");
            a.pushString("date");
            a.pushString("fig");
            if (a.length() != 5) { return 0; }
            var s1: string = a.getString(0);
            var s2: string = a.getString(4);
            if (s1.length() != 6) { return 0; }
            if (s2.length() != 3) { return 0; }
            a.clear();
            if (a.length() != 0) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, MixedOperationsComprehensive) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushInt64(100);
            a.pushInt64(50);
            a.pushInt64(200);
            a.pushInt64(25);
            a.pushInt64(150);
            if (a.length() != 5) { return 0; }
            if (a.empty() != 0) { return 0; }
            a.sort();
            if (a.getInt64(0) != 25) { return 0; }
            if (a.getInt64(4) != 200) { return 0; }
            a.reverse();
            if (a.getInt64(0) != 200) { return 0; }
            if (a.getInt64(4) != 25) { return 0; }
            var idx = a.binarySearch(100);
            if (idx < 0) { return 0; }
            a.sort();
            if (a.getInt64(0) != 25) { return 0; }
            if (a.getInt64(1) != 50) { return 0; }
            if (a.getInt64(2) != 100) { return 0; }
            if (a.getInt64(3) != 150) { return 0; }
            if (a.getInt64(4) != 200) { return 0; }
            a.clear();
            if (a.length() != 0) { return 0; }
            if (a.empty() != 1) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ArrayOfStringsLengthSum) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            a.pushString("foo");
            a.pushString("bar");
            var total = 0;
            var i: int64 = 0;
            while (i < 4) {
                var s: string = a.getString(i);
                total = total + s.length();
                i = i + 1;
            }
            if (total != 16) { return 0; }
            return total;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 16);
}

TEST_F(HooArrayJitTest, DoubleBinarySearch) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            a.pushDouble(1.1);
            a.pushDouble(2.2);
            a.pushDouble(3.3);
            a.pushDouble(4.4);
            var r = 1;
            if (a.getDouble(0) != 1.1) { r = 0; }
            if (a.getDouble(1) != 2.2) { r = 0; }
            if (a.getDouble(2) != 3.3) { r = 0; }
            if (a.getDouble(3) != 4.4) { r = 0; }
            return r;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, LargeInt64Array) {
    const std::string source = R"(
        import hoo;
        func :int64 test() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 1000) {
                a.pushInt64(i * 2);
                i = i + 1;
            }
            if (a.length() != 1000) { return 0; }
            if (a.getInt64(0) != 0) { return 0; }
            if (a.getInt64(999) != 1998) { return 0; }
            if (a.getInt64(500) != 1000) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooArrayJitTest, ClassInstanceArrayRoundtrip) {
    const std::string source = R"(
        import hoo;
        class Counter {
            var val: int64;
            constructor(start: int64) {
                this.val = start;
            }
            func :void increment() {
                this.val = this.val + 1;
            }
            func :int64 getVal() {
                return this.val;
            }
        }
        func :int64 test() {
            var a = new Array();
            var c1 = new Counter(10);
            var c2 = new Counter(20);
            a.pushObject(c1);
            a.pushObject(c2);
            c1.increment();
            c1.increment();
            var r1: Counter = a.getObject(0);
            var r2: Counter = a.getObject(1);
            if (r1.getVal() != 12) { return 0; }
            if (r2.getVal() != 20) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

// ============================================================================
// Hybrid tests: arrays created via JIT, verified via C API
// ============================================================================

TEST_F(HooArrayJitTest, HybridPopViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 3);

    int64_t dest = 0;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_EQ(dest, 30);
    EXPECT_EQ(hoo_array_length(arr), 2);
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_EQ(dest, 20);
    EXPECT_EQ(hoo_array_length(arr), 1);
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_EQ(dest, 10);
    EXPECT_EQ(hoo_array_length(arr), 0);
    EXPECT_EQ(hoo_array_pop(arr, &dest), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridSetViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(10);
            a.pushInt64(20);
            a.pushInt64(30);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    int64_t new_val = 999;
    EXPECT_EQ(hoo_array_set(arr, 1, &new_val), 1);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(arr, 0, &dest), 1);
    EXPECT_EQ(dest, 10);
    EXPECT_EQ(hoo_array_get_int64(arr, 1, &dest), 1);
    EXPECT_EQ(dest, 999);
    EXPECT_EQ(hoo_array_get_int64(arr, 2, &dest), 1);
    EXPECT_EQ(dest, 30);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeInt64) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_STREQ(hoo_array_element_type(arr), "int64");
    EXPECT_EQ(hoo_array_is_type(arr, "int64"), 1);
    EXPECT_EQ(hoo_array_is_type(arr, "double"), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeDouble) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushDouble(1.0);
            a.pushDouble(2.0);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_STREQ(hoo_array_element_type(arr), "double");
    EXPECT_EQ(hoo_array_is_type(arr, "double"), 1);
    EXPECT_EQ(hoo_array_is_type(arr, "int64"), 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeBool) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushBool(1);
            a.pushBool(0);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 2);
    int64_t bdest = 0;
    EXPECT_EQ(hoo_array_get_bool(arr, 0, &bdest), 1);
    EXPECT_EQ(bdest, 1);
    EXPECT_EQ(hoo_array_get_bool(arr, 1, &bdest), 1);
    EXPECT_EQ(bdest, 0);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeString) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushString("hello");
            a.pushString("world");
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 2);
    const char* dest = nullptr;
    EXPECT_EQ(hoo_array_get_string(arr, 0, &dest), 1);
    EXPECT_STREQ(dest, "hello");
    EXPECT_EQ(hoo_array_get_string(arr, 1, &dest), 1);
    EXPECT_STREQ(dest, "world");
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeObject) {
    const std::string source = R"(
        import hoo;
        class Foo {
            var x: int64;
            constructor(v: int64) { this.x = v; }
        }
        func :ptr test() {
            var a = new Array();
            var f1 = new Foo(1);
            var f2 = new Foo(2);
            a.pushObject(f1);
            a.pushObject(f2);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 2);
    void* obj = nullptr;
    EXPECT_EQ(hoo_array_get_object(arr, 0, &obj), 1);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(hoo_array_get_object(arr, 1, &obj), 1);
    EXPECT_NE(obj, nullptr);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridElementTypeEmpty) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            return new Array();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_element_type(arr), nullptr);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridPopManagedElement) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushString("alpha");
            a.pushString("beta");
            a.pushString("gamma");
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 3);

    void* dest = nullptr;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    ASSERT_NE(dest, nullptr);
    EXPECT_STREQ(reinterpret_cast<const char*>(dest), "gamma");
    EXPECT_EQ(hoo_array_length(arr), 2);

    dest = nullptr;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_STREQ(reinterpret_cast<const char*>(dest), "beta");
    EXPECT_EQ(hoo_array_length(arr), 1);

    dest = nullptr;
    EXPECT_EQ(hoo_array_pop(arr, &dest), 1);
    EXPECT_STREQ(reinterpret_cast<const char*>(dest), "alpha");
    EXPECT_EQ(hoo_array_length(arr), 0);

    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridRepeatViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(42);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    int64_t val = 7;
    HooArray repeated = hoo_array_repeat(&val, 5);
    ASSERT_NE(repeated, nullptr);
    EXPECT_EQ(hoo_array_length(repeated), 5);
    int64_t dest = 0;
    for (int64_t i = 0; i < 5; i++) {
        EXPECT_EQ(hoo_array_get_int64(repeated, i, &dest), 1);
        EXPECT_EQ(dest, 7);
    }
    hoo_array_release(repeated);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridFromBufferViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            return new Array();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    int64_t data[] = {100, 200, 300};
    HooArray from_buf = hoo_array_from_buffer(data, 3);
    ASSERT_NE(from_buf, nullptr);
    EXPECT_EQ(hoo_array_length(from_buf), 3);
    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(from_buf, 0, &dest), 1);
    EXPECT_EQ(dest, 100);
    EXPECT_EQ(hoo_array_get_int64(from_buf, 1, &dest), 1);
    EXPECT_EQ(dest, 200);
    EXPECT_EQ(hoo_array_get_int64(from_buf, 2, &dest), 1);
    EXPECT_EQ(dest, 300);
    hoo_array_release(from_buf);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridBinarySearchDoubleViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushDouble(1.1);
            a.pushDouble(2.2);
            a.pushDouble(3.3);
            a.pushDouble(4.4);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 4);

    EXPECT_EQ(hoo_array_binary_search_double(arr, 1.1), 0);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 3.3), 2);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 4.4), 3);
    EXPECT_EQ(hoo_array_binary_search_double(arr, 2.5), -1);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridSortRangeViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(50);
            a.pushInt64(40);
            a.pushInt64(30);
            a.pushInt64(20);
            a.pushInt64(10);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    hoo_array_sort_range(arr, 1, 4);
    int64_t dest = 0;
    int64_t expected[] = {50, 20, 30, 40, 10};
    for (int64_t i = 0; i < 5; i++) {
        EXPECT_EQ(hoo_array_get_int64(arr, i, &dest), 1);
        EXPECT_EQ(dest, expected[i]) << "mismatch at index " << i;
    }
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridShuffleViaCapi) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            var i: int64 = 0;
            while (i < 100) {
                a.pushInt64(i);
                i = i + 1;
            }
            a.shuffle();
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 100);

    int64_t dest = 0;
    int64_t sum = 0;
    int64_t i = 0;
    while (i < 100) {
        EXPECT_EQ(hoo_array_get_int64(arr, i, &dest), 1);
        sum += dest;
        i = i + 1;
    }
    EXPECT_EQ(sum, 4950);
    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridRetainReleaseRefcount) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(1);
            a.pushInt64(2);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    int64_t rc1 = hoo_array_refcount(arr);
    hoo_array_retain(arr);
    int64_t rc2 = hoo_array_refcount(arr);
    EXPECT_EQ(rc2, rc1 + 1);

    hoo_array_release(arr);
    int64_t rc3 = hoo_array_refcount(arr);
    EXPECT_EQ(rc3, rc1);

    hoo_array_release(arr);
}

TEST_F(HooArrayJitTest, HybridPushVectorInt64) {
    const std::string source = R"(
        import hoo;
        func :ptr test() {
            var a = new Array();
            a.pushInt64(0);
            return a;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    HooArray arr = (HooArray)jit.run("_F_M_test_E_test_p");
    ASSERT_NE(arr, nullptr);

    int64_t batch[] = {10, 20, 30, 40, 50};
    HooArray result = hoo_array_push_vector_int64(arr, batch, 5);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 6);

    int64_t dest = 0;
    EXPECT_EQ(hoo_array_get_int64(result, 0, &dest), 1);
    EXPECT_EQ(dest, 0);
    for (int64_t i = 0; i < 5; i++) {
        EXPECT_EQ(hoo_array_get_int64(result, i + 1, &dest), 1);
        EXPECT_EQ(dest, batch[i]);
    }
    hoo_array_release(result);
}
