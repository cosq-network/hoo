# ISSUE-029: Missing Unsigned Comparison Instructions in HVM

## 1. Overview
The HVM ISA (v1.4 and proposed v1.5) currently lacks explicit support for unsigned integer comparisons (`<`, `<=`, `>`, `>=`). This causes semantic errors when using the `byte` type, which is intended to be unsigned.

## 2. Technical Analysis
Currently, `Opcode::CMP` (0x40) only implements signed comparisons:
- `func 2`: `CMPLT` (Signed Less Than)
- `func 3`: `CMPLE` (Signed Less Than or Equal)

When the Hoo compiler encounters `byte a < byte b`, it generates a `CMPLT` instruction. If `a = 200` and `b = 10`, a signed comparison treats `200` (as `int8`) as `-56`, resulting in `-56 < 10 == true`, which is incorrect for unsigned bytes (`200 < 10` should be `false`).

## 3. Requirements
- **ISA Update**: Add `CMPULT` (Unsigned Less Than) and `CMPULE` (Unsigned Less Than or Equal) to the `CMP` family (Opcode 0x40) using new `func` codes (e.g., 4 and 5).
- **HVM 1.5 Expansion**: Add `CMPULT.B` and `CMPULE.B` to the `CMP_B` family for native 8-bit unsigned comparison.
- **JIT Update**: Implement these in `HVMJIT.cpp` using `builder.CreateICmpULT` and `builder.CreateICmpULE`.
- **Codegen Update**: Update `visitBinaryExpression` to check if operands are unsigned (`byte`) and emit the unsigned comparison opcodes.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: High (Crucial for `byte` type correctness)
- **Audit 2026-06-21**: No unsigned comparison opcodes or LLVM unsigned integer comparisons were found for byte/sub-word comparisons; this remains unimplemented.
