# Math & Utilities

The `hoo.math` module exposes mathematical primitives and random number generation capabilities backed by the standard C++ `<cmath>` and `<random>` libraries.

## 1. Constants
- `hoo_math_get_pi()` -> `3.141592653589793`
- `hoo_math_get_e()` -> `2.718281828459045`
- `hoo_math_get_tau()` -> `6.283185307179586`
- `hoo_math_get_inf()`, `hoo_math_get_neg_inf()`, `hoo_math_get_nan()`

## 2. Operations
The math library provides both `int64` and `double` variants for standard operations.
- **Basic**: `abs`, `min`, `max`, `clamp`, `sign`.
- **Power/Roots**: `pow`, `sqrt`, `cbrt`, `hypot`.
- **Trigonometry**: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`.
- **Exponentials**: `exp`, `exp2`, `expm1`, `log`, `log10`, `log2`, `log1p`.
- **Rounding**: `floor`, `ceil`, `round`, `trunc`, `fract`.

## 3. Number Utilities
- `hoo_math_is_even(n)`, `hoo_math_is_odd(n)`
- `hoo_math_is_prime(n)`
- `hoo_math_gcd(a, b)`, `hoo_math_lcm(a, b)`
- `hoo_math_factorial(n)` (Clamped to max 20 to fit in 64 bits)
- `hoo_math_fibonacci(n)` (Clamped to max 92 to fit in 64 bits)

## 4. Random Number Generation
Random generation uses an ARC-managed opaque handle representing a `std::mt19937_64` generator (`HooRandomImpl`).

- **Creation**: `hoo_math_random_new()` (auto-seeded) or `hoo_math_random_new_with_seed(int64)`.
- **Operations**:
  - `hoo_math_random_next_int(state)`
  - `hoo_math_random_next_int_max(state, max)`
  - `hoo_math_random_next_double(state)`
  - `hoo_math_random_next_bool(state)`
  - `hoo_math_random_next_bytes(state, buffer, count)`
