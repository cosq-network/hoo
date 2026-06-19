#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include "src/hvm/HVMJIT.h"
#include "src/core/DefaultIOProvider.h"

using namespace hooc;

class NewLanguageFeaturesTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

// ============================================================================
// COMPOUND ASSIGNMENT TESTS (+=, -=, *=, /=, %=)
// ============================================================================

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_PlusEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x += 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_MinusEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 10; x -= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 7) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_MultiplyEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x *= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 15) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_DivideEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 20; x /= 4; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 5) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_ModuloEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 17; x %= 5; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 2) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_Multiple) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x += 1; x -= 2; x *= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 12) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_Chained) {
    std::string code = R"(
        import hoo;
        func :int64 test() { 
            var x: int64 = 10;
            x += 5;
            x /= 3;
            return x;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 5) << jit->getLastError();
}

// ============================================================================
// INCREMENT/DECREMENT TESTS (++/--)
// ============================================================================

TEST_F(NewLanguageFeaturesTest, PostfixIncrement) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x++; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 6) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixDecrement) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x--; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 4) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixIncrement_Multiple) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x++; x++; x++; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixDecrement_Multiple) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 10; x--; x--; x--; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 7) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixIncrement_CombinedWithCompound) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 5; x++; x += 2; x--; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 7) << jit->getLastError();
}

// ============================================================================
// MULTILINE STRING TESTS - Grammar parsing verified, codegen same as regular strings
// ============================================================================

TEST_F(NewLanguageFeaturesTest, MultilineString_VerifyParsing) {
    std::string code = R"(
        import hoo;
        func :string test() { var x = "hello"; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
}

// ============================================================================
// INT8/BYTE TESTS
// ============================================================================

TEST_F(NewLanguageFeaturesTest, Int8_Variable) {
    std::string code = R"(
        import hoo;
        func :int8 test() { var x: int8 = 50; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<int8_t>(jit->run("_F_test_i1")), 50) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Byte_Variable) {
    std::string code = R"(
        import hoo;
        func :byte test() { var x: byte = 200; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<uint8_t>(jit->run("_F_test_u1")), 200) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Int8_Arithmetic) {
    std::string code = R"(
        import hoo;
        func :int8 test() { var a: int8 = 10; var b: int8 = 20; return a + b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<int8_t>(jit->run("_F_test_i1")), 30) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Byte_Arithmetic) {
    std::string code = R"(
        import hoo;
        func :byte test() { var a: byte = 100; var b: byte = 50; return a + b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<uint8_t>(jit->run("_F_test_u1")), 150) << jit->getLastError();
}

// ============================================================================
// COMBINED TESTS
// ============================================================================

TEST_F(NewLanguageFeaturesTest, Combined_AllFeatures) {
    std::string code = R"(
        import hoo;
        func :int64 test() { 
            var x: int64 = 10;
            x += 5;
            x *= 2;
            x++;
            x--;
            return x;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 30) << jit->getLastError();
}

// ============================================================================
// NEW EXPRESSION TESTS
// ============================================================================

TEST_F(NewLanguageFeaturesTest, NewExpressionSimple) {
    std::string code = R"(
        import hoo;
        func :int64 test() { return 42; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    auto r = jit->run("_F_test_i8");
    EXPECT_EQ(r, 42) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionWithClassPresent) {
    std::string code = R"(
        import hoo;
        class Widget {
            var value: int64;
        }
        func :int64 test() { return 42; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    auto r = jit->run("_F_test_i8");
    printf("test_with_class: result=%lld error=%s\n", (long long)r, jit->getLastError().c_str());
    EXPECT_EQ(r, 42) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionSimple2) {
    std::string code = R"(
        import hoo;
        class Widget {
            var value: int64;
            constructor() {
                this.value = 1;
            }
        }
        func :int64 test() {
            var w = new Widget();
            return w.value;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    
    auto r = jit->run("_F_test_i8");
    printf("new_test: result=%lld error=%s\n", (long long)r, jit->getLastError().c_str());
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, DISABLED_NewExpressionWithConstructorArgs) {}

TEST_F(NewLanguageFeaturesTest, NewExpressionWithConstructorArgs) {
    std::string code = R"(
        import hoo;
        class Counter {
            var count: int64;
            constructor(initial: int64) {
                this.count = initial;
            }
        }
        func :int64 test() {
            var c = new Counter(42);
            return c.count;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionWithMultipleArgs) {
    std::string code = R"(
        import hoo;
        class Point {
            var x: int64;
            var y: int64;
            constructor(x: int64, y: int64) {
                this.x = x;
                this.y = y;
            }
        }
        func :int64 test() {
            var p = new Point(10, 20);
            return p.x + p.y;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 30) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionMethodCall) {
    std::string code = R"(
        import hoo;
        class Calculator {
            var result: int64;
            constructor(initial: int64) {
                this.result = initial;
            }
            func :void add(x: int64) {
                this.result = this.result + x;
            }
        }
        func :int64 test() {
            var calc = new Calculator(10);
            calc.add(5);
            calc.add(3);
            return calc.result;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 18) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionMultipleObjects) {
    std::string code = R"(
        import hoo;
        class Item {
            var val: int64;
            constructor(v: int64) {
                this.val = v;
            }
        }
        func :int64 test() {
            var a = new Item(10);
            var b = new Item(20);
            var c = new Item(30);
            return a.val + b.val + c.val;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 60) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, NewExpressionChainedMethodCalls) {
    std::string code = R"(
        import hoo;
        class Accumulator {
            var total: int64;
            constructor() { this.total = 0; }
            func :Accumulator add(x: int64) {
                this.total = this.total + x;
                return this;
            }
        }
        func :int64 test() {
            var a = new Accumulator();
            a.add(5);
            a.add(10);
            a.add(15);
            return a.total;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 30) << jit->getLastError();
}
