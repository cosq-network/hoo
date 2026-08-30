#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>

#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/string/hoo_string.h"
#include "runtime/lib/generic_array/hoo_generic_array.h"

using namespace hooc;

class BugFixVerificationTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
    }

    bool loadCode(const std::string& code) {
        return jit->loadSourceCode("test", code);
    }

    int64_t runEntry(const std::string& entry = "_F_test_i8") {
        return jit->run(entry);
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

// ============================================================================
// Issue #008 - Short-circuit evaluation (&& and ||)
// ============================================================================

TEST_F(BugFixVerificationTest, ShortCircuit_And_ReturnsCorrectValue) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: int64 = false && true;
            var b: int64 = true && false;
            var c: int64 = true && true;
            return a + b * 10 + c * 100;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 100) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_Or_ReturnsCorrectValue) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: int64 = false || false;
            var b: int64 = false || true;
            var c: int64 = true || false;
            return a + b * 10 + c * 100;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 110) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_And_LeftFalseSkipsRecursion) {
    std::string code = R"(
        import hoo;
        func :bit loop() { return loop(); }
        func :int64 test() {
            var r: bit = false && loop();
            return 1;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_And_LeftTrueEvaluatesRight) {
    std::string code = R"(
        import hoo;
        func :bit returnsTrue() { return 1b; }
        func :int64 test() {
            var r: bit = true && returnsTrue();
            return r;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_Or_LeftTrueSkipsRecursion) {
    std::string code = R"(
        import hoo;
        func :bit loop() { return loop(); }
        func :int64 test() {
            var r: bit = true || loop();
            return 1;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_Or_LeftFalseEvaluatesRight) {
    std::string code = R"(
        import hoo;
        func :bit returnsTrue() { return 1b; }
        func :int64 test() {
            var r: bit = false || returnsTrue();
            return r;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_ChainedAndOr) {
    std::string code = R"(
        import hoo;
        func :bit loop() { return loop(); }
        func :int64 test() {
            var r: bit = (false && loop()) || (true || loop());
            return 1;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, ShortCircuit_AndInIfCondition) {
    std::string code = R"(
        import hoo;
        func :bit loop() { return loop(); }
        func :int64 test() {
            if (false && loop()) { return 0; }
            return 1;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

// ============================================================================
// Issue #009 - String concatenation with + operator
// ============================================================================

TEST_F(BugFixVerificationTest, StringConcat_TwoStrings) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "hello " + "world";
            return s.equals("hello world");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_Chained) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "a" + "b" + "c";
            return s.equals("abc");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_LengthOfResult) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "abc" + "def";
            return s.length();
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 6) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringLiteral_EmbeddedNulPreserved) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "a\0b";
            return s.length();
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 3) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringLiteral_EmbeddedNulInInterpolatedPart) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "x\0yz";
            return s.length();
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 4) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_Int64RightOperand) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "n=" + 42;
            return s.equals("n=42");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_Int64LeftOperand) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = 7 + "!";
            return s.equals("7!");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_DoubleOperand) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "d=" + 3.5;
            return s.equals("d=3.5");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, StringConcat_BoolOperand) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "t=" + true;
            return s.equals("t=true");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

// ============================================================================
// Issue #013 - Unicode escape sequences in string literals
// ============================================================================

TEST_F(BugFixVerificationTest, UnicodeEscape_U0048IsH) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "\u0048";
            return s.equals("H");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnicodeEscape_HelloInUnicode) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "\u0048\u0065\u006C\u006C\u006F";
            return s.equals("Hello");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnicodeEscape_Length) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "\u0048\u0065\u006C\u006C\u006F";
            return s.length();
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 5) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnicodeEscape_XHexEscape) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "h\x65llo";
            return s.equals("hello");
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnicodeEscape_VerticalTab) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var s = "a\vb";
            return s.length();
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 3) << jit->getLastError();
}

// ============================================================================
// Issue #029 - Unsigned comparisons for byte type
// ============================================================================

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteGt) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 200;
            var b: byte = 100;
            if (a > b) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteLt) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 50;
            var b: byte = 200;
            if (a < b) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteGte) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 200;
            var b: byte = 200;
            if (a >= b) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteLte) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 100;
            var b: byte = 200;
            if (a <= b) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteLargeVsSmall) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 250;
            var b: byte = 10;
            return a > b;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteZeroVsLarge) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 0;
            var b: byte = 255;
            return a < b;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteMaxBoundary) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 200;
            var b: byte = 199;
            if (a > b) { return 1; }
            return 0;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteEquality) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 200;
            var b: byte = 200;
            return a == b;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, UnsignedCmp_ByteInequality) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var a: byte = 200;
            var b: byte = 201;
            return a != b;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_test_i8"), 1) << jit->getLastError();
}

// ============================================================================
// Issue #007 - ARC memory leak fix in new expression
// ============================================================================

TEST_F(BugFixVerificationTest, NewExpression_SimpleObject) {
    std::string code = R"(
        import hoo;
        class Widget {
            var value: int64;
            constructor() { this.value = 42; }
        }
        func :int64 test() {
            var w = new Widget();
            return w.value;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 42) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, NewExpression_MultipleObjects) {
    std::string code = R"(
        import hoo;
        class Point {
            var x: int64;
            var y: int64;
            constructor(xv: int64, yv: int64) {
                this.x = xv;
                this.y = yv;
            }
        }
        func :int64 test() {
            var a = new Point(10, 20);
            var b = new Point(30, 40);
            return a.x + a.y + b.x + b.y;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 100) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, NewExpression_WithMethodCall) {
    std::string code = R"(
        import hoo;
        class Counter {
            var count: int64;
            constructor() { this.count = 0; }
            func :void inc() { this.count = this.count + 1; }
        }
        func :int64 test() {
            var c = new Counter();
            c.inc();
            c.inc();
            c.inc();
            return c.count;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 3) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, NewExpression_MultipleObjectsWithMethods) {
    std::string code = R"(
        import hoo;
        class Accum {
            var total: int64;
            constructor() { this.total = 0; }
            func :Accum add(x: int64) {
                this.total = this.total + x;
                return this;
            }
        }
        func :int64 test() {
            var a = new Accum();
            var b = new Accum();
            a.add(5);
            a.add(10);
            b.add(20);
            return a.total + b.total;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 35) << jit->getLastError();
}

// ============================================================================
// Issue #016 - JIT memory corruption safeguards
// ============================================================================

TEST_F(BugFixVerificationTest, JitMemory_MutualRecursion) {
    std::string code = R"(
        import hoo;
        func :int64 isEven(n: int64) {
            if (n == 0) { return 1; }
            return isOdd(n - 1);
        }
        func :int64 isOdd(n: int64) {
            if (n == 0) { return 0; }
            return isEven(n - 1);
        }
        func :int64 test() {
            return isEven(10);
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 1) << jit->getLastError();
}

// ============================================================================
// ARC ownership of string/object array elements
// ============================================================================

TEST_F(BugFixVerificationTest, ArrayLiteralStringElementsOwned) {
    std::string code = R"(
        import hoo;
        func :ptr test() {
            var a = ["hello", "world"];
            return a;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    int64_t r = runEntry("_F_M_test_E_test_p");
    ASSERT_GT(r, 0) << jit->getLastError();
    HooArray a = (HooArray)r;
    EXPECT_EQ(hoo_array_length(a), 2);
    EXPECT_EQ(((int64_t*)a)[2], HOO_TYPE_STRING);
    HooString s0 = nullptr, s1 = nullptr;
    ASSERT_TRUE(hoo_array_get_object(a, 0, (void**)&s0));
    ASSERT_TRUE(hoo_array_get_object(a, 1, (void**)&s1));
    EXPECT_STREQ("hello", hoo_string_data(s0));
    EXPECT_STREQ("world", hoo_string_data(s1));
    EXPECT_EQ(hoo_get_refcount(s0), 1);
    EXPECT_EQ(hoo_get_refcount(s1), 1);
    int64_t rc = hoo_get_refcount(a);
    for (int64_t i = 0; i < rc; ++i) {
        hoo_array_release(a);
    }
    EXPECT_EQ(hoo_is_managed_object(s0), 0);
    EXPECT_EQ(hoo_is_managed_object(s1), 0);
}

TEST_F(BugFixVerificationTest, MapStringKeysAreValidStrings) {
    std::string code = R"(
        import hoo;
        func :int64 test() {
            var m: map<string, int64> = new Map(4, 1);
            m.setStringInt64("aa", 1);
            m.setStringInt64("bb", 2);
            var total: int64 = 0;
            for key in m {
                total = total + key.length();
            }
            return total;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    EXPECT_EQ(runEntry("_F_M_test_E_test_i8"), 4) << jit->getLastError();
}

TEST_F(BugFixVerificationTest, RegexSplitPartsAreValidStrings) {
    std::string code = R"(
        import hoo;
        import hoo.regex;
        func :ptr test() {
            var parts = regex_split(",", "a,b,c");
            return parts;
        }
    )";
    ASSERT_TRUE(loadCode(code)) << jit->getLastError();
    int64_t r = runEntry("_F_M_test_E_test_p");
    ASSERT_GT(r, 0) << jit->getLastError();
    HooArray a = (HooArray)r;
    EXPECT_EQ(hoo_array_length(a), 3);
    EXPECT_EQ(((int64_t*)a)[2], HOO_TYPE_STRING);
    HooString s0 = nullptr, s1 = nullptr, s2 = nullptr;
    ASSERT_TRUE(hoo_array_get_object(a, 0, (void**)&s0));
    ASSERT_TRUE(hoo_array_get_object(a, 1, (void**)&s1));
    ASSERT_TRUE(hoo_array_get_object(a, 2, (void**)&s2));
    EXPECT_STREQ("a", hoo_string_data(s0));
    EXPECT_STREQ("b", hoo_string_data(s1));
    EXPECT_STREQ("c", hoo_string_data(s2));
    int64_t rc2 = hoo_get_refcount(a);
    for (int64_t i = 0; i < rc2; ++i) {
        hoo_array_release(a);
    }
    EXPECT_EQ(hoo_is_managed_object(s0), 0);
    EXPECT_EQ(hoo_is_managed_object(s1), 0);
    EXPECT_EQ(hoo_is_managed_object(s2), 0);
}
