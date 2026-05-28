#include <gtest/gtest.h>
#include <cstring>
#include "runtime/lib/hoo_regex.h"

class HooRegexTest : public ::testing::Test {
protected:
    void TearDown() override {
        if (re_) {
            hoo_regex_release(re_);
            re_ = nullptr;
        }
    }

    HooRegex re_ = nullptr;
};

TEST_F(HooRegexTest, Compile) {
    re_ = hoo_regex_compile("hello");
    EXPECT_NE(re_, nullptr);
}

TEST_F(HooRegexTest, CompileWithFlags) {
    re_ = hoo_regex_compile_with_flags("hello", "i");
    ASSERT_NE(re_, nullptr);
    EXPECT_EQ(hoo_regex_match(re_, "HELLO"), 1);
}

TEST_F(HooRegexTest, Match) {
    re_ = hoo_regex_compile("hello");
    ASSERT_NE(re_, nullptr);
    EXPECT_EQ(hoo_regex_match(re_, "hello"), 1);
    EXPECT_EQ(hoo_regex_match(re_, "world"), 0);
}

TEST_F(HooRegexTest, Search) {
    re_ = hoo_regex_compile("world");
    ASSERT_NE(re_, nullptr);
    EXPECT_EQ(hoo_regex_search(re_, "hello world"), 1);
    EXPECT_EQ(hoo_regex_search(re_, "abc xyz"), 0);
}

TEST_F(HooRegexTest, Find) {
    re_ = hoo_regex_compile("\\w+");
    ASSERT_NE(re_, nullptr);
    char* result = hoo_regex_find(re_, "hello world");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "hello");
    hoo_regex_free_string(result);
}

TEST_F(HooRegexTest, FindAll) {
    re_ = hoo_regex_compile("\\d");
    ASSERT_NE(re_, nullptr);
    char** matches = nullptr;
    int64_t count = 0;
    int64_t ret = hoo_regex_find_all(re_, "abc123def456", &matches, &count);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(count, 6);
    ASSERT_NE(matches, nullptr);
    for (int64_t i = 0; i < count; i++) {
        EXPECT_NE(matches[i], nullptr);
    }
    hoo_regex_free_matches(matches, count);
}

TEST_F(HooRegexTest, Replace) {
    re_ = hoo_regex_compile("world");
    ASSERT_NE(re_, nullptr);
    char* result = hoo_regex_replace(re_, "hello world", "there");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "hello there");
    hoo_regex_free_string(result);
}

TEST_F(HooRegexTest, Split) {
    re_ = hoo_regex_compile(",");
    ASSERT_NE(re_, nullptr);
    int64_t count = 0;
    char** parts = hoo_regex_split(re_, "a,b,c", &count);
    ASSERT_NE(parts, nullptr);
    EXPECT_EQ(count, 3);
    EXPECT_STREQ(parts[0], "a");
    EXPECT_STREQ(parts[1], "b");
    EXPECT_STREQ(parts[2], "c");
    hoo_regex_free_matches(parts, count);
}

TEST_F(HooRegexTest, Group) {
    re_ = hoo_regex_compile("(\\w+)@(\\w+)");
    ASSERT_NE(re_, nullptr);
    char* g0 = hoo_regex_group(re_, "user@host", 0);
    ASSERT_NE(g0, nullptr);
    EXPECT_STREQ(g0, "user@host");
    hoo_regex_free_string(g0);

    char* g1 = hoo_regex_group(re_, "user@host", 1);
    ASSERT_NE(g1, nullptr);
    EXPECT_STREQ(g1, "user");
    hoo_regex_free_string(g1);

    char* g2 = hoo_regex_group(re_, "user@host", 2);
    ASSERT_NE(g2, nullptr);
    EXPECT_STREQ(g2, "host");
    hoo_regex_free_string(g2);
}

TEST_F(HooRegexTest, Error) {
    HooRegex bad = hoo_regex_compile("[");
    EXPECT_EQ(bad, nullptr);
    const char* err = hoo_regex_error();
    EXPECT_NE(err, nullptr);
}

TEST_F(HooRegexTest, RetainRelease) {
    re_ = hoo_regex_compile("test");
    ASSERT_NE(re_, nullptr);
    HooRegex r = hoo_regex_retain(re_);
    EXPECT_EQ(r, re_);
    hoo_regex_release(r);
    hoo_regex_release(r);
    re_ = nullptr;
}

TEST_F(HooRegexTest, FreeMatches) {
    hoo_regex_free_matches(nullptr, 0);
    hoo_regex_free_matches(nullptr, 5);
}
