# ISSUE-028: Inconsistent Sub-word Modulo and Shift Semantics

## 1. Overview
The current implementation of HVM 1.5 sub-word precision focuses on basic arithmetic (`ADD.B`, `SUB.B`, `MUL.B`, `DIV.B`). However, the `MODULO` (`%`) and `SHIFT` (`<<`, `>>`) operators are still lowered to 64-bit `Opcode::ARITH` or `Opcode::SHIFT` instructions regardless of whether the operands are `int8`, `byte`, or `bit`.

## 2. Technical Analysis
In `HVMCodeGenerator.cpp`, binary operator lowering for `MODULO` is hardcoded:
```cpp
case ast::BinaryOperator::MODULO: func = 7; break; // Always Opcode::ARITH
```
For an `int8` value, `127 % 5` works fine in 64-bit, but `(-128) % 5` might yield different results if not truncated correctly to 8-bit signed space before or after the operation.

Similarly, for `SHIFT`:
- `byte << 4` where `byte = 0xF0` should result in `0x00` (8-bit wrap), but currently results in `0xF00` (64-bit).

## 3. Requirements
- **HVM 1.5 Expansion**: Add `REM.B` (8-bit Remainder) to the `ARITH_B` family.
- **HVM 1.5 Expansion**: Add `SHL.B`, `SHR.B`, `SAR.B` (8-bit shifts) to a new `SHIFT_B` family or as part of `ARITH_B`.
- **Codegen Update**: Update `visitBinaryExpression` to dispatch to these new sub-word instructions when operand types are `int8`, `byte`, or `bit`.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: Medium (Correctness issue for low-precision types)
