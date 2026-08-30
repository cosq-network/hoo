#include "runtime/lib/datetime/hoo_datetime.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include "runtime/lib/string/hoo_string.h"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>
#include <time.h>

#ifdef _WIN32
#include <malloc.h>
#define hoo_strdup _strdup
static struct tm* win_gmtime_r(const time_t* t, struct tm* buf) {
    if (gmtime_s(buf, t) != 0) return nullptr;
    return buf;
}
// Number of days since 1970-01-01 for a civil date (Howard Hinnant's
// civil-from-days algorithm). Handles pre-1970 years correctly, unlike MSVC's
// _mkgmtime which returns -1 for any date before 1970.
static long long days_from_civil(int y, unsigned m, unsigned d) {
    y -= static_cast<int>(m <= 2);
    const long long era = static_cast<long long>((y >= 0 ? y : y - 399) / 400);
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return era * 146097LL + static_cast<long long>(doe) - 719468LL;
}

static time_t win_timegm(struct tm* buf) {
    long long days = days_from_civil(buf->tm_year + 1900,
                                     static_cast<unsigned>(buf->tm_mon + 1),
                                     static_cast<unsigned>(buf->tm_mday));
    long long secs = days * 86400LL
                   + buf->tm_hour * 3600LL
                   + buf->tm_min * 60LL
                   + buf->tm_sec;
    return static_cast<time_t>(secs);
}
static char* win_strptime(const char* s, const char* fmt, struct tm* buf) {
    if (!s || !fmt || !buf) return nullptr;
    int val;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;
            switch (*fmt) {
                case 'Y': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = 0;
                    for (int i = 0; i < 4; i++) {
                        if (*s < '0' || *s > '9') return nullptr;
                        val = val * 10 + (*s - '0');
                        s++;
                    }
                    buf->tm_year = val - 1900;
                    break;
                }
                case 'm': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = (*s - '0') * 10; s++;
                    if (*s < '0' || *s > '9') return nullptr;
                    val += (*s - '0'); s++;
                    if (val < 1 || val > 12) return nullptr;
                    buf->tm_mon = val - 1;
                    break;
                }
                case 'd': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = (*s - '0') * 10; s++;
                    if (*s < '0' || *s > '9') return nullptr;
                    val += (*s - '0'); s++;
                    buf->tm_mday = val;
                    break;
                }
                case 'H': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = (*s - '0') * 10; s++;
                    if (*s < '0' || *s > '9') return nullptr;
                    val += (*s - '0'); s++;
                    buf->tm_hour = val;
                    break;
                }
                case 'M': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = (*s - '0') * 10; s++;
                    if (*s < '0' || *s > '9') return nullptr;
                    val += (*s - '0'); s++;
                    buf->tm_min = val;
                    break;
                }
                case 'S': {
                    if (*s < '0' || *s > '9') return nullptr;
                    val = (*s - '0') * 10; s++;
                    if (*s < '0' || *s > '9') return nullptr;
                    val += (*s - '0'); s++;
                    buf->tm_sec = val;
                    break;
                }
                default: return nullptr;
            }
            fmt++;
        } else {
            if (*s != *fmt) return nullptr;
            s++; fmt++;
        }
    }
    return const_cast<char*>(s);
}
#define gmtime_r win_gmtime_r
#define timegm win_timegm
#define strptime win_strptime
#else
#define hoo_strdup strdup
#endif

// ============================================================================
// Internal Helpers (raw int64 timestamp operations, not part of public API)
// ============================================================================

// Sentinel returned by the parse helpers when the input string cannot be
// parsed. INT64_MIN is never a legitimate epoch-millisecond timestamp, so it
// is unambiguous even for pre-1970 dates (negative timestamps are valid).
#ifndef HOO_DATETIME_PARSE_ERROR
#define HOO_DATETIME_PARSE_ERROR INT64_MIN
#endif

// timegm() legitimately returns -1 for the instant 1969-12-31T23:59:59 UTC,
// so a bare `result == (time_t)-1` check cannot be used to detect failure.
// This helper identifies that single valid instant.
static bool is_epoch_minus_one_second(const struct tm& tm_buf) {
    return tm_buf.tm_year == 69 && tm_buf.tm_mon == 11 && tm_buf.tm_mday == 31 &&
           tm_buf.tm_hour == 23 && tm_buf.tm_min == 59 && tm_buf.tm_sec == 59;
}

static int64_t now_ms(void) {
    auto now = std::chrono::system_clock::now();
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());
}

static HooDateTimeFields decompose(int64_t epoch_ms) {
    HooDateTimeFields fields = {};

    int64_t ms = epoch_ms % 1000;
    if (ms < 0) {
        ms += 1000;
        epoch_ms -= 1000;
    }

    time_t secs = static_cast<time_t>(epoch_ms / 1000);
    struct tm tm_buf;
    if (gmtime_r(&secs, &tm_buf) == nullptr) {
        return fields;
    }

    fields.year        = tm_buf.tm_year + 1900;
    fields.month       = tm_buf.tm_mon + 1;
    fields.day         = tm_buf.tm_mday;
    fields.hour        = tm_buf.tm_hour;
    fields.minute      = tm_buf.tm_min;
    fields.second      = tm_buf.tm_sec;
    fields.millisecond = ms;
    fields.weekday     = tm_buf.tm_wday;
    fields.yearday     = tm_buf.tm_yday;

    return fields;
}

static int64_t compose(HooDateTimeFields f) {
    struct tm tm_buf = {};
    tm_buf.tm_year = static_cast<int>(f.year - 1900);
    tm_buf.tm_mon  = static_cast<int>(f.month - 1);
    tm_buf.tm_mday = static_cast<int>(f.day);
    tm_buf.tm_hour = static_cast<int>(f.hour);
    tm_buf.tm_min  = static_cast<int>(f.minute);
    tm_buf.tm_sec  = static_cast<int>(f.second);

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1) && !is_epoch_minus_one_second(tm_buf)) {
        return HOO_DATETIME_PARSE_ERROR;
    }

    return static_cast<int64_t>(secs) * 1000 + f.millisecond;
}

static int64_t compose_utc(HooDateTimeFields f) {
    return compose(f);
}

static char* format_ts(int64_t epoch_ms, const char* format) {
    if (!format) return nullptr;

    int64_t ms = epoch_ms % 1000;
    if (ms < 0) {
        ms += 1000;
        epoch_ms -= 1000;
    }

    time_t secs = static_cast<time_t>(epoch_ms / 1000);
    struct tm tm_buf;
    if (gmtime_r(&secs, &tm_buf) == nullptr) return nullptr;

    std::string result;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '\0') { result += '%'; break; }

            switch (*p) {
                case '%': result += '%'; break;
                case 'Y': { char buf[8]; snprintf(buf, sizeof(buf), "%04d", tm_buf.tm_year + 1900); result += buf; break; }
                case 'm': { char buf[4]; snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_mon + 1); result += buf; break; }
                case 'd': { char buf[4]; snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_mday); result += buf; break; }
                case 'H': { char buf[4]; snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_hour); result += buf; break; }
                case 'M': { char buf[4]; snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_min); result += buf; break; }
                case 'S': { char buf[4]; snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_sec); result += buf; break; }
                case 'f': { char buf[8]; snprintf(buf, sizeof(buf), "%03lld", static_cast<long long>(ms)); result += buf; break; }
                case 'w': { char buf[4]; snprintf(buf, sizeof(buf), "%d", tm_buf.tm_wday); result += buf; break; }
                case 'j': { char buf[8]; snprintf(buf, sizeof(buf), "%03d", tm_buf.tm_yday); result += buf; break; }
                default: {
                    char fmt[4] = {'%', static_cast<char>(*p), '\0'};
                    char buf[256]; buf[0] = '\0';
                    strftime(buf, sizeof(buf), fmt, &tm_buf);
                    result += buf;
                    break;
                }
            }
            p++;
        } else {
            result += *p;
            p++;
        }
    }

    return hoo_strdup(result.c_str());
}

static int64_t parse_ts(const char* str, const char* format) {
    if (!str || !format) return HOO_DATETIME_PARSE_ERROR;

    struct tm tm_buf = {};
    int64_t ms_part = 0;
    const char* s = str;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '\0') break;

            if (*p == '%') {
                if (*s != '%') return HOO_DATETIME_PARSE_ERROR;
                s++; p++;
            } else if (*p == 'f') {
                int count = 0;
                int64_t val = 0;
                while (*s && *s >= '0' && *s <= '9' && count < 3) {
                    val = val * 10 + (*s - '0');
                    s++; count++;
                }
                if (count == 0) return HOO_DATETIME_PARSE_ERROR;
                while (count < 3) { val *= 10; count++; }
                ms_part = val;
                p++;
            } else {
                char fmt[4] = {'%', *p, '\0'};
                p++;
                char* result = strptime(s, fmt, &tm_buf);
                if (result == nullptr) return HOO_DATETIME_PARSE_ERROR;
                s = result;
            }
        } else {
            if (*s != *p) return HOO_DATETIME_PARSE_ERROR;
            s++; p++;
        }
    }

    // Trailing characters after the format means the string did not fully match.
    if (*s != '\0') return HOO_DATETIME_PARSE_ERROR;

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1) && !is_epoch_minus_one_second(tm_buf)) {
        return HOO_DATETIME_PARSE_ERROR;
    }

    return static_cast<int64_t>(secs) * 1000 + ms_part;
}

static char* iso8601_ts(int64_t epoch_ms) {
    int64_t ms = epoch_ms % 1000;
    if (ms < 0) { ms += 1000; epoch_ms -= 1000; }

    time_t secs = static_cast<time_t>(epoch_ms / 1000);
    struct tm tm_buf;
    if (gmtime_r(&secs, &tm_buf) == nullptr) return nullptr;

    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);

    char result[80];
    snprintf(result, sizeof(result), "%s.%03lldZ", date_buf, static_cast<long long>(ms));

    return hoo_strdup(result);
}

static int64_t from_iso8601_ts(const char* str) {
    if (!str) return HOO_DATETIME_PARSE_ERROR;

    struct tm tm_buf = {};
    int64_t ms_part = 0;
    int tz_minutes = 0;
    int tz_sign = 0;

    const char* s = str;

    char* result = strptime(s, "%Y-%m-%dT%H:%M:%S", &tm_buf);
    if (result == nullptr) {
        result = strptime(s, "%Y-%m-%dT%H:%M", &tm_buf);
        if (result == nullptr) return HOO_DATETIME_PARSE_ERROR;
    }
    s = result;

    if (*s == '.') {
        s++;
        long long val = 0;
        int count = 0;
        while (*s && *s >= '0' && *s <= '9' && count < 9) {
            val = val * 10 + (*s - '0');
            s++; count++;
        }
        if (count < 3) {
            for (int i = count; i < 3; i++) val *= 10;
        } else if (count > 3) {
            for (int i = 3; i < count; i++) val /= 10;
        }
        ms_part = static_cast<int64_t>(val);
    }

    if (*s == 'Z' || *s == 'z') {
        s++;
    } else if (*s == '+' || *s == '-') {
        tz_sign = (*s == '-') ? -1 : 1;
        s++;

        int h = 0, m = 0;
        if (*s >= '0' && *s <= '9') { h = (*s - '0') * 10; s++; if (*s >= '0' && *s <= '9') { h += (*s - '0'); s++; } }
        if (*s == ':') s++;
        if (*s >= '0' && *s <= '9') { m = (*s - '0') * 10; s++; if (*s >= '0' && *s <= '9') { m += (*s - '0'); s++; } }
        tz_minutes = h * 60 + m;
    }

    // Trailing characters after the timezone means the string is not valid ISO 8601.
    if (*s != '\0') return HOO_DATETIME_PARSE_ERROR;

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1) && !is_epoch_minus_one_second(tm_buf)) {
        return HOO_DATETIME_PARSE_ERROR;
    }

    int64_t epoch_ms_result = static_cast<int64_t>(secs) * 1000 + ms_part;
    if (tz_sign != 0) {
        epoch_ms_result -= static_cast<int64_t>(tz_sign) * tz_minutes * 60000LL;
    }

    return epoch_ms_result;
}

// Returns false if `a + b` would overflow int64; otherwise stores the sum.
static bool safe_add(int64_t a, int64_t b, int64_t* out) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
        return false;
    }
    *out = a + b;
    return true;
}

// Adds `n * ms_per_unit` to a timestamp without signed-overflow UB. If the
// delta or the sum is not representable in int64, the original timestamp is
// returned unchanged.
static int64_t add_scaled(int64_t epoch_ms, int64_t n, int64_t ms_per_unit) {
    if (n < INT64_MIN / ms_per_unit || n > INT64_MAX / ms_per_unit) {
        return epoch_ms;
    }
    int64_t delta = n * ms_per_unit;
    int64_t result = 0;
    if (!safe_add(epoch_ms, delta, &result)) {
        return epoch_ms;
    }
    return result;
}

static int64_t add_days(int64_t epoch_ms, int64_t days) {
    return add_scaled(epoch_ms, days, 86400000LL);
}

static int64_t add_hours(int64_t epoch_ms, int64_t hours) {
    return add_scaled(epoch_ms, hours, 3600000LL);
}

static int64_t add_minutes(int64_t epoch_ms, int64_t minutes) {
    return add_scaled(epoch_ms, minutes, 60000LL);
}

static int64_t add_seconds(int64_t epoch_ms, int64_t seconds) {
    return add_scaled(epoch_ms, seconds, 1000LL);
}

static int64_t add_milliseconds(int64_t epoch_ms, int64_t ms) {
    int64_t result = 0;
    if (!safe_add(epoch_ms, ms, &result)) {
        return epoch_ms;
    }
    return result;
}

static int64_t diff_days(int64_t from, int64_t to) {
    return (to - from) / 86400000LL;
}

static int64_t diff_hours(int64_t from, int64_t to) {
    return (to - from) / 3600000LL;
}

static double diff_seconds(int64_t from, int64_t to) {
    return static_cast<double>(to - from) / 1000.0;
}

static int64_t compare_ts(int64_t a, int64_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

// ============================================================================
// Public API — Time Utilities (raw values, no DateTime instance)
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

int64_t hoo_datetime_now_seconds(void) {
    auto now = std::chrono::system_clock::now();
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count());
}

double hoo_datetime_now_precise(void) {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(
        now.time_since_epoch()).count();
}

// ============================================================================
// Public API — DateTime Class Instance API
// ============================================================================

void* hoo_datetime_new(int64_t epoch_ms) {
    void* obj = hoo_alloc(sizeof(int64_t), HOO_TYPE_DATETIME);
    if (!obj) return nullptr;
    int64_t* ts = static_cast<int64_t*>(obj);
    *ts = epoch_ms;
    return obj;
}

void* hoo_datetime_new_now(void) {
    return hoo_datetime_new(now_ms());
}

void* hoo_datetime_new_from_iso8601(const char* str) {
    int64_t ts = from_iso8601_ts(str);
    if (ts == INT64_MIN) return nullptr;
    return hoo_datetime_new(ts);
}

void* hoo_datetime_new_parse(const char* str, const char* format) {
    int64_t ts = parse_ts(str, format);
    if (ts == INT64_MIN) return nullptr;
    return hoo_datetime_new(ts);
}

int64_t hoo_datetime_get_timestamp(void* dt) {
    if (!dt) return 0;
    return *static_cast<int64_t*>(dt);
}

void* hoo_datetime_instance_format(void* dt, const char* format) {
    if (!dt || !format) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    char* result = format_ts(ts, format);
    if (!result) return nullptr;
    void* str = hoo_string_from_cstr(result);
    hoo_datetime_free_string(result);
    return str;
}

void* hoo_datetime_instance_iso8601(void* dt) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    char* result = iso8601_ts(ts);
    if (!result) return nullptr;
    void* str = hoo_string_from_cstr(result);
    hoo_datetime_free_string(result);
    return str;
}

void* hoo_datetime_instance_add_days(void* dt, int64_t days) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return hoo_datetime_new(add_days(ts, days));
}

void* hoo_datetime_instance_add_hours(void* dt, int64_t hours) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return hoo_datetime_new(add_hours(ts, hours));
}

void* hoo_datetime_instance_add_minutes(void* dt, int64_t minutes) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return hoo_datetime_new(add_minutes(ts, minutes));
}

void* hoo_datetime_instance_add_seconds(void* dt, int64_t seconds) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return hoo_datetime_new(add_seconds(ts, seconds));
}

void* hoo_datetime_instance_add_milliseconds(void* dt, int64_t ms) {
    if (!dt) return nullptr;
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return hoo_datetime_new(add_milliseconds(ts, ms));
}

int64_t hoo_datetime_instance_diff_days(void* from, void* to) {
    if (!from || !to) return 0;
    int64_t ts_from = hoo_datetime_get_timestamp(from);
    int64_t ts_to = hoo_datetime_get_timestamp(to);
    return diff_days(ts_from, ts_to);
}

int64_t hoo_datetime_instance_diff_hours(void* from, void* to) {
    if (!from || !to) return 0;
    int64_t ts_from = hoo_datetime_get_timestamp(from);
    int64_t ts_to = hoo_datetime_get_timestamp(to);
    return diff_hours(ts_from, ts_to);
}

double hoo_datetime_instance_diff_seconds(void* from, void* to) {
    if (!from || !to) return 0.0;
    int64_t ts_from = hoo_datetime_get_timestamp(from);
    int64_t ts_to = hoo_datetime_get_timestamp(to);
    return diff_seconds(ts_from, ts_to);
}

int64_t hoo_datetime_instance_compare(void* a, void* b) {
    if (!a || !b) return 0;
    int64_t ts_a = hoo_datetime_get_timestamp(a);
    int64_t ts_b = hoo_datetime_get_timestamp(b);
    return compare_ts(ts_a, ts_b);
}

HooDateTimeFields hoo_datetime_instance_decompose(void* dt) {
    if (!dt) return HooDateTimeFields{};
    int64_t ts = hoo_datetime_get_timestamp(dt);
    return decompose(ts);
}

// ============================================================================
// Module-Level Free Function API (thin wrappers around instance API)
// ============================================================================

void* hoo_datetime_now(void) {
    return hoo_datetime_new_now();
}

void* hoo_datetime_from_iso8601(const char* str) {
    return hoo_datetime_new_from_iso8601(str);
}

void* hoo_datetime_parse(const char* str, const char* format) {
    return hoo_datetime_new_parse(str, format);
}

void* hoo_datetime_format(void* dt, const char* format) {
    return hoo_datetime_instance_format(dt, format);
}

void* hoo_datetime_iso8601(void* dt) {
    return hoo_datetime_instance_iso8601(dt);
}

void* hoo_datetime_add_days(void* dt, int64_t days) {
    return hoo_datetime_instance_add_days(dt, days);
}

void* hoo_datetime_add_hours(void* dt, int64_t hours) {
    return hoo_datetime_instance_add_hours(dt, hours);
}

void* hoo_datetime_add_minutes(void* dt, int64_t minutes) {
    return hoo_datetime_instance_add_minutes(dt, minutes);
}

void* hoo_datetime_add_seconds(void* dt, int64_t seconds) {
    return hoo_datetime_instance_add_seconds(dt, seconds);
}

void* hoo_datetime_add_milliseconds(void* dt, int64_t ms) {
    return hoo_datetime_instance_add_milliseconds(dt, ms);
}

int64_t hoo_datetime_diff_days(void* from, void* to) {
    return hoo_datetime_instance_diff_days(from, to);
}

int64_t hoo_datetime_diff_hours(void* from, void* to) {
    return hoo_datetime_instance_diff_hours(from, to);
}

double hoo_datetime_diff_seconds(void* from, void* to) {
    return hoo_datetime_instance_diff_seconds(from, to);
}

int64_t hoo_datetime_compare(void* a, void* b) {
    return hoo_datetime_instance_compare(a, b);
}

HooDateTimeFields hoo_datetime_decompose(void* dt) {
    return hoo_datetime_instance_decompose(dt);
}

// ============================================================================
// Memory Management
// ============================================================================

void hoo_datetime_free_string(char* str) {
    free(str);
}

#ifdef __cplusplus
}
#endif
