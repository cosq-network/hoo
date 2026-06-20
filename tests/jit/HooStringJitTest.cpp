#include <gtest/gtest.h>
#include "hvm/HVMJIT.h"
#include "core/DefaultIOProvider.h"
#include "runtime/lib/hoo_string.h"
#include <cstring>

using namespace hooc;

class HooStringJitTest : public ::testing::Test {
protected:
    DefaultIOProvider io;
    HVMJIT jit{io};
};

TEST_F(HooStringJitTest, NewString) {
    const std::string source = R"(
        import hoo;
        func :string test() { return ""; }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, FromCStr) {
    const std::string source = R"(
        import hoo;
        func :string test() { return "hello"; }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    printf("DEBUG: r = %lld, s = %p, data = %p\n", (long long)r, (void*)s, (void*)hoo_string_data(s));
    EXPECT_STREQ("hello", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, Length) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hello".length(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 5);
}

TEST_F(HooStringJitTest, IsEmpty) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "".isEmpty(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStringJitTest, IsNotEmpty) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hi".isEmpty(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooStringJitTest, Concat) {
    const std::string source = R"(
        import hoo;
        func :string test() { return "a".concat("b"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("ab", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, ToLower) {
    const std::string source = R"(
        import hoo;
        func :string test() { return "HELLO".toLower(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("hello", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, Equals) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "abc".equals("abc"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStringJitTest, NotEquals) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "abc".equals("def"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooStringJitTest, Contains) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hello world".contains("world"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStringJitTest, ContainsNo) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hello world".contains("planet"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 0);
}

TEST_F(HooStringJitTest, StartsWith) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hello".startsWith("he"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 1);
}

TEST_F(HooStringJitTest, Trim) {
    const std::string source = R"(
        import hoo;
        func :string test() { return "  hi  ".trim(); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("hi", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, Repeat) {
    const std::string source = R"(
        import hoo;
        func :string test() { return string_repeat(65, 3); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    auto r = jit.run("_F_M_test_E_test_s");
    ASSERT_GT(r, 0) << jit.getLastError();
    HooString s = (HooString)r;
    EXPECT_STREQ("AAA", hoo_string_data(s));
    hoo_string_release(s);
}

TEST_F(HooStringJitTest, IndexOf) {
    const std::string source = R"(
        import hoo;
        func :int64 test() { return "hello".indexOf("l"); }
    )";
    ASSERT_TRUE(jit.loadSourceCode("test", source)) << jit.getLastError();
    EXPECT_EQ(jit.run("_F_M_test_E_test_i8"), 2);
}
