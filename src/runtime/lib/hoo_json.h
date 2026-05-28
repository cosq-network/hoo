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
HooJson  hoo_json_new_bool(int64_t b);
HooJson  hoo_json_new_null(void);
void     hoo_json_retain(HooJson json);
void     hoo_json_release(HooJson json);
void     hoo_json_free_string(char* str);

#ifdef __cplusplus
}
#endif
