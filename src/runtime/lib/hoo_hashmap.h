#pragma once

#include "hoo_any.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooHashMap;

HooHashMap hoo_hashmap_new(int64_t key_type_id, int64_t value_type_id);
HooHashMap hoo_hashmap_retain(HooHashMap map);
void       hoo_hashmap_release(HooHashMap map);
int64_t    hoo_hashmap_refcount(HooHashMap map);

int64_t    hoo_hashmap_count(HooHashMap map);
int64_t    hoo_hashmap_key_type(HooHashMap map);
int64_t    hoo_hashmap_value_type(HooHashMap map);
void       hoo_hashmap_clear(HooHashMap map);
int64_t    hoo_hashmap_remove_i8(HooHashMap map, int64_t key);

int64_t    hoo_hashmap_set_fixed_i8(HooHashMap map, int64_t key, uint64_t data);
int64_t    hoo_hashmap_get_fixed_i8(HooHashMap map, int64_t key, uint64_t* out);
int64_t    hoo_hashmap_set_any_i8(HooHashMap map, int64_t key, int64_t type_id, uint64_t data);
int64_t    hoo_hashmap_get_any_i8(HooHashMap map, int64_t key, HooAnyValue* out);

int64_t    hoo_hashmap_get_keys_i8(HooHashMap map, int64_t* keys, int64_t max_count);
int64_t    hoo_hashmap_get_fixed_at_i8(HooHashMap map, int64_t key, uint64_t* out);
int64_t    hoo_hashmap_get_any_at_i8(HooHashMap map, int64_t key, HooAnyValue* out);

#ifdef __cplusplus
}
#endif
