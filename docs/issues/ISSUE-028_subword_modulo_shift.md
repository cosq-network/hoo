# ISSUE-028: Inconsistent Sub-word Modulo and Shift Semantics

## 1. Overview
The HVM 1.5 scalar implementation now provides type-dispatched sub-word modulo
through `REM.B` and `REMU.B`. Shifts remain wide `Opcode::SHIFT` operations;
there is no `SHIFT_B` opcode in the ISSUE-027 ISA, so shift-width semantics
remain a separate follow-up.

## 2. Technical Analysis
In `HVMCodeGenerator.cpp`, binary operator lowering for `MODULO` is hardcoded:
```cpp
case ast::BinaryOperator::MODULO: func = 7; break; // Always Opcode::ARITH
```
For an `int8` value, `127 % 5` and `(-128) % 5` now use signed 8-bit
remainder semantics. `byte` uses unsigned `REMU.B` semantics.

Similarly, for `SHIFT`:
- `byte << 4` where `byte = 0xF0` should result in `0x00` (8-bit wrap), but currently results in `0xF00` (64-bit).

## 3. Requirements
- **HVM 1.5 Expansion**: Add `REM.B` (8-bit Remainder) to the `ARITH_B` family.
- **HVM 1.5 Expansion**: Add `SHL.B`, `SHR.B`, `SAR.B` (8-bit shifts) to a new `SHIFT_B` family or as part of `ARITH_B`.
- **Codegen Update**: Update `visitBinaryExpression` to dispatch to these new sub-word instructions when operand types are `int8`, `byte`, or `bit`.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PARTIALLY IMPLEMENTED** — modulo complete; shift extension remains proposed
- **Priority**: Medium (Correctness issue for low-precision types)
- **Audit 2026-08-05**: `REM.B`/`REMU.B` are registered, interpreted, JIT-lowered,
  and selected by codegen. No `SHIFT_B` family has been added.
