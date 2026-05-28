#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooDateTime - Date and Time Functions
// ============================================================================
//
// Provides date/time manipulation functions for the hoo.datetime module.
// All timestamps are int64_t representing milliseconds since Unix epoch (1970-01-01 UTC).
// Functions returning char* allocate strings that the caller must free with hoo_datetime_free_string.

// ============================================================================
// DateTime Fields
// ============================================================================

/**
 * Decomposed date and time fields.
 * All fields use int64_t for consistent C-ABI across languages.
 */
typedef struct {
    int64_t year;
    int64_t month;       // 1-12
    int64_t day;         // 1-31
    int64_t hour;        // 0-23
    int64_t minute;      // 0-59
    int64_t second;      // 0-59
    int64_t millisecond; // 0-999
    int64_t weekday;     // 0=Sunday, 1=Monday, ..., 6=Saturday
    int64_t yearday;     // 0-365
} HooDateTimeFields;

// ============================================================================
// Current Time
// ============================================================================

/**
 * Get current time as Unix epoch milliseconds
 * @return Milliseconds since Unix epoch
 */
int64_t hoo_datetime_now(void);

/**
 * Get current time as Unix epoch seconds
 * @return Seconds since Unix epoch
 */
int64_t hoo_datetime_now_seconds(void);

/**
 * Get current time with sub-second precision
 * @return Seconds since Unix epoch as double
 */
double hoo_datetime_now_precise(void);

// ============================================================================
// Decompose / Compose
// ============================================================================

/**
 * Decompose a timestamp into date/time fields
 * @param epoch_ms Timestamp in milliseconds
 * @return Decomposed date/time fields
 */
HooDateTimeFields hoo_datetime_decompose(int64_t epoch_ms);

/**
 * Compose date/time fields into a timestamp (local time)
 * @param fields Date/time fields
 * @return Timestamp in milliseconds
 */
int64_t hoo_datetime_compose(HooDateTimeFields fields);

/**
 * Compose date/time fields into a timestamp (UTC)
 * @param fields Date/time fields
 * @return Timestamp in milliseconds
 */
int64_t hoo_datetime_compose_utc(HooDateTimeFields fields);

// ============================================================================
// Formatting and Parsing
// ============================================================================

/**
 * Format a timestamp using strftime-style format string
 * Supported specifiers: %Y, %m, %d, %H, %M, %S, %f (milliseconds), %w (weekday), %j (yearday)
 * @param epoch_ms Timestamp in milliseconds
 * @param format Format string
 * @return Allocated formatted string (caller must free with hoo_datetime_free_string)
 */
char* hoo_datetime_format(int64_t epoch_ms, const char* format);

/**
 * Parse a string using strftime-style format string
 * @param str Input string to parse
 * @param format Format string
 * @return Timestamp in milliseconds, or -1 on failure
 */
int64_t hoo_datetime_parse(const char* str, const char* format);

/**
 * Format a timestamp as ISO 8601 string ("2024-01-15T10:30:00Z")
 * @param epoch_ms Timestamp in milliseconds
 * @return Allocated ISO 8601 string (caller must free with hoo_datetime_free_string)
 */
char* hoo_datetime_iso8601(int64_t epoch_ms);

/**
 * Parse an ISO 8601 string into a timestamp
 * @param str ISO 8601 string
 * @return Timestamp in milliseconds, or -1 on failure
 */
int64_t hoo_datetime_from_iso8601(const char* str);

// ============================================================================
// Duration Helpers
// ============================================================================

/**
 * Add days to a timestamp
 * @param epoch_ms Base timestamp in milliseconds
 * @param days Number of days to add (can be negative)
 * @return Resulting timestamp in milliseconds
 */
int64_t hoo_datetime_add_days(int64_t epoch_ms, int64_t days);

/**
 * Add hours to a timestamp
 * @param epoch_ms Base timestamp in milliseconds
 * @param hours Number of hours to add (can be negative)
 * @return Resulting timestamp in milliseconds
 */
int64_t hoo_datetime_add_hours(int64_t epoch_ms, int64_t hours);

/**
 * Add minutes to a timestamp
 * @param epoch_ms Base timestamp in milliseconds
 * @param minutes Number of minutes to add (can be negative)
 * @return Resulting timestamp in milliseconds
 */
int64_t hoo_datetime_add_minutes(int64_t epoch_ms, int64_t minutes);

/**
 * Add seconds to a timestamp
 * @param epoch_ms Base timestamp in milliseconds
 * @param seconds Number of seconds to add (can be negative)
 * @return Resulting timestamp in milliseconds
 */
int64_t hoo_datetime_add_seconds(int64_t epoch_ms, int64_t seconds);

/**
 * Add milliseconds to a timestamp
 * @param epoch_ms Base timestamp in milliseconds
 * @param ms Milliseconds to add (can be negative)
 * @return Resulting timestamp in milliseconds
 */
int64_t hoo_datetime_add_milliseconds(int64_t epoch_ms, int64_t ms);

/**
 * Calculate difference in days between two timestamps
 * @param from Start timestamp in milliseconds
 * @param to End timestamp in milliseconds
 * @return Number of days
 */
int64_t hoo_datetime_diff_days(int64_t from, int64_t to);

/**
 * Calculate difference in hours between two timestamps
 * @param from Start timestamp in milliseconds
 * @param to End timestamp in milliseconds
 * @return Number of hours
 */
int64_t hoo_datetime_diff_hours(int64_t from, int64_t to);

/**
 * Calculate difference in seconds between two timestamps (with fractional precision)
 * @param from Start timestamp in milliseconds
 * @param to End timestamp in milliseconds
 * @return Number of seconds as double
 */
double hoo_datetime_diff_seconds(int64_t from, int64_t to);

// ============================================================================
// Comparison
// ============================================================================

/**
 * Compare two timestamps
 * @param a First timestamp in milliseconds
 * @param b Second timestamp in milliseconds
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int64_t hoo_datetime_compare(int64_t a, int64_t b);

// ============================================================================
// Memory Management
// ============================================================================

/**
 * Free a string allocated by a hoo_datetime_* function
 * @param str String to free
 */
void hoo_datetime_free_string(char* str);

#ifdef __cplusplus
}
#endif
