#include "runtime/lib/system/hoo_process.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include "core/Platform.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

extern "C" {

int64_t hoo_process_spawn(const char* command, const char* const* argv, int64_t* out_pid) {
#ifdef _WIN32
    (void)command;
    std::string cmd;
    for (int i = 0; argv[i]; i++) {
        if (i > 0) cmd += " ";
        cmd += argv[i];
    }
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char* mutable_cmd = strdup(cmd.c_str());
    if (!mutable_cmd) return -1;
    BOOL ok = CreateProcessA(NULL, mutable_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(mutable_cmd);
    if (!ok) return -1;
    CloseHandle(pi.hThread);
    *out_pid = (int64_t)pi.dwProcessId;
    CloseHandle(pi.hProcess);
    return 0;
#else
    pid_t pid = fork();
    if (pid == -1) return -1;
    if (pid == 0) {
        execvp(command, (char* const*)argv);
        _exit(127);
    }
    *out_pid = (int64_t)pid;
    return 0;
#endif
}

int64_t hoo_process_wait(int64_t pid, int64_t* out_exit_code) {
#ifdef _WIN32
    HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
    if (!h) return -1;
    WaitForSingleObject(h, INFINITE);
    DWORD code;
    BOOL ok = GetExitCodeProcess(h, &code);
    CloseHandle(h);
    if (!ok) return -1;
    *out_exit_code = (int64_t)code;
    return 0;
#else
    int status;
    if (waitpid((pid_t)pid, &status, 0) == -1) return -1;
    *out_exit_code = WIFEXITED(status) ? (int64_t)WEXITSTATUS(status) : -1;
    return 0;
#endif
}

int64_t hoo_process_kill(int64_t pid, int64_t signal) {
#ifdef _WIN32
    if (signal == 0) {
        // POSIX semantics: signal 0 checks if process exists
        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
        if (!h) return -1;
        CloseHandle(h);
        return 0;
    }
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (!h) return -1;
    BOOL ok = TerminateProcess(h, 1);
    CloseHandle(h);
    return ok ? 0 : -1;
#else
    return kill((pid_t)pid, (int)signal) == -1 ? -1 : 0;
#endif
}

int64_t hoo_process_self_pid(void) {
#ifdef _WIN32
    return (int64_t)GetCurrentProcessId();
#else
    return (int64_t)getpid();
#endif
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
#ifdef _WIN32
    int status = pclose(fp);
    if (status == -1) { std::free(buf); *out_stdout = NULL; *out_exit_code = -1; return -1; }
    buf[len] = '\0';
    *out_stdout = buf;
    *out_exit_code = (int64_t)status;
#else
    int status = pclose(fp);
    if (status == -1) { std::free(buf); *out_stdout = NULL; *out_exit_code = -1; return -1; }
    buf[len] = '\0';
    *out_stdout = buf;
    *out_exit_code = WIFEXITED(status) ? (int64_t)WEXITSTATUS(status) : -1;
#endif
    return 0;
}

void hoo_process_free_string(char* str) {
    std::free(str);
}

} // extern "C"
