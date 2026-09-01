#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef void* HooRegex;

HooRegex hoo_regex_compile(const char* pattern);
HooRegex hoo_regex_compile_with_flags(const char* pattern, const char* flags);

int64_t  hoo_regex_match(HooRegex re, const char* str);
int64_t  hoo_regex_search(HooRegex re, const char* str);
char*    hoo_regex_find(HooRegex re, const char* str);
int64_t  hoo_regex_find_all(HooRegex re, const char* str, char*** out_matches, int64_t* out_count);

char*    hoo_regex_replace(HooRegex re, const char* str, const char* replacement);
char**   hoo_regex_split(HooRegex re, const char* str, int64_t* out_count);
void     hoo_regex_free_matches(char** matches, int64_t count);

char*    hoo_regex_group(HooRegex re, const char* str, int64_t group_index);

const char* hoo_regex_error(void);

HooRegex hoo_regex_retain(HooRegex re);
void     hoo_regex_release(HooRegex re);

void     hoo_regex_free_string(char* str);

#if defined(__cplusplus)
}
#endif
