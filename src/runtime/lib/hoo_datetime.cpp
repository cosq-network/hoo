#include "hoo_datetime.h"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <string>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Current Time
// ============================================================================

int64_t hoo_datetime_now(void) {
    auto now = std::chrono::system_clock::now();
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());
}

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
// Decompose / Compose
// ============================================================================

HooDateTimeFields hoo_datetime_decompose(int64_t epoch_ms) {
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

int64_t hoo_datetime_compose(HooDateTimeFields fields) {
    struct tm tm_buf = {};
    tm_buf.tm_year = static_cast<int>(fields.year - 1900);
    tm_buf.tm_mon  = static_cast<int>(fields.month - 1);
    tm_buf.tm_mday = static_cast<int>(fields.day);
    tm_buf.tm_hour = static_cast<int>(fields.hour);
    tm_buf.tm_min  = static_cast<int>(fields.minute);
    tm_buf.tm_sec  = static_cast<int>(fields.second);
    tm_buf.tm_isdst = -1;

    time_t secs = mktime(&tm_buf);
    if (secs == static_cast<time_t>(-1)) {
        return -1;
    }

    return static_cast<int64_t>(secs) * 1000 + fields.millisecond;
}

int64_t hoo_datetime_compose_utc(HooDateTimeFields fields) {
    struct tm tm_buf = {};
    tm_buf.tm_year = static_cast<int>(fields.year - 1900);
    tm_buf.tm_mon  = static_cast<int>(fields.month - 1);
    tm_buf.tm_mday = static_cast<int>(fields.day);
    tm_buf.tm_hour = static_cast<int>(fields.hour);
    tm_buf.tm_min  = static_cast<int>(fields.minute);
    tm_buf.tm_sec  = static_cast<int>(fields.second);

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1)) {
        return -1;
    }

    return static_cast<int64_t>(secs) * 1000 + fields.millisecond;
}

// ============================================================================
// Formatting and Parsing
// ============================================================================

char* hoo_datetime_format(int64_t epoch_ms, const char* format) {
    if (!format) return nullptr;

    int64_t ms = epoch_ms % 1000;
    if (ms < 0) {
        ms += 1000;
        epoch_ms -= 1000;
    }

    time_t secs = static_cast<time_t>(epoch_ms / 1000);
    struct tm tm_buf;
    if (gmtime_r(&secs, &tm_buf) == nullptr) {
        return nullptr;
    }

    std::string result;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '\0') {
                result += '%';
                break;
            }

            switch (*p) {
                case '%': result += '%'; break;
                case 'Y': {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%04d", tm_buf.tm_year + 1900);
                    result += buf;
                    break;
                }
                case 'm': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_mon + 1);
                    result += buf;
                    break;
                }
                case 'd': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_mday);
                    result += buf;
                    break;
                }
                case 'H': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_hour);
                    result += buf;
                    break;
                }
                case 'M': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_min);
                    result += buf;
                    break;
                }
                case 'S': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02d", tm_buf.tm_sec);
                    result += buf;
                    break;
                }
                case 'f': {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%03lld", static_cast<long long>(ms));
                    result += buf;
                    break;
                }
                case 'w': {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%d", tm_buf.tm_wday);
                    result += buf;
                    break;
                }
                case 'j': {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%03d", tm_buf.tm_yday);
                    result += buf;
                    break;
                }
                default: {
                    char fmt[4] = {'%', static_cast<char>(*p), '\0'};
                    char buf[256];
                    buf[0] = '\0';
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

    return strdup(result.c_str());
}

int64_t hoo_datetime_parse(const char* str, const char* format) {
    if (!str || !format) return -1;

    struct tm tm_buf = {};
    int64_t ms_part = 0;
    const char* s = str;
    const char* p = format;

    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '\0') break;

            if (*p == '%') {
                if (*s != '%') return -1;
                s++;
                p++;
            } else if (*p == 'f') {
                int count = 0;
                int64_t val = 0;
                while (*s && *s >= '0' && *s <= '9' && count < 3) {
                    val = val * 10 + (*s - '0');
                    s++;
                    count++;
                }
                if (count == 0) return -1;
                while (count < 3) { val *= 10; count++; }
                ms_part = val;
                p++;
            } else {
                char fmt[4] = {'%', *p, '\0'};
                p++;
                char* result = strptime(s, fmt, &tm_buf);
                if (result == nullptr) return -1;
                s = result;
            }
        } else {
            if (*s != *p) return -1;
            s++;
            p++;
        }
    }

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1)) return -1;

    return static_cast<int64_t>(secs) * 1000 + ms_part;
}

char* hoo_datetime_iso8601(int64_t epoch_ms) {
    int64_t ms = epoch_ms % 1000;
    if (ms < 0) {
        ms += 1000;
        epoch_ms -= 1000;
    }

    time_t secs = static_cast<time_t>(epoch_ms / 1000);
    struct tm tm_buf;
    if (gmtime_r(&secs, &tm_buf) == nullptr) {
        return nullptr;
    }

    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);

    char result[80];
    snprintf(result, sizeof(result), "%s.%03lldZ", date_buf, static_cast<long long>(ms));

    return strdup(result);
}

int64_t hoo_datetime_from_iso8601(const char* str) {
    if (!str) return -1;

    struct tm tm_buf = {};
    int64_t ms_part = 0;
    int tz_minutes = 0;
    int tz_sign = 0;

    const char* s = str;

    char* result = strptime(s, "%Y-%m-%dT%H:%M:%S", &tm_buf);
    if (result == nullptr) {
        result = strptime(s, "%Y-%m-%dT%H:%M", &tm_buf);
        if (result == nullptr) {
            return -1;
        }
    }
    s = result;

    if (*s == '.') {
        s++;
        long long val = 0;
        int count = 0;
        while (*s && *s >= '0' && *s <= '9' && count < 9) {
            val = val * 10 + (*s - '0');
            s++;
            count++;
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
        if (*s >= '0' && *s <= '9') {
            h = (*s - '0') * 10;
            s++;
            if (*s >= '0' && *s <= '9') {
                h += (*s - '0');
                s++;
            }
        }
        if (*s == ':') s++;
        if (*s >= '0' && *s <= '9') {
            m = (*s - '0') * 10;
            s++;
            if (*s >= '0' && *s <= '9') {
                m += (*s - '0');
                s++;
            }
        }
        tz_minutes = h * 60 + m;
    }

    time_t secs = timegm(&tm_buf);
    if (secs == static_cast<time_t>(-1)) return -1;

    int64_t epoch_ms_result = static_cast<int64_t>(secs) * 1000 + ms_part;

    if (tz_sign != 0) {
        epoch_ms_result -= static_cast<int64_t>(tz_sign) * tz_minutes * 60000LL;
    }

    return epoch_ms_result;
}

// ============================================================================
// Duration Helpers
// ============================================================================

int64_t hoo_datetime_add_days(int64_t epoch_ms, int64_t days) {
    return epoch_ms + days * 86400000LL;
}

int64_t hoo_datetime_add_hours(int64_t epoch_ms, int64_t hours) {
    return epoch_ms + hours * 3600000LL;
}

int64_t hoo_datetime_add_minutes(int64_t epoch_ms, int64_t minutes) {
    return epoch_ms + minutes * 60000LL;
}

int64_t hoo_datetime_add_seconds(int64_t epoch_ms, int64_t seconds) {
    return epoch_ms + seconds * 1000LL;
}

int64_t hoo_datetime_add_milliseconds(int64_t epoch_ms, int64_t ms) {
    return epoch_ms + ms;
}

int64_t hoo_datetime_diff_days(int64_t from, int64_t to) {
    return (to - from) / 86400000LL;
}

int64_t hoo_datetime_diff_hours(int64_t from, int64_t to) {
    return (to - from) / 3600000LL;
}

double hoo_datetime_diff_seconds(int64_t from, int64_t to) {
    return static_cast<double>(to - from) / 1000.0;
}

// ============================================================================
// Comparison
// ============================================================================

int64_t hoo_datetime_compare(int64_t a, int64_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
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
