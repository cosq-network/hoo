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

## 5. Status
- **Date**: 2026-06-23
- **Status**: **PROPOSED**
- **Priority**: **MEDIUM**
