#include "runtime/lib/system/hoo_system.h"
#include "runtime/lib/system/hoo_fs.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <thread>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <winternl.h>
#else
#include "core/Platform.h"
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <cerrno>
#include <cstring>
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/time.h>
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif
#endif

// ============================================================================
// Last-error reporting (thread-local)
// ============================================================================

static thread_local std::string g_lastError;

static void set_last_error(const char* message) {
    g_lastError = message ? message : "";
}

#ifdef _WIN32
static std::string wide16_to_utf8(const wchar_t* str, size_t len) {
    if (len == 0) return std::string();
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str, static_cast<int>(len),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str, static_cast<int>(len),
                        &out[0], size, nullptr, nullptr);
    return out;
}

// Set (non-null value) or delete (null value) an environment variable through
// the Win32 process environment block.  Returns true on success.  Names and
// values are converted to/from UTF-16 so the stored block round-trips through
// UTF-8 Hoo strings on any code page.
static bool set_wide_env(const char* name, const char* value) {
    const int nameLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name,
                                            static_cast<int>(std::strlen(name)), nullptr, 0);
    if (nameLen <= 0) return false;
    std::vector<wchar_t> wideName(static_cast<size_t>(nameLen) + 1, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, static_cast<int>(std::strlen(name)),
                        wideName.data(), nameLen);

    std::vector<wchar_t> wideValue;
    const wchar_t* valuePtr = nullptr;
    if (value) {
        const int valueLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value,
                                                 static_cast<int>(std::strlen(value)), nullptr, 0);
        if (valueLen <= 0) return false;
        wideValue.resize(static_cast<size_t>(valueLen) + 1, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, static_cast<int>(std::strlen(value)),
                            wideValue.data(), valueLen);
        valuePtr = wideValue.data();
    }
    return SetEnvironmentVariableW(wideName.data(), valuePtr) != 0;
}

static std::string utf16_win_error(DWORD error) {
    wchar_t* buffer = nullptr;
    const DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                    FORMAT_MESSAGE_IGNORE_INSERTS,
                                    nullptr, error, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (n == 0 || !buffer) {
        if (buffer) LocalFree(buffer);
        return std::string();
    }
    std::wstring wmsg(buffer, n);
    LocalFree(buffer);
    while (!wmsg.empty() && (wmsg.back() == L'\r' || wmsg.back() == L'\n')) wmsg.pop_back();
    std::string msg = wide16_to_utf8(wmsg.c_str(), wmsg.size());
    if (msg.empty()) msg = "unknown error";
    return msg;
}

static void set_last_error_win(void) {
    const DWORD error = GetLastError();
    char codeBuf[64];
    std::snprintf(codeBuf, sizeof(codeBuf), " (Win32 error %lu)", static_cast<unsigned long>(error));
    g_lastError = utf16_win_error(error) + codeBuf;
}
#else
static void set_last_error_posix(void) {
    g_lastError = std::strerror(errno);
}
#endif

const char* hoo_system_error_message(void) {
    return g_lastError.c_str();
}

// ============================================================================
// Environment
// ============================================================================

char* hoo_system_get_env(const char* name) {
    if (!name) return nullptr;
#ifdef _WIN32
    // Read from the Win32 process environment block so that get/set/unset stay
    // consistent (the UCRT getenv() keeps a separate copy that SetEnvironment
    // VariableW does not update).  Convert through UTF-16 so the returned Hoo
    // string is always valid UTF-8 regardless of the active code page.
    std::string sname(name);
    const int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sname.c_str(),
                                            static_cast<int>(sname.size()), nullptr, 0);
    if (wideLen <= 0) { set_last_error("environment variable name is not valid UTF-8"); return nullptr; }
    std::vector<wchar_t> wideName(static_cast<size_t>(wideLen) + 1, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sname.c_str(), static_cast<int>(sname.size()),
                        wideName.data(), wideLen);
    const DWORD size = GetEnvironmentVariableW(wideName.data(), nullptr, 0);
    if (size == 0) return nullptr;
    std::vector<wchar_t> value(size + 1, L'\0');
    const DWORD copied = GetEnvironmentVariableW(wideName.data(), value.data(), size);
    if (copied == 0) return nullptr;
    std::string utf8 = wide16_to_utf8(value.data(), copied);
    return utf8.empty() ? strdup("") : strdup(utf8.c_str());
#else
    const char* val = getenv(name);
    if (!val) return nullptr;
    return strdup(val);
#endif
}

int64_t hoo_system_set_env(const char* name, const char* value) {
    if (!name || !value) return -1;
    if (value[0] == '\0') {
        // An empty value is normalized to "unset" so behaviour is identical on
        // every platform (the Win32 environment block cannot represent an
        // empty-but-present variable separately from an absent one).
        return hoo_system_unset_env(name);
    }
#ifdef _WIN32
    if (!set_wide_env(name, value)) {
        set_last_error_win();
        return -1;
    }
    return 0;
#else
    if (setenv(name, value, 1) != 0) {
        set_last_error_posix();
        return -1;
    }
    return 0;
#endif
}

int64_t hoo_system_unset_env(const char* name) {
    if (!name) return -1;
#ifdef _WIN32
    // SetEnvironmentVariableW with a null value truly deletes the variable.
    // _putenv_s(name, "") only sets an empty value that is merely hidden from
    // getenv() by the UCRT but still visible to child processes.
    if (!set_wide_env(name, nullptr)) {
        set_last_error_win();
        return -1;
    }
    return 0;
#else
    if (unsetenv(name) != 0) {
        set_last_error_posix();
        return -1;
    }
    return 0;
#endif
}

// ============================================================================
// System info
// ============================================================================

char* hoo_system_hostname(void) {
#ifdef _WIN32
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = static_cast<DWORD>(sizeof(buf) / sizeof(buf[0]));
    if (GetComputerNameW(buf, &len)) {
        std::string utf8 = wide16_to_utf8(buf, len);
        if (!utf8.empty()) return strdup(utf8.c_str());
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
    // GetVersionExA is deprecated and, without an application compatibility
    // manifest, reports 6.2.x on Windows 8.1+.  Query ntdll's RtlGetVersion
    // instead, which always reports the real version.
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
        auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (fn) {
            RTL_OSVERSIONINFOW info;
            info.dwOSVersionInfoSize = sizeof(info);
            if (fn(&info) == 0) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%lu.%lu.%lu",
                              info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
                return strdup(buf);
            }
        }
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
    set_last_error("kern.boottime query failed");
    return -1;
#elif defined(__linux__)
    FILE* f = fopen("/proc/uptime", "r");
    if (!f) {
        set_last_error("cannot open /proc/uptime");
        return -1;
    }
    double uptime;
    int matched = fscanf(f, "%lf", &uptime);
    fclose(f);
    if (matched == 1) {
        return static_cast<int64_t>(uptime * 1000.0);
    }
    set_last_error("cannot parse /proc/uptime");
    return -1;
#elif defined(_WIN32)
    return static_cast<int64_t>(GetTickCount64());
#else
    set_last_error("uptime is not supported on this platform");
    return -1;
#endif
}

// ============================================================================
// Process
// ============================================================================

void hoo_system_exit(int64_t code) {
    // The exit status of a Hoo program is part of its software contract with
    // the host: clamp to the 0..255 range a process can report, and flush the
    // C streams so buffered output is not lost.  This terminates immediately
    // and does NOT run ARC/drop teardown or unwind the shadow stack; embedders
    // that need clean unwinding should raise a Hoo exception instead.
    const int exitCode = static_cast<int>(code) & 0xFF;
    std::fflush(stdout);
    std::fflush(stderr);
    exit(exitCode);
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
    if (!command) return nullptr;
    FILE* pipe = open_pipe(command, "r");
    if (!pipe) {
#ifdef _WIN32
        set_last_error_win();
#else
        set_last_error_posix();
#endif
        return nullptr;
    }

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
    if (!pipe) {
#ifdef _WIN32
        set_last_error_win();
#else
        set_last_error_posix();
#endif
        return -1;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
    }
    const int status = close_pipe(pipe);
#ifdef _WIN32
    // _pclose already returns the child process exit code.
    return status;
#else
    // Normalize the raw pclose() status: the exit code is encoded in the high
    // byte; a signal-terminated child is reported as 128 + signal, matching
    // common shell conventions.  -1 means the status could not be determined.
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
#endif
}

// ============================================================================
// User info
// ============================================================================

char* hoo_system_user_home(void) {
#ifdef _WIN32
    char* home = hoo_system_get_env("USERPROFILE");
#else
    char* home = hoo_system_get_env("HOME");
#endif
    if (!home) return strdup("");
    return home;
}

char* hoo_system_user_name(void) {
#ifdef _WIN32
    char* user = hoo_system_get_env("USERNAME");
#else
    char* user = hoo_system_get_env("USER");
    if (!user) user = hoo_system_get_env("LOGNAME");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_name) user = strdup(pw->pw_name);
    }
#endif
    if (!user) return strdup("");
    return user;
}

char* hoo_system_current_dir(void) {
    // Single source of truth lives in the fs module (hoo::fs::currentDir).
    char* dir = hoo_fs_current_dir();
    if (!dir) return strdup("");
    return dir;
}

int64_t hoo_system_set_current_dir(const char* path) {
    if (!path) return -1;
    try {
        fs::current_path(path);
        return 0;
    } catch (const fs::filesystem_error& e) {
        set_last_error(e.what());
        return -1;
    } catch (...) {
        set_last_error("failed to change current directory");
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
    set_last_error("hw.memsize query failed");
    return -1;
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<int64_t>(info.totalram) * static_cast<int64_t>(info.mem_unit);
    }
    set_last_error("sysinfo failed");
    return -1;
#elif defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int64_t>(status.ullTotalPhys);
    }
    set_last_error_win();
    return -1;
#else
    set_last_error("memory query is not supported on this platform");
    return -1;
#endif
}

int64_t hoo_system_free_memory(void) {
#if defined(__APPLE__)
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    mach_port_t host = mach_host_self();
    bool queried = host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS;
    // mach_host_self() returns a send right on the host port that must be
    // released, otherwise each free_memory() call leaks a send right.
    mach_port_deallocate(mach_task_self(), host);
    if (queried) {
        uint32_t page_size = 0;
        size_t size = sizeof(page_size);
        if (sysctlbyname("hw.pagesize", &page_size, &size, nullptr, 0) == 0) {
            return static_cast<int64_t>(vm_stats.free_count) * static_cast<int64_t>(page_size);
        }
    }
    set_last_error("host statistics query failed");
    return -1;
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<int64_t>(info.freeram) * static_cast<int64_t>(info.mem_unit);
    }
    set_last_error("sysinfo failed");
    return -1;
#elif defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int64_t>(status.ullAvailPhys);
    }
    set_last_error_win();
    return -1;
#else
    set_last_error("memory query is not supported on this platform");
    return -1;
#endif
}

// ============================================================================
// Free string
// ============================================================================

void hoo_system_free_string(char* str) {
    free(str);
}
