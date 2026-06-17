#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "runtime/lib/hoo_csv.h"

class HooCsvTest : public ::testing::Test {
};

TEST_F(HooCsvTest, ParseSimple) {
    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw("a,b,c\n1,2,3", &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 2);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "a");
    EXPECT_STREQ(table[0][1], "b");
    EXPECT_STREQ(table[0][2], "c");
    EXPECT_STREQ(table[1][0], "1");
    EXPECT_STREQ(table[1][1], "2");
    EXPECT_STREQ(table[1][2], "3");
    hoo_csv_free_table(table, rows, cols);
}

TEST_F(HooCsvTest, ParseWithQuotes) {
    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw("a,\"b,c\",d", &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 1);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "a");
    EXPECT_STREQ(table[0][1], "b,c");
    EXPECT_STREQ(table[0][2], "d");
    hoo_csv_free_table(table, rows, cols);
}

TEST_F(HooCsvTest, ParseEmptyFields) {
    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw("a,,c", &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 1);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "a");
    EXPECT_STREQ(table[0][1], "");
    EXPECT_STREQ(table[0][2], "c");
    hoo_csv_free_table(table, rows, cols);
}

TEST_F(HooCsvTest, ParseQuotedNewlines) {
    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw("x,\"hello\nworld\",y", &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 1);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "x");
    EXPECT_STREQ(table[0][1], "hello\nworld");
    EXPECT_STREQ(table[0][2], "y");
    hoo_csv_free_table(table, rows, cols);
}

TEST_F(HooCsvTest, GenerateSimple) {
    const char* headers[] = {"Name", "Age"};
    const char* row0[] = {"Alice", "30"};
    const char* row1[] = {"Bob", "25"};
    const char** data[] = {row0, row1};
    char* csv = hoo_csv_generate_raw(headers, data, 2, 2);
    ASSERT_NE(csv, nullptr);
    EXPECT_STREQ(csv, "Name,Age\nAlice,30\nBob,25\n");
    hoo_csv_free_string(csv);
}

TEST_F(HooCsvTest, GenerateWithQuotes) {
    const char* headers[] = {"Name", "Comment"};
    const char* row0[] = {"Alice", "He said \"hello\""};
    const char** data[] = {row0};
    char* csv = hoo_csv_generate_raw(headers, data, 1, 2);
    ASSERT_NE(csv, nullptr);
    EXPECT_STREQ(csv, "Name,Comment\nAlice,\"He said \"\"hello\"\"\"\n");
    hoo_csv_free_string(csv);
}

TEST_F(HooCsvTest, ReadWriteFile) {
#ifdef _WIN32
    char tmp_dir[MAX_PATH + 1] = {0};
    GetTempPathA(MAX_PATH, tmp_dir);
    char path[MAX_PATH + 1] = {0};
    snprintf(path, MAX_PATH, "%s\\hoo_csv_test.tmp", tmp_dir);
#else
    const char* path = "/tmp/hoo_csv_test.tmp";
#endif
    const char* headers[] = {"X", "Y"};
    const char* row0[] = {"1", "2"};
    const char** data[] = {row0};
    int64_t res = hoo_csv_write_file_raw(path, headers, data, 1, 2);
    EXPECT_EQ(res, 0);

    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_read_file_raw(path, &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 2);
    EXPECT_EQ(cols, 2);
    EXPECT_STREQ(table[0][0], "X");
    EXPECT_STREQ(table[0][1], "Y");
    EXPECT_STREQ(table[1][0], "1");
    EXPECT_STREQ(table[1][1], "2");
    hoo_csv_free_table(table, rows, cols);

    std::remove(path);
}

TEST_F(HooCsvTest, ParseWithOpts) {
    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw_with_opts("a;b;c", ';', '"', &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 1);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "a");
    EXPECT_STREQ(table[0][1], "b");
    EXPECT_STREQ(table[0][2], "c");
    hoo_csv_free_table(table, rows, cols);
}

TEST_F(HooCsvTest, FreeString) {
    hoo_csv_free_string(nullptr);
}

TEST_F(HooCsvTest, FreeTable) {
    hoo_csv_free_table(nullptr, 0, 0);
}
