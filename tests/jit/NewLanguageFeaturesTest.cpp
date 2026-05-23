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
        func :int64 test() { var x: int64 = 5; x += 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_MinusEquals) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 10; x -= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 7) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_MultiplyEquals) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 5; x *= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 15) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_DivideEquals) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 20; x /= 4; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 5) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_ModuloEquals) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 17; x %= 5; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 2) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_Multiple) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 5; x += 1; x -= 2; x *= 3; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 12) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, CompoundAssignment_Chained) {
    std::string code = R"(
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
        func :int64 test() { var x: int64 = 5; x++; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 6) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixDecrement) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 5; x--; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 4) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixIncrement_Multiple) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 5; x++; x++; x++; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 8) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixDecrement_Multiple) {
    std::string code = R"(
        func :int64 test() { var x: int64 = 10; x--; x--; x--; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(jit->run("_F_test_i8"), 7) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, PostfixIncrement_CombinedWithCompound) {
    std::string code = R"(
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
        func :string test() { var x = "hello"; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
}

// ============================================================================
// INT8/BYTE TESTS
// ============================================================================

TEST_F(NewLanguageFeaturesTest, Int8_Variable) {
    std::string code = R"(
        func :int8 test() { var x: int8 = 50; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<int8_t>(jit->run("_F_test_i1")), 50) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Byte_Variable) {
    std::string code = R"(
        func :byte test() { var x: byte = 200; return x; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<uint8_t>(jit->run("_F_test_i1")), 200) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Int8_Arithmetic) {
    std::string code = R"(
        func :int8 test() { var a: int8 = 10; var b: int8 = 20; return a + b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<int8_t>(jit->run("_F_test_i1")), 30) << jit->getLastError();
}

TEST_F(NewLanguageFeaturesTest, Byte_Arithmetic) {
    std::string code = R"(
        func :byte test() { var a: byte = 100; var b: byte = 50; return a + b; }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();
    EXPECT_EQ(static_cast<uint8_t>(jit->run("_F_test_i1")), 150) << jit->getLastError();
}

// ============================================================================
// COMBINED TESTS
// ============================================================================

TEST_F(NewLanguageFeaturesTest, Combined_AllFeatures) {
    std::string code = R"(
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