#include "hoo_math.h"
#include "hoo_runtime.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <atomic>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Math Constants
// ============================================================================

double hoo_math_get_pi(void) { return 3.141592653589793; }
double hoo_math_get_e(void) { return 2.718281828459045; }
double hoo_math_get_tau(void) { return 6.283185307179586; }
double hoo_math_get_inf(void) { return INFINITY; }
double hoo_math_get_neg_inf(void) { return -INFINITY; }
double hoo_math_get_nan(void) { return NAN; }

// ============================================================================
// Basic Functions
// ============================================================================

int64_t hoo_math_abs_int64(int64_t x) { return x >= 0 ? x : -x; }
double hoo_math_abs_double(double x) { return std::fabs(x); }

int64_t hoo_math_min_int64(int64_t a, int64_t b) { return a < b ? a : b; }
double hoo_math_min_double(double a, double b) { return std::fmin(a, b); }

int64_t hoo_math_max_int64(int64_t a, int64_t b) { return a > b ? a : b; }
double hoo_math_max_double(double a, double b) { return std::fmax(a, b); }

double hoo_math_clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int64_t hoo_math_sign_int64(int64_t x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

double hoo_math_sign_double(double x) {
    if (x > 0) return 1.0;
    if (x < 0) return -1.0;
    return 0.0;
}

// ============================================================================
// Power and Roots
// ============================================================================

double hoo_math_pow(double base, double exponent) { return std::pow(base, exponent); }
double hoo_math_sqrt(double x) { return std::sqrt(x); }
double hoo_math_cbrt(double x) { return std::cbrt(x); }
double hoo_math_hypot(double x, double y) { return std::hypot(x, y); }

// ============================================================================
// Trigonometric Functions
// ============================================================================

double hoo_math_sin(double x) { return std::sin(x); }
double hoo_math_cos(double x) { return std::cos(x); }
double hoo_math_tan(double x) { return std::tan(x); }
double hoo_math_asin(double x) { return std::asin(x); }
double hoo_math_acos(double x) { return std::acos(x); }
double hoo_math_atan(double x) { return std::atan(x); }
double hoo_math_atan2(double y, double x) { return std::atan2(y, x); }
double hoo_math_sinh(double x) { return std::sinh(x); }
double hoo_math_cosh(double x) { return std::cosh(x); }
double hoo_math_tanh(double x) { return std::tanh(x); }

// ============================================================================
// Exponential and Logarithmic
// ============================================================================

double hoo_math_exp(double x) { return std::exp(x); }
double hoo_math_exp2(double x) { return std::exp2(x); }
double hoo_math_expm1(double x) { return std::expm1(x); }
double hoo_math_log(double x) { return std::log(x); }
double hoo_math_log10(double x) { return std::log10(x); }
double hoo_math_log2(double x) { return std::log2(x); }
double hoo_math_log1p(double x) { return std::log1p(x); }

// ============================================================================
// Rounding Functions
// ============================================================================

double hoo_math_floor(double x) { return std::floor(x); }
double hoo_math_ceil(double x) { return std::ceil(x); }
double hoo_math_round(double x) { return std::round(x); }
double hoo_math_trunc(double x) { return std::trunc(x); }
double hoo_math_fract(double x) { return x - std::floor(x); }

// ============================================================================
// Random Number Generation
// ============================================================================

struct HooRandomImpl {
    std::mt19937_64 rng;
};

void* hoo_math_random_new(void) {
    std::random_device rd;
    void* mem = hoo_alloc(sizeof(HooRandomImpl), HOO_TYPE_RANDOM);
    HooRandomImpl* impl = new (mem) HooRandomImpl();
    impl->rng.seed(rd());
    return impl;
}

void* hoo_math_random_new_with_seed(int64_t seed) {
    void* mem = hoo_alloc(sizeof(HooRandomImpl), HOO_TYPE_RANDOM);
    HooRandomImpl* impl = new (mem) HooRandomImpl();
    impl->rng.seed(static_cast<uint64_t>(seed));
    return impl;
}

int64_t hoo_math_random_next_int(void* state) {
    if (!state) return 0;
    HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
    return static_cast<int64_t>(impl->rng());
}

int64_t hoo_math_random_next_int_max(void* state, int64_t max) {
    if (!state || max <= 0) return 0;
    HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
    std::uniform_int_distribution<int64_t> dist(0, max - 1);
    return dist(impl->rng);
}

double hoo_math_random_next_double(void* state) {
    if (!state) return 0.0;
    HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(impl->rng);
}

int64_t hoo_math_random_next_bool(void* state) {
    if (!state) return 0;
    HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
    std::uniform_int_distribution<int64_t> dist(0, 1);
    return dist(impl->rng);
}

int64_t hoo_math_random_next_bytes(void* state, int8_t* buffer, int64_t count) {
    if (!state || !buffer || count <= 0) return 0;
    HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
    for (int64_t i = 0; i < count; i++) {
        buffer[i] = static_cast<int8_t>(impl->rng() & 0xFF);
    }
    return count;
}

void* hoo_math_random_retain(void* state) {
    return (void*)hoo_retain(state);
}

void hoo_math_random_release(void* state) {
    if (!state) return;

    if (hoo_get_refcount(state) == 1) {
        HooRandomImpl* impl = static_cast<HooRandomImpl*>(state);
        impl->~HooRandomImpl();
    }

    hoo_release(state);
}

// ============================================================================
// Number Utilities
// ============================================================================

int64_t hoo_math_is_even(int64_t n) { return (n % 2 == 0) ? 1 : 0; }
int64_t hoo_math_is_odd(int64_t n) { return (n % 2 != 0) ? 1 : 0; }

int64_t hoo_math_is_prime(int64_t n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

int64_t hoo_math_gcd(int64_t a, int64_t b) {
    a = hoo_math_abs_int64(a);
    b = hoo_math_abs_int64(b);
    while (b != 0) {
        int64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int64_t hoo_math_lcm(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return 0;
    return hoo_math_abs_int64(a / hoo_math_gcd(a, b) * b);
}

int64_t hoo_math_factorial(int64_t n) {
    if (n < 0) return 0;
    if (n <= 1) return 1;
    if (n > 20) n = 20;
    int64_t result = 1;
    for (int64_t i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int64_t hoo_math_fibonacci(int64_t n) {
    if (n < 0) return 0;
    if (n <= 1) return n;
    if (n > 92) n = 92;
    int64_t a = 0, b = 1;
    for (int64_t i = 2; i <= n; i++) {
        int64_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

#ifdef __cplusplus
}
#endif