#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooDateTime - Date and Time Instance API
// ============================================================================
//
// DateTime is an ARC-managed object (type ID 119) containing a single
// int64_t field 'timestamp' at offset 0 (after the 16-byte ARC header).
// All operations return string values as ARC-managed HooString objects
// unless otherwise noted.

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
// Time Utilities (raw values, no DateTime instance required)
// ============================================================================

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
// DateTime Class Instance API
// ============================================================================

/**
 * Create a new DateTime instance with the given timestamp.
 * @param epoch_ms Timestamp in milliseconds since Unix epoch
 * @return ARC-managed DateTime handle (caller must retain if storing beyond current scope)
 */
void* hoo_datetime_new(int64_t epoch_ms);

/**
 * Create a new DateTime instance representing the current system time.
 * @return ARC-managed DateTime handle
 */
void* hoo_datetime_new_now(void);

/**
 * Create a new DateTime instance by parsing an ISO 8601 string.
 * @param str ISO 8601 string (e.g. "2024-01-15T10:30:00Z")
 * @return ARC-managed DateTime handle, or null on parse failure
 */
void* hoo_datetime_new_from_iso8601(const char* str);

/**
 * Create a new DateTime instance by parsing a custom-format string.
 * @param str Input string to parse
 * @param format Format string (strftime-style)
 * @return ARC-managed DateTime handle, or null on parse failure
 */
void* hoo_datetime_new_parse(const char* str, const char* format);

/**
 * Extract the timestamp from a DateTime instance.
 * @param dt DateTime handle
 * @return Timestamp in milliseconds since Unix epoch
 */
int64_t hoo_datetime_get_timestamp(void* dt);

/**
 * Format a DateTime instance using a strftime-style format string.
 * @param dt DateTime handle
 * @param format Format string
 * @return ARC-managed HooString
 */
void* hoo_datetime_instance_format(void* dt, const char* format);

/**
 * Format a DateTime instance as ISO 8601 string.
 * @param dt DateTime handle
 * @return ARC-managed HooString
 */
void* hoo_datetime_instance_iso8601(void* dt);

/**
 * Add days to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param days Number of days to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_instance_add_days(void* dt, int64_t days);

/**
 * Add hours to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param hours Number of hours to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_instance_add_hours(void* dt, int64_t hours);

/**
 * Add minutes to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param minutes Number of minutes to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_instance_add_minutes(void* dt, int64_t minutes);

/**
 * Add seconds to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param seconds Number of seconds to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_instance_add_seconds(void* dt, int64_t seconds);

/**
 * Add milliseconds to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param ms Milliseconds to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_instance_add_milliseconds(void* dt, int64_t ms);

/**
 * Calculate difference in days between two DateTime instances.
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of days
 */
int64_t hoo_datetime_instance_diff_days(void* from, void* to);

/**
 * Calculate difference in hours between two DateTime instances.
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of hours
 */
int64_t hoo_datetime_instance_diff_hours(void* from, void* to);

/**
 * Calculate difference in seconds between two DateTime instances (fractional).
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of seconds as double
 */
double hoo_datetime_instance_diff_seconds(void* from, void* to);

/**
 * Compare two DateTime instances.
 * @param a First DateTime handle
 * @param b Second DateTime handle
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int64_t hoo_datetime_instance_compare(void* a, void* b);

/**
 * Decompose a DateTime instance into date/time fields.
 * @param dt DateTime handle
 * @return Decomposed date/time fields
 */
HooDateTimeFields hoo_datetime_instance_decompose(void* dt);

// ============================================================================
// Module-Level Free Function API
// ============================================================================
//
// Convenience wrappers that delegate to the instance API. These match the
// `datetime_*()` module-level functions available in the Hoo language.
// All DateTime parameters and return values are ARC-managed handles.

/**
 * Get current time as a DateTime instance.
 * @return ARC-managed DateTime handle
 */
void* hoo_datetime_now(void);

/**
 * Parse an ISO 8601 string into a DateTime instance.
 * @param str ISO 8601 string (e.g. "2024-01-15T10:30:00Z")
 * @return ARC-managed DateTime handle, or null on parse failure
 */
void* hoo_datetime_from_iso8601(const char* str);

/**
 * Parse a date-time string using a custom format.
 * @param str Input string to parse
 * @param format Format string (strftime-style)
 * @return ARC-managed DateTime handle, or null on parse failure
 */
void* hoo_datetime_parse(const char* str, const char* format);

/**
 * Format a DateTime instance using a strftime-style format string.
 * @param dt DateTime handle
 * @param format Format string
 * @return ARC-managed HooString
 */
void* hoo_datetime_format(void* dt, const char* format);

/**
 * Format a DateTime instance as ISO 8601 string.
 * @param dt DateTime handle
 * @return ARC-managed HooString
 */
void* hoo_datetime_iso8601(void* dt);

/**
 * Add days to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param days Number of days to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_add_days(void* dt, int64_t days);

/**
 * Add hours to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param hours Number of hours to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_add_hours(void* dt, int64_t hours);

/**
 * Add minutes to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param minutes Number of minutes to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_add_minutes(void* dt, int64_t minutes);

/**
 * Add seconds to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param seconds Number of seconds to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_add_seconds(void* dt, int64_t seconds);

/**
 * Add milliseconds to a DateTime instance (returns a new DateTime).
 * @param dt DateTime handle
 * @param ms Milliseconds to add (can be negative)
 * @return New ARC-managed DateTime handle
 */
void* hoo_datetime_add_milliseconds(void* dt, int64_t ms);

/**
 * Calculate difference in days between two DateTime instances.
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of days
 */
int64_t hoo_datetime_diff_days(void* from, void* to);

/**
 * Calculate difference in hours between two DateTime instances.
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of hours
 */
int64_t hoo_datetime_diff_hours(void* from, void* to);

/**
 * Calculate difference in seconds between two DateTime instances (fractional).
 * @param from First DateTime handle
 * @param to Second DateTime handle
 * @return Number of seconds as double
 */
double hoo_datetime_diff_seconds(void* from, void* to);

/**
 * Compare two DateTime instances.
 * @param a First DateTime handle
 * @param b Second DateTime handle
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int64_t hoo_datetime_compare(void* a, void* b);

/**
 * Decompose a DateTime instance into date/time fields.
 * @param dt DateTime handle
 * @return Decomposed date/time fields
 */
HooDateTimeFields hoo_datetime_decompose(void* dt);

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
