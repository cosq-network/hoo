# Math API Reference

## Module

`hoo.math`

## Import Statement

```hoo
import hoo.math;
```

## Module Description

The `hoo.math` module provides double-precision mathematical functions (abs, min, max, clamp, floor, ceil, round, sqrt, pow, cbrt, log, log10, log2, exp, exp2, sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh, asinh, acosh, atanh, mean, median, variance, stddev), integer overloads for min and max, and a `Random` class for pseudo-random number generation. All free functions operate on `double` values unless otherwise noted.

## Constants

### math_get_pi() :double

Returns the value of &pi;.

- **Returns:** `double` — 3.141592653589793.

```hoo
var pi = math_get_pi();
```

### math_get_e() :double

Returns the value of e.

- **Returns:** `double` — 2.718281828459045.

```hoo
var e = math_get_e();
```

### math_get_tau() :double

Returns the value of &tau;.

- **Returns:** `double` — 6.283185307179586.

```hoo
var tau = math_get_tau();
```

### math_get_inf() :double

Returns positive infinity.

- **Returns:** `double` — +Infinity.

```hoo
var inf = math_get_inf();
```

### math_get_neg_inf() :double

Returns negative infinity.

- **Returns:** `double` — -Infinity.

```hoo
var neg_inf = math_get_neg_inf();
```

### math_get_nan() :double

Returns Not-a-Number (NaN).

- **Returns:** `double` — NaN.

```hoo
var nan = math_get_nan();
```

## Free Functions

### abs(x: double) :double

Returns the absolute value of `x`.

**Syntax:**
```hoo
abs(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Input value |

- **Returns:** `double` — the absolute value of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = abs(-3.14);
    println(v);  // 3.14
}
```

### min(x: double, y: double) :double

Returns the smaller of two doubles.

**Syntax:**
```hoo
min(x: double, y: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | First value |
| `y` | `double` | Second value |

- **Returns:** `double` — the minimum of `x` and `y`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = min(3.5, 2.8);
    println(v);  // 2.8
}
```

### max(x: double, y: double) :double

Returns the larger of two doubles.

**Syntax:**
```hoo
max(x: double, y: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | First value |
| `y` | `double` | Second value |

- **Returns:** `double` — the maximum of `x` and `y`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = max(3.5, 2.8);
    println(v);  // 3.5
}
```

### clamp(val: double, min_val: double, max_val: double) :double

Clamps `val` within the inclusive range `[min_val, max_val]`.

**Syntax:**
```hoo
clamp(val: double, min_val: double, max_val: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `val` | `double` | Value to clamp |
| `min_val` | `double` | Lower bound |
| `max_val` | `double` | Upper bound |

- **Returns:** `double` — `val` if in range, `min_val` if `val < min_val`, `max_val` if `val > max_val`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = clamp(15.0, 0.0, 10.0);
    println(v);  // 10.0
}
```

### min_int64(a: int64, b: int64) :int64

Returns the smaller of two 64-bit integers.

**Syntax:**
```hoo
min_int64(a: int64, b: int64) :int64
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `a` | `int64` | First value |
| `b` | `int64` | Second value |

- **Returns:** `int64` — the minimum of `a` and `b`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = min_int64(10, 20);
    println(v);  // 10
}
```

### max_int64(a: int64, b: int64) :int64

Returns the larger of two 64-bit integers.

**Syntax:**
```hoo
max_int64(a: int64, b: int64) :int64
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `a` | `int64` | First value |
| `b` | `int64` | Second value |

- **Returns:** `int64` — the maximum of `a` and `b`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = max_int64(10, 20);
    println(v);  // 20
}
```

### floor(x: double) :double

Returns the largest integer less than or equal to `x`.

**Syntax:**
```hoo
floor(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Input value |

- **Returns:** `double` — the floor of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = floor(3.7);
    println(v);  // 3.0
}
```

### ceil(x: double) :double

Returns the smallest integer greater than or equal to `x`.

**Syntax:**
```hoo
ceil(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Input value |

- **Returns:** `double` — the ceiling of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = ceil(3.2);
    println(v);  // 4.0
}
```

### round(x: double) :double

Returns the nearest integer to `x`, rounding half away from zero.

**Syntax:**
```hoo
round(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Input value |

- **Returns:** `double` — the rounded value.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = round(3.5);
    println(v);  // 4.0
}
```

### sqrt(x: double) :double

Returns the square root of `x`.

**Syntax:**
```hoo
sqrt(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Non-negative input |

- **Returns:** `double` — the square root of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = sqrt(9.0);
    println(v);  // 3.0
}
```

### pow(base: double, exp: double) :double

Returns `base` raised to the power of `exp`.

**Syntax:**
```hoo
pow(base: double, exp: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `base` | `double` | The base |
| `exp` | `double` | The exponent |

- **Returns:** `double` — `base^exp`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = pow(2.0, 3.0);
    println(v);  // 8.0
}
```

### cbrt(x: double) :double

Returns the cube root of `x`.

**Syntax:**
```hoo
cbrt(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Input value |

- **Returns:** `double` — the cube root of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = cbrt(27.0);
    println(v);  // 3.0
}
```

### log(x: double) :double

Returns the natural logarithm (base e) of `x`.

**Syntax:**
```hoo
log(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Positive input |

- **Returns:** `double` — the natural logarithm of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = log(2.71828);
    println(v);  // ~1.0
}
```

### log10(x: double) :double

Returns the base-10 logarithm of `x`.

**Syntax:**
```hoo
log10(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Positive input |

- **Returns:** `double` — the base-10 logarithm of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = log10(100.0);
    println(v);  // 2.0
}
```

### log2(x: double) :double

Returns the base-2 logarithm of `x`.

**Syntax:**
```hoo
log2(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Positive input |

- **Returns:** `double` — the base-2 logarithm of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = log2(8.0);
    println(v);  // 3.0
}
```

### exp(x: double) :double

Returns e raised to the power of `x`.

**Syntax:**
```hoo
exp(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | The exponent |

- **Returns:** `double` — `e^x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = exp(1.0);
    println(v);  // ~2.71828
}
```

### exp2(x: double) :double

Returns 2 raised to the power of `x`.

**Syntax:**
```hoo
exp2(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | The exponent |

- **Returns:** `double` — `2^x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = exp2(3.0);
    println(v);  // 8.0
}
```

### sin(x: double) :double

Returns the sine of `x`.

**Syntax:**
```hoo
sin(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Angle in radians |

- **Returns:** `double` — the sine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = sin(0.0);
    println(v);  // 0.0
}
```

### cos(x: double) :double

Returns the cosine of `x`.

**Syntax:**
```hoo
cos(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Angle in radians |

- **Returns:** `double` — the cosine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = cos(0.0);
    println(v);  // 1.0
}
```

### tan(x: double) :double

Returns the tangent of `x`.

**Syntax:**
```hoo
tan(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Angle in radians |

- **Returns:** `double` — the tangent of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = tan(0.0);
    println(v);  // 0.0
}
```

### asin(x: double) :double

Returns the arc sine of `x` in radians.

**Syntax:**
```hoo
asin(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Value in [-1, 1] |

- **Returns:** `double` — the arc sine in [-&pi;/2, &pi;/2].
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = asin(1.0);
    println(v);  // ~1.5708
}
```

### acos(x: double) :double

Returns the arc cosine of `x` in radians.

**Syntax:**
```hoo
acos(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Value in [-1, 1] |

- **Returns:** `double` — the arc cosine in [0, &pi;].
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = acos(0.0);
    println(v);  // ~1.5708
}
```

### atan(x: double) :double

Returns the arc tangent of `x` in radians.

**Syntax:**
```hoo
atan(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Any number |

- **Returns:** `double` — the arc tangent in [-&pi;/2, &pi;/2].
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = atan(1.0);
    println(v);  // ~0.7854
}
```

### atan2(y: double, x: double) :double

Returns the arc tangent of `y/x`, using the signs of both arguments to determine the quadrant.

**Syntax:**
```hoo
atan2(y: double, x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `y` | `double` | y-coordinate |
| `x` | `double` | x-coordinate |

- **Returns:** `double` — the angle in [-&pi;, &pi;].
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = atan2(1.0, 0.0);
    println(v);  // ~1.5708
}
```

### sinh(x: double) :double

Returns the hyperbolic sine of `x`.

**Syntax:**
```hoo
sinh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Any number |

- **Returns:** `double` — the hyperbolic sine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = sinh(0.0);
    println(v);  // 0.0
}
```

### cosh(x: double) :double

Returns the hyperbolic cosine of `x`.

**Syntax:**
```hoo
cosh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Any number |

- **Returns:** `double` — the hyperbolic cosine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = cosh(0.0);
    println(v);  // 1.0
}
```

### tanh(x: double) :double

Returns the hyperbolic tangent of `x`.

**Syntax:**
```hoo
tanh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Any number |

- **Returns:** `double` — the hyperbolic tangent of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = tanh(0.5);
    println(v);  // ~0.4621
}
```

### asinh(x: double) :double

Returns the inverse hyperbolic sine of `x`.

**Syntax:**
```hoo
asinh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | Any number |

- **Returns:** `double` — the inverse hyperbolic sine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = asinh(1.0);
    println(v);  // ~0.8814
}
```

### acosh(x: double) :double

Returns the inverse hyperbolic cosine of `x`.

**Syntax:**
```hoo
acosh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | x >= 1 |

- **Returns:** `double` — the inverse hyperbolic cosine of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = acosh(2.0);
    println(v);  // ~1.3170
}
```

### atanh(x: double) :double

Returns the inverse hyperbolic tangent of `x`.

**Syntax:**
```hoo
atanh(x: double) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `x` | `double` | x in (-1, 1) |

- **Returns:** `double` — the inverse hyperbolic tangent of `x`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var v = atanh(0.5);
    println(v);  // ~0.5493
}
```

### mean(values: double[]) :double

Returns the arithmetic mean of a double array.

**Syntax:**
```hoo
mean(values: double[]) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `values` | `double[]` | Array of values |

- **Returns:** `double` — the arithmetic mean.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var data = [1.0, 2.0, 3.0, 4.0, 5.0];
    var m = mean(data);
    println(m);  // 3.0
}
```

### median(values: double[]) :double

Returns the median of a double array.

**Syntax:**
```hoo
median(values: double[]) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `values` | `double[]` | Array of values |

- **Returns:** `double` — the median value.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var data = [1.0, 2.0, 3.0, 4.0, 5.0];
    var m = median(data);
    println(m);  // 3.0
}
```

### variance(values: double[]) :double

Returns the population variance of a double array.

**Syntax:**
```hoo
variance(values: double[]) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `values` | `double[]` | Array of values |

- **Returns:** `double` — the population variance.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var data = [1.0, 2.0, 3.0, 4.0, 5.0];
    var v = variance(data);
    println(v);  // 2.0
}
```

### stddev(values: double[]) :double

Returns the population standard deviation of a double array.

**Syntax:**
```hoo
stddev(values: double[]) :double
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `values` | `double[]` | Array of values |

- **Returns:** `double` — the population standard deviation.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var data = [1.0, 2.0, 3.0, 4.0, 5.0];
    var v = stddev(data);
    println(v);  // ~1.4142
}
```

## Class: Random

### Declaration

```hoo
class Random
```

The `Random` class provides a pseudo-random number generator. Instances are created with `new Random()` and must be released with `.release()` when no longer needed.

### Public Fields

None.

### Constructor

#### Random() :Random

Creates a new auto-seeded random number generator.

**Syntax:**
```hoo
new Random() :Random
```

- **Returns:** `Random` — a new `Random` instance.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    // ...
    rng.release();
}
```

### Public Instance Functions

#### rng.next_int64(max: int64) :int64

Returns a random integer uniformly distributed in `[0, max)`.

**Syntax:**
```hoo
rng.next_int64(max: int64) :int64
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `max` | `int64` | Exclusive upper bound |

- **Returns:** `int64` — random integer in `[0, max)`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    var roll = rng.next_int64(6) + 1;
    rng.release();
}
```

#### rng.next_double() :double

Returns a random double uniformly distributed in `[0, 1)`.

**Syntax:**
```hoo
rng.next_double() :double
```

- **Returns:** `double` — random value in `[0, 1)`.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    var v = rng.next_double();
    rng.release();
}
```

#### rng.retain() :Random

Increments the reference count.

**Syntax:**
```hoo
rng.retain() :Random
```

- **Returns:** `Random` — The same Random handle.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    var rng2 = rng.retain();
    rng.release();
    rng2.release();
}
```

#### rng.release() :void

Releases the random generator handle.

**Syntax:**
```hoo
rng.release() :void
```

- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    rng.release();
}
```

#### rng.refcount() :int64

Returns the current reference count.

**Syntax:**
```hoo
rng.refcount() :int64
```

- **Returns:** `int64` — The reference count.
- **Errors:** None.

**Example:**
```hoo
import hoo.math;

func :void example() {
    var rng = new Random();
    var rc = rng.refcount();
    rng.release();
}
```

## Usage Example

```hoo
import hoo.math;

func :int64 main() {
    var radius = 5.0;
    var area = math_get_pi() * pow(radius, 2.0);

    if (floor(area).toInt64() % 2 == 0) {
        println("Rounded area is even.");
    }

    var rng = new Random();
    var roll = rng.next_int64(6) + 1;
    rng.release();
    println("You rolled a ".concat(roll.toString()));

    return 0;
}
```
