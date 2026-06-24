# ISSUE-051: Integer Overflow Not Checked in JIT Arithmetic

## 1. Overview
The JIT emits native LLVM arithmetic instructions (add, sub, mul, div) without overflow checking, saturation, or trapping. Silent integer overflow produces incorrect results without any diagnostic.

## 2. Technical Analysis
- **Location**: `src/hvm/HVMJIT.cpp` — ARITH operations (ADD func 0, SUB func 1, MUL func 2, DIV func 3)
- **Current behavior**: Uses `IRBuilder` to emit standard LLVM `add`, `sub`, `mul`, `sdiv` — none of which trap on overflow.
- **Missing**: No `llvm.ssub.with.overflow`, `llvm.sadd.with.overflow`, or explicit bounds check before arithmetic.

## 3. Impact
- `INT64_MAX + 1` silently wraps to `INT64_MIN` instead of producing a documented error.
- Division of `INT64_MIN` by `-1` produces undefined LLVM behavior.
- Hard to diagnose debugging sessions where values overflow silently.

## 4. Suggested Fix
1. Add a language-level overflow policy (wrap, saturate, trap, or checked).
2. For checked arithmetic: use LLVM's `*_with_overflow` intrinsics and conditionally trap.
3. For debug builds: always use checked arithmetic; for release, support configurable policy.
4. Add overflow tests for all arithmetic operations.

## 5. Resolution

### Fix Applied
- **Interpreter** (`HVMJIT.cpp` interpreter loop, line ~5693): Added manual overflow checks for ADD (func 0), SUB (func 1), MUL (func 2), SDIV (func 5), and REM (func 7). Uses safe multiplication check via `hoo_math::multiplyWouldOverflow` for MUL; direct comparison for INT64_MIN / -1 for SDIV/REM.
- **JIT Compiler** (`HVMJIT.cpp` JIT codegen, line ~7180): Replaced bare `CreateAdd`/`CreateSub`/`CreateMul` with LLVM `sadd_with_overflow`/`ssub_with_overflow`/`smul_with_overflow` intrinsics. Added INT64_MIN / -1 guard for SDIV and REM (matching the div-by-zero split-block pattern). On overflow, emits `ret i64 -1` to signal error to the runtime.

### Tests Added
5 new test cases in `tests/hvm/HVMJITInstructionSemanticsTest.cpp`:
- `AddOverflowReportsError` — INT64_MAX + 1
- `SubOverflowReportsError` — INT64_MIN - 1
- `MulOverflowReportsError` — INT64_MAX * 2
- `DivOverflowReportsError` — INT64_MIN / -1
- `ModOverflowReportsError` — INT64_MIN % -1

All 1845 tests pass (1840 existing + 5 new).

### Files Changed
- `src/hvm/HVMJIT.cpp` — Added `#include "llvm/IR/Intrinsics.h"`, fixed interpreter and JIT ARITH paths.
- `tests/hvm/HVMJITInstructionSemanticsTest.cpp` — Added 5 overflow test cases.
- `docs/issues/ISSUE-051_integer_overflow_not_checked.md` — Updated status.

## 6. Status
- **Date**: 2026-06-24
- **Status**: **RESOLVED**
- **Priority**: **MEDIUM**
