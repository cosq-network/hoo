#pragma once

// Platform detection macros
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
#define HOO_PLATFORM_WINDOWS 1
#else
#define HOO_PLATFORM_POSIX 1
#endif

// Include common headers based on platform
#ifdef HOO_PLATFORM_WINDOWS
    // Windows headers
    #include <windows.h>
    #include <conio.h>
    #include <process.h>
    #include <io.h>
#else
    // POSIX headers
    #ifdef HOO_PLATFORM_POSIX
#include <unistd.h>
#include <sys/select.h>
#endif
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <errno.h>
#endif

// Export macro for shared library (if needed elsewhere)
#if defined(_WIN32) || defined(_WIN64)
    #define HOO_API __declspec(dllexport)
#else
    #define HOO_API __attribute__((visibility("default")))
#endif
