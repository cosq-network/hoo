#pragma once

#include "runtime/lib/mem/hoo_any.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooDict;

HooDict hoo_dict_new(int64_t key_type_id, int64_t value_type_id);
HooDict hoo_dict_retain(HooDict map);
void       hoo_dict_release(HooDict map);
int64_t    hoo_dict_refcount(HooDict map);

int64_t    hoo_dict_count(HooDict map);
int64_t    hoo_dict_key_type(HooDict map);
int64_t    hoo_dict_value_type(HooDict map);
void       hoo_dict_clear(HooDict map);
int64_t    hoo_dict_remove_i8(HooDict map, int64_t key);

int64_t    hoo_dict_set_fixed_i8(HooDict map, int64_t key, uint64_t data);
int64_t    hoo_dict_get_fixed_i8(HooDict map, int64_t key, uint64_t* out);
int64_t    hoo_dict_set_any_i8(HooDict map, int64_t key, int64_t type_id, uint64_t data);
int64_t    hoo_dict_get_any_i8(HooDict map, int64_t key, HooAnyValue* out);

int64_t    hoo_dict_get_keys_i8(HooDict map, int64_t* keys, int64_t max_count);
int64_t    hoo_dict_get_fixed_at_i8(HooDict map, int64_t key, uint64_t* out);
int64_t    hoo_dict_get_any_at_i8(HooDict map, int64_t key, HooAnyValue* out);

#ifdef __cplusplus
}
#endif
