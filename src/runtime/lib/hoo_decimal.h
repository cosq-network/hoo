#pragma once
#include <cstdint>

extern "C" {
    typedef void* HooDecimal;
    HooDecimal hoo_decimal_from_literal(const char* text, int32_t precision, int32_t scale);
}