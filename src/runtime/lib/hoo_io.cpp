#include "hoo_io.h"
#include "hoo_string.h"
#include "hoo_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>

// ============================================================================
// I/O Implementation
// ============================================================================

void hoo_print(void* str) {
    if (str == nullptr) {
        std::printf("null");
    } else {
        const char* data = hoo_string_data(str);
        if (data) {
            std::printf("%s", data);
        } else {
            std::printf("(null)");
        }
    }
    std::fflush(stdout);
}

void hoo_println(void* str) {
    if (str == nullptr) {
        std::printf("null\n");
    } else {
        const char* data = hoo_string_data(str);
        if (data) {
            std::printf("%s\n", data);
        } else {
            std::printf("(null)\n");
        }
    }
    std::fflush(stdout);
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

int64_t hoo_readchar(void) {
    int ch = std::getchar();
    if (ch == EOF) {
        return -1;
    }
    return static_cast<int64_t>(ch);
}