#pragma once

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif
