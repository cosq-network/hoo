#include "hoo_args.h"
#include <cstdlib>
#include <cstring>

extern "C" {

HooArgsResult hoo_args_parse(int64_t argc, const char* const* argv) {
    HooArgsResult result = {NULL, 0};
    int64_t capacity = 0;
    int64_t positional_index = 0;
    int positional_mode = 0;

    for (int64_t i = 1; i < argc; i++) {
        const char* token = argv[i];

        if (!positional_mode && strcmp(token, "--") == 0) {
            positional_mode = 1;
            continue;
        }

        const char* key = NULL;
        const char* value = NULL;
        int64_t idx = -1;

        if (!positional_mode && token[0] == '-' && token[1] == '-') {
            const char* eq = strchr(token + 2, '=');
            if (eq) {
                size_t key_len = (size_t)(eq - (token + 2));
                char* k = (char*)malloc(key_len + 1);
                memcpy(k, token + 2, key_len);
                k[key_len] = '\0';
                key = k;
                value = strdup(eq + 1);
            } else {
                key = strdup(token + 2);
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    i++;
                    value = strdup(argv[i]);
                } else {
                    value = strdup("");
                }
            }
        } else if (!positional_mode && token[0] == '-' && token[1] != '\0') {
            char* k = (char*)malloc(2);
            k[0] = token[1];
            k[1] = '\0';
            key = k;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
                value = strdup(argv[i]);
            } else {
                value = strdup("");
            }
        } else {
            key = strdup("");
            value = strdup(token);
            idx = positional_index++;
        }

        if (result.count >= capacity) {
            capacity = capacity ? capacity * 2 : 16;
            result.args = (HooArg*)realloc(result.args, (size_t)capacity * sizeof(HooArg));
        }

        HooArg* arg = &result.args[result.count++];
        arg->key = key;
        arg->value = value;
        arg->index = idx;
    }

    return result;
}

void hoo_args_free(HooArgsResult result) {
    for (int64_t i = 0; i < result.count; i++) {
        free((void*)result.args[i].key);
        free((void*)result.args[i].value);
    }
    free(result.args);
}

const char* hoo_args_get(const HooArgsResult* result, const char* key) {
    for (int64_t i = 0; i < result->count; i++) {
        if (strcmp(result->args[i].key, key) == 0)
            return result->args[i].value;
    }
    return NULL;
}

int64_t hoo_args_has(const HooArgsResult* result, const char* key) {
    for (int64_t i = 0; i < result->count; i++) {
        if (strcmp(result->args[i].key, key) == 0)
            return 1;
    }
    return 0;
}

int64_t hoo_args_count(const HooArgsResult* result) {
    int64_t count = 0;
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].index >= 0)
            count++;
    }
    return count;
}

const char* hoo_args_positional(const HooArgsResult* result, int64_t index) {
    for (int64_t i = 0; i < result->count; i++) {
        if (result->args[i].index == index)
            return result->args[i].value;
    }
    return NULL;
}

} // extern "C"
