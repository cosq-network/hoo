#include <gtest/gtest.h>
#include <cstring>
#include "runtime/lib/hoo_datetime.h"

class HooDateTimeTest : public ::testing::Test {
};

TEST_F(HooDateTimeTest, Now) {
    int64_t now_ms = hoo_datetime_now();
    int64_t now_s  = hoo_datetime_now_seconds();
    double  now_p  = hoo_datetime_now_precise();

    EXPECT_GT(now_ms, 1700000000000LL);
    EXPECT_GT(now_s, 1700000000LL);
    EXPECT_GT(now_p, 1700000000.0);
    EXPECT_LT(now_p - static_cast<double>(now_s), 1.0);
}

TEST_F(HooDateTimeTest, Decompose) {
    HooDateTimeFields f = hoo_datetime_decompose(1700000000000LL);

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

TEST_F(HooDateTimeTest, Compose) {
    HooDateTimeFields f = hoo_datetime_decompose(1700000000000LL);
    int64_t roundtrip = hoo_datetime_compose_utc(f);
    EXPECT_EQ(roundtrip, 1700000000000LL);

    HooDateTimeFields f2 = {};
    f2.year  = 2023;
    f2.month = 11;
    f2.day   = 14;
    f2.hour  = 22;
    f2.minute = 13;
    f2.second = 20;
    int64_t composed_utc = hoo_datetime_compose_utc(f2);
    EXPECT_EQ(composed_utc, 1700000000000LL);

    int64_t composed_local = hoo_datetime_compose(f2);
    EXPECT_NE(composed_local, -1);
}

TEST_F(HooDateTimeTest, Format) {
    char* s1 = hoo_datetime_format(1700000000000LL, "%Y-%m-%d");
    ASSERT_NE(s1, nullptr);
    EXPECT_STREQ(s1, "2023-11-14");
    hoo_datetime_free_string(s1);

    char* s2 = hoo_datetime_format(1700000000000LL, "%H:%M:%S");
    ASSERT_NE(s2, nullptr);
    EXPECT_STREQ(s2, "22:13:20");
    hoo_datetime_free_string(s2);
}

TEST_F(HooDateTimeTest, Iso8601) {
    char* iso = hoo_datetime_iso8601(1700000000000LL);
    ASSERT_NE(iso, nullptr);
    EXPECT_TRUE(strstr(iso, "2023-11-14") != nullptr);
    EXPECT_TRUE(strstr(iso, "22:13:20") != nullptr);
    EXPECT_TRUE(strstr(iso, "Z") != nullptr);
    hoo_datetime_free_string(iso);
}

TEST_F(HooDateTimeTest, AddDays) {
    int64_t base = 1700000000000LL;
    int64_t plus1  = hoo_datetime_add_days(base, 1);
    int64_t minus1 = hoo_datetime_add_days(base, -1);
    int64_t plus7  = hoo_datetime_add_days(base, 7);

    EXPECT_EQ(plus1,  base + 86400000LL);
    EXPECT_EQ(minus1, base - 86400000LL);
    EXPECT_EQ(plus7,  base + 7 * 86400000LL);
}

TEST_F(HooDateTimeTest, AddHours) {
    int64_t base = 1700000000000LL;
    int64_t plus1  = hoo_datetime_add_hours(base, 1);
    int64_t minus1 = hoo_datetime_add_hours(base, -1);
    int64_t plus12 = hoo_datetime_add_hours(base, 12);

    EXPECT_EQ(plus1,  base + 3600000LL);
    EXPECT_EQ(minus1, base - 3600000LL);
    EXPECT_EQ(plus12, base + 12 * 3600000LL);
}

TEST_F(HooDateTimeTest, DiffSeconds) {
    int64_t t1 = 1700000000000LL;
    int64_t t2 = 1700000005000LL;
    double diff = hoo_datetime_diff_seconds(t1, t2);
    EXPECT_DOUBLE_EQ(diff, 5.0);

    double diff_rev = hoo_datetime_diff_seconds(t2, t1);
    EXPECT_DOUBLE_EQ(diff_rev, -5.0);

    double diff_zero = hoo_datetime_diff_seconds(t1, t1);
    EXPECT_DOUBLE_EQ(diff_zero, 0.0);
}

TEST_F(HooDateTimeTest, Compare) {
    int64_t small = 1000000LL;
    int64_t same  = 1000000LL;
    int64_t big   = 2000000LL;

    EXPECT_EQ(hoo_datetime_compare(small, big),  -1);
    EXPECT_EQ(hoo_datetime_compare(big,   small),  1);
    EXPECT_EQ(hoo_datetime_compare(small, same),   0);
}

TEST_F(HooDateTimeTest, Parse) {
    int64_t orig = 1700000000000LL;
    char*  fmt   = hoo_datetime_format(orig, "%Y-%m-%dT%H:%M:%S");
    ASSERT_NE(fmt, nullptr);

    int64_t parsed = hoo_datetime_parse(fmt, "%Y-%m-%dT%H:%M:%S");
    EXPECT_EQ(parsed, orig);

    hoo_datetime_free_string(fmt);
}

TEST_F(HooDateTimeTest, FreeString) {
    hoo_datetime_free_string(nullptr);

    char* s = hoo_datetime_format(1700000000000LL, "%Y-%m-%d");
    ASSERT_NE(s, nullptr);
    hoo_datetime_free_string(s);
}
