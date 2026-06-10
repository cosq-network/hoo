#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── Low-level raw argv access (existing API) ──────────────────────────────

void   hoo_args_init(int64_t argc, const char* const* argv);
void   hoo_args_shutdown(void);
void*  hoo_args_new(void);

int64_t       hoo_args_count(void* args);
const char*   hoo_args_get(void* args, int64_t index);
int64_t       hoo_args_has(void* args, const char* key);
const char*   hoo_args_value(void* args, const char* key);
const char*   hoo_args_program_name(void* args);

// ── Argparse-style high-level API ──────────────────────────────────────────

// Argument types
#define HOO_ARG_STRING 0
#define HOO_ARG_INT    1
#define HOO_ARG_FLAG   2
#define HOO_ARG_FLOAT  3

// Add an argument definition to the parser handle.
// type: HOO_ARG_STRING, HOO_ARG_INT, HOO_ARG_FLAG, or HOO_ARG_FLOAT
// name: destination key name
// short_opt: short flag (e.g. "-o"), pass "" if none
// long_opt: long flag (e.g. "--output"), pass "" if none
// help: help text description
// default_str: default value as string (for STRING)
// default_int: default int64 value (for INT)
// default_float: default double value (for FLOAT)
// is_positional: 1 if positional, 0 if named flag
void hoo_args_add_arg(void* args, int type,
                      const char* name,
                      const char* short_opt,
                      const char* long_opt,
                      const char* help,
                      const char* default_str,
                      int64_t default_int,
                      double default_float,
                      int is_positional);

// Convenience wrappers
void hoo_args_add_string(void* args, const char* name,
                         const char* short_opt, const char* long_opt,
                         const char* help, const char* default_val);
void hoo_args_add_int(void* args, const char* name,
                      const char* short_opt, const char* long_opt,
                      const char* help, int64_t default_val);
void hoo_args_add_flag(void* args, const char* name,
                       const char* short_opt, const char* long_opt,
                       const char* help);
void hoo_args_add_float(void* args, const char* name,
                        const char* short_opt, const char* long_opt,
                        const char* help, double default_val);
void hoo_args_add_positional(void* args, const char* name, const char* help);

// Parse against defined arguments. Returns 1 on success, 0 on failure
// (e.g. --help flag or missing required argument).
int64_t hoo_args_parse(void* args);

// Access parsed values
const char* hoo_args_get_string(void* args, const char* name);
int64_t     hoo_args_get_int(void* args, const char* name);
int64_t     hoo_args_get_bool(void* args, const char* name);
double      hoo_args_get_float(void* args, const char* name);

// Generate help text (returns a C string that must be freed by caller)
char* hoo_args_help_text(void* args);

// Clear parsed values and argument definitions, allowing reuse
void hoo_args_clear(void* args);

#ifdef __cplusplus
}
#endif
