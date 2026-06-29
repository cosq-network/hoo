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
- **Status**: **PARTIALLY IMPLEMENTED**
- **Priority**: High (Crucial for `byte` type correctness)
- **Audit 2026-06-21**: No unsigned comparison opcodes or LLVM unsigned integer comparisons were found for byte/sub-word comparisons; this remains unimplemented.
- **Fix 2026-06-24**: Codegen emits `CMP` with func=4 (`CMPULT`) / func=5 (`CMPULE`) when either operand is `byte` (typeId==6). JIT lowers to LLVM `ICmpULT`/`ICmpULE`. ISA mnemonics `cmpult`/`cmpule` registered in `HVMInstruction.cpp`.
- **Fix 2026-06-29**: Added test coverage: 4 JIT instruction semantics tests for func=4/5, 2 codegen tests verifying unsigned opcode emission, and an end-to-end `.hoo` test with all byte comparison operators. `CMP_B` family (native 8-bit ops) not yet implemented.

## 5. Remaining Gaps
- `CMP_B` family (native 8-bit unsigned comparison, HVM 1.5 extension) not implemented
- No zero-extension issue: byte values are stored/loaded as full 64-bit via `ST_D`/`LD_D`, so unsigned semantics work correctly for HVM's current 64-bit register model
