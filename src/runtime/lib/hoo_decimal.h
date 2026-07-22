#pragma once
#include <cstdint>
#include "hoo_string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooDecimal;

// ============================================================================
// Exception Type IDs for Decimal
// ============================================================================
#define HOO_EXCEPTION_DECIMAL_OVERFLOW   100
#define HOO_EXCEPTION_DECIMAL_DIV_ZERO   101
#define HOO_EXCEPTION_DECIMAL_MOD_ZERO   102

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create a Decimal from a string literal.
 * @param text      String representation (e.g., "19.99")
 * @param precision Total number of significant digits
 * @param scale     Number of digits after decimal point
 * @return          New Decimal, or NULL on parse error
 */
HooDecimal hoo_decimal_from_literal(const char* text, int32_t precision, int32_t scale);

/**
 * Create a Decimal from a scaled integer mantissa.
 * @param mantissa  The value * 10^scale
 * @param precision Total number of significant digits
 * @param scale     Number of digits after decimal point
 * @return          New Decimal
 */
HooDecimal hoo_decimal_new(int64_t mantissa, int32_t precision, int32_t scale);

/**
 * Release a Decimal (decrement reference count).
 */
void hoo_decimal_release(HooDecimal d);

/**
 * Retain a Decimal (increment reference count).
 */
HooDecimal hoo_decimal_retain(HooDecimal d);

// ============================================================================
// Accessors
// ============================================================================

/**
 * Get the mantissa (value * 10^scale).
 */
int64_t hoo_decimal_mantissa(HooDecimal d);

/**
 * Get the precision (total significant digits).
 */
int32_t hoo_decimal_precision(HooDecimal d);

/**
 * Get the scale (digits after decimal point).
 */
int32_t hoo_decimal_scale(HooDecimal d);

// ============================================================================
// Arithmetic (returns new Decimal; throws on overflow/division-by-zero)
// ============================================================================

/**
 * Add two Decimals.
 * @throws HOO_EXCEPTION_DECIMAL_OVERFLOW if result exceeds precision
 */
HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b);

/**
 * Subtract two Decimals.
 * @throws HOO_EXCEPTION_DECIMAL_OVERFLOW if result exceeds precision
 */
HooDecimal hoo_decimal_sub(HooDecimal a, HooDecimal b);

/**
 * Multiply two Decimals.
 * @throws HOO_EXCEPTION_DECIMAL_OVERFLOW if result exceeds precision
 */
HooDecimal hoo_decimal_mul(HooDecimal a, HooDecimal b);

/**
 * Divide two Decimals.
 * @throws HOO_EXCEPTION_DECIMAL_DIV_ZERO if divisor is zero
 * @throws HOO_EXCEPTION_DECIMAL_OVERFLOW if result exceeds precision
 */
HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b);

/**
 * Modulo two Decimals.
 * @throws HOO_EXCEPTION_DECIMAL_MOD_ZERO if divisor is zero
 */
HooDecimal hoo_decimal_mod(HooDecimal a, HooDecimal b);

/**
 * Negate a Decimal (unary minus).
 */
HooDecimal hoo_decimal_neg(HooDecimal d);

// ============================================================================
// Comparison (returns 1 or 0)
// ============================================================================

int64_t hoo_decimal_eq(HooDecimal a, HooDecimal b);
int64_t hoo_decimal_ne(HooDecimal a, HooDecimal b);
int64_t hoo_decimal_lt(HooDecimal a, HooDecimal b);
int64_t hoo_decimal_le(HooDecimal a, HooDecimal b);
int64_t hoo_decimal_gt(HooDecimal a, HooDecimal b);
int64_t hoo_decimal_ge(HooDecimal a, HooDecimal b);

// ============================================================================
// String representation
// ============================================================================

/**
 * Convert Decimal to string.
 * @return ARC-managed HooString with refcount=1. Caller should hoo_release() when done.
 */
HooString hoo_decimal_to_string(HooDecimal d);

#ifdef __cplusplus
}
#endif

