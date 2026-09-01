#include "runtime/lib/io/hoo_io.h"
#include "runtime/lib/text/hoo_string.h"
#include "runtime/lib/core/hoo_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "core/Platform.h"
#ifdef HOO_PLATFORM_WINDOWS
    #include <io.h>
    #include <fcntl.h>
#endif
#include "runtime/lib/text/hoo_character.h"
// ============================================================================
// I/O Implementation
// ============================================================================

namespace {

const char* resolve_print_data(void* str) {
    if (str == nullptr) return "null";
    const char* data = hoo_string_data(str);
    return data ? data : "null";
}

void print_impl(void* str, const char* suffix) {
    std::printf("%s%s", resolve_print_data(str), suffix);
    std::fflush(stdout);
}

} // anonymous namespace

void hoo_print(void* str) {
    print_impl(str, "");
}

void
hoo_println(void* str) {
    print_impl(str, "\n");
}

void* hoo_readline(void) {
    std::string line;
    
    // Use std::getline to read a line from stdin
    if (!std::getline(std::cin, line)) {
        // EOF or error - return empty string
        return hoo_string_new();
    }
    
    // Convert to HooString
    return hoo_string_from_cstr(line.c_str());
}

HooCharacter hoo_readchar(void) {
#ifdef HOO_PLATFORM_WINDOWS
    // Detect whether stdin is a console (interactive) or redirected.
    if (_isatty(_fileno(stdin))) {
        // Interactive console – use _kbhit for non‑blocking check.
        if (_kbhit()) {
            int ch = _getch();
            if (ch == EOF) return NULL;
            unsigned char byte = static_cast<unsigned char>(ch);
            return hoo_character_from_utf8(reinterpret_cast<const char*>(&byte), 1);
        }
        // No key pressed yet.
        return NULL;
    } else {
        // Redirected input – perform a blocking read of a single byte.
        unsigned char ch;
        int ret = _read(_fileno(stdin), &ch, 1);
        if (ret == 1) return hoo_character_from_utf8(reinterpret_cast<const char*>(&ch), 1);
        return NULL;
    }
#else
    int fd = fileno(stdin);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval timeout = {0, 0}; // zero timeout – non‑blocking
    if (select(fd + 1, &set, nullptr, nullptr, &timeout) > 0) {
        int ch = std::getchar();
        if (ch == EOF) {
            return NULL;
        }
        unsigned char byte = static_cast<unsigned char>(ch);
        return hoo_character_from_utf8(reinterpret_cast<const char*>(&byte), 1);
    }
    // No data available – indicate EOF/non‑blocking
    return NULL;
#endif
}

