#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include "src/hvm/HVMJIT.h"
#include "src/core/DefaultIOProvider.h"

using namespace hooc;

class IntegerTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        io = std::make_unique<DefaultIOProvider>();
        jit = std::make_unique<HVMJIT>(*io);
    }

    std::unique_ptr<IOProvider> io;
    std::unique_ptr<HVMJIT> jit;
};

// ============================================================================
// INT64 TESTS
// ============================================================================

TEST_F(IntegerTypesTest, Int64_ReturnConstant) {
    std::string code = R"(
        import hoo;
        func :int64 test() { return 42; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_VariableAssignment) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 100; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 100) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Addition) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 10; var b: int64 = 32; return a + b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Subtraction) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 100; var b: int64 = 58; return a - b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Multiplication) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 6; var b: int64 = 7; return a * b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Division) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 210; var b: int64 = 5; return a / b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Modulo) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 100; var b: int64 = 58; return a % b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_CompoundAssignment) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 10; x = x + 32; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_ComparisonEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 42; var b: int64 = 42; if (a == b) { return 1; } return 0; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_ComparisonNotEquals) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 42; var b: int64 = 43; if (a != b) { return 1; } return 0; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_ComparisonLessThan) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 10; var b: int64 = 42; if (a < b) { return 1; } return 0; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_ComparisonGreaterThan) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var a: int64 = 100; var b: int64 = 42; if (a > b) { return 1; } return 0; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 1) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_Negation) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = -42; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), -42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_UnaryMinus) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 42; return -x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), -42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Int64_ComplexExpression) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var x: int64 = 10; var y: int64 = 5; return (x + y) * (x - y); }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 75) << jit->getLastError();
}

// ============================================================================
// TYPE CONVERSION TESTS
// ============================================================================

TEST_F(IntegerTypesTest, Convert_ByteToInt64) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var b: byte = 42; var x: int64 = b; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}

TEST_F(IntegerTypesTest, Convert_Int8ToInt64) {
    std::string code = R"(
        import hoo;
        func :int64 test() { var b: int8 = 42; var x: int64 = b; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 42) << jit->getLastError();
}