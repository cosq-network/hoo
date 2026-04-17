#pragma once

#include "RuntimeMethodRegistry.h"

namespace hooc {
namespace runtime {

BEGIN_RUNTIME_CLASS(array, "array")
    // Instance methods - operate on this array
    RUNTIME_METHOD(length, "hoo_array_length")
    RUNTIME_METHOD(empty, "hoo_array_empty")
    RUNTIME_METHOD(clear, "hoo_array_clear")
    RUNTIME_METHOD(release, "hoo_array_release")
    RUNTIME_METHOD(refcount, "hoo_array_refcount")
    RUNTIME_METHOD(elementType, "hoo_array_element_type")

    // Generic operations
    RUNTIME_METHOD(push, "hoo_array_push")
    RUNTIME_METHOD(pop, "hoo_array_pop")
    RUNTIME_METHOD(get, "hoo_array_get")
    RUNTIME_METHOD(set, "hoo_array_set")

    // Type-specific push operations
    RUNTIME_METHOD(pushInt64, "hoo_array_push_int64")
    RUNTIME_METHOD(pushDouble, "hoo_array_push_double")
    RUNTIME_METHOD(pushFloat, "hoo_array_push_float")
    RUNTIME_METHOD(pushBool, "hoo_array_push_bool")
    RUNTIME_METHOD(pushChar, "hoo_array_push_char")
    RUNTIME_METHOD(pushString, "hoo_array_push_string")
    RUNTIME_METHOD(pushObject, "hoo_array_push_object")
    RUNTIME_METHOD(pushArray, "hoo_array_push_array")

    // Type-specific get operations
    RUNTIME_METHOD(getInt64, "hoo_array_get_int64")
    RUNTIME_METHOD(getDouble, "hoo_array_get_double")
    RUNTIME_METHOD(getFloat, "hoo_array_get_float")
    RUNTIME_METHOD(getBool, "hoo_array_get_bool")
    RUNTIME_METHOD(getChar, "hoo_array_get_char")
    RUNTIME_METHOD(getString, "hoo_array_get_string")
    RUNTIME_METHOD(getObject, "hoo_array_get_object")
    RUNTIME_METHOD(getArray, "hoo_array_get_array")
END_RUNTIME_CLASS(array, "array")

} // namespace runtime
} // namespace hooc
