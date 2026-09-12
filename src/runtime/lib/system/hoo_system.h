#pragma once

#include <stdint.h>

// hoo_system.h — Operating-system integration for the Hoo runtime.
//
// String-returning functions allocate the result with strdup(); the caller
// must release a non-null result with hoo_system_free_string().  The bridge
// layer must always pass the result to hoo_string_from_cstr and may only omit
// the null check where the function below contractually never returns null.
//
// The functions here are the C-ABI layer only.  The Hoo-callable surface is
// defined by the code-generator registry (isSystemFreeFunction) and the JIT
// symbol table; failures surface to Hoo code as RuntimeExceptions (raised by
// the jit_* bridges via the HVM throw syscall), so Hoo `try/catch` works.
//
// Conventions:
//   * hoo_system_get_env returns NULL when the variable is not set (expected
//     absence -> nil at the Hoo level, never an exception).
//   * The remaining string getters never return NULL on query failure: they
//     return "unknown" (hostname/os_name/os_version), "" (user_home/user_name/
//     current_dir) or NULL (exec: the child could not be spawned).
//   * int64 callers return 0 on success and -1 on unexpected failure.  The
//     jit_* bridges convert unexpected failures (spawn failure, set_env/
//     unset_env/set_current_dir failure, uptime/memory query failure) into a
//     RuntimeException.  hoo_system_error_message() describes the last failure
//     on the calling thread.
//   * hoo_system_exec_status returns the child's exit code, or -1 when the
//     command could not be spawned.
//   * hoo_system_exit(0..255) flushes stdout/stderr and terminates the
//     process immediately.  It bypasses ARC teardown and shadow-stack
//     unwinding; embedders that need clean unwinding should raise a Hoo
//     exception instead.
//   * On Windows the module reads and writes the process environment block
//     through the wide (UTF-16) APIs so Hoo strings stay valid UTF-8 on any
//     code page.

#ifdef __cplusplus
extern "C" {
#endif

// Environment
char*   hoo_system_get_env(const char* name);
int64_t hoo_system_set_env(const char* name, const char* value);
int64_t hoo_system_unset_env(const char* name);

// System info
char*   hoo_system_hostname(void);
char*   hoo_system_os_name(void);
char*   hoo_system_os_version(void);
int64_t hoo_system_cpu_count(void);
int64_t hoo_system_process_id(void);
int64_t hoo_system_uptime_ms(void);

// Process
void    hoo_system_exit(int64_t code);
char*   hoo_system_exec(const char* command);
int64_t hoo_system_exec_status(const char* command);

// User info
char*   hoo_system_user_home(void);
char*   hoo_system_user_name(void);
char*   hoo_system_current_dir(void);
int64_t hoo_system_set_current_dir(const char* path);

// Memory info
int64_t hoo_system_total_memory(void);
int64_t hoo_system_free_memory(void);

// Free string
void    hoo_system_free_string(char* str);

// Last error
// Returns a thread-local, NUL-terminated description of the last failure on
// the calling thread ("" when none).  The pointer stays valid until the next
// failure on that thread.
const char* hoo_system_error_message(void);

#ifdef __cplusplus
}
#endif
