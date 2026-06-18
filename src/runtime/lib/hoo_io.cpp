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

void hoo_println(void* str) {
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

int64_t hoo_readchar(void) {
    int ch = std::getchar();
    if (ch == EOF) {
        return -1;
    }
    return static_cast<int64_t>(ch);
}