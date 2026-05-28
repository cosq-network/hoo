#include "hoo_process.h"
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdio>

extern "C" {

int64_t hoo_process_spawn(const char* command, const char* const* argv, int64_t* out_pid) {
    pid_t pid = fork();
    if (pid == -1) return -1;
    if (pid == 0) {
        execvp(command, (char* const*)argv);
        _exit(127);
    }
    *out_pid = (int64_t)pid;
    return 0;
}

int64_t hoo_process_wait(int64_t pid, int64_t* out_exit_code) {
    int status;
    if (waitpid((pid_t)pid, &status, 0) == -1) return -1;
    *out_exit_code = WIFEXITED(status) ? (int64_t)WEXITSTATUS(status) : -1;
    return 0;
}

int64_t hoo_process_kill(int64_t pid, int64_t signal) {
    return kill((pid_t)pid, (int)signal) == -1 ? -1 : 0;
}

int64_t hoo_process_self_pid(void) {
    return (int64_t)getpid();
}

char* hoo_process_capture(const char* command) {
    FILE* fp = popen(command, "r");
    if (!fp) return NULL;
    size_t cap = 4096, len = 0;
    char* buf = (char*)std::malloc(cap);
    if (!buf) { pclose(fp); return NULL; }
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        if (len + n > cap) {
            while (cap < len + n + 1) cap *= 2;
            char* nb = (char*)std::realloc(buf, cap);
            if (!nb) { std::free(buf); pclose(fp); return NULL; }
            buf = nb;
        }
        std::memcpy(buf + len, chunk, n);
        len += n;
    }
    int rc = pclose(fp);
    if (rc == -1) { std::free(buf); return NULL; }
    buf[len] = '\0';
    return buf;
}

int64_t hoo_process_capture_status(const char* command, char** out_stdout, int64_t* out_exit_code) {
    FILE* fp = popen(command, "r");
    if (!fp) { *out_stdout = NULL; *out_exit_code = -1; return -1; }
    size_t cap = 4096, len = 0;
    char* buf = (char*)std::malloc(cap);
    if (!buf) { pclose(fp); *out_stdout = NULL; *out_exit_code = -1; return -1; }
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        if (len + n > cap) {
            while (cap < len + n + 1) cap *= 2;
            char* nb = (char*)std::realloc(buf, cap);
            if (!nb) { std::free(buf); pclose(fp); *out_stdout = NULL; *out_exit_code = -1; return -1; }
            buf = nb;
        }
        std::memcpy(buf + len, chunk, n);
        len += n;
    }
    int status = pclose(fp);
    if (status == -1) { std::free(buf); *out_stdout = NULL; *out_exit_code = -1; return -1; }
    buf[len] = '\0';
    *out_stdout = buf;
    *out_exit_code = WIFEXITED(status) ? (int64_t)WEXITSTATUS(status) : -1;
    return 0;
}

void hoo_process_free_string(char* str) {
    std::free(str);
}

} // extern "C"
