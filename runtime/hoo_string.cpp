#include "hoo_string.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdarg>

// ============================================================================
// Internal Structure (Hidden from hoo Code)
// ============================================================================

/**
 * Internal string header containing metadata.
 * The actual string data follows immediately after in memory.
 */
struct HooStringImpl {
    int64_t refcount;    // Reference count for ARC
    int64_t length;      // Byte length (UTF-8)
    int64_t capacity;    // Allocated capacity (including null terminator)
    char data[1];        // Flexible array member - actual string data
};

// Size of header before the data field
#define HEADER_SIZE offsetof(HooStringImpl, data)

// ============================================================================
// Utility Functions
// ============================================================================

static HooStringImpl* get_impl(HooString str) {
    return (HooStringImpl*)str;
}

static HooString from_impl(HooStringImpl* impl) {
    return (HooString)impl;
}

/**
 * Internal allocation - creates a new string with given capacity
 */
static HooString allocate_string(int64_t length) {
    if (length < 0) length = 0;

    size_t total_size = HEADER_SIZE + length + 1;  // +1 for null terminator

    HooStringImpl* impl = (HooStringImpl*)std::malloc(total_size);
    if (!impl) {
        std::fprintf(stderr, "ERROR: Out of memory allocating HooString (size: %zu bytes)\n", total_size);
        std::exit(1);
    }

    impl->refcount = 1;
    impl->length = length;
    impl->capacity = length + 1;
    impl->data[length] = '\0';  // Always null-terminate

    return from_impl(impl);
}

// ============================================================================
// Creation and Destruction
// ============================================================================

HooString hoo_string_from_cstr(const char* cstr) {
    if (!cstr) {
        return hoo_string_new();
    }

    int64_t len = std::strlen(cstr);
    HooString str = allocate_string(len);
    HooStringImpl* impl = get_impl(str);
    std::memcpy(impl->data, cstr, len);

    return str;
}

HooString hoo_string_new(void) {
    return allocate_string(0);
}

HooString hoo_string_from_bytes(const char* data, int64_t length) {
    if (!data || length <= 0) {
        return hoo_string_new();
    }

    HooString str = allocate_string(length);
    HooStringImpl* impl = get_impl(str);
    std::memcpy(impl->data, data, length);

    return str;
}

HooString hoo_string_repeat(char ch, int64_t count) {
    if (count <= 0) {
        return hoo_string_new();
    }

    HooString str = allocate_string(count);
    HooStringImpl* impl = get_impl(str);
    std::memset(impl->data, (unsigned char)ch, count);

    return str;
}

// ============================================================================
// String Manipulation
// ============================================================================

HooString hoo_string_concat(HooString dst, HooString src) {
    // Handle NULL inputs
    if (!dst) dst = hoo_string_new();
    if (!src) src = hoo_string_new();

    HooStringImpl* dst_impl = get_impl(dst);
    HooStringImpl* src_impl = get_impl(src);

    int64_t new_length = dst_impl->length + src_impl->length;
    HooString result = allocate_string(new_length);
    HooStringImpl* result_impl = get_impl(result);

    // Copy destination, then source
    std::memcpy(result_impl->data, dst_impl->data, dst_impl->length);
    std::memcpy(result_impl->data + dst_impl->length, src_impl->data, src_impl->length);

    return result;
}

HooString hoo_string_substring(HooString str, int64_t start, int64_t length) {
    if (!str) {
        return hoo_string_new();
    }

    HooStringImpl* impl = get_impl(str);

    // Clamp start and length to valid bounds
    if (start < 0) start = 0;
    if (start >= impl->length) {
        return hoo_string_new();
    }

    if (length < 0) {
        return hoo_string_new();
    }

    int64_t actual_length = std::min(length, impl->length - start);
    if (actual_length <= 0) {
        return hoo_string_new();
    }

    return hoo_string_from_bytes(impl->data + start, actual_length);
}

HooString hoo_string_to_upper(HooString str) {
    if (!str) {
        return hoo_string_new();
    }

    HooStringImpl* impl = get_impl(str);
    HooString result = allocate_string(impl->length);
    HooStringImpl* result_impl = get_impl(result);

    for (int64_t i = 0; i < impl->length; i++) {
        result_impl->data[i] = std::toupper((unsigned char)impl->data[i]);
    }

    return result;
}

HooString hoo_string_to_lower(HooString str) {
    if (!str) {
        return hoo_string_new();
    }

    HooStringImpl* impl = get_impl(str);
    HooString result = allocate_string(impl->length);
    HooStringImpl* result_impl = get_impl(result);

    for (int64_t i = 0; i < impl->length; i++) {
        result_impl->data[i] = std::tolower((unsigned char)impl->data[i]);
    }

    return result;
}

HooString hoo_string_trim(HooString str) {
    if (!str) {
        return hoo_string_new();
    }

    HooStringImpl* impl = get_impl(str);
    const char* data = impl->data;
    int64_t length = impl->length;

    // Skip leading whitespace
    int64_t start = 0;
    while (start < length && std::isspace((unsigned char)data[start])) {
        start++;
    }

    // Skip trailing whitespace
    int64_t end = length - 1;
    while (end >= start && std::isspace((unsigned char)data[end])) {
        end--;
    }

    int64_t trimmed_length = end - start + 1;
    if (trimmed_length <= 0) {
        return hoo_string_new();
    }

    return hoo_string_from_bytes(data + start, trimmed_length);
}

HooString hoo_string_replace(HooString str, HooString old, HooString replacement) {
    if (!str || !old || !replacement) {
        return str ? hoo_string_retain(str) : hoo_string_new();
    }

    HooStringImpl* str_impl = get_impl(str);
    HooStringImpl* old_impl = get_impl(old);
    HooStringImpl* replacement_impl = get_impl(replacement);

    if (old_impl->length == 0) {
        return hoo_string_retain(str);  // No match possible
    }

    // Build result string by finding and replacing occurrences
    HooString result = hoo_string_new();
    int64_t pos = 0;

    while (pos < str_impl->length) {
        // Find next occurrence of old substring
        const char* match = std::strstr(str_impl->data + pos, old_impl->data);

        if (match) {
            int64_t match_pos = match - str_impl->data;

            // Copy from current position to match
            HooString part = hoo_string_from_bytes(str_impl->data + pos, match_pos - pos);
            HooString temp = hoo_string_concat(result, part);
            hoo_string_release(result);
            hoo_string_release(part);
            result = temp;

            // Append replacement
            temp = hoo_string_concat(result, replacement);
            hoo_string_release(result);
            result = temp;

            pos = match_pos + old_impl->length;
        } else {
            // No more matches - append remainder
            HooString part = hoo_string_from_bytes(str_impl->data + pos, str_impl->length - pos);
            HooString temp = hoo_string_concat(result, part);
            hoo_string_release(result);
            hoo_string_release(part);
            result = temp;
            break;
        }
    }

    return result;
}

// ============================================================================
// String Query
// ============================================================================

int64_t hoo_string_length(HooString str) {
    if (!str) return 0;
    return get_impl(str)->length;
}

const char* hoo_string_data(HooString str) {
    if (!str) return "";
    return get_impl(str)->data;
}

int64_t hoo_string_byte_at(HooString str, int64_t index) {
    if (!str) return -1;

    HooStringImpl* impl = get_impl(str);
    if (index < 0 || index >= impl->length) {
        return -1;
    }

    return (unsigned char)impl->data[index];
}

int64_t hoo_string_is_empty(HooString str) {
    if (!str) return 1;
    return get_impl(str)->length == 0 ? 1 : 0;
}

int64_t hoo_string_index_of(HooString haystack, HooString needle) {
    if (!haystack || !needle) return -1;

    HooStringImpl* hay_impl = get_impl(haystack);
    HooStringImpl* need_impl = get_impl(needle);

    if (need_impl->length == 0) return 0;
    if (need_impl->length > hay_impl->length) return -1;

    const char* result = std::strstr(hay_impl->data, need_impl->data);
    if (!result) return -1;

    return result - hay_impl->data;
}

int64_t hoo_string_last_index_of(HooString haystack, HooString needle) {
    if (!haystack || !needle) return -1;

    HooStringImpl* hay_impl = get_impl(haystack);
    HooStringImpl* need_impl = get_impl(needle);

    if (need_impl->length == 0 || need_impl->length > hay_impl->length) {
        return -1;
    }

    int64_t last_pos = -1;
    const char* pos = hay_impl->data;

    while ((pos = std::strstr(pos, need_impl->data)) != nullptr) {
        last_pos = pos - hay_impl->data;
        pos++;
    }

    return last_pos;
}

int64_t hoo_string_contains(HooString haystack, HooString needle) {
    return hoo_string_index_of(haystack, needle) >= 0 ? 1 : 0;
}

int64_t hoo_string_starts_with(HooString str, HooString prefix) {
    if (!str || !prefix) return 0;

    HooStringImpl* str_impl = get_impl(str);
    HooStringImpl* prefix_impl = get_impl(prefix);

    if (prefix_impl->length > str_impl->length) return 0;
    if (prefix_impl->length == 0) return 1;

    return std::strncmp(str_impl->data, prefix_impl->data, prefix_impl->length) == 0 ? 1 : 0;
}

int64_t hoo_string_ends_with(HooString str, HooString suffix) {
    if (!str || !suffix) return 0;

    HooStringImpl* str_impl = get_impl(str);
    HooStringImpl* suffix_impl = get_impl(suffix);

    if (suffix_impl->length > str_impl->length) return 0;
    if (suffix_impl->length == 0) return 1;

    const char* str_end = str_impl->data + str_impl->length - suffix_impl->length;
    return std::strcmp(str_end, suffix_impl->data) == 0 ? 1 : 0;
}

// ============================================================================
// String Comparison
// ============================================================================

int64_t hoo_string_compare(HooString a, HooString b) {
    if (!a) a = hoo_string_new();
    if (!b) b = hoo_string_new();

    HooStringImpl* a_impl = get_impl(a);
    HooStringImpl* b_impl = get_impl(b);

    int result = std::strcmp(a_impl->data, b_impl->data);

    if (result < 0) return -1;
    if (result > 0) return 1;
    return 0;
}

int64_t hoo_string_equals(HooString a, HooString b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;

    HooStringImpl* a_impl = get_impl(a);
    HooStringImpl* b_impl = get_impl(b);

    if (a_impl->length != b_impl->length) return 0;

    return std::strcmp(a_impl->data, b_impl->data) == 0 ? 1 : 0;
}

int64_t hoo_string_equals_ignore_case(HooString a, HooString b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;

    HooStringImpl* a_impl = get_impl(a);
    HooStringImpl* b_impl = get_impl(b);

    if (a_impl->length != b_impl->length) return 0;

    for (int64_t i = 0; i < a_impl->length; i++) {
        if (std::tolower((unsigned char)a_impl->data[i]) !=
            std::tolower((unsigned char)b_impl->data[i])) {
            return 0;
        }
    }

    return 1;
}

// ============================================================================
// Reference Counting
// ============================================================================

HooString hoo_string_retain(HooString str) {
    if (!str) return nullptr;
    get_impl(str)->refcount++;
    return str;
}

void hoo_string_release(HooString str) {
    if (!str) return;

    HooStringImpl* impl = get_impl(str);
    impl->refcount--;

    if (impl->refcount == 0) {
        std::free(impl);
    } else if (impl->refcount < 0) {
        std::fprintf(stderr, "ERROR: String refcount went negative! Value: %lld\n", impl->refcount);
        std::exit(1);
    }
}

int64_t hoo_string_refcount(HooString str) {
    if (!str) return 0;
    return get_impl(str)->refcount;
}

// ============================================================================
// Conversion and Formatting
// ============================================================================

HooString hoo_string_from_int64(int64_t value) {
    char buffer[64];
    int len = std::snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
    if (len < 0) len = 0;
    return hoo_string_from_bytes(buffer, len);
}

HooString hoo_string_from_double(double value) {
    char buffer[64];
    int len = std::snprintf(buffer, sizeof(buffer), "%g", value);
    if (len < 0) len = 0;
    return hoo_string_from_bytes(buffer, len);
}

HooString hoo_string_from_bool(int64_t value) {
    return value ? hoo_string_from_cstr("true") : hoo_string_from_cstr("false");
}

int64_t hoo_string_to_int64(HooString str) {
    if (!str) return 0;

    HooStringImpl* impl = get_impl(str);
    return std::strtoll(impl->data, nullptr, 10);
}

double hoo_string_to_double(HooString str) {
    if (!str) return 0.0;

    HooStringImpl* impl = get_impl(str);
    return std::strtod(impl->data, nullptr);
}

HooString hoo_string_format(const char* format, ...) {
    if (!format) return hoo_string_new();

    va_list args;
    va_start(args, format);

    // First pass: determine required buffer size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        return hoo_string_new();
    }

    // Second pass: format into buffer
    char* buffer = (char*)std::malloc(size + 1);
    if (!buffer) {
        std::fprintf(stderr, "ERROR: Out of memory in hoo_string_format\n");
        va_end(args);
        return hoo_string_new();
    }

    std::vsnprintf(buffer, size + 1, format, args);
    va_end(args);

    HooString result = hoo_string_from_cstr(buffer);
    std::free(buffer);

    return result;
}

// ============================================================================
// Debugging and Utilities
// ============================================================================

void hoo_string_print(HooString str) {
    if (!str) {
        std::printf("<null string>");
    } else {
        HooStringImpl* impl = get_impl(str);
        std::printf("%.*s", (int)impl->length, impl->data);
    }
    std::fflush(stdout);
}

void hoo_string_println(HooString str) {
    hoo_string_print(str);
    std::printf("\n");
    std::fflush(stdout);
}

HooString hoo_string_debug(HooString str) {
    if (!str) {
        return hoo_string_from_cstr("<null>");
    }

    HooStringImpl* impl = get_impl(str);

    char buffer[256];
    int len = std::snprintf(buffer, sizeof(buffer),
        "HooString { len=%lld, cap=%lld, refcount=%lld, data=\"%.*s\" }",
        (long long)impl->length,
        (long long)impl->capacity,
        (long long)impl->refcount,
        (int)std::min(impl->length, (int64_t)50),
        impl->data
    );

    if (len < 0) len = 0;
    return hoo_string_from_bytes(buffer, len);
}
