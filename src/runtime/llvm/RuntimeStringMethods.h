#pragma once

#include "RuntimeMethodRegistry.h"

namespace hooc {
namespace runtime {

BEGIN_RUNTIME_CLASS(string, "string")
    RUNTIME_METHOD(length, "hoo_string_length")
    RUNTIME_METHOD(isEmpty, "hoo_string_is_empty")
    RUNTIME_METHOD(byteAt, "hoo_string_byte_at")
    RUNTIME_METHOD(concat, "hoo_string_concat")
    RUNTIME_METHOD(substring, "hoo_string_substring")
    RUNTIME_METHOD(toUpper, "hoo_string_to_upper")
    RUNTIME_METHOD(toLower, "hoo_string_to_lower")
    RUNTIME_METHOD(trim, "hoo_string_trim")
    RUNTIME_METHOD(replace, "hoo_string_replace")
    RUNTIME_METHOD(split, "hoo_string_split")
    RUNTIME_METHOD(indexOf, "hoo_string_index_of")
    RUNTIME_METHOD(lastIndexOf, "hoo_string_last_index_of")
    RUNTIME_METHOD(contains, "hoo_string_contains")
    RUNTIME_METHOD(startsWith, "hoo_string_starts_with")
    RUNTIME_METHOD(endsWith, "hoo_string_ends_with")
    RUNTIME_METHOD(compare, "hoo_string_compare")
    RUNTIME_METHOD(equals, "hoo_string_equals")
    RUNTIME_METHOD(equalsIgnoreCase, "hoo_string_equals_ignore_case")
    RUNTIME_METHOD(fromInt64, "hoo_string_from_int64")
    RUNTIME_METHOD(fromDouble, "hoo_string_from_double")
    RUNTIME_METHOD(fromBool, "hoo_string_from_bool")
    RUNTIME_METHOD(toInt64, "hoo_string_to_int64")
    RUNTIME_METHOD(toDouble, "hoo_string_to_double")
END_RUNTIME_CLASS(string, "string")

} // namespace runtime
} // namespace hooc
