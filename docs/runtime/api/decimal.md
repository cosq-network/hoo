# Decimal

Fixed-precision decimal arithmetic.

**Import:** `import hoo;`

**Pattern:** Value type with `Decimal<P,S>` type parameters.

## Overview

`Decimal<P,S>` stores a signed fixed-point number with `P` significant digits of
precision and `S` digits after the decimal point. Storage is an `int64`
mantissa scaled by `10^S` (i.e. `mantissa = value * 10^S`), so values are
exact and never suffer binary floating-point rounding.

Limits:

- The mantissa must fit in `int64`, so effectively up to ~19 significant
  digits are representable regardless of `P`.
- A literal that cannot fit in the mantissa throws
  `DecimalOverflow` at parse time.
- Trailing zeros are normalized away (e.g. `100.00m` and `100m` are
  both `100`).

## Literals

Decimal literals use the `m` suffix and an optional explicit `Decimal<P,S>`
type:

```hoo
var amount: Decimal<38,2> = 19.99m;
var volume: Decimal<38,3> = -0.05m;
```

## Arithmetic

| Operator | Behavior |
|----------|----------|
| `+` | Addition (throws `DecimalOverflow` on overflow) |
| `-` | Subtraction (throws `DecimalOverflow` on overflow) |
| `*` | Multiplication (throws `DecimalOverflow` on overflow) |
| `/` | Division (throws `DecimalDivZero` on zero divisor, `DecimalOverflow` if the result exceeds the declared precision) |
| `%` | Modulo (throws `DecimalModZero` on zero divisor) |
| `-` (unary) | Negation |

Division computes up to the declared precision digits; the divisor and
dividend scales are aligned first. For small-precision decimals the result
scale is clamped to the precision so valid quotients do not overflow.

## Comparison

`==`, `!=`, `<`, `<=`, `>` and `>=` compare by algebraic value, ignoring
declared precision (e.g. `19.99m == 19.990m` is true).

## String conversion

`toString()` produces the canonical decimal representation:

```hoo
var s: string = amount.toString(); // "19.99"
var t: string = (0.05m).toString(); // "0.05"
```

Values whose magnitude is less than `1` are rendered with a leading zero
(`0.05`, `-0.5`, ...) rather than bare digits.

## Exceptions

| ID | Name | Raised when |
|----|------|-------------|
| 100 | `DecimalOverflow` | a value or literal exceeds the representable mantissa or precision |
| 101 | `DecimalDivZero` | division by zero |
| 102 | `DecimalModZero` | modulo by zero |

```hoo
func :bool safeDivide(a: Decimal<38,2>, b: Decimal<38,2>): bool {
    if (b == 0.00m) {
        return false;
    }
    return a / b == 25.00m;
}
```