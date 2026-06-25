# ISSUE-032: Invalid Modulo Lowering for Floating Point Types

## 1. Overview
The Hoo compiler currently lowers the `MODULO` (`%`) operator to the HVM `REM` (Integer Remainder) function regardless of the operand types. This produces incorrect results or runtime errors when used with `double`, `float`, or `f8` types.

## 2. Technical Analysis
In `HVMCodeGenerator.cpp` (around line 1471), the binary operator switch handles `MODULO` by setting `func = 7` (REM) but fails to check if `isFloatExpr` is true.

```cpp
case ast::BinaryOperator::MODULO: func = 7; break; // Always uses ARITH func 7
```

If `isFloatExpr` is true, the `op` might be set to `Opcode::FLOAT_ARITH`. However, `FLOAT_ARITH` in HVM v1.4 only supports:
- `0`: FADD
- `1`: FSUB
- `2`: FMUL
- `3`: FDIV

There is no `FREM` (Floating Point Remainder) in the HVM ISA.

## 3. Requirements
- **Runtime Support**: Implement `hoo_math_fmod(double, double)` in `hoort`.
- **Codegen Update**: If operands are floating point, emit a call to `_F_M_hoo_E_Math_fmod_static_d_d` instead of a raw HVM instruction.
- **ISA Potential**: Alternatively, consider adding `FREM` (0x30, func 7) to HVM 1.5.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **FIXED**
- **Priority**: Medium (Bug in mathematical correctness)
- **Audit 2026-06-24**: Float modulo implemented via `hoo_math_fmod(double, double)` wrapping `std::fmod`. Emits `CALL _F_M_hoo_E_math_fmod_d_p_p` for float `%` in binary expressions and `%=` in compound assignments. Verified by `FloatModuloEmitsFmodCall`, `Fmod`, `FmodNonZero`, `FmodNegative` tests.
- **Fix 2026-06-24**: Implemented `hoo_math_fmod(double, double)` in `hoort`, registered `jit_math_fmod` JIT bridge, and updated codegen `MODULO` and `MODULO_ASSIGN` cases to emit a `CALL` to `_F_M_hoo_E_math_fmod_d_p_p` when operands are float types (`double`/`f8`). Tested with `FloatModuloEmitsFmodCall` (codegen) and `Fmod`, `FmodNonZero`, `FmodNegative` (JIT).
