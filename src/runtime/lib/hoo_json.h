#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOO_JSON_NULL    0
#define HOO_JSON_BOOL    1
#define HOO_JSON_INT     2
#define HOO_JSON_STRING  3
#define HOO_JSON_ARRAY   4
#define HOO_JSON_OBJECT  5
#define HOO_JSON_FLOAT   6

typedef void* HooJson;

HooJson  hoo_json_parse(const char* json);
char*    hoo_json_stringify(HooJson json);
HooJson  hoo_json_get(HooJson obj, const char* key);
int64_t  hoo_json_get_int(HooJson obj, const char* key);
char*    hoo_json_get_string(HooJson obj, const char* key);
int64_t  hoo_json_set(HooJson obj, const char* key, HooJson val);
HooJson  hoo_json_array_get(HooJson arr, int64_t index);
int64_t  hoo_json_array_push(HooJson arr, HooJson val);
int64_t  hoo_json_array_length(HooJson arr);
int64_t  hoo_json_type(HooJson json);
HooJson  hoo_json_new_object(void);
HooJson  hoo_json_new_array(void);
HooJson  hoo_json_new_string(const char* s);
HooJson  hoo_json_new_int(int64_t n);
HooJson  hoo_json_new_float(double f);
HooJson  hoo_json_new_bool(int64_t b);
HooJson  hoo_json_new_null(void);
void     hoo_json_retain(HooJson json);
void     hoo_json_release(HooJson json);
void     hoo_json_free_string(char* str);

// ============================================================================
// HooMap Interop
// ============================================================================

typedef void* HooMap;
typedef void* HooString;

/**
 * Parse a JSON object string into a HooMap<string, string>.
 *
 * Only flat JSON objects are supported (no nested objects or arrays).
 * String values are stored without quotes/escaping. Numbers are stored as
 * their string representation. Bools become "true"/"false". Null becomes "null".
 *
 * @param json  Null-terminated JSON string
 * @return HooMap (HOO_MAP_VAL_STRING) with refcount=1, or NULL on parse error
 */
HooMap hoo_json_parse_to_map(const char* json);

/**
 * Serialize a HooMap<string, string> to a JSON object string.
 *
 * Values are auto-detected: "null" → null, "true"/"false" → bool,
 * parseable integers → JSON number, otherwise → JSON string (escaped).
 *
 * @param map  HooMap with HOO_MAP_VAL_STRING
 * @return HooString of the JSON representation (refcount=1), or NULL on error
 */
HooString hoo_json_serialize_map(HooMap map);

// ============================================================================
// String Transformation
// ============================================================================

/**
 * Minify a JSON string by removing all insignificant whitespace.
 *
 * Whitespace inside string literals is preserved.
 *
 * @param json  Null-terminated JSON string
 * @return HooString of the minified JSON (refcount=1), or NULL on error
 */
HooString hoo_json_minify(const char* json);

/**
 * Beautify / pretty-print a JSON string with 2-space indentation.
 *
 * Newlines are added after each key-value pair and array element.
 *
 * @param json  Null-terminated JSON string
 * @return HooString of the beautified JSON (refcount=1), or NULL on error
 */
HooString hoo_json_beautify(const char* json);

#ifdef __cplusplus
}
#endif
