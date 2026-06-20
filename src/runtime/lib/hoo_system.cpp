#include "hoo_system.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include "core/Platform.h"
#include <sys/utsname.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/time.h>
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif
#endif

// ============================================================================
// Environment
// ============================================================================

char* hoo_system_get_env(const char* name) {
    if (!name) return nullptr;
    const char* val = getenv(name);
    if (!val) return nullptr;
    return strdup(val);
}

int64_t hoo_system_set_env(const char* name, const char* value) {
    if (!name || !value) return -1;
#ifdef _WIN32
    return _putenv_s(name, value) == 0 ? 0 : -1;
#else
    return setenv(name, value, 1) == 0 ? 0 : -1;
#endif
}

int64_t hoo_system_unset_env(const char* name) {
    if (!name) return -1;
#ifdef _WIN32
    return _putenv_s(name, "") == 0 ? 0 : -1;
#else
    return unsetenv(name) == 0 ? 0 : -1;
#endif
}

// ============================================================================
// System info
// ============================================================================

char* hoo_system_hostname(void) {
#ifdef _WIN32
    char buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = sizeof(buf);
    if (GetComputerNameA(buf, &len)) {
        return strdup(buf);
    }
    return strdup("unknown");
#else
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return strdup(buf);
    }
    return strdup("unknown");
#endif
}

char* hoo_system_os_name(void) {
#if defined(__APPLE__)
    return strdup("macOS");
#elif defined(__linux__)
    return strdup("Linux");
#elif defined(_WIN32)
    return strdup("Windows");
#else
    return strdup("Unknown");
#endif
}

char* hoo_system_os_version(void) {
#if defined(__APPLE__)
    char version[256];
    size_t len = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &len, nullptr, 0) == 0) {
        return strdup(version);
    }
    len = sizeof(version);
    if (sysctlbyname("kern.osrelease", version, &len, nullptr, 0) == 0) {
        return strdup(version);
    }
    return strdup("unknown");
#elif defined(__linux__)
    struct utsname buf;
    if (uname(&buf) == 0) {
        return strdup(buf.release);
    }
    return strdup("unknown");
#elif defined(_WIN32)
    OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (GetVersionExA(&vi)) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%lu.%lu.%lu",
                      vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
        return strdup(buf);
    }
    return strdup("unknown");
#else
    return strdup("unknown");
#endif
}

int64_t hoo_system_cpu_count(void) {
    unsigned int count = std::thread::hardware_concurrency();
    return count > 0 ? static_cast<int64_t>(count) : 1;
}

int64_t hoo_system_process_id(void) {
#ifdef _WIN32
    return static_cast<int64_t>(GetCurrentProcessId());
#else
    return static_cast<int64_t>(getpid());
#endif
}

int64_t hoo_system_uptime_ms(void) {
#if defined(__APPLE__)
    struct timeval boottime;
    size_t len = sizeof(boottime);
    if (sysctlbyname("kern.boottime", &boottime, &len, nullptr, 0) == 0) {
        time_t now;
        time(&now);
        int64_t secs = static_cast<int64_t>(now - boottime.tv_sec);
        return secs * 1000;
    }
    return -1;
#elif defined(__linux__)
    FILE* f = fopen("/proc/uptime", "r");
    if (!f) return -1;
    double uptime;
    int matched = fscanf(f, "%lf", &uptime);
    fclose(f);
    if (matched == 1) {
        return static_cast<int64_t>(uptime * 1000.0);
    }
    return -1;
#elif defined(_WIN32)
    return static_cast<int64_t>(GetTickCount64());
#else
    return -1;
#endif
}

// ============================================================================
// Process
// ============================================================================

void hoo_system_exit(int64_t code) {
    exit(static_cast<int>(code));
}

static FILE* open_pipe(const char* command, const char* mode) {
#ifdef _WIN32
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

static int close_pipe(FILE* pipe) {
#ifdef _WIN32
    return _pclose(pipe);
#else
    return pclose(pipe);
#endif
}

char* hoo_system_exec(const char* command) {
    if (!command) return strdup("");
    FILE* pipe = open_pipe(command, "r");
    if (!pipe) return strdup("");

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    close_pipe(pipe);
    return strdup(result.c_str());
}

int64_t hoo_system_exec_status(const char* command) {
    if (!command) return -1;
    FILE* pipe = open_pipe(command, "r");
    if (!pipe) return -1;

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
    }
    return close_pipe(pipe);
}

// ============================================================================
// User info
// ============================================================================

char* hoo_system_user_home(void) {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
#else
    const char* home = getenv("HOME");
#endif
    if (!home) return strdup("");
    return strdup(home);
}

char* hoo_system_user_name(void) {
#ifdef _WIN32
    const char* user = getenv("USERNAME");
#else
    const char* user = getenv("USER");
#endif
    if (!user) return strdup("");
    return strdup(user);
}

char* hoo_system_current_dir(void) {
    try {
        fs::path cwd = fs::current_path();
        return strdup(cwd.string().c_str());
    } catch (...) {
        return strdup("");
    }
}

int64_t hoo_system_set_current_dir(const char* path) {
    if (!path) return -1;
    try {
        fs::current_path(path);
        return 0;
    } catch (...) {
        return -1;
    }
}

// ============================================================================
// Memory info
// ============================================================================

int64_t hoo_system_total_memory(void) {
#if defined(__APPLE__)
    int64_t mem = 0;
    size_t len = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0) {
        return mem;
    }
    return -1;
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<int64_t>(info.totalram) * static_cast<int64_t>(info.mem_unit);
    }
    return -1;
#elif defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int64_t>(status.ullTotalPhys);
    }
    return -1;
#else
    return -1;
#endif
}

int64_t hoo_system_free_memory(void) {
#if defined(__APPLE__)
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    mach_port_t host = mach_host_self();
    if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        uint32_t page_size = 0;
        size_t size = sizeof(page_size);
        if (sysctlbyname("hw.pagesize", &page_size, &size, nullptr, 0) == 0) {
            return static_cast<int64_t>(vm_stats.free_count) * static_cast<int64_t>(page_size);
        }
    }
    return -1;
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<int64_t>(info.freeram) * static_cast<int64_t>(info.mem_unit);
    }
    return -1;
#elif defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int64_t>(status.ullAvailPhys);
    }
    return -1;
#else
    return -1;
#endif
}

// ============================================================================
// Free string
// ============================================================================

void hoo_system_free_string(char* str) {
    free(str);
}
