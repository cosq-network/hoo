#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooAnyArray;
typedef void* HooHashMap;
typedef void* HooString;

/**
 * Serialize a HashMap to a JSON object string.
 *
 * Supported map values:
 * - fixed scalar HashMap values: int64, int8, byte, bool, string
 * - any values containing int64, int8, byte, bool, f64, string, Buffer,
 *   HashMap, AnyArray, and Tensor.
 *
 * Buffers and tensors use tagged objects. HashMap keys are numeric in Hoo and
 * are serialized as JSON object field names.
 * Returns a HooString with refcount=1, or NULL on unsupported input.
 */
HooString hoo_json_serialize_hashmap(HooHashMap map);

/**
 * Serialize an AnyArray to a JSON array string.
 *
 * Supported elements: int64, int8, byte, bool, f64, string, HashMap, AnyArray.
 * Returns a HooString with refcount=1, or NULL on unsupported input.
 */
HooString hoo_json_serialize_anyarray(HooAnyArray array);

/**
 * Deserialize a JSON object into a HashMap<int64, any>.
 *
 * JSON object field names must be valid int64 keys. Values may be null, bool,
 * integer, floating-point, string, object, or array. Throws a runtime exception
 * on invalid JSON or unsupported object keys.
 */
HooHashMap hoo_json_deserialize_hashmap(const char* json);

/**
 * Deserialize a JSON array into an AnyArray.
 *
 * Values may be null, bool, integer, floating-point, string, object, or array.
 * Throws a runtime exception on invalid JSON or non-array input.
 */
HooAnyArray hoo_json_deserialize_anyarray(const char* json);

/**
 * Minify a JSON string by removing insignificant whitespace.
 * Returns a HooString with refcount=1. Throws a runtime exception on invalid
 * JSON.
 */
HooString hoo_json_minify(const char* json);

/**
 * Pretty-print a JSON string using 2-space indentation.
 * Returns a HooString with refcount=1. Throws a runtime exception on invalid
 * JSON.
 */
HooString hoo_json_beautify(const char* json);

#ifdef __cplusplus
}
#endif
