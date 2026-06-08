# ISSUE-019: Register Leaks on Control Flow Breaks

## 1. Overview
When `break` or `continue` is used inside loops (for-range, for-in, while), the `emitJump(JMP)` to the break/continue label bypasses the `freeRegister` calls at the bottom of the loop body. Registers allocated for the loop variable, condition, and body expressions are leaked, causing register exhaustion.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 511-622

### Affected code paths:
1. **`emitWhileStatement`** (lines 511-554):
   - `breakLabel` and `continueLabel` jumps skip `freeRegister(condReg)` at line 548.

2. **`emitForRangeStatement`** (lines 553-555):
   - `breakLabel` and `continueLabel` jumps skip register cleanup.

3. **`emitForInStatement`** (lines 608-610):
   - `breakLabel` and `continueLabel` jumps skip register cleanup.

### Mechanism:
- The `break`/`continue` is compiled as `emitJump(Opcode::JMP, breakLabel)`.
- The target label is placed after the loop's register-freeing code.
- Registers allocated within the loop body (loop variables, iterators, conditions) are never freed.
- The register allocator (`usedRegs_` bitmap) has no mechanism for abnormal control flow.

## 3. Impact
- Register exhaustion after repeated `break`/`continue` inside loops.
- Corrupted register state on continued loop iterations.
- Hard-to-debug crashes in loops with early exits.

## 4. Suggested Fix
Before emitting the `JMP` to `breakLabel` or `continueLabel`, free all registers currently allocated within the loop scope. Options:

1. **Save/restore register state**: Snapshots `usedRegs_` before entering the loop and restores it on `break`/`continue`.
2. **Free on jump**: Track which registers were allocated since the last scope boundary and emit `freeRegister` calls for each before the `JMP`.
3. **Lexical scoping**: Implement proper register scoping by pushing/popping register allocation state alongside `scopeStack_`.

Approach 3 is cleanest — treat each loop body as a register scope, and free all loop-local registers at scope exit (including `break`/`continue` targets).

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **MEDIUM**
