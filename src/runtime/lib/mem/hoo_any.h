#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOO_TYPE_ANY 0

typedef struct HooAnyValue {
    int64_t type_id;
    uint64_t data;
} HooAnyValue;

int64_t hoo_any_is_managed(int64_t type_id);
void    hoo_any_retain(HooAnyValue value);
void    hoo_any_release(HooAnyValue value);

#ifdef __cplusplus
}
#endif
