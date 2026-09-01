#pragma once

#include "runtime/lib/mem/hoo_any.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooList;

HooList hoo_list_new(void);
HooList hoo_list_new_capacity(int64_t capacity);
HooList hoo_list_retain(HooList array);
void        hoo_list_release(HooList array);
int64_t     hoo_list_refcount(HooList array);

int64_t     hoo_list_length(HooList array);
int64_t     hoo_list_push(HooList array, int64_t type_id, uint64_t data);
int64_t     hoo_list_set(HooList array, int64_t index, int64_t type_id, uint64_t data);
int64_t     hoo_list_get(HooList array, int64_t index, HooAnyValue* out);
int64_t     hoo_list_pop(HooList array, HooAnyValue* out);
void        hoo_list_clear(HooList array);

#ifdef __cplusplus
}
#endif
