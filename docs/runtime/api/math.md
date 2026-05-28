# Math API Reference (`hoo.math`)

The `math` module provides a comprehensive suite of mathematical constants, basic functions, power/root operations, and random number generation.

## 1. Constants

### `math_get_pi() -> double`
Returns the value of $\pi$ (approximately 3.14159).

### `math_get_e() -> double`
Returns the value of $e$ (approximately 2.71828).

### `math_get_inf() -> double`
Returns positive infinity.

### `math_get_nan() -> double`
Returns Not-a-Number (NaN).

## 2. Basic Functions

### `math_abs_int64(x: int64) -> int64`
Returns the absolute value of an integer.

### `math_abs_double(x: double) -> double`
Returns the absolute value of a double.

### `math_min_int64(a: int64, b: int64) -> int64`
Returns the smaller of two integers.

### `math_max_double(a: double, b: double) -> double`
Returns the larger of two doubles.

### `math_clamp(val: double, min: double, max: double) -> double`
Clamps a value within the specified range.

## 3. Power and Roots

### `math_pow(base: double, exp: double) -> double`
Returns `base` raised to the power of `exp`.

### `math_sqrt(x: double) -> double`
Returns the square root of `x`.

### `math_hypot(x: double, y: double) -> double`
Returns `sqrt(x*x + y*y)`.

## 4. Trigonometric Functions

### `math_sin(x: double) -> double`
Returns the sine of `x` (radians).

### `math_cos(x: double) -> double`
Returns the cosine of `x` (radians).

### `math_tan(x: double) -> double`
Returns the tangent of `x` (radians).

## 5. Random Number Generation

Hoo random number generation uses a managed state object.

### `math_random_new() -> ptr`
Creates a new, auto-seeded random number generator state.

### `math_random_next_int(state: ptr) -> int64`
Returns a random 64-bit integer.

### `math_random_next_int_max(state: ptr, max: int64) -> int64`
Returns a random integer in the range `[0, max)`.

### `math_random_next_double(state: ptr) -> double`
Returns a random double in the range `[0, 1)`.

---

## Usage Example

```hoo
func :int64 main() {
    var radius = 5.0;
    var area = math_get_pi() * math_pow(radius, 2.0);
    
    println(string_concat("Area: ", string_from_double(area)));
    
    var rng = math_random_new();
    var roll = math_random_next_int_max(rng, 6) + 1;
    
    println(string_concat("Dice Roll: ", string_from_int64(roll)));
    
    return 0;
}
```
