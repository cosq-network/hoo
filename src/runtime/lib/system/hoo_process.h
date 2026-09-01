#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t hoo_process_spawn(const char* command, const char* const* argv, int64_t* out_pid);
int64_t hoo_process_wait(int64_t pid, int64_t* out_exit_code);
int64_t hoo_process_kill(int64_t pid, int64_t signal);
int64_t hoo_process_self_pid(void);
char*   hoo_process_capture(const char* command);
int64_t hoo_process_capture_status(const char* command, char** out_stdout, int64_t* out_exit_code);
void    hoo_process_free_string(char* str);

#ifdef __cplusplus
}
#endif
