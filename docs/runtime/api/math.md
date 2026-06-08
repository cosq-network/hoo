# Math API Reference (`Math`)

The `Math` class provides a comprehensive suite of mathematical constants, basic functions, power/root operations, and random number generation.

## 1. Constants

### `Math.get_pi() -> double`
Returns the value of $\pi$ (approximately 3.14159).

### `Math.get_e() -> double`
Returns the value of $e$ (approximately 2.71828).

### `Math.get_inf() -> double`
Returns positive infinity.

### `Math.get_nan() -> double`
Returns Not-a-Number (NaN).

## 2. Basic Functions

### `Math.abs(x: int64) -> int64`
Returns the absolute value of an integer.

### `Math.abs(x: double) -> double`
Returns the absolute value of a double.

### `Math.min(a: int64, b: int64) -> int64`
Returns the smaller of two integers.

### `Math.max(a: double, b: double) -> double`
Returns the larger of two doubles.

### `Math.clamp(val: double, min: double, max: double) -> double`
Clamps a value within the specified range.

## 3. Power and Roots

### `Math.pow(base: double, exp: double) -> double`
Returns `base` raised to the power of `exp`.

### `Math.sqrt(x: double) -> double`
Returns the square root of `x`.

### `Math.hypot(x: double, y: double) -> double`
Returns `sqrt(x*x + y*y)`.

## 4. Trigonometric Functions

### `Math.sin(x: double) -> double`
Returns the sine of `x` (radians).

### `Math.cos(x: double) -> double`
Returns the cosine of `x` (radians).

### `Math.tan(x: double) -> double`
Returns the tangent of `x` (radians).

## 5. Random Number Generation

Hoo random number generation uses a managed state object via the `Random` class.

### `Random.new() -> ptr`
Creates a new, auto-seeded random number generator state.

### `state.next_int() -> int64`
Returns a random 64-bit integer.

### `state.next_int_max(max: int64) -> int64`
Returns a random integer in the range `[0, max)`.

### `state.next_double() -> double`
Returns a random double in the range `[0, 1)`.

---

## Usage Example

```hoo
func :int64 main() {
    var radius = 5.0;
    var area = Math.get_pi() * Math.pow(radius, 2.0);
    
    println("Area: ".concat(area.to_string()));
    
    var rng = Random.new();
    var roll = rng.next_int_max(6) + 1;
    
    println("Dice Roll: ".concat(roll.to_string()));
    
    return 0;
}
```
