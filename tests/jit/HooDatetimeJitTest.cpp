#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"

using namespace hooc;

class HooDatetimeJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooDatetimeJitTest, Now) {
    const std::string source = R"(
        func :int64 test() { return DateTime.now(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Should be a positive timestamp (after epoch)
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000000);
}

TEST_F(HooDatetimeJitTest, NowSeconds) {
    const std::string source = R"(
        func :int64 test() { return DateTime.nowSeconds(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000);
}

TEST_F(HooDatetimeJitTest, Iso8601) {
    const std::string source = R"(
        func :int64 test() {
            var ts = DateTime.now();
            var str = DateTime.iso8601(ts);
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // ISO 8601 like "2024-01-15T10:30:00.000Z" is 24 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 24);
}

TEST_F(HooDatetimeJitTest, ParseIso8601) {
    const std::string source = R"(
        func :int64 test() { return DateTime.fromIso8601("2024-01-15T10:30:00Z"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooDatetimeJitTest, AddDays) {
    const std::string source = R"(
        func :int64 test() {
            var base = DateTime.fromIso8601("2024-01-01T00:00:00Z");
            return DateTime.addDays(base, 1);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Jan 2 = Jan 1 + 1 day (86400000 ms)
    int64_t result = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(result, 1704067200000);
}

TEST_F(HooDatetimeJitTest, Compare) {
    const std::string source = R"(
        func :int64 test() {
            var a = DateTime.fromIso8601("2024-01-01T00:00:00Z");
            var b = DateTime.fromIso8601("2024-06-15T00:00:00Z");
            return DateTime.compare(a, b);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1);
}

TEST_F(HooDatetimeJitTest, Format) {
    const std::string source = R"(
        func :int64 test() {
            var ts = DateTime.fromIso8601("2024-01-15T10:30:00Z");
            var str = DateTime.format(ts, "%Y-%m-%d");
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // "%Y-%m-%d" produces "2024-01-15" = 10 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 10);
}

TEST_F(HooDatetimeJitTest, ParseCustom) {
    const std::string source = R"(
        func :int64 test() {
            var ts = DateTime.parse("2024-06-15", "%Y-%m-%d");
            return ts;
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // 2024-06-15 00:00:00 UTC in ms
    int64_t result = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(result, 1700000000000);
}

TEST_F(HooDatetimeJitTest, FormatLiteral) {
    const std::string source = R"(
        func :int64 test() {
            var ts = DateTime.fromIso8601("2024-01-15T10:30:00Z");
            var str = DateTime.format(ts, "Hello");
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Literal format "Hello" produces "Hello" = 5 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}
