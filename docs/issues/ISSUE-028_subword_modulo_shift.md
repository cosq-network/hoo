# ISSUE-028: Inconsistent Sub-word Modulo and Shift Semantics

## 1. Overview
The HVM 1.5 scalar implementation provides type-dispatched sub-word modulo
through `REM.B` and `REMU.B`, and now provides `SHIFT_B` for both standalone
shift expressions and compound shift assignments. The grammar, AST, codegen,
interpreter, and JIT now agree on the sub-word shift behavior.

## 2. Technical Analysis
`HVMCodeGenerator.cpp` now selects `ARITH_B` for sub-word modulo. For an
`int8` value, `127 % 5` and `(-128) % 5` use signed 8-bit remainder
semantics. `byte` uses unsigned `REMU.B` semantics. Compound modulo follows
the same dispatch.

Similarly, for `SHIFT`:
- `byte << 4` where `byte = 0xF0` now results in `0x00` through `SHL.B`
  (8-bit wrap), while wide integer shifts retain their existing 64-bit
  behavior.

## 3. Requirements
- **HVM 1.5 Expansion**: Add `REM.B` and `REMU.B` to the `ARITH_B` family. **Complete.**
- **HVM 1.5 Expansion**: Add `SHL.B`, `SHR.B`, and `SAR.B` to the new
  `SHIFT_B` family. **Complete.**
- **Codegen Update**: Dispatch supported sub-word modulo and compound shifts
  for `int8`, `byte`, and `bit`, with final sign/zero/bit extension. **Complete.**

## 4. Status
- **Date**: 2026-08-05
- **Status**: **IMPLEMENTED**
- **Priority**: Medium (Correctness issue for low-precision types)
- **Audit 2026-08-05**: `REM.B`/`REMU.B` and `SHIFT_B` are registered,
  interpreted, LLVM-JIT-lowered, and selected by codegen. Regression tests cover
  CSV parity, direct JIT execution, byte wrapping, signed arithmetic shifts,
  binary modulo dispatch, standalone shift expressions, and compound shift
  dispatch.
