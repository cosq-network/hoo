# ISSUE-059: Decimal Arithmetic Overflow Not Checked

## Status
- **Date**: 2026-07-19
- **Status**: **IMPLEMENTED** (2026-07-19)
- **Priority**: 🔴 **P0 - CRITICAL** (Must fix immediately - correctness issue for financial/precision-critical applications)
- **Sprint**: Week 1 (Days 1-3)
- **Estimate**: 2-3 days
- **Actual**: 1 day

---

## 1. Overview
The Decimal intrinsic type (`Decimal<P, S>`) implementation in `src/runtime/lib/hoo_decimal.cpp` did not check for overflow during arithmetic operations. The `normalize()` function also didn't validate that the mantissa fits within the declared precision.

## 2. Technical Analysis

### Original Issues Found

#### normalize() function
```c
static void normalize(HooDecimalImpl* impl) {
    int64_t m = impl->mantissa;
    int32_t s = impl->scale;
    // Remove trailing zeros from mantissa (reduce scale)
    while (s > 0 && m % 10 == 0) {
        m /= 10;
        --s;
    }
    impl->mantissa = m;
    impl->scale = s;
}
```

**Problem**: No validation that `|m|` fits within `precision` digits.

#### hoo_decimal_add()
```c
extern "C" HooDecimal hoo_decimal_add(HooDecimal a, HooDecimal b) {
    // ... scale alignment ...
    int64_t result = ma + mb;  // Can overflow!
    int32_t prec = std::max(da->precision, db->precision);
    return hoo_decimal_new(result, prec, sa);
}
```

**Problem**: `result` can overflow `int64_t` without detection.

### Division by Zero Issue
```c
extern "C" HooDecimal hoo_decimal_div(HooDecimal a, HooDecimal b) {
    if (db->mantissa == 0) return nullptr; // Division by zero - SILENT FAILURE!
}
```

**Problem**: Division by zero returned `nullptr` instead of throwing an exception.

## 3. Implementation

### Changes Made

1. **Added overflow detection helpers**:
   - `countDigits()`: Count digits in an integer
   - `fitsPrecision()`: Check if value fits within precision
   - `addWouldOverflow()`: Check for int64_t addition overflow
   - `subWouldOverflow()`: Check for int64_t subtraction overflow
   - `mulWouldOverflow()`: Check for int64_t multiplication overflow

2. **Added exception throwing helpers**:
   - `throwDecimalOverflow()`: Throws `HOO_EXCEPTION_DECIMAL_OVERFLOW`
   - `throwDecimalDivZero()`: Throws `HOO_EXCEPTION_DECIMAL_DIV_ZERO`

3. **Updated all arithmetic operations**:
   - `hoo_decimal_add()`: Checks overflow before and after addition
   - `hoo_decimal_sub()`: Checks overflow before and after subtraction
   - `hoo_decimal_mul()`: Checks overflow before and after multiplication
   - `hoo_decimal_div()`: Throws exception on division by zero
   - `hoo_decimal_mod()`: Throws exception on modulo by zero

4. **Added unary negation operator**:
   - `hoo_decimal_neg()`: New function for `-decimal` syntax

5. **Updated header file**:
   - Added exception type IDs: `HOO_EXCEPTION_DECIMAL_OVERFLOW`, `HOO_EXCEPTION_DECIMAL_DIV_ZERO`, `HOO_EXCEPTION_DECIMAL_MOD_ZERO`
   - Added `hoo_decimal_neg()` declaration
   - Updated documentation with `@throws` annotations

### Files Modified
- `src/runtime/lib/hoo_decimal.h` - Added exception IDs and negation function
- `src/runtime/lib/hoo_decimal.cpp` - Added overflow checks and proper exceptions

## 4. Test Coverage

### New Tests Added (tests/jit/HooDecimalJitTest.cpp)
- `AdditionOverflowDetection`: Tests overflow detection in addition
- `SubtractionUnderflowDetection`: Tests overflow detection in subtraction
- `MultiplicationOverflowDetection`: Tests overflow detection in multiplication
- `DivisionByZeroDetection`: Tests division by zero exception
- `ModuloByZeroDetection`: Tests modulo by zero exception
- `Negation`: Tests unary negation operator
- `NegationZero`: Tests negation of zero
- `DoubleNegation`: Tests double negation
- `NegationInExpression`: Tests negation in expressions
- `PrecisionPreservedInAddition`: Tests precision preservation
- `ScaleAlignmentInAddition`: Tests scale alignment
- `MultiplicationPrecisionAccumulation`: Tests precision in multiplication
- `ZeroArithmetic`: Tests operations with zero
- `NegativeNumbers`: Tests negative number arithmetic
- `MixedSignArithmetic`: Tests mixed sign operations
- `ChainedOperations`: Tests chained operations
- `LargeNumbers`: Tests large number arithmetic
- `SmallNumbers`: Tests small number arithmetic
- `CompareWithDifferentScales`: Tests comparison with different scales
- `CompareNegativeNumbers`: Tests comparison of negative numbers
- `CompareWithZero`: Tests comparison with zero
- `ExactPrecisionFit`: Tests precision boundaries
- `PrecisionWithDecimals`: Tests precision with decimal places

## 5. Acceptance Criteria
- [x] Arithmetic operations throw `DecimalOverflowException` when result exceeds precision
- [x] Division by zero throws `DecimalDivisionByZeroException`
- [x] Modulo by zero throws `DecimalModuloByZeroException`
- [x] Overflow is detected before operations, not after
- [x] Precision validation works for all arithmetic operations
- [x] Unary negation operator works correctly
- [x] Test coverage added for all scenarios

## 6. Notes
- Overflow detection uses conservative checks to prevent false positives
- Scale alignment includes overflow checks during multiplication by 10
- The implementation uses `[[noreturn]]` for exception throwing functions to help compiler optimization
- Exception messages include operation name and precision details for debugging
