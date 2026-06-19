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
        func :int64 test() {
            var dt = datetime_now();
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Should be a positive timestamp (after epoch)
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000000);
}

TEST_F(HooDatetimeJitTest, NowSeconds) {
    const std::string source = R"(
        func :int64 test() { return datetime_nowSeconds(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000);
}

TEST_F(HooDatetimeJitTest, Iso8601) {
    const std::string source = R"(
        func :int64 test() {
            var ts = datetime_now();
            var str = datetime_iso8601(ts);
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // ISO 8601 like "2024-01-15T10:30:00.000Z" is 24 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 24);
}

TEST_F(HooDatetimeJitTest, ParseIso8601) {
    const std::string source = R"(
        func :int64 test() { return datetime_fromIso8601("2024-01-15T10:30:00Z"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooDatetimeJitTest, AddDays) {
    const std::string source = R"(
        func :int64 test() {
            var base = datetime_fromIso8601("2024-01-01T00:00:00Z");
            var result = base.addDays(1);
            return result.getTimestamp();
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
            var a = datetime_fromIso8601("2024-01-01T00:00:00Z");
            var b = datetime_fromIso8601("2024-06-15T00:00:00Z");
            return datetime_compare(a, b);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), -1);
}

TEST_F(HooDatetimeJitTest, Format) {
    const std::string source = R"(
        func :int64 test() {
            var ts = datetime_fromIso8601("2024-01-15T10:30:00Z");
            var str = datetime_format(ts, "%Y-%m-%d");
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
            var dt = datetime_parse("2024-06-15", "%Y-%m-%d");
            return dt.getTimestamp();
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
            var ts = datetime_fromIso8601("2024-01-15T10:30:00Z");
            var str = datetime_format(ts, "Hello");
            return str.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // Literal format "Hello" produces "Hello" = 5 chars
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooDatetimeJitTest, FreeFuncNow) {
    const std::string source = R"(
        func :int64 test() {
            var dt = datetime_now();
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000000);
}

TEST_F(HooDatetimeJitTest, FreeFuncNew) {
    const std::string source = R"(
        func :int64 test() {
            var dt = datetime_new(1704067200000);
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1704067200000);
}

TEST_F(HooDatetimeJitTest, FreeFuncParse) {
    const std::string source = R"(
        func :int64 test() {
            var dt = datetime_parse("2024-06-15", "%Y-%m-%d");
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    int64_t result = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(result, 1700000000000);
}

TEST_F(HooDatetimeJitTest, FreeFuncIso8601) {
    const std::string source = R"(
        func :int64 test() {
            var dt = datetime_fromIso8601("2024-01-01T00:00:00Z");
            return dt.getTimestamp();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooDatetimeJitTest, FreeFuncNowSeconds) {
    const std::string source = R"(
        func :int64 test() { return datetime_nowSeconds(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000);
}

TEST_F(HooDatetimeJitTest, FreeFuncNowPrecise) {
    const std::string source = R"(
        func :int64 test() { return datetime_nowPrecise(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_GT(jit.run("_F_M_test_E_test_i8"), 1700000000);
}

TEST_F(HooDatetimeJitTest, BlockedStaticCall) {
    const std::string source = R"(
        func :int64 test() {
            var dt = DateTime.now();
            return 1;
        }
    )";
    ASSERT_FALSE(jit.loadSourceCode("test", source));
    EXPECT_TRUE(jit.getLastError().find("DateTime.now is not supported as a static method; use free function datetime_now()") != std::string::npos);
}
