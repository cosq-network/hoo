#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_string.h"

using namespace hooc;

class HooClassApiTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooClassApiTest, StringLengthMethod) {
    const std::string source = R"(
        func :int64 test() {
            var s = "hello";
            return s.length();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooClassApiTest, StringConcatMethod) {
    const std::string source = R"(
        func :string test() {
            var a = "hello ";
            var b = "world";
            return a.concat(b);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("hello world", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooClassApiTest, StringIsEmptyMethod) {
    const std::string source = R"(
        func :int64 test() {
            var s = "";
            return s.is_empty();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooClassApiTest, StaticDateTimeNow) {
    const std::string source = R"(
        func :int64 test() {
            return DateTime.now();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    // Now should return a non-zero timestamp
    EXPECT_GT(r, 1000000);
}

TEST_F(HooClassApiTest, StaticMathAbs) {
    const std::string source = R"(
        func :int64 test() {
            return Math.abs(-42);
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 42);
}

TEST_F(HooClassApiTest, StaticMathGetPi) {
    const std::string source = R"(
        func :int64 test() {
            return Math.get_pi();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    // get_pi returns a double; just verify it's non-zero
    EXPECT_NE(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooClassApiTest, StaticDateTimeNowSeconds) {
    const std::string source = R"(
        func :int64 test() {
            return DateTime.now_seconds();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    EXPECT_GT(r, 1000000);
}

TEST_F(HooClassApiTest, StaticSystemHostname) {
    const std::string source = R"(
        func :string test() {
            return System.hostname();
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_GT(hoo_string_length(s), 0);
    hoo_string_release(s);
}

TEST_F(HooClassApiTest, StaticFsExists) {
    const std::string source = R"(
        func :int64 test() {
            return Fs.exists(".");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooClassApiTest, StaticRegexCompile) {
    const std::string source = R"(
        func :int64 test() {
            return Regex.compile("[a-z]+");
        }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_i8");
    // compile should return a non-zero pointer value
    EXPECT_NE(r, 0);
}
