#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooString - UTF-8 String Type with Reference Counting
// ============================================================================
//
// Opaque handle representing a hoo string value.
// Internally managed with automatic reference counting.
// All strings are UTF-8 encoded and null-terminated for C compatibility.
//

typedef void* HooString;
typedef void* HooArray;

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a string from a null-terminated C string (UTF-8)
 *
 * The returned HooString has refcount=1. Caller is responsible for
 * releasing when no longer needed.
 *
 * @param cstr Null-terminated UTF-8 C string (may be NULL)
 * @return New HooString with refcount=1, or NULL on allocation failure
 */
HooString hoo_string_from_cstr(const char* cstr);

/**
 * Create an empty string
 *
 * @return New empty HooString with refcount=1
 */
HooString hoo_string_new(void);

/**
 * Create a string from raw bytes with explicit length
 *
 * Useful for binary-safe string creation or UTF-8 data without null terminator.
 * The result will be null-terminated for C compatibility.
 *
 * @param data Pointer to UTF-8 data (may be NULL)
 * @param length Number of bytes to copy
 * @return New HooString with refcount=1
 */
HooString hoo_string_from_bytes(const char* data, int64_t length);

/**
 * Create a string by repeating a character n times
 *
 * @param ch Character to repeat
 * @param count Number of times to repeat
 * @return New HooString containing the repeated character
 */
HooString hoo_string_repeat(char ch, int64_t count);

// ============================================================================
// String Manipulation
// ============================================================================

/**
 * Concatenate two strings
 *
 * Returns a new string containing dst followed by src.
 * Neither input string is modified.
 *
 * @param dst First string (may be NULL, treated as empty)
 * @param src Second string (may be NULL, treated as empty)
 * @return New concatenated HooString with refcount=1
 */
HooString hoo_string_concat(HooString dst, HooString src);

/**
 * Get a substring
 *
 * @param str Source string
 * @param start Starting byte index (0-based)
 * @param length Number of bytes to extract
 * @return New HooString containing the substring
 */
HooString hoo_string_substring(HooString str, int64_t start, int64_t length);

/**
 * Convert string to uppercase
 *
 * Only ASCII letters are converted (A-Z).
 *
 * @param str Source string
 * @return New uppercase HooString
 */
HooString hoo_string_to_upper(HooString str);

/**
 * Convert string to lowercase
 *
 * Only ASCII letters are converted (a-z).
 *
 * @param str Source string
 * @return New lowercase HooString
 */
HooString hoo_string_to_lower(HooString str);

/**
 * Trim leading and trailing whitespace
 *
 * Whitespace includes spaces, tabs, newlines, carriage returns.
 *
 * @param str Source string
 * @return New trimmed HooString
 */
HooString hoo_string_trim(HooString str);

/**
 * Replace all occurrences of a substring
 *
 * @param str Source string
 * @param old Pattern to search for
 * @param replacement Replacement string
 * @return New string with replacements made
 */
HooString hoo_string_replace(HooString str, HooString old, HooString replacement);

/**
 * Split string by delimiter
 *
 * Returns an array of strings split by the delimiter.
 * Empty strings are not included in the result.
 *
 * @param str Source string
 * @param delimiter Delimiter string
 * @return HooArray of HooString (must be released by caller)
 */
HooArray hoo_string_split(HooString str, HooString delimiter);

// ============================================================================
// String Query
// ============================================================================

/**
 * Get the length of a string in bytes
 *
 * For ASCII strings, this is the character count.
 * For UTF-8 strings with multi-byte characters, this is byte count.
 *
 * @param str String to measure (may be NULL)
 * @return Length in bytes, or 0 if str is NULL
 */
int64_t hoo_string_length(HooString str);

/**
 * Get pointer to internal UTF-8 data
 *
 * The returned pointer is valid as long as the HooString is alive.
 * The data is null-terminated and read-only.
 *
 * DO NOT modify the returned data or free the pointer.
 *
 * @param str String to get data from (may be NULL)
 * @return Pointer to UTF-8 data, or empty string "" if str is NULL
 */
const char* hoo_string_data(HooString str);

/**
 * Get byte at specific index
 *
 * Returns the byte value at the given index.
 *
 * @param str String to index
 * @param index Byte index (0-based)
 * @return Byte value (0-255), or -1 if index out of bounds
 */
int64_t hoo_string_byte_at(HooString str, int64_t index);

/**
 * Check if string is empty
 *
 * @param str String to check
 * @return 1 if empty (length == 0), 0 otherwise
 */
int64_t hoo_string_is_empty(HooString str);

/**
 * Find first occurrence of a substring
 *
 * @param haystack String to search in
 * @param needle Pattern to search for
 * @return Byte index of first match, or -1 if not found
 */
int64_t hoo_string_index_of(HooString haystack, HooString needle);

/**
 * Find last occurrence of a substring
 *
 * @param haystack String to search in
 * @param needle Pattern to search for
 * @return Byte index of last match, or -1 if not found
 */
int64_t hoo_string_last_index_of(HooString haystack, HooString needle);

/**
 * Check if string contains a substring
 *
 * @param haystack String to search in
 * @param needle Pattern to search for
 * @return 1 if found, 0 otherwise
 */
int64_t hoo_string_contains(HooString haystack, HooString needle);

/**
 * Check if string starts with prefix
 *
 * @param str String to check
 * @param prefix Prefix pattern
 * @return 1 if starts with prefix, 0 otherwise
 */
int64_t hoo_string_starts_with(HooString str, HooString prefix);

/**
 * Check if string ends with suffix
 *
 * @param str String to check
 * @param suffix Suffix pattern
 * @return 1 if ends with suffix, 0 otherwise
 */
int64_t hoo_string_ends_with(HooString str, HooString suffix);

// ============================================================================
// String Comparison
// ============================================================================

/**
 * Compare two strings
 *
 * Performs lexicographic comparison.
 *
 * @param a First string
 * @param b Second string
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int64_t hoo_string_compare(HooString a, HooString b);

/**
 * Check if two strings are equal
 *
 * @param a First string
 * @param b Second string
 * @return 1 if strings are equal, 0 otherwise
 */
int64_t hoo_string_equals(HooString a, HooString b);

/**
 * Check if two strings are equal (case-insensitive for ASCII)
 *
 * Only ASCII letters are compared case-insensitively.
 *
 * @param a First string
 * @param b Second string
 * @return 1 if strings are equal (ignoring case), 0 otherwise
 */
int64_t hoo_string_equals_ignore_case(HooString a, HooString b);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count
 *
 * Used when assigning a string to a new variable.
 * Every hoo_string_retain() should be paired with hoo_string_release().
 *
 * @param str String to retain (may be NULL)
 * @return The same string (for chaining)
 */
HooString hoo_string_retain(HooString str);

/**
 * Decrement reference count and free if reaches zero
 *
 * Should be called when a string variable goes out of scope.
 *
 * @param str String to release (may be NULL)
 */
void hoo_string_release(HooString str);

/**
 * Get current reference count (for debugging)
 *
 * @param str String to check (may be NULL)
 * @return Current refcount, or 0 if str is NULL
 */
int64_t hoo_string_refcount(HooString str);

// ============================================================================
// Conversion and Formatting
// ============================================================================

/**
 * Convert integer to string
 *
 * @param value Integer value to convert
 * @return New HooString representing the integer
 */
HooString hoo_string_from_int64(int64_t value);

/**
 * Convert double to string with default precision
 *
 * @param value Double value to convert
 * @return New HooString representing the double
 */
HooString hoo_string_from_double(double value);

/**
 * Convert boolean to string
 *
 * Returns "true" or "false".
 *
 * @param value Boolean value
 * @return New HooString ("true" or "false")
 */
HooString hoo_string_from_bool(int64_t value);

/**
 * Convert string to integer
 *
 * Parses leading decimal digits. Returns 0 if no digits found.
 *
 * @param str String to parse
 * @return Parsed integer value
 */
int64_t hoo_string_to_int64(HooString str);

/**
 * Convert string to double
 *
 * Parses floating point format. Returns 0.0 if parsing fails.
 *
 * @param str String to parse
 * @return Parsed double value
 */
double hoo_string_to_double(HooString str);

/**
 * Format a string (printf-style)
 *
 * Supports: %s (string), %d (int64), %f (double), %x (hex), %%
 *
 * @param format Format string
 * @param ... Arguments corresponding to format specifiers
 * @return New formatted HooString
 */
HooString hoo_string_format(const char* format, ...);

// ============================================================================
// Debugging and Utilities
// ============================================================================

/**
 * Print string to stdout
 *
 * Outputs the string content without newline.
 *
 * @param str String to print (may be NULL)
 */
void hoo_string_print(HooString str);

/**
 * Print string to stdout with newline
 *
 * @param str String to print (may be NULL)
 */
void hoo_string_println(HooString str);

/**
 * Get debug representation of string
 *
 * Returns a new string showing the contents and metadata.
 * Useful for debugging.
 *
 * @param str String to inspect
 * @return New debug string (must be released by caller)
 */
HooString hoo_string_debug(HooString str);

#ifdef __cplusplus
}
#endif
