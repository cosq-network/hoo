#include <gtest/gtest.h>
#include <cstring>
#include "runtime/lib/system/hoo_fs.h"

class HooPathTest : public ::testing::Test {
};

TEST_F(HooPathTest, Dirname) {
    char* result = hoo_path_dirname("/foo/bar/file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "/foo/bar");
    hoo_path_free_string(result);

    result = hoo_path_dirname("file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, ".");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Basename) {
    char* result = hoo_path_basename("/foo/bar/file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "file.txt");
    hoo_path_free_string(result);

    result = hoo_path_basename("/foo/bar/");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "bar");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Extension) {
    char* result = hoo_path_extension("file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, ".txt");
    hoo_path_free_string(result);

    result = hoo_path_extension("file");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    hoo_path_free_string(result);

    result = hoo_path_extension(".hidden");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Stem) {
    char* result = hoo_path_stem("file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "file");
    hoo_path_free_string(result);

    result = hoo_path_stem("archive.tar.gz");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "archive.tar");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Root) {
    char* result = hoo_path_root("/foo/bar");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "/");
    hoo_path_free_string(result);

    result = hoo_path_root("C:\\foo");
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_STREQ(result, "C:\\");
#else
    EXPECT_STREQ(result, "");
#endif
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Join) {
    char* result = hoo_path_join("/foo", "bar");
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_STREQ(result, "/foo\\bar");
#else
    EXPECT_STREQ(result, "/foo/bar");
#endif
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, JoinMulti) {
    const char* parts[] = {"a", "b", "c"};
    char* result = hoo_path_join_multi(parts, 3);
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_STREQ(result, "a\\b\\c");
#else
    EXPECT_STREQ(result, "a/b/c");
#endif
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Normalize) {
    char* result = hoo_path_normalize("/foo/../bar/./baz");
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_STREQ(result, "\\bar\\baz");
#else
    EXPECT_STREQ(result, "/bar/baz");
#endif
    hoo_path_free_string(result);

    result = hoo_path_normalize("a//b///c");
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_STREQ(result, "a\\b\\c");
#else
    EXPECT_STREQ(result, "a/b/c");
#endif
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Absolute) {
    char* result = hoo_path_absolute("relative/path");
    ASSERT_NE(result, nullptr);
    EXPECT_GT(strlen(result), strlen("relative/path"));
#ifdef _WIN32
    EXPECT_TRUE(isalpha((unsigned char)result[0]) && result[1] == ':');
#else
    EXPECT_EQ(result[0], '/');
#endif
    hoo_path_free_string(result);

    result = hoo_path_absolute("/already/absolute");
    ASSERT_NE(result, nullptr);
#ifdef _WIN32
    EXPECT_TRUE(isalpha((unsigned char)result[0]) && result[1] == ':');
#else
    EXPECT_STREQ(result, "/already/absolute");
#endif
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Relative) {
    char* result = hoo_path_relative("/foo/bar/baz", "/foo/bar");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "baz");
    hoo_path_free_string(result);

    result = hoo_path_relative("/foo/bar", "/foo/bar/baz");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "..");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, IsAbsolute) {
#ifdef _WIN32
    EXPECT_EQ(hoo_path_is_absolute("C:\\foo"), 1);
#else
    EXPECT_EQ(hoo_path_is_absolute("/foo"), 1);
#endif
    EXPECT_EQ(hoo_path_is_absolute("foo"), 0);
    EXPECT_EQ(hoo_path_is_absolute(""), 0);
}

TEST_F(HooPathTest, IsRelative) {
    EXPECT_EQ(hoo_path_is_relative("foo"), 1);
#ifdef _WIN32
    EXPECT_EQ(hoo_path_is_relative("C:\\foo"), 0);
#else
    EXPECT_EQ(hoo_path_is_relative("/foo"), 0);
#endif
    EXPECT_EQ(hoo_path_is_relative(""), 1);
}

TEST_F(HooPathTest, HasExtension) {
    EXPECT_EQ(hoo_path_has_extension("file.txt"), 1);
    EXPECT_EQ(hoo_path_has_extension("file"), 0);
}

TEST_F(HooPathTest, Split) {
    int64_t count = 0;
    char** parts = hoo_path_split("foo/bar/baz", &count);
    ASSERT_NE(parts, nullptr);
    ASSERT_EQ(count, 3);
    EXPECT_STREQ(parts[0], "foo");
    EXPECT_STREQ(parts[1], "bar");
    EXPECT_STREQ(parts[2], "baz");
    hoo_path_free_parts(parts, count);
}

TEST_F(HooPathTest, Separator) {
#ifdef _WIN32
    EXPECT_EQ(hoo_path_separator(), '\\');
#else
    EXPECT_EQ(hoo_path_separator(), '/');
#endif
}

TEST_F(HooPathTest, ListSeparator) {
#ifdef _WIN32
    EXPECT_EQ(hoo_path_list_separator(), ';');
#else
    EXPECT_EQ(hoo_path_list_separator(), ':');
#endif
}

TEST_F(HooPathTest, HasRoot) {
#ifdef _WIN32
    EXPECT_EQ(hoo_path_has_root("C:\\foo"), 1);
    EXPECT_EQ(hoo_path_has_root("\\foo"), 1);
#else
    EXPECT_EQ(hoo_path_has_root("/foo"), 1);
    EXPECT_EQ(hoo_path_has_root("foo"), 0);
#endif
}

TEST_F(HooPathTest, FreeString) {
    hoo_path_free_string(nullptr);

    char* s = hoo_path_dirname("/foo");
    ASSERT_NE(s, nullptr);
    hoo_path_free_string(s);
}

TEST_F(HooPathTest, FreeParts) {
    hoo_path_free_parts(nullptr, 0);
}

TEST_F(HooPathTest, Filename) {
    char* result = hoo_path_filename("/foo/bar/file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "file.txt");
    hoo_path_free_string(result);

    result = hoo_path_filename("file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "file.txt");
    hoo_path_free_string(result);

    result = hoo_path_filename("/foo/bar/");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "bar");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, Parent) {
    char* result = hoo_path_parent("/foo/bar/file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "/foo/bar");
    hoo_path_free_string(result);

    result = hoo_path_parent("file.txt");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, ".");
    hoo_path_free_string(result);

    result = hoo_path_parent("/foo/bar/");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "/foo/bar");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, EmptyPathNeverReturnsNull) {
    char* result = hoo_path_basename("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    hoo_path_free_string(result);

    result = hoo_path_dirname("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, ".");
    hoo_path_free_string(result);

    result = hoo_path_stem("file");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "file");
    hoo_path_free_string(result);

    result = hoo_path_stem("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    hoo_path_free_string(result);

    result = hoo_path_filename("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    hoo_path_free_string(result);

    result = hoo_path_parent("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, ".");
    hoo_path_free_string(result);
}

TEST_F(HooPathTest, NullPathReturnsNull) {
    char* result = hoo_path_basename(nullptr);
    EXPECT_EQ(result, nullptr);

    result = hoo_path_dirname(nullptr);
    EXPECT_EQ(result, nullptr);

    result = hoo_path_filename(nullptr);
    EXPECT_EQ(result, nullptr);

    result = hoo_path_parent(nullptr);
    EXPECT_EQ(result, nullptr);
}
