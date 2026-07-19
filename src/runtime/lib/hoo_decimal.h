#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooDecimal;

// Lifecycle
HooDecimal hoo_decimal_from_literal(const char* text, int32_t precision, int32_t scale);
HooDecimal hoo_decimal_new(int64_t mantissa, int32_t precision, int32_t scale);
void       hoo_decimal_release(HooDecimal d);
HooDecimal hoo_decimal_retain(HooDecimal d);

// Accessors
int64_t hoo_decimal_mantissa(HooDecimal d);
int32_t hoo_decimal_precision(HooDecimal d);
int32_t hoo_decimal_scale(HooDecimal d);

// Arithmetic (returns new Decimal)
HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b);
HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b);

// Comparison (returns 1 or 0)
int64_t    hoo_decimal_eq(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_ne(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_lt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_le(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_gt(HooDecimal a, HooDecimal b);
int64_t    hoo_decimal_ge(HooDecimal a, HooDecimal b);

// String representation
char*      hoo_decimal_to_string(HooDecimal d);

#ifdef __cplusplus
}
#endif
