# Math & Utilities

The `Math` class exposes mathematical primitives and random number generation capabilities backed by the standard C++ `<cmath>` and `<random>` libraries.

## 1. Constants
- `Math.get_pi()` -> `3.141592653589793`
- `Math.get_e()` -> `2.718281828459045`
- `Math.get_tau()` -> `6.283185307179586`
- `Math.get_inf()`, `Math.get_neg_inf()`, `Math.get_nan()`

## 2. Operations
The math library provides both `int64` and `double` variants for standard operations.
- **Basic**: `abs`, `min`, `max`, `clamp`, `sign`.
  - `Math.abs(n)` clamps `INT64_MIN` to `INT64_MAX` to avoid overflow.
- **Power/Roots**: `pow`, `sqrt`, `cbrt`, `hypot`.
- **Trigonometry**: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`.
- **Exponentials**: `exp`, `exp2`, `expm1`, `log`, `log10`, `log2`, `log1p`.
- **Rounding**: `floor`, `ceil`, `round`, `trunc`, `fract`.

## 3. Number Utilities
- `Math.is_even(n)`, `Math.is_odd(n)`
- `Math.is_prime(n)`
- `Math.gcd(a, b)`, `Math.lcm(a, b)`
- `Math.factorial(n)` (Clamped to max 20 to fit in 64 bits)
- `Math.fibonacci(n)` (Clamped to max 92 to fit in 64 bits)

## 4. Random Number Generation
Random generation uses an ARC-managed opaque handle representing a `std::mt19937_64` generator (`RandomImpl`).

Release operations are thread-safe — a mutex guards the refcount check during final cleanup.

- **Creation**: `Random.new()` (auto-seeded) or `Random.new_with_seed(int64)`.
- **Operations**:
  - `state.next_int()`
  - `state.next_int(max)`
  - `state.next_double()`
  - `state.next_bool()`
  - `state.next_bytes(buffer, count)`
