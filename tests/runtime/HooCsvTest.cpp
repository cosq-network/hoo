#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include "runtime/lib/hoo_csv.h"
#include "runtime/lib/hoo_generic_array.h"
#include "runtime/lib/hoo_map.h"
#include "runtime/lib/hoo_string.h"
#include "runtime/lib/hoo_exception.h"

// Helper: parse CSV into HooArray<HooMap> using the OOP API
static HooArray parse_to_maps(const char* csv_str) {
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_parse_as_maps(csv, csv_str);
    hoo_csv_release(csv);
    return result;
}

// Helper: create a simple HooArray<HooString> from C string list
static HooArray make_string_array(const char** strs, int64_t count) {
    HooArray arr = hoo_array_new();
    for (int64_t i = 0; i < count; i++) {
        HooString s = hoo_string_from_cstr(strs[i]);
        hoo_array_push_object(arr, s);
        hoo_string_release(s);
    }
    return arr;
}

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
    auto tmp = std::filesystem::temp_directory_path() / "hoo_csv_test.tmp";
    std::string path_str = tmp.generic_string();
    const char* path = path_str.c_str();
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

TEST_F(HooCsvTest, FromOptsCreatesInstanceWithCustomDelimiter) {
    HooCsv csv = hoo_csv_from_opts(59, 39);
    ASSERT_NE(csv, nullptr);

    int64_t rows = 0, cols = 0;
    char*** table = hoo_csv_parse_raw_with_opts("a;b;c", ';', '\'', &rows, &cols);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(rows, 1);
    EXPECT_EQ(cols, 3);
    EXPECT_STREQ(table[0][0], "a");
    EXPECT_STREQ(table[0][1], "b");
    EXPECT_STREQ(table[0][2], "c");
    hoo_csv_free_table(table, rows, cols);

    // Same test using the instance-based OOP API
    HooArray arr = hoo_csv_parse(csv, "a;b;c");
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(hoo_array_length(arr), 1);
    hoo_array_release(arr);

    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, FreeString) {
    hoo_csv_free_string(nullptr);
}

TEST_F(HooCsvTest, FreeTable) {
    hoo_csv_free_table(nullptr, 0, 0);
}

// ============================================================================
// Aggregation Tests
// ============================================================================

TEST_F(HooCsvTest, CountReturnsNumberOfNonEmptyValues) {
    HooArray data = parse_to_maps("name,age\nAlice,30\nBob,\nCharlie,25");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    EXPECT_EQ(hoo_csv_count(csv, data, "name"), 3);
    EXPECT_EQ(hoo_csv_count(csv, data, "age"), 2);
    EXPECT_EQ(hoo_csv_count(csv, data, "missing"), 0);

    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, CountEmptyDataReturnsZero) {
    HooArray data = parse_to_maps("col\n");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_count(csv, data, "col"), 0);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SumReturnsIntegerSum) {
    HooArray data = parse_to_maps("x\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_sum(csv, data, "x"), 60);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SumEmptyReturnsZero) {
    HooArray data = parse_to_maps("x\n");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_sum(csv, data, "x"), 0);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, AvgReturnsFormattedDouble) {
    HooArray data = parse_to_maps("score\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooString result = hoo_csv_avg(csv, data, "score");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "20");
    hoo_string_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, AvgReturnsNullForEmpty) {
    HooArray data = parse_to_maps("x\n");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_avg(csv, data, "x"), nullptr);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, MinLexicographic) {
    HooArray data = parse_to_maps("val\nbanana\napple\ncherry");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooString result = hoo_csv_min(csv, data, "val");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "apple");
    hoo_string_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, MaxLexicographic) {
    HooArray data = parse_to_maps("val\nbanana\napple\ncherry");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooString result = hoo_csv_max(csv, data, "val");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(hoo_string_data(result), "cherry");
    hoo_string_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, MinMaxHandlesEmptyColumn) {
    HooArray data = parse_to_maps("val\n");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_min(csv, data, "val"), nullptr);
    EXPECT_EQ(hoo_csv_max(csv, data, "val"), nullptr);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

// ============================================================================
// Transformation Tests
// ============================================================================

TEST_F(HooCsvTest, SelectSubsetOfColumns) {
    HooArray data = parse_to_maps("a,b,c\n1,2,3\n4,5,6");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    const char* names[] = {"a", "c"};
    HooArray cols = make_string_array(names, 2);

    HooArray result = hoo_csv_select(csv, data, cols);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);

    // Check first row has only a and c
    HooMap row0 = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&row0));
    const char* val_a = NULL;
    const char* val_c = NULL;
    EXPECT_TRUE(hoo_map_try_get(row0, "a", &val_a));
    EXPECT_TRUE(hoo_map_try_get(row0, "c", &val_c));
    EXPECT_STREQ(val_a, "1");
    EXPECT_STREQ(val_c, "3");

    // b should not exist
    const char* val_b = NULL;
    EXPECT_FALSE(hoo_map_try_get(row0, "b", &val_b));

    hoo_array_release(cols);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SelectReturnsNullForMissingDataOrColumns) {
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_select(csv, nullptr, nullptr), nullptr);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, FilterEquals) {
    HooArray data = parse_to_maps("name,age\nAlice,30\nBob,25\nCharlie,30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    HooArray result = hoo_csv_filter(csv, data, "age", "==", "30");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);

    HooArray result2 = hoo_csv_filter(csv, data, "name", "==", "Bob");
    ASSERT_NE(result2, nullptr);
    EXPECT_EQ(hoo_array_length(result2), 1);

    hoo_array_release(result);
    hoo_array_release(result2);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterNotEquals) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_filter(csv, data, "val", "!=", "20");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterGreaterThan) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_filter(csv, data, "val", ">", "15");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterLessThanOrEqual) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_filter(csv, data, "val", "<=", "20");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterReturnsNullOnInvalidInput) {
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_filter(csv, nullptr, "col", "==", "x"), nullptr);
    EXPECT_EQ(hoo_csv_filter(csv, nullptr, nullptr, nullptr, nullptr), nullptr);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, SortAscending) {
    HooArray data = parse_to_maps("name\nCharlie\nAlice\nBob");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    HooArray result = hoo_csv_sort(csv, data, "name", 1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 3);

    HooMap row0 = NULL, row2 = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&row0));
    ASSERT_TRUE(hoo_array_get_object(result, 2, (void**)&row2));
    const char* first = NULL;
    const char* last = NULL;
    EXPECT_TRUE(hoo_map_try_get(row0, "name", &first));
    EXPECT_TRUE(hoo_map_try_get(row2, "name", &last));
    EXPECT_STREQ(first, "Alice");
    EXPECT_STREQ(last, "Charlie");

    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SortDescending) {
    HooArray data = parse_to_maps("name\nCharlie\nAlice\nBob");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    HooArray result = hoo_csv_sort(csv, data, "name", 0);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 3);

    HooMap row0 = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&row0));
    const char* first = NULL;
    EXPECT_TRUE(hoo_map_try_get(row0, "name", &first));
    EXPECT_STREQ(first, "Charlie");

    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SortReturnsNullOnInvalidInput) {
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_sort(csv, nullptr, "col", 1), nullptr);
    hoo_csv_release(csv);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(HooCsvTest, DescribeReturnsMapWithStats) {
    HooArray data = parse_to_maps("val\n10\n20\n30\n40");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    HooMap stats = hoo_csv_describe(csv, data, "val");
    ASSERT_NE(stats, nullptr);

    const char* count = NULL;
    const char* sum = NULL;
    const char* avg = NULL;
    const char* min = NULL;
    const char* max = NULL;

    EXPECT_TRUE(hoo_map_try_get(stats, "count", &count));
    EXPECT_TRUE(hoo_map_try_get(stats, "sum", &sum));
    EXPECT_TRUE(hoo_map_try_get(stats, "avg", &avg));
    EXPECT_TRUE(hoo_map_try_get(stats, "min", &min));
    EXPECT_TRUE(hoo_map_try_get(stats, "max", &max));

    EXPECT_STREQ(count, "4");
    EXPECT_STREQ(sum, "100");
    EXPECT_STREQ(min, "10");
    EXPECT_STREQ(max, "40");

    hoo_map_release(stats);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, DescribeReturnsCountZeroForEmpty) {
    HooArray data = parse_to_maps("val\n");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();

    HooMap stats = hoo_csv_describe(csv, data, "val");
    ASSERT_NE(stats, nullptr);

    const char* count = NULL;
    EXPECT_TRUE(hoo_map_try_get(stats, "count", &count));
    EXPECT_STREQ(count, "0");

    hoo_map_release(stats);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, DescribeReturnsNullForNullInput) {
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_describe(csv, nullptr, "col"), nullptr);
    EXPECT_EQ(hoo_csv_describe(csv, nullptr, nullptr), nullptr);
    hoo_csv_release(csv);
}

// ============================================================================
// Exception Tests — type mismatch validation
// ============================================================================

TEST_F(HooCsvTest, SumThrowsOnNonNumeric) {
    HooArray data = parse_to_maps("val\n10\nabc\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_THROW(hoo_csv_sum(csv, data, "val"), std::exception);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, AvgThrowsOnNonNumeric) {
    HooArray data = parse_to_maps("val\n10\nxyz\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_THROW(hoo_csv_avg(csv, data, "val"), std::exception);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, DescribeThrowsOnNonNumeric) {
    HooArray data = parse_to_maps("val\n10\nnotanumber\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_THROW(hoo_csv_describe(csv, data, "val"), std::exception);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterOrderingThrowsOnNonNumericColumnValue) {
    HooArray data = parse_to_maps("val\n10\nabc\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_THROW(hoo_csv_filter(csv, data, "val", ">", "5"), std::exception);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterOrderingThrowsOnNonNumericComparisonValue) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_THROW(hoo_csv_filter(csv, data, "val", ">", "abc"), std::exception);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, FilterEqualitySkipsTypeCheck) {
    HooArray data = parse_to_maps("name\nAlice\nBob\nCharlie");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_filter(csv, data, "name", "==", "Bob");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 1);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, SumWorksForAllNumeric) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    EXPECT_EQ(hoo_csv_sum(csv, data, "val"), 60);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

TEST_F(HooCsvTest, AvgWorksForAllNumeric) {
    HooArray data = parse_to_maps("val\n10\n20\n30\n40");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooString avg = hoo_csv_avg(csv, data, "val");
    ASSERT_NE(avg, nullptr);
    const char* s = hoo_string_data(avg);
    EXPECT_STREQ(s, "25");
    hoo_string_release(avg);
    hoo_csv_release(csv);
    hoo_array_release(data);
}

// ============================================================================
// ARC Memory Management Tests
// ============================================================================

TEST_F(HooCsvTest, CsvInstanceRetainReleaseRefcount) {
    HooCsv csv = hoo_csv_new();
    ASSERT_NE(csv, nullptr);
    EXPECT_EQ(hoo_csv_refcount(csv), 1);

    HooCsv csv2 = hoo_csv_retain(csv);
    EXPECT_EQ(csv2, csv);
    EXPECT_EQ(hoo_csv_refcount(csv), 2);

    hoo_csv_release(csv);
    EXPECT_EQ(hoo_csv_refcount(csv), 1);

    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, ParseResultElementsIndependentlyOwned) {
    HooCsv csv = hoo_csv_new();
    ASSERT_NE(csv, nullptr);

    HooArray result = hoo_csv_parse(csv, "a,b\n1,2\n3,4");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 3);

    // Access each row and verify content
    HooArray row0 = NULL;
    ASSERT_TRUE(hoo_array_get_array(result, 0, &row0));
    HooString s0 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row0, 0, (void**)&s0));
    EXPECT_STREQ(hoo_string_data(s0), "a");
    HooString s1 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row0, 1, (void**)&s1));
    EXPECT_STREQ(hoo_string_data(s1), "b");

    HooArray row1 = NULL;
    ASSERT_TRUE(hoo_array_get_array(result, 1, &row1));
    HooString s2 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row1, 0, (void**)&s2));
    EXPECT_STREQ(hoo_string_data(s2), "1");
    HooString s3 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row1, 1, (void**)&s3));
    EXPECT_STREQ(hoo_string_data(s3), "2");

    HooArray row2 = NULL;
    ASSERT_TRUE(hoo_array_get_array(result, 2, &row2));
    HooString s4 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row2, 0, (void**)&s4));
    EXPECT_STREQ(hoo_string_data(s4), "3");
    HooString s5 = NULL;
    ASSERT_TRUE(hoo_array_get_object(row2, 1, (void**)&s5));
    EXPECT_STREQ(hoo_string_data(s5), "4");

    // Retain the result to keep it alive, release, then verify accessibility
    hoo_array_retain(result);
    hoo_array_release(result);

    // Data should still be accessible via retained reference
    ASSERT_TRUE(hoo_array_get_array(result, 0, &row0));
    ASSERT_TRUE(hoo_array_get_object(row0, 1, (void**)&s1));
    EXPECT_STREQ(hoo_string_data(s1), "b");

    hoo_array_release(result);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, ParseAsMapsResultSurvivesAfterRelease) {
    HooCsv csv = hoo_csv_new();
    ASSERT_NE(csv, nullptr);

    HooArray result = hoo_csv_parse_as_maps(csv, "name,val\nAlice,10\nBob,20");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);

    // Retain result, release, verify maps survive
    hoo_array_retain(result);
    hoo_array_release(result);

    HooMap map = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&map));
    ASSERT_NE(map, nullptr);

    const char* name = NULL;
    ASSERT_TRUE(hoo_map_try_get(map, "name", &name));
    EXPECT_STREQ(name, "Alice");

    hoo_array_release(result);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, FilterResultSurvivesAfterInputReleased) {
    // filter now retains maps, so the result should live independently
    HooArray data = parse_to_maps("x\n10\n20\n30");
    ASSERT_NE(data, nullptr);

    HooCsv csv = hoo_csv_new();
    ASSERT_NE(csv, nullptr);

    HooArray result = hoo_csv_filter(csv, data, "x", ">", "15");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);

    // Release input data — result should still be valid
    hoo_array_release(data);

    // Access maps in the result
    HooMap map = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&map));
    ASSERT_NE(map, nullptr);

    const char* val = NULL;
    ASSERT_TRUE(hoo_map_try_get(map, "x", &val));
    EXPECT_STREQ(val, "20");

    hoo_array_release(result);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, SortResultSurvivesAfterInputReleased) {
    HooArray data = parse_to_maps("name\nCharlie\nAlice\nBob");
    ASSERT_NE(data, nullptr);

    HooCsv csv = hoo_csv_new();
    ASSERT_NE(csv, nullptr);

    HooArray result = hoo_csv_sort(csv, data, "name", 1);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 3);

    // Release input — result should still be valid
    hoo_array_release(data);

    HooMap map = NULL;
    ASSERT_TRUE(hoo_array_get_object(result, 0, (void**)&map));
    ASSERT_NE(map, nullptr);

    const char* name = NULL;
    ASSERT_TRUE(hoo_map_try_get(map, "name", &name));
    EXPECT_STREQ(name, "Alice");

    hoo_array_release(result);
    hoo_csv_release(csv);
}

TEST_F(HooCsvTest, CsvReleaseNullIsSafe) {
    hoo_csv_release(nullptr);
}

TEST_F(HooCsvTest, CsvRetainNullReturnsNull) {
    EXPECT_EQ(hoo_csv_retain(nullptr), nullptr);
}

TEST_F(HooCsvTest, CsvRefcountNullReturnsZero) {
    EXPECT_EQ(hoo_csv_refcount(nullptr), 0);
}

TEST_F(HooCsvTest, FilterGreaterThanWorksForAllNumeric) {
    HooArray data = parse_to_maps("val\n10\n20\n30");
    ASSERT_NE(data, nullptr);
    HooCsv csv = hoo_csv_new();
    HooArray result = hoo_csv_filter(csv, data, "val", ">", "15");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(hoo_array_length(result), 2);
    hoo_array_release(result);
    hoo_csv_release(csv);
    hoo_array_release(data);
}
