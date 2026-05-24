# HVMJIT Detailed Test Plan (Current Progress)

This document describes the **current HVMJIT test coverage**, known gaps, and pending test work.

---

## 1. Current Test Strategy in Use

Implemented test strategy:
1. Programmatic `HOModule` synthesis (instruction-level control over `.text` and symbols).
2. In-memory `IOProvider` fixture for deterministic virtual filesystem and dependency tests.
3. Dual-path verification where relevant:
   - ORC execution path behavior
   - interpreter fallback behavior

---

## 2. Existing Test Suites

### 2.1 `HVMJITLoaderTest`

Covers:
1. missing input and parse/IO failures
2. `.ho` and `.hoo` load paths
3. dotted module-name resolution
4. dependency loading and cycle rejection
5. module init behavior
6. rollback on dependency load failure
7. logical search order construction
8. validation gates (pointer-size, section flags, symbol bounds)
9. runtime bootstrap sanity and runtime symbol availability
10. runtime bridge extended symbol lookup checks
11. native import success/failure resolution checks
12. inbound callback trampoline registration/dispatch checks

### 2.2 `HVMJITInstructionSemanticsTest`

Covers:
1. branch and call semantics
2. stack/frame operations (`ENTER/LEAVE/PUSH/POP/FRAME/ADJSP`)
3. arithmetic and division-by-zero error behavior
4. load/store width behavior (`B/H/W/D`) and alignment checks
5. `JALR`, `TAILCALL`, `NOT`
6. float arithmetic and float comparison opcodes
7. syscall runtime bridge operations:
   - alloc / retain / release / refcount / type-id
   - runtime exception object creation
8. ARC-aware `ST.D` retain/store/release behavior for managed handles
9. runtime string-data bridge path (`hoo_string_data`) behavior

---

## 3. Current Pass Status

Latest verified status in workspace:
1. `HVMJIT*` suites are passing.
2. Full `hoo-tests` suite is passing.
3. `ctest --preset macos-homebrew-ninja` is passing.

---

## 4. What Is Not Yet Fully Covered

Even with current passing suites, these areas are still under-covered for “full coverage”:
1. Full structured exception unwinding semantics (try/catch/finally control-flow transitions).
2. Cleanup-stack ownership release behavior for every control-flow exit category.
3. Ownership contract stress under complex CFG (nested branches, early return, cross-call ownership promotion).
4. Multi-threaded ARC race stress and runtime audit/debug-mode coverage.
5. Full bridge matrix from guide sections 7.19–7.20 (all listed validation modes).

---

## 5. Pending Test Work (Next)

### 5.1 Exception and Unwind Tests

1. Throw path should transfer control to registered handler.
2. Catch path should receive expected exception handle in register convention.
3. Finally path should execute under both normal and exceptional exits.

### 5.2 Cleanup / Ownership Tests

1. release-on-return for owned locals.
2. release-on-branch-exit from nested blocks.
3. release-on-error/unwind for partially-initialized state.

### 5.3 ARC Stress Tests

1. repeated `ST.D` overwrite of managed handles under loop-heavy bytecode.
2. mixed managed/non-managed values in same memory slots.
3. retained object lifetime after cross-function call boundaries.

### 5.4 Runtime Bridge Matrix Tests

1. mandatory intrinsic symbol absence should fail bootstrap deterministically.
2. extended runtime symbol mismatch diagnostics.
3. module init ordering with runtime bridge dependencies.

---

## 6. Coverage Goal Definition

“Full coverage” for HVMJIT in this project should mean:
1. instruction-semantic coverage for all implemented opcodes/func variants.
2. branch/error-path coverage for loader, linker, bootstrap, runtime bridge.
3. explicit exception-path and cleanup-path coverage.
4. stress validation for ARC/runtime boundary correctness.

Current status: strong functional coverage for implemented features through completed Phase 4 and current Phase 5 slices, but not yet complete against the full Phase 5 guide matrix.
