# ISSUE-019: Register Leaks on Control Flow Breaks

## 1. Overview
When `break` or `continue` is used inside loops, code generation can branch
around the normal fallthrough path. A function-wide temporary-register bitmap
cannot safely represent that control-flow merge unless the allocator state is
restored at the branch and loop join points.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` loop/control-flow emission
  and `src/codegen/HVMCodeGenerator.h` `ControlFlowScope`

### Affected code paths:
1. Loop bodies can contain nested blocks and branches whose temporary state
   differs at compile-time.
2. `ControlFlowScope` previously tracked only labels and managed-local scope
   depth, not temporary-register state.
3. `emitScopeCleanup()` needs a temporary register to preserve `r1`; stale
   allocator state could make that cleanup itself fail under pressure.

### Mechanism:
- The `break`/`continue` is compiled as an unconditional jump.
- The target label can merge with code generated from the normal loop path.
- The allocator's `usedRegs_` bitmap is compile-time state and needs explicit
  checkpoints at those merges.

## 3. Impact
- Register exhaustion after repeated `break`/`continue` inside loops.
- Incorrect register reuse or false register-pressure errors after early exits.
- Hard-to-debug failures in nested loops and control-flow-heavy functions.

## 4. Implementation

`ControlFlowScope` now stores register masks for its break and continue
targets. Loop emitters capture:

- the register state before loop setup;
- the state at loop-body entry, including loop-carried registers.

Before emitting `break` or `continue`, the generator restores the target
state, then emits managed-local cleanup and the jump. After generating a loop
body, it restores the body-entry state before emitting step/fallthrough code.
At the loop exit it restores the pre-loop state. Switch scopes use the same
mechanism so a `break` inside a switch nested in a loop does not corrupt the
enclosing loop state.

## 5. Status
- **Date**: 2026-08-05
- **Status**: **RESOLVED**
- **Priority**: **MEDIUM**
- Temporary registers remain limited to 12 (`r9`–`r20`); this issue does not
  add spilling.
- Regression coverage verifies while, do-while, stepped for-range, and for-in
  loops containing `break` and `continue`.
