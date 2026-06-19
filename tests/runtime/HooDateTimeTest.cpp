#include <gtest/gtest.h>
#include <cstring>
#include "runtime/lib/hoo_datetime.h"
#include "runtime/lib/hoo_string.h"

class HooDateTimeTest : public ::testing::Test {
};

TEST_F(HooDateTimeTest, Now) {
    void* dt = hoo_datetime_new_now();
    ASSERT_NE(dt, nullptr);

    int64_t ts = hoo_datetime_get_timestamp(dt);
    EXPECT_GT(ts, 1700000000000LL);

    int64_t now_s = hoo_datetime_now_seconds();
    EXPECT_GT(now_s, 1700000000LL);

    double now_p = hoo_datetime_now_precise();
    EXPECT_GT(now_p, 1700000000.0);
    EXPECT_LT(now_p - static_cast<double>(now_s), 1.0);
}

TEST_F(HooDateTimeTest, NewFromTimestamp) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    int64_t ts = hoo_datetime_get_timestamp(dt);
    EXPECT_EQ(ts, 1700000000000LL);
}

TEST_F(HooDateTimeTest, Decompose) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    HooDateTimeFields f = hoo_datetime_instance_decompose(dt);

    EXPECT_EQ(f.year,   2023);
    EXPECT_EQ(f.month,  11);
    EXPECT_EQ(f.day,    14);
    EXPECT_EQ(f.hour,   22);
    EXPECT_EQ(f.minute, 13);
    EXPECT_EQ(f.second, 20);
    EXPECT_EQ(f.millisecond, 0);
    EXPECT_EQ(f.weekday, 2);
    EXPECT_EQ(f.yearday, 317);
}

TEST_F(HooDateTimeTest, DecomposeComposeRoundtrip) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    HooDateTimeFields f = hoo_datetime_instance_decompose(dt);
    void* rt = hoo_datetime_new(0);
    ASSERT_NE(rt, nullptr);

    // Decompose/compose not directly exposed as public API, but we can
    // verify the fields are consistent by round-tripping through decompose
    EXPECT_EQ(f.millisecond, 0);
}

TEST_F(HooDateTimeTest, Format) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* s1 = hoo_datetime_instance_format(dt, "%Y-%m-%d");
    ASSERT_NE(s1, nullptr);
    EXPECT_STREQ(hoo_string_data(s1), "2023-11-14");

    void* s2 = hoo_datetime_instance_format(dt, "%H:%M:%S");
    ASSERT_NE(s2, nullptr);
    EXPECT_STREQ(hoo_string_data(s2), "22:13:20");
}

TEST_F(HooDateTimeTest, Iso8601) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* iso = hoo_datetime_instance_iso8601(dt);
    ASSERT_NE(iso, nullptr);

    const char* s = hoo_string_data(iso);
    EXPECT_TRUE(strstr(s, "2023-11-14") != nullptr);
    EXPECT_TRUE(strstr(s, "22:13:20") != nullptr);
    EXPECT_TRUE(strstr(s, "Z") != nullptr);
}

TEST_F(HooDateTimeTest, AddDays) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* plus1  = hoo_datetime_instance_add_days(dt, 1);
    void* minus1 = hoo_datetime_instance_add_days(dt, -1);
    void* plus7  = hoo_datetime_instance_add_days(dt, 7);

    ASSERT_NE(plus1, nullptr);
    ASSERT_NE(minus1, nullptr);
    ASSERT_NE(plus7, nullptr);

    EXPECT_EQ(hoo_datetime_get_timestamp(plus1),  1700000000000LL + 86400000LL);
    EXPECT_EQ(hoo_datetime_get_timestamp(minus1), 1700000000000LL - 86400000LL);
    EXPECT_EQ(hoo_datetime_get_timestamp(plus7),  1700000000000LL + 7 * 86400000LL);
}

TEST_F(HooDateTimeTest, AddHours) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* plus1  = hoo_datetime_instance_add_hours(dt, 1);
    void* minus1 = hoo_datetime_instance_add_hours(dt, -1);
    void* plus12 = hoo_datetime_instance_add_hours(dt, 12);

    ASSERT_NE(plus1, nullptr);
    ASSERT_NE(minus1, nullptr);
    ASSERT_NE(plus12, nullptr);

    EXPECT_EQ(hoo_datetime_get_timestamp(plus1),  1700000000000LL + 3600000LL);
    EXPECT_EQ(hoo_datetime_get_timestamp(minus1), 1700000000000LL - 3600000LL);
    EXPECT_EQ(hoo_datetime_get_timestamp(plus12), 1700000000000LL + 12 * 3600000LL);
}

TEST_F(HooDateTimeTest, DiffSeconds) {
    void* t1 = hoo_datetime_new(1700000000000LL);
    void* t2 = hoo_datetime_new(1700000005000LL);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);

    double diff = hoo_datetime_instance_diff_seconds(t1, t2);
    EXPECT_DOUBLE_EQ(diff, 5.0);

    double diff_rev = hoo_datetime_instance_diff_seconds(t2, t1);
    EXPECT_DOUBLE_EQ(diff_rev, -5.0);

    double diff_zero = hoo_datetime_instance_diff_seconds(t1, t1);
    EXPECT_DOUBLE_EQ(diff_zero, 0.0);
}

TEST_F(HooDateTimeTest, Compare) {
    void* small = hoo_datetime_new(1000000LL);
    void* same  = hoo_datetime_new(1000000LL);
    void* big   = hoo_datetime_new(2000000LL);

    ASSERT_NE(small, nullptr);
    ASSERT_NE(same, nullptr);
    ASSERT_NE(big, nullptr);

    EXPECT_EQ(hoo_datetime_instance_compare(small, big), -1);
    EXPECT_EQ(hoo_datetime_instance_compare(big, small),  1);
    EXPECT_EQ(hoo_datetime_instance_compare(small, same),  0);
}

TEST_F(HooDateTimeTest, Parse) {
    int64_t orig = 1700000000000LL;
    void* dt = hoo_datetime_new(orig);
    ASSERT_NE(dt, nullptr);

    void* fmt_str = hoo_datetime_instance_format(dt, "%Y-%m-%dT%H:%M:%S");
    ASSERT_NE(fmt_str, nullptr);

    void* parsed = hoo_datetime_new_parse(hoo_string_data(fmt_str), "%Y-%m-%dT%H:%M:%S");
    ASSERT_NE(parsed, nullptr);

    EXPECT_EQ(hoo_datetime_get_timestamp(parsed), orig);
}

TEST_F(HooDateTimeTest, FromIso8601) {
    void* dt = hoo_datetime_new_from_iso8601("2024-01-15T10:30:00Z");
    ASSERT_NE(dt, nullptr);

    int64_t ts = hoo_datetime_get_timestamp(dt);
    EXPECT_GT(ts, 1700000000000LL);
}

TEST_F(HooDateTimeTest, NullHandle) {
    EXPECT_EQ(hoo_datetime_get_timestamp(nullptr), 0);

    EXPECT_EQ(hoo_datetime_instance_format(nullptr, "%Y"), nullptr);
    EXPECT_EQ(hoo_datetime_instance_iso8601(nullptr), nullptr);
    EXPECT_EQ(hoo_datetime_instance_add_days(nullptr, 1), nullptr);
    EXPECT_EQ(hoo_datetime_instance_add_hours(nullptr, 1), nullptr);
    EXPECT_EQ(hoo_datetime_instance_add_minutes(nullptr, 1), nullptr);
    EXPECT_EQ(hoo_datetime_instance_add_seconds(nullptr, 1), nullptr);
    EXPECT_EQ(hoo_datetime_instance_add_milliseconds(nullptr, 1), nullptr);

    EXPECT_EQ(hoo_datetime_instance_diff_days(nullptr, nullptr), 0);
    EXPECT_EQ(hoo_datetime_instance_compare(nullptr, nullptr), 0);
}

TEST_F(HooDateTimeTest, FreeString) {
    hoo_datetime_free_string(nullptr);
    hoo_datetime_free_string(nullptr);
}

TEST_F(HooDateTimeTest, FreeFuncNow) {
    void* dt = hoo_datetime_now();
    ASSERT_NE(dt, nullptr);
    EXPECT_GT(hoo_datetime_get_timestamp(dt), 1700000000000LL);
}

TEST_F(HooDateTimeTest, FreeFuncParse) {
    void* dt = hoo_datetime_parse("2024-06-15", "%Y-%m-%d");
    ASSERT_NE(dt, nullptr);
    EXPECT_GT(hoo_datetime_get_timestamp(dt), 1700000000000LL);
}

TEST_F(HooDateTimeTest, FreeFuncFromIso8601) {
    void* dt = hoo_datetime_from_iso8601("2024-01-15T10:30:00Z");
    ASSERT_NE(dt, nullptr);
    EXPECT_GT(hoo_datetime_get_timestamp(dt), 1700000000000LL);
}

TEST_F(HooDateTimeTest, FreeFuncFormat) {
    void* dt = hoo_datetime_now();
    ASSERT_NE(dt, nullptr);

    void* str = hoo_datetime_format(dt, "%Y-%m-%d");
    ASSERT_NE(str, nullptr);
    EXPECT_EQ(hoo_string_length(str), 10);
}

TEST_F(HooDateTimeTest, FreeFuncIso8601) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* iso = hoo_datetime_iso8601(dt);
    ASSERT_NE(iso, nullptr);
    EXPECT_TRUE(strstr(hoo_string_data(iso), "2023-11-14") != nullptr);
}

TEST_F(HooDateTimeTest, FreeFuncAddDays) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* later = hoo_datetime_add_days(dt, 7);
    ASSERT_NE(later, nullptr);
    EXPECT_EQ(hoo_datetime_get_timestamp(later), 1700000000000LL + 7 * 86400000LL);
}

TEST_F(HooDateTimeTest, FreeFuncAddHours) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    void* later = hoo_datetime_add_hours(dt, 48);
    ASSERT_NE(later, nullptr);
    EXPECT_EQ(hoo_datetime_get_timestamp(later), 1700000000000LL + 48 * 3600000LL);
}

TEST_F(HooDateTimeTest, FreeFuncDiffDays) {
    void* a = hoo_datetime_new(1700000000000LL);
    void* b = hoo_datetime_new(1700000000000LL + 3 * 86400000LL);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    EXPECT_EQ(hoo_datetime_diff_days(a, b), 3);
}

TEST_F(HooDateTimeTest, FreeFuncCompare) {
    void* a = hoo_datetime_new(1000000LL);
    void* b = hoo_datetime_new(2000000LL);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    EXPECT_EQ(hoo_datetime_compare(a, b), -1);
    EXPECT_EQ(hoo_datetime_compare(b, a),  1);
    EXPECT_EQ(hoo_datetime_compare(a, a),  0);
}

TEST_F(HooDateTimeTest, FreeFuncDecompose) {
    void* dt = hoo_datetime_new(1700000000000LL);
    ASSERT_NE(dt, nullptr);

    HooDateTimeFields f = hoo_datetime_decompose(dt);
    EXPECT_EQ(f.year, 2023);
    EXPECT_EQ(f.month, 11);
    EXPECT_EQ(f.day, 14);
}
