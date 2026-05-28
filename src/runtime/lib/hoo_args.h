#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* key;
    const char* value;
    int64_t index;
} HooArg;

typedef struct {
    HooArg* args;
    int64_t count;
} HooArgsResult;

HooArgsResult hoo_args_parse(int64_t argc, const char* const* argv);
void          hoo_args_free(HooArgsResult result);
const char*   hoo_args_get(const HooArgsResult* result, const char* key);
int64_t       hoo_args_has(const HooArgsResult* result, const char* key);
int64_t       hoo_args_count(const HooArgsResult* result);
const char*   hoo_args_positional(const HooArgsResult* result, int64_t index);

#ifdef __cplusplus
}
#endif
