# ISSUE-029: Missing Unsigned Comparison Instructions in HVM

## 1. Overview
Before this fix, the HVM ISA implementation lacked explicit native support
for unsigned sub-word integer comparisons. This caused semantic errors when
using the `byte` type, which is intended to be unsigned.

## 2. Technical Analysis
The original implementation of `Opcode::CMP` (0x40) only implemented signed comparisons:
- `func 2`: `CMPLT` (Signed Less Than)
- `func 3`: `CMPLE` (Signed Less Than or Equal)

When the Hoo compiler encounters `byte a < byte b`, it generates a `CMPLT` instruction. If `a = 200` and `b = 10`, a signed comparison treats `200` (as `int8`) as `-56`, resulting in `-56 < 10 == true`, which is incorrect for unsigned bytes (`200 < 10` should be `false`).

## 3. Requirements
- **ISA Update**: Add `CMPULT` (Unsigned Less Than) and `CMPULE` (Unsigned Less Than or Equal) to the `CMP` family (Opcode 0x40) using new `func` codes (e.g., 4 and 5).
- **HVM 1.5 Expansion**: Add `CMPULT.B` and `CMPULE.B` to the `CMP_B` family for native 8-bit unsigned comparison.
- **JIT Update**: Implement these in `HVMJIT.cpp` using `builder.CreateICmpULT` and `builder.CreateICmpULE`.
- **Codegen Update**: Update `visitBinaryExpression` to check if operands are unsigned (`byte`) and emit the unsigned comparison opcodes.

## 4. Status
- **Date**: 2026-08-09
- **Status**: **FIXED**
- **Priority**: High (Crucial for `byte` type correctness)
- **Audit 2026-06-21**: No unsigned comparison opcodes or LLVM unsigned integer comparisons were found for byte/sub-word comparisons; this was the pre-fix state.
- **Fix 2026-06-24**: Codegen emits `CMP` with func=4 (`CMPULT`) / func=5 (`CMPULE`) when either operand is `byte` (typeId==6). JIT lowers to LLVM `ICmpULT`/`ICmpULE`. ISA mnemonics `cmpult`/`cmpule` registered in `HVMInstruction.cpp`.
- **Fix 2026-06-29**: Added test coverage for the wide `CMPULT`/`CMPULE` implementation. The native `CMP_B` family remained for the follow-up fix.

## 5. Resolution
The HVM 1.5 `CMP_B` family is implemented as opcode `0x42`, including signed
and unsigned 8-bit equality/ordering forms. The interpreter and LLVM ORC
translator compare truncated 8-bit operands, and byte-to-byte Hoo comparisons
emit `CMP_B` while mixed byte/integer comparisons retain the existing wide
`CMPULT`/`CMPULE` semantics. No zero-extension issue exists: byte values are
stored/loaded as full 64-bit values via `ST_D`/`LD_D`.
