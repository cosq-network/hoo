# Math API Reference (`Math`)

The `Math` class provides constants, basic functions, power/root operations, trigonometry, exponentials, rounding, number utilities, and random number generation via the `Random` class. `Math` is a singleton utility API, so call its methods directly on the class.

---

## 1. Constants

### `Math.getPi() :double`

Returns the value of π.

- **Parameters:** None
- **Returns:** `double` — approximate value 3.14159.

```hoo
func :void example() {
    var pi = Math.getPi();
    println(pi.toString());
}
```

---

### `Math.getE() :double`

Returns the value of e.

- **Parameters:** None
- **Returns:** `double` — approximate value 2.71828.

```hoo
func :void example() {
    var e = Math.getE();
    println(e.toString());
}
```

---

### `Math.getTau() :double`

Returns the value of τ (tau).

- **Parameters:** None
- **Returns:** `double` — approximate value 6.28318.

```hoo
func :void example() {
    var tau = Math.getTau();
    println(tau.toString());
}
```

---

### `Math.getInf() :double`

Returns positive infinity.

- **Parameters:** None
- **Returns:** `double` — positive infinity.

```hoo
func :void example() {
    var inf = Math.getInf();
    println(inf.toString());
}
```

---

### `Math.getNegInf() :double`

Returns negative infinity.

- **Parameters:** None
- **Returns:** `double` — negative infinity.

```hoo
func :void example() {
    var neg_inf = Math.getNegInf();
    println(neg_inf.toString());
}
```

---

### `Math.getNan() :double`

Returns Not-a-Number (NaN).

- **Parameters:** None
- **Returns:** `double` — NaN.

```hoo
func :void example() {
    var nan = Math.getNan();
    println(nan.toString());
}
```

---

## 2. Basic Functions

### `Math.abs(x: int64) :int64`

Returns the absolute value of an integer.

- **Parameters:**
  - `x: int64` — the input value.
- **Returns:** `int64` — the absolute value of `x`.

```hoo
func :int64 example() {
    return Math.abs(-42);
}
```

---

### `Math.abs(x: int8) :int8`

Returns the absolute value of an 8-bit signed integer.

- **Parameters:**
  - `x: int8` — the input value.
- **Returns:** `int8` — the absolute value of `x`.

---

### `Math.abs(x: byte) :byte`

Returns the absolute value of an 8-bit unsigned byte.

- **Parameters:**
  - `x: byte` — the input value.
- **Returns:** `byte` — the same value of `x`.

---

### `Math.abs(x: double) :double`

Returns the absolute value of a double.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the absolute value of `x`.

```hoo
func :double example() {
    return Math.abs(-3.14);
}
```

---

### `Math.abs(x: f8) :double`

Returns the absolute value of an f8 precision value (promoted to double).

- **Parameters:**
  - `x: f8` — the input value.
- **Returns:** `double` — the absolute value of `x`.

---

### `Math.sign(x: int64) :int64`

Returns the sign of an integer.

- **Parameters:**
  - `x: int64` — the input value.
- **Returns:** `int64` — `-1` if `x < 0`, `0` if `x == 0`, `1` if `x > 0`.

```hoo
func :int64 example() {
    return Math.sign(-7);
}
```

---

### `Math.sign(x: int8) :int8`

Returns the sign of an 8-bit signed integer.

- **Parameters:**
  - `x: int8` — the input value.
- **Returns:** `int8` — `-1` if `x < 0`, `0` if `x == 0`, `1` if `x > 0`.

---

### `Math.sign(x: byte) :byte`

Returns the sign of an 8-bit unsigned byte.

- **Parameters:**
  - `x: byte` — the input value.
- **Returns:** `byte` — `0` if `x == 0`, `1` if `x > 0`.

---

### `Math.sign(x: double) :double`

Returns the sign of a double.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — `-1.0` if `x < 0`, `0.0` if `x == 0`, `1.0` if `x > 0`.

```hoo
func :double example() {
    return Math.sign(-3.14);
}
```

---

### `Math.sign(x: f8) :double`

Returns the sign of an f8 precision value (promoted to double).

- **Parameters:**
  - `x: f8` — the input value.
- **Returns:** `double` — `-1.0` if `x < 0`, `0.0` if `x == 0`, `1.0` if `x > 0`.

---

### `Math.min(a: int64, b: int64) :int64`

Returns the smaller of two integers.

- **Parameters:**
  - `a: int64` — first value.
  - `b: int64` — second value.
- **Returns:** `int64` — the minimum of `a` and `b`.

```hoo
func :int64 example() {
    return Math.min(10, 20);
}
```

---

### `Math.min(a: int8, b: int8) :int8`

Returns the smaller of two 8-bit signed integers.

---

### `Math.min(a: byte, b: byte) :byte`

Returns the smaller of two 8-bit unsigned bytes.

---

### `Math.min(a: double, b: double) :double`

Returns the smaller of two doubles.

- **Parameters:**
  - `a: double` — first value.
  - `b: double` — second value.
- **Returns:** `double` — the minimum of `a` and `b`.

```hoo
func :double example() {
    return Math.min(3.5, 2.8);
}
```

---

### `Math.min(a: f8, b: f8) :double`

Returns the smaller of two f8 values.

---

### `Math.max(a: int64, b: int64) :int64`

Returns the larger of two integers.

- **Parameters:**
  - `a: int64` — first value.
  - `b: int64` — second value.
- **Returns:** `int64` — the maximum of `a` and `b`.

```hoo
func :int64 example() {
    return Math.max(10, 20);
}
```

---

### `Math.max(a: int8, b: int8) :int8`

Returns the larger of two 8-bit signed integers.

---

### `Math.max(a: byte, b: byte) :byte`

Returns the larger of two 8-bit unsigned bytes.

---

### `Math.max(a: double, b: double) :double`

Returns the larger of two doubles.

- **Parameters:**
  - `a: double` — first value.
  - `b: double` — second value.
- **Returns:** `double` — the maximum of `a` and `b`.

```hoo
func :double example() {
    return Math.max(3.5, 2.8);
}
```

---

### `Math.max(a: f8, b: f8) :double`

Returns the larger of two f8 values.

---

### `Math.clamp(val: double, min: double, max: double) :double`

Clamps a value within the inclusive range `[min, max]`.

- **Parameters:**
  - `val: double` — the value to clamp.
  - `min: double` — the lower bound.
  - `max: double` — the upper bound.
- **Returns:** `double` — `val` if within range, `min` if `val < min`, `max` if `val > max`.

```hoo
func :double example() {
    return Math.clamp(15.0, 0.0, 10.0);
}
```

---

## 3. Power and Roots

### `Math.pow(base: double, exp: double) :double`

Returns `base` raised to the power of `exp`.

- **Parameters:**
  - `base: double` — the base.
  - `exp: double` — the exponent.
- **Returns:** `double` — `base` raised to `exp`.

```hoo
func :double example() {
    return Math.pow(2.0, 3.0);
}
```

---

### `Math.sqrt(x: double) :double`

Returns the square root of `x`.

- **Parameters:**
  - `x: double` — a non-negative number.
- **Returns:** `double` — the square root of `x`.

```hoo
func :double example() {
    return Math.sqrt(9.0);
}
```

---

### `Math.cbrt(x: double) :double`

Returns the cube root of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the cube root of `x`.

```hoo
func :double example() {
    return Math.cbrt(27.0);
}
```

---

### `Math.hypot(x: double, y: double) :double`

Returns `sqrt(x * x + y * y)` without unnecessary overflow or underflow.

- **Parameters:**
  - `x: double` — first leg.
  - `y: double` — second leg.
- **Returns:** `double` — the length of the hypotenuse.

```hoo
func :double example() {
    return Math.hypot(3.0, 4.0);
}
```

---

## 4. Trigonometric Functions

### `Math.sin(x: double) :double`

Returns the sine of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the sine of `x`.

```hoo
func :double example() {
    return Math.sin(0.0);
}
```

---

### `Math.cos(x: double) :double`

Returns the cosine of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the cosine of `x`.

```hoo
func :double example() {
    return Math.cos(0.0);
}
```

---

### `Math.tan(x: double) :double`

Returns the tangent of `x`.

- **Parameters:**
  - `x: double` — angle in radians.
- **Returns:** `double` — the tangent of `x`.

```hoo
func :double example() {
    return Math.tan(0.0);
}
```

---

### `Math.asin(x: double) :double`

Returns the arc sine of `x` in radians.

- **Parameters:**
  - `x: double` — value in the range `[-1, 1]`.
- **Returns:** `double` — the arc sine in `[-π/2, π/2]`.

```hoo
func :double example() {
    return Math.asin(1.0);
}
```

---

### `Math.acos(x: double) :double`

Returns the arc cosine of `x` in radians.

- **Parameters:**
  - `x: double` — value in the range `[-1, 1]`.
- **Returns:** `double` — the arc cosine in `[0, π]`.

```hoo
func :double example() {
    return Math.acos(0.0);
}
```

---

### `Math.atan(x: double) :double`

Returns the arc tangent of `x` in radians.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the arc tangent in `[-π/2, π/2]`.

```hoo
func :double example() {
    return Math.atan(1.0);
}
```

---

### `Math.atan2(y: double, x: double) :double`

Returns the arc tangent of `y / x` using the signs of both to determine the quadrant.

- **Parameters:**
  - `y: double` — the y-coordinate.
  - `x: double` — the x-coordinate.
- **Returns:** `double` — the angle in `[-π, π]`.

```hoo
func :double example() {
    return Math.atan2(1.0, 0.0);
}
```

---

### `Math.sinh(x: double) :double`

Returns the hyperbolic sine of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic sine of `x`.

```hoo
func :double example() {
    return Math.sinh(0.0);
}
```

---

### `Math.cosh(x: double) :double`

Returns the hyperbolic cosine of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic cosine of `x`.

```hoo
func :double example() {
    return Math.cosh(0.0);
}
```

---

### `Math.tanh(x: double) :double`

Returns the hyperbolic tangent of `x`.

- **Parameters:**
  - `x: double` — any number.
- **Returns:** `double` — the hyperbolic tangent of `x`.

```hoo
func :double example() {
    return Math.tanh(0.5);
}
```

---

## 5. Exponential and Logarithmic

### `Math.exp(x: double) :double`

Returns `e` raised to the power of `x`.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `e^x`.

```hoo
func :double example() {
    return Math.exp(1.0);
}
```

---

### `Math.exp2(x: double) :double`

Returns 2 raised to the power of `x`.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `2^x`.

```hoo
func :double example() {
    return Math.exp2(3.0);
}
```

---

### `Math.expm1(x: double) :double`

Returns `e^x - 1` accurately even when `x` is near zero.

- **Parameters:**
  - `x: double` — the exponent.
- **Returns:** `double` — `e^x - 1`.

```hoo
func :double example() {
    return Math.expm1(0.001);
}
```

---

### `Math.log(x: double) :double`

Returns the natural logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the natural logarithm of `x`.

```hoo
func :double example() {
    return Math.log(2.71828);
}
```

---

### `Math.log10(x: double) :double`

Returns the base-10 logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the base-10 logarithm of `x`.

```hoo
func :double example() {
    return Math.log10(100.0);
}
```

---

### `Math.log2(x: double) :double`

Returns the base-2 logarithm of `x`.

- **Parameters:**
  - `x: double` — a positive number.
- **Returns:** `double` — the base-2 logarithm of `x`.

```hoo
func :double example() {
    return Math.log2(8.0);
}
```

---

### `Math.log1p(x: double) :double`

Returns the natural logarithm of `1 + x` accurately even when `x` is near zero.

- **Parameters:**
  - `x: double` — a number greater than `-1`.
- **Returns:** `double` — `ln(1 + x)`.

```hoo
func :double example() {
    return Math.log1p(0.001);
}
```

---

## 6. Rounding Functions

### `Math.floor(x: double) :double`

Returns the largest integer less than or equal to `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the floor of `x`.

```hoo
func :double example() {
    return Math.floor(3.7);
}
```

---

### `Math.ceil(x: double) :double`

Returns the smallest integer greater than or equal to `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the ceiling of `x`.

```hoo
func :double example() {
    return Math.ceil(3.2);
}
```

---

### `Math.round(x: double) :double`

Returns the nearest integer to `x`, rounding half away from zero.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the rounded value.

```hoo
func :double example() {
    return Math.round(3.5);
}
```

---

### `Math.trunc(x: double) :double`

Returns the integer part of `x`, discarding the fractional part (truncates toward zero).

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the truncated value.

```hoo
func :double example() {
    return Math.trunc(3.7);
}
```

---

### `Math.fract(x: double) :double`

Returns the fractional part of `x`.

- **Parameters:**
  - `x: double` — the input value.
- **Returns:** `double` — the fractional part of `x` (same sign as `x`).

```hoo
func :double example() {
    return Math.fract(3.7);
}
```

---

## 7. Number Utilities

### `Math.isEven(n: int64) :int64`

Returns 1 if `n` is even, 0 otherwise.

- **Parameters:**
  - `n: int64` — the number to test.
- **Returns:** `int64` — `1` if even, `0` if odd.

```hoo
func :int64 example() {
    return Math.isEven(42);
}
```

---

### `Math.isOdd(n: int64) :int64`

Returns 1 if `n` is odd, 0 otherwise.

- **Parameters:**
  - `n: int64` — the number to test.
- **Returns:** `int64` — `1` if odd, `0` if even.

```hoo
func :int64 example() {
    return Math.isOdd(43);
}
```

---

### `Math.isPrime(n: int64) :int64`

Returns 1 if `n` is prime, 0 otherwise.

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — `1` if prime, `0` otherwise.

```hoo
func :int64 example() {
    return Math.isPrime(17);
}
```

---

### `Math.gcd(a: int64, b: int64) :int64`

Returns the greatest common divisor of `a` and `b`.

- **Parameters:**
  - `a: int64` — first integer.
  - `b: int64` — second integer.
- **Returns:** `int64` — the GCD of `a` and `b`.

```hoo
func :int64 example() {
    return Math.gcd(12, 18);
}
```

---

### `Math.lcm(a: int64, b: int64) :int64`

Returns the least common multiple of `a` and `b`.

- **Parameters:**
  - `a: int64` — first integer.
  - `b: int64` — second integer.
- **Returns:** `int64` — the LCM of `a` and `b`.

```hoo
func :int64 example() {
    return Math.lcm(4, 6);
}
```

---

### `Math.factorial(n: int64) :int64`

Returns `n!` (n factorial), the product of all positive integers up to `n`.

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — the factorial of `n`.

```hoo
func :int64 example() {
    return Math.factorial(5);
}
```

---

### `Math.fibonacci(n: int64) :int64`

Returns the `n`-th Fibonacci number (`F(0) = 0`, `F(1) = 1`).

- **Parameters:**
  - `n: int64` — a non-negative integer.
- **Returns:** `int64` — the `n`-th Fibonacci number.

```hoo
func :int64 example() {
    return Math.fibonacci(10);
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
func :void example() {
    var rng = new Random(42);
    rng.release();
}
```

---

## Usage Example

```hoo
func :int64 main() {
    var radius = 5.0;
    var area = Math.getPi() * Math.pow(radius, 2.0);

    if (Math.isEven(Math.round(area).toInt64())) {
        println("Rounded area is even.");
    }

    var rng = new Random(12345);
    var roll = rng.nextIntMax(6) + 1;
    rng.release();
    println("You rolled a ".concat(roll.toString()));

    return 0;
}
```
