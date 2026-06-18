#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooMath - Mathematical Functions and Constants
// ============================================================================
//
// Provides mathematical constants, functions, and a Random class for the hoo.math module.
// All functions use double precision unless otherwise specified.
//
// Constants:
//   - PI (3.141592653589793)
//   - E (2.718281828459045)
//   - TAU (6.283185307179586)
//   - INF (positive infinity)
//   - NEG_INF (negative infinity)
//   - NAN (not-a-number)

// ============================================================================
// Math Constants
// ============================================================================

/**
 * Get value of PI
 * @return 3.141592653589793
 */
double hoo_math_get_pi(void);

/**
 * Get value of E
 * @return 2.718281828459045
 */
double hoo_math_get_e(void);

/**
 * Get value of TAU
 * @return 6.283185307179586
 */
double hoo_math_get_tau(void);

/**
 * Get positive infinity
 * @return Infinity
 */
double hoo_math_get_inf(void);

/**
 * Get negative infinity
 * @return -Infinity
 */
double hoo_math_get_neg_inf(void);

/**
 * Get NaN
 * @return NaN
 */
double hoo_math_get_nan(void);

// ============================================================================
// Basic Functions
// ============================================================================

/**
 * Absolute value of int64
 * @param x Input value
 * @return Absolute value
 */
int64_t hoo_math_abs_int64(int64_t x);

/**
 * Absolute value of int8
 * @param x Input value
 * @return Absolute value
 */
int8_t hoo_math_abs_int8(int8_t x);

/**
 * Absolute value of byte (uint8)
 * @param x Input value
 * @return Absolute value
 */
uint8_t hoo_math_abs_byte(uint8_t x);

/**
 * Absolute value of double
 * @param x Input value
 * @return Absolute value
 */
double hoo_math_abs_double(double x);

/**
 * Absolute value of f8 (promoted to double)
 * @param x Input value
 * @return Absolute value
 */
double hoo_math_abs_f8(double x);

/**
 * Minimum of two int64 values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
int64_t hoo_math_min_int64(int64_t a, int64_t b);

/**
 * Minimum of two int8 values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
int8_t hoo_math_min_int8(int8_t a, int8_t b);

/**
 * Minimum of two byte (uint8) values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
uint8_t hoo_math_min_byte(uint8_t a, uint8_t b);

/**
 * Minimum of two double values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
double hoo_math_min_double(double a, double b);

/**
 * Minimum of two f8 (promoted to double) values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
double hoo_math_min_f8(double a, double b);

/**
 * Maximum of two int64 values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
int64_t hoo_math_max_int64(int64_t a, int64_t b);

/**
 * Maximum of two int8 values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
int8_t hoo_math_max_int8(int8_t a, int8_t b);

/**
 * Maximum of two byte (uint8) values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
uint8_t hoo_math_max_byte(uint8_t a, uint8_t b);

/**
 * Maximum of two double values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
double hoo_math_max_double(double a, double b);

/**
 * Maximum of two f8 (promoted to double) values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
double hoo_math_max_f8(double a, double b);

/**
 * Clamp value between min and max
 * @param value Value to clamp
 * @param min Minimum bound
 * @param max Maximum bound
 * @return Clamped value
 */
double hoo_math_clamp(double value, double min, double max);

/**
 * Sign of int64 value
 * @param x Input value
 * @return -1 if negative, 0 if zero, 1 if positive
 */
int64_t hoo_math_sign_int64(int64_t x);

/**
 * Sign of int8 value
 * @param x Input value
 * @return -1 if negative, 0 if zero, 1 if positive
 */
int8_t hoo_math_sign_int8(int8_t x);

/**
 * Sign of byte (uint8) value
 * @param x Input value
 * @return 0 if zero, 1 if positive
 */
uint8_t hoo_math_sign_byte(uint8_t x);

/**
 * Sign of double value
 * @param x Input value
 * @return -1.0 if negative, 0.0 if zero, 1.0 if positive
 */
double hoo_math_sign_double(double x);

/**
 * Sign of f8 (promoted to double) value
 * @param x Input value
 * @return -1.0 if negative, 0.0 if zero, 1.0 if positive
 */
double hoo_math_sign_f8(double x);

// ============================================================================
// Power and Roots
// ============================================================================

/**
 * Power function (base^exponent)
 * @param base Base value
 * @param exponent Exponent
 * @return base raised to the power of exponent
 */
double hoo_math_pow(double base, double exponent);

/**
 * Square root
 * @param x Input value
 * @return Square root
 */
double hoo_math_sqrt(double x);

/**
 * Cube root
 * @param x Input value
 * @return Cube root
 */
double hoo_math_cbrt(double x);

/**
 * Hypotenuse (sqrt(x^2 + y^2))
 * @param x First value
 * @param y Second value
 * @return Hypotenuse
 */
double hoo_math_hypot(double x, double y);

// ============================================================================
// Trigonometric Functions
// ============================================================================

/**
 * Sine (radians)
 * @param x Input in radians
 * @return Sine
 */
double hoo_math_sin(double x);

/**
 * Cosine (radians)
 * @param x Input in radians
 * @return Cosine
 */
double hoo_math_cos(double x);

/**
 * Tangent (radians)
 * @param x Input in radians
 * @return Tangent
 */
double hoo_math_tan(double x);

/**
 * Arc sine (radians)
 * @param x Input [-1, 1]
 * @return Arc sine in radians
 */
double hoo_math_asin(double x);

/**
 * Arc cosine (radians)
 * @param x Input [-1, 1]
 * @return Arc cosine in radians
 */
double hoo_math_acos(double x);

/**
 * Arc tangent (radians)
 * @param x Input
 * @return Arc tangent in radians
 */
double hoo_math_atan(double x);

/**
 * Arc tangent of y/x (radians)
 * @param y Y coordinate
 * @param x X coordinate
 * @return Arc tangent in radians
 */
double hoo_math_atan2(double y, double x);

/**
 * Hyperbolic sine
 * @param x Input
 * @return Hyperbolic sine
 */
double hoo_math_sinh(double x);

/**
 * Hyperbolic cosine
 * @param x Input
 * @return Hyperbolic cosine
 */
double hoo_math_cosh(double x);

/**
 * Hyperbolic tangent
 * @param x Input
 * @return Hyperbolic tangent
 */
double hoo_math_tanh(double x);

// ============================================================================
// Exponential and Logarithmic
// ============================================================================

/**
 * e raised to the power x
 * @param x Exponent
 * @return e^x
 */
double hoo_math_exp(double x);

/**
 * 2 raised to the power x
 * @param x Exponent
 * @return 2^x
 */
double hoo_math_exp2(double x);

/**
 * e^x - 1 (for small x)
 * @param x Exponent
 * @return e^x - 1
 */
double hoo_math_expm1(double x);

/**
 * Natural logarithm (base e)
 * @param x Input (> 0)
 * @return Natural log
 */
double hoo_math_log(double x);

/**
 * Base 10 logarithm
 * @param x Input (> 0)
 * @return Log base 10
 */
double hoo_math_log10(double x);

/**
 * Base 2 logarithm
 * @param x Input (> 0)
 * @return Log base 2
 */
double hoo_math_log2(double x);

/**
 * ln(1 + x) (for small x)
 * @param x Input
 * @return ln(1 + x)
 */
double hoo_math_log1p(double x);

// ============================================================================
// Rounding Functions
// ============================================================================

/**
 * Floor (largest integer <= x)
 * @param x Input
 * @return Floor value
 */
double hoo_math_floor(double x);

/**
 * Ceiling (smallest integer >= x)
 * @param x Input
 * @return Ceiling value
 */
double hoo_math_ceil(double x);

/**
 * Round to nearest integer
 * @param x Input
 * @return Rounded value
 */
double hoo_math_round(double x);

/**
 * Truncate toward zero
 * @param x Input
 * @return Truncated value
 */
double hoo_math_trunc(double x);

/**
 * Fractional part
 * @param x Input
 * @return Fractional part
 */
double hoo_math_fract(double x);

// ============================================================================
// Random Number Generation
// ============================================================================

/**
 * Create a new Random generator with random seed
 * @return Pointer to Random state (refcount=1)
 */
void* hoo_math_random_new(void);

/**
 * Create a Random generator with specified seed
 * @param seed Initial seed value
 * @return Pointer to Random state (refcount=1)
 */
void* hoo_math_random_new_with_seed(int64_t seed);

/**
 * Get next random int64
 * @param state Random state
 * @return Next random int64
 */
int64_t hoo_math_random_next_int(void* state);

/**
 * Get next random int64 in range [0, max)
 * @param state Random state
 * @param max Upper bound (exclusive)
 * @return Random int64 in [0, max)
 */
int64_t hoo_math_random_next_int_max(void* state, int64_t max);

/**
 * Get next random double in [0, 1)
 * @param state Random state
 * @return Random double
 */
double hoo_math_random_next_double(void* state);

/**
 * Get next random bool
 * @param state Random state
 * @return Random true/false
 */
int64_t hoo_math_random_next_bool(void* state);

/**
 * Fill buffer with random bytes
 * @param state Random state
 * @param buffer HooBuffer handle to fill
 * @param count Number of bytes
 * @return Number of bytes written
 */
int64_t hoo_math_random_next_bytes(void* state, void* buffer, int64_t count);

/**
 * Retain random state
 * @param state Random state
 * @return Same state
 */
void* hoo_math_random_retain(void* state);

/**
 * Release random state
 * @param state Random state
 */
void hoo_math_random_release(void* state);

// ============================================================================
// Number Utilities
// ============================================================================

/**
 * Check if number is even
 * @param n Input
 * @return 1 if even, 0 otherwise
 */
int64_t hoo_math_is_even(int64_t n);

/**
 * Check if number is odd
 * @param n Input
 * @return 1 if odd, 0 otherwise
 */
int64_t hoo_math_is_odd(int64_t n);

/**
 * Check if number is prime
 * @param n Input (positive)
 * @return 1 if prime, 0 otherwise
 */
int64_t hoo_math_is_prime(int64_t n);

/**
 * Greatest common divisor
 * @param a First value
 * @param b Second value
 * @return GCD
 */
int64_t hoo_math_gcd(int64_t a, int64_t b);

/**
 * Least common multiple
 * @param a First value
 * @param b Second value
 * @return LCM
 */
int64_t hoo_math_lcm(int64_t a, int64_t b);

/**
 * Factorial (n!)
 * @param n Input (0-20)
 * @return Factorial
 */
int64_t hoo_math_factorial(int64_t n);

/**
 * Fibonacci number
 * @param n Index (0-92)
 * @return Fibonacci number
 */
int64_t hoo_math_fibonacci(int64_t n);

#ifdef __cplusplus
}
#endif
