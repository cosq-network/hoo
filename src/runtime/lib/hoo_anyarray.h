#pragma once

#include "hoo_any.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooAnyArray;

HooAnyArray hoo_anyarray_new(void);
HooAnyArray hoo_anyarray_new_capacity(int64_t capacity);
HooAnyArray hoo_anyarray_retain(HooAnyArray array);
void        hoo_anyarray_release(HooAnyArray array);
int64_t     hoo_anyarray_refcount(HooAnyArray array);

int64_t     hoo_anyarray_length(HooAnyArray array);
int64_t     hoo_anyarray_push(HooAnyArray array, int64_t type_id, uint64_t data);
int64_t     hoo_anyarray_set(HooAnyArray array, int64_t index, int64_t type_id, uint64_t data);
int64_t     hoo_anyarray_get(HooAnyArray array, int64_t index, HooAnyValue* out);
int64_t     hoo_anyarray_pop(HooAnyArray array, HooAnyValue* out);
void        hoo_anyarray_clear(HooAnyArray array);

#ifdef __cplusplus
}
#endif
