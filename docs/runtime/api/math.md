# Math API Reference (`hoo.math`)

**Import Requirement:**
```hoo
import hoo.math;
```

The `hoo.math` module provides constants, basic functions, power/root operations, trigonometry, exponentials, rounding, number utilities, and random number generation via the `Random` class. All math functions are free functions in the `hoo.math` module.

---

## 1. Constants

### `math_get_pi() :double`

Returns the value of π.

- **Parameters:** None
- **Returns:** `double` — approximate value 3.14159.

```hoo
import hoo.math;

func :void example() {
    var pi = math_get_pi();
    println(pi.toString());
}
```

---

### `math_get_e() :double`

Returns the value of e.

- **Parameters:** None
- **Returns:** `double` — approximate value 2.71828.

```hoo
import hoo.math;

func :void example() {
    var e = math_get_e();
    println(e.toString());
}
```

---

### `math_get_tau() :double`

Returns the value of τ (tau).

- **Parameters:** None
- **Returns:** `double` — approximate value 6.28318.

```hoo
import hoo.math;

func :void example() {
    var tau = math_get_tau();
    println(tau.toString());
}
```

---

### `math_get_inf() :double`

Returns positive infinity.

- **Parameters:** None
- **Returns:** `double` — positive infinity.

```hoo
import hoo.math;

func :void example() {
    var inf = math_get_inf();
    println(inf.toString());
}
```

---

### `math_get_neg_inf() :double`

Returns negative infinity.

- **Parameters:** None
- **Returns:** `double` — negative infinity.

```hoo
import hoo.math;

func :void example() {
    var neg_inf = math_get_neg_inf();
    println(neg_inf.toString());
}
```

---

### `math_get_nan() :double`

Returns Not-a-Number (NaN).

- **Parameters:** None
- **Returns:** `double` — NaN.

```hoo
import hoo.math;

func :void example() {
    var nan = math_get_nan();
    println(nan.toString());
}
```

---

## 2. Basic Functions

### `math_abs(x: int64) :int64`

Returns the absolute value of an integer.

- **Parameters:**
  - `x: int64` — the input value.
- **Returns:** `int64` — the absolute value of `x`.

```hoo
import hoo.math;

func :int64 example() {
    return math_abs(-42);
}
```

---

### `math_abs(x: int8) :int8`

Returns the absolute value of an 8-bit signed integer.

- **Parameters:**
  - `x: int8` — the input value.
- **Returns:** `int8` — the absolute value of `x`.

---

### `math_abs(x: byte) :byte`

Returns the absolute value of an 8-bit unsigned byte.

- **Parameters:**
  - `x: byte` — the input value.
- **Returns:** `byte` — the same value of `x`.

---

### `math_abs(x: double) :double`

Returns the absolute value of a double.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the absolute value of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_abs(-3.14);
}
```

---

### `math_abs(x: f8) :double`

Returns the absolute value of an f8 precision value (promoted to double).

- **Parameters:**
  - `x: f8` — the input value.
- **Returns:** `double` — the absolute value of `x`.

---

### `math_sign(x: int64) :int64`

Returns the sign of an integer.

- **Parameters:**
  - `x: int64` — the input value.
- **Returns:** `int64` — `-1` if `x < 0`, `0` if `x == 0`, `1` if `x > 0`.

```hoo
import hoo.math;

func :int64 example() {
    return math_sign(-7);
}
```

---

### `math_sign(x: int8) :int8`

Returns the sign of an 8-bit signed integer.

- **Parameters:**
  - `x: int8` — the input value.
- **Returns:** `int8` — `-1` if `x < 0`, `0` if `x == 0`, `1` if `x > 0`.

---

### `math_sign(x: byte) :byte`

Returns the sign of an 8-bit unsigned byte.

- **Parameters:**
  - `x: byte` — the input value.
- **Returns:** `byte` — `0` if `x == 0`, `1` if `x > 0`.

---

### `math_sign(x: double) :double`

Returns the sign of a double.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — `-1.0` if `x < 0`, `0.0` if `x == 0`, `1.0` if `x > 0`.

```hoo
import hoo.math;

func :double example() {
    return math_sign(-3.14);
}
```

---

### `math_sign(x: f8) :double`

Returns the sign of an f8 precision value (promoted to double).

- **Parameters:**
  - `x: f8` — the input value.
- **Returns:** `double` — `-1.0` if `x < 0`, `0.0` if `x == 0`, `1.0` if `x > 0`.

---

### `math_min(a: int64, b: int64) :int64`

Returns the smaller of two integers.

- **Parameters:**
  - `a: int64` — first value.
  - `b: int64` — second value.
- **Returns:** `int64` — the minimum of `a` and `b`.

```hoo
import hoo.math;

func :int64 example() {
    return math_min(10, 20);
}
```

---

### `math_min(a: int8, b: int8) :int8`

Returns the smaller of two 8-bit signed integers.

---

### `math_min(a: byte, b: byte) :byte`

Returns the smaller of two 8-bit unsigned bytes.

---

### `math_min(a: double, b: double) :double`

Returns the smaller of two doubles.

- **Parameters:**
  - `a: double` — first value.
  - `b: double` — second value.
- **Returns:** `double` — the minimum of `a` and `b`.

```hoo
import hoo.math;

func :double example() {
    return math_min(3.5, 2.8);
}
```

---

### `math_min(a: f8, b: f8) :double`

Returns the smaller of two f8 values.

---

### `math_max(a: int64, b: int64) :int64`

Returns the larger of two integers.

- **Parameters:**
  - `a: int64` — first value.
  - `b: int64` — second value.
- **Returns:** `int64` — the maximum of `a` and `b`.

```hoo
import hoo.math;

func :int64 example() {
    return math_max(10, 20);
}
```

---

### `math_max(a: int8, b: int8) :int8`

Returns the larger of two 8-bit signed integers.

---

### `math_max(a: byte, b: byte) :byte`

Returns the larger of two 8-bit unsigned bytes.

---

### `math_max(a: double, b: double) :double`

Returns the larger of two doubles.

- **Parameters:**
  - `a: double` — first value.
  - `b: double` — second value.
- **Returns:** `double` — the maximum of `a` and `b`.

```hoo
import hoo.math;

func :double example() {
    return math_max(3.5, 2.8);
}
```

---

### `math_max(a: f8, b: f8) :double`

Returns the larger of two f8 values.

---

### `math_clamp(val: double, min: double, max: double) :double`

Clamps a value within the inclusive range `[min, max]`.

- **Parameters:**
  - `val: double` — the value to clamp.
  - `min: double` — the lower bound.
  - `max: double` — the upper bound.
- **Returns:** `double` — `val` if within range, `min` if `val < min`, `max` if `val > max`.

```hoo
import hoo.math;

func :double example() {
    return math_clamp(15.0, 0.0, 10.0);
}
```

---

## 3. Power and Roots

### `math_pow(base: double, exp: double) :double`

Returns `base` raised to the power of `exp`.

- **Parameters:**
  - `base: double` — the base.
  - `exp: double` — the exponent.
- **Returns:** `double` — `base` raised to `exp`.

```hoo
import hoo.math;

func :double example() {
    return math_pow(2.0, 3.0);
}
```

---

### `math_sqrt(x: double) :double`

Returns the square root of `x`.

- **Parameters:**
  - `x: double` — a non-negative number.
- **Returns:** `double` — the square root of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_sqrt(9.0);
}
```

---

### `math_cbrt(x: double) :double`

Returns the cube root of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the cube root of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_cbrt(27.0);
}
```

---

### `math_hypot(x: double, y: double) :double`

Returns `sqrt(x * x + y * y)` without unnecessary overflow or underflow.

- **Parameters:**
  - `x: double` — first leg.
  - `y: double` — second leg.
- **Returns:** `double` — the length of the hypotenuse.

```hoo
import hoo.math;

func :double example() {
    return math_hypot(3.0, 4.0);
}
```

---

## 4. Trigonometric Functions

### `math_sin(x: double) :double`

Returns the sine of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the sine of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_sin(0.0);
}
```

---

### `math_cos(x: double) :double`

Returns the cosine of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the cosine of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_cos(0.0);
}
```

---

### `math_tan(x: double) :double`

Returns the tangent of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the tangent of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_tan(0.0);
}
```

---

### `math_asin(x: double) :double`

Returns the arc sine of `x` in radians.

- **Parameters:**
  - `x: double` — value in the range `[-1, 1]`.
- **Returns:** `double` — the arc sine in `[-π/2, π/2]`.

```hoo
import hoo.math;

func :double example() {
    return math_asin(1.0);
}
```

---

### `math_acos(x: double) :double`

Returns the arc cosine of `x` in radians.

- **Parameters:**
  - `x: double` — value in the range `[-1, 1]`.
- **Returns:** `double` — the arc cosine in `[0, π]`.

```hoo
import hoo.math;

func :double example() {
    return math_acos(0.0);
}
```

---

### `math_atan(x: double) :double`

Returns the arc tangent of `x` in radians.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the arc tangent in `[-π/2, π/2]`.

```hoo
import hoo.math;

func :double example() {
    return math_atan(1.0);
}
```

---

### `math_atan2(y: double, x: double) :double`

Returns the arc tangent of `y / x` using the signs of both to determine the quadrant.

- **Parameters:**
  - `y: double` — the y-coordinate.
  - `x: double` — the x-coordinate.
- **Returns:** `double` — the angle in `[-π, π]`.

```hoo
import hoo.math;

func :double example() {
    return math_atan2(1.0, 0.0);
}
```

---

### `math_sinh(x: double) :double`

Returns the hyperbolic sine of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic sine of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_sinh(0.0);
}
```

---

### `math_cosh(x: double) :double`

Returns the hyperbolic cosine of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic cosine of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_cosh(0.0);
}
```

---

### `math_tanh(x: double) :double`

Returns the hyperbolic tangent of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic tangent of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_tanh(0.5);
}
```

---

## 5. Exponential and Logarithmic

### `math_exp(x: double) :double`

Returns `e` raised to the power of `x`.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `e^x`.

```hoo
import hoo.math;

func :double example() {
    return math_exp(1.0);
}
```

---

### `math_exp2(x: double) :double`

Returns 2 raised to the power of `x`.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `2^x`.

```hoo
import hoo.math;

func :double example() {
    return math_exp2(3.0);
}
```

---

### `math_expm1(x: double) :double`

Returns `e^x - 1` accurately even when `x` is near zero.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `e^x - 1`.

```hoo
import hoo.math;

func :double example() {
    return math_expm1(0.001);
}
```

---

### `math_log(x: double) :double`

Returns the natural logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the natural logarithm of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_log(2.71828);
}
```

---

### `math_log10(x: double) :double`

Returns the base-10 logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the base-10 logarithm of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_log10(100.0);
}
```

---

### `math_log2(x: double) :double`

Returns the base-2 logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the base-2 logarithm of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_log2(8.0);
}
```

---

### `math_log1p(x: double) :double`

Returns the natural logarithm of `1 + x` accurately even when `x` is near zero.

- **Parameters:**
  - `x: double` — a number greater than `-1`.
- **Returns:** `double` — `ln(1 + x)`.

```hoo
import hoo.math;

func :double example() {
    return math_log1p(0.001);
}
```

---

## 6. Rounding Functions

### `math_floor(x: double) :double`

Returns the largest integer less than or equal to `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the floor of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_floor(3.7);
}
```

---

### `math_ceil(x: double) :double`

Returns the smallest integer greater than or equal to `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the ceiling of `x`.

```hoo
import hoo.math;

func :double example() {
    return math_ceil(3.2);
}
```

---

### `math_round(x: double) :double`

Returns the nearest integer to `x`, rounding half away from zero.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the rounded value.

```hoo
import hoo.math;

func :double example() {
    return math_round(3.5);
}
```

---

### `math_trunc(x: double) :double`

Returns the integer part of `x`, discarding the fractional part (truncates toward zero).

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the truncated value.

```hoo
import hoo.math;

func :double example() {
    return math_trunc(3.7);
}
```

---

### `math_fract(x: double) :double`

Returns the fractional part of `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the fractional part of `x` (same sign as `x`).

```hoo
import hoo.math;

func :double example() {
    return math_fract(3.7);
}
```

---

## 7. Number Utilities

### `math_is_even(n: int64) :int64`

Returns 1 if `n` is even, 0 otherwise.

- **Parameters:**
  - `n: int64` — the number to test.
- **Returns:** `int64` — `1` if even, `0` if odd.

```hoo
import hoo.math;

func :int64 example() {
    return math_is_even(42);
}
```

---

### `math_is_odd(n: int64) :int64`

Returns 1 if `n` is odd, 0 otherwise.

- **Parameters:**
  - `n: int64` — the number to test.
- **Returns:** `int64` — `1` if odd, `0` if even.

```hoo
import hoo.math;

func :int64 example() {
    return math_is_odd(43);
}
```

---

### `math_is_prime(n: int64) :int64`

Returns 1 if `n` is prime, 0 otherwise.

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — `1` if prime, `0` otherwise.

```hoo
import hoo.math;

func :int64 example() {
    return math_is_prime(17);
}
```

---

### `math_gcd(a: int64, b: int64) :int64`

Returns the greatest common divisor of `a` and `b`.

- **Parameters:**
  - `a: int64` — first integer.
  - `b: int64` — second integer.
- **Returns:** `int64` — the GCD of `a` and `b`.

```hoo
import hoo.math;

func :int64 example() {
    return math_gcd(12, 18);
}
```

---

### `math_lcm(a: int64, b: int64) :int64`

Returns the least common multiple of `a` and `b`.

- **Parameters:**
  - `a: int64` — first integer.
  - `b: int64` — second integer.
- **Returns:** `int64` — the LCM of `a` and `b`.

```hoo
import hoo.math;

func :int64 example() {
    return math_lcm(4, 6);
}
```

---

### `math_factorial(n: int64) :int64`

Returns `n!` (n factorial), the product of all positive integers up to `n`.

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — the factorial of `n`.

```hoo
import hoo.math;

func :int64 example() {
    return math_factorial(5);
}
```

---

### `math_fibonacci(n: int64) :int64`

Returns the `n`-th Fibonacci number (`F(0) = 0`, `F(1) = 1`).

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — the `n`-th Fibonacci number.

```hoo
import hoo.math;

func :int64 example() {
    return math_fibonacci(10);
}
```

---

## 8. Random Number Generation (`Random`)

Hoo random number generation uses a managed state object via the `Random` class. Create instances with the `new` keyword, then call methods on the instance.

### `new Random()`

Creates a new, auto-seeded random number generator.

- **Parameters:** None
- **Returns:** a `Random` instance with a random seed.

```hoo
import hoo.math;

func :int64 example() {
    var rng = new Random();
    var value = rng.nextInt();
    rng.release();
    return value;
}
```

---

### `new Random(seed: int64)`

Creates a new random number generator seeded with `seed`. Two generators created with the same seed produce identical sequences.

- **Parameters:**
  - `seed: int64` — the seed value.
- **Returns:** a `Random` instance with the given seed.

```hoo
import hoo.math;

func :int64 example() {
    var rng = new Random(42);
    var value = rng.nextIntMax(100);
    rng.release();
    return value;
}
```

---

### `state.nextInt() :int64`

Returns a random 64-bit integer from the full `int64` range.

- **Parameters:** None
- **Returns:** `int64` — a uniformly distributed random integer.

```hoo
import hoo.math;

func :int64 example() {
    var rng = new Random();
    var value = rng.nextInt();
    rng.release();
    return value;
}
```

---

### `state.nextIntMax(max: int64) :int64`

Returns a random integer uniformly distributed in `[0, max)`.

- **Parameters:**
  - `max: int64` — the exclusive upper bound.
- **Returns:** `int64` — a random integer in `[0, max)`.

```hoo
import hoo.math;

func :int64 example() {
    var rng = new Random();
    var roll = rng.nextIntMax(6) + 1;
    rng.release();
    return roll;
}
```

---

### `state.nextDouble() :double`

Returns a random double uniformly distributed in `[0, 1)`.

- **Parameters:** None
- **Returns:** `double` — a random value in `[0, 1)`.

```hoo
import hoo.math;

func :double example() {
    var rng = new Random();
    var value = rng.nextDouble();
    rng.release();
    return value;
}
```

---

### `state.nextBool() :bool`

Returns `true` or `false` with equal probability.

- **Parameters:** None
- **Returns:** `bool` — random boolean value.

```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    if (rng.nextBool()) {
        println("Heads");
    } else {
        println("Tails");
    }
    rng.release();
}
```

---

### `state.nextBytes(buffer: Buffer, count: int64) :int64`

Fills the provided buffer with random bytes.

- **Parameters:**
  - `buffer: Buffer` — the destination buffer.
  - `count: int64` — the number of bytes to write.
- **Returns:** `int64` — the number of bytes successfully written.

```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    var buf = new Buffer();
    rng.nextBytes(buf, 10);
    rng.release();
}
```

---

### `state.release() :void`

Releases a random generator handle.

- **Parameters:** None
- **Returns:** Nothing.

```hoo
import hoo.math;

func :void example() {
    var rng = new Random(42);
    rng.release();
}
```

---

## Usage Example

```hoo
import hoo.math;

func :int64 main() {
    var radius = 5.0;
    var area = math_get_pi() * math_pow(radius, 2.0);

    if (math_is_even(math_round(area).toInt64())) {
        println("Rounded area is even.");
    }

    var rng = new Random(12345);
    var roll = rng.nextIntMax(6) + 1;
    rng.release();
    println("You rolled a ".concat(roll.toString()));

    return 0;
}
```
