# HVMJIT Technical Implementation Plan (Current Status)

This document tracks the **actual implementation status** of `HVMJIT` in `src/hvm/HVMJIT.cpp` and the remaining roadmap.

---

## 1. Current Delivery Snapshot

- **Status**: Phase 1 and Phase 2 are substantially implemented and test-verified. Phase 3 is partially implemented with meaningful runtime bridge coverage.
- **Primary executable path**: ORC JIT path first, interpreter fallback second.
- **Input abstraction**: `IOProvider` is the canonical front-door for `.ho` and `.hoo` loading.

---

## 2. Implemented Features

### 2.1 Loader / Module System (Phase 1)

Implemented in `HVMJIT`:
1. `.ho` and `.hoo` input dispatch (`loadInput`, `loadBytecode`, `loadSource`).
2. Dependency loading through imports with cycle detection.
3. Canonical module identity and collision handling.
4. Structured loader states and typed error model.
5. Validation gates:
   - magic/version
   - endianness
   - pointer size
   - section policy (`.text`, `.rodata`, `.data`)
   - symbol/index bounds
6. Runtime module bootstrap (`hoo`) and per-module JITDylib setup.
7. Logical search order wiring (`self -> deps -> hoo -> process`).

### 2.2 Core ISA Lowering (Phase 2)

Implemented lowering + interpreter parity for major core ISA subset:
1. Integer ALU: `ARITH`, `SHIFT`, `LOGIC`, `CMP`, `NOT`.
2. Control flow: `BEQ/BNE/BLT/BLE/JMP/JAL/JALR/CALL/TAILCALL/RET`.
3. Memory: `LD/ST` for `B/H/W/D`, `LDA`, stack/frame ops (`ENTER/LEAVE/ADJSP/FRAME/PUSH/POP`).
4. Escaped/system opcodes: `SYSCALL`, `BREAK`.
5. Float ops: `FLOAT_ARITH`, `FCMP`.
6. `LD.D/ST.D` alignment checks.

### 2.3 Runtime Bridge & Intrinsics (Phase 3, partial)

Implemented:
1. Mandatory intrinsic bootstrap checks (`hoo_alloc`, `hoo_retain`, `hoo_release`).
2. Runtime self-test probe at bootstrap (`alloc/retain/release/refcount`).
3. ORC symbol exports for runtime intrinsics and selected runtime APIs.
4. Managed-handle tracking wrappers used by HVM bridge:
   - `hooc_hvm_sys_alloc/retain/release/refcount/typeid`
   - `hooc_hvm_arc_retain_if_managed`
   - `hooc_hvm_arc_release_if_managed`
5. ARC-aware `ST.D` path (retain-new, store, release-old) guarded by managed-handle tracking.
6. Runtime-exception object bridge syscall (`imm15=6`) via `hoo_exception_runtime`.

---

## 3. Test Verification Status

Current suites and outcomes:
1. `HVMJITLoaderTest` and `HVMJITInstructionSemanticsTest` are active and passing.
2. Full project test target `hoo-tests` passes in direct execution.
3. `ctest --test-dir build` passes.

Coverage currently includes:
1. Loader/validation/rollback/dependency/cycle behavior.
2. Core instruction semantics for implemented subset.
3. Runtime syscall bridge for alloc/retain/release/refcount/type-id and exception object creation.
4. ARC-sensitive `ST.D` managed object behavior.

---

## 4. Known Limitations (Important)

Not yet complete for full guide-level Phase 3/4 semantics:
1. No full structured try/catch/finally IR unwind model (`hoo_push_handler` + resume semantics).
2. No complete lexical ownership dataflow with cleanup-stack release insertion across all CFG exits.
3. ARC ownership model is runtime-handle tracked, but not yet full compiler-proven ownership typing for all call/store edges.
4. Runtime concurrency stress/audit/debug modes from guide sections 7.17+ are not fully implemented.
5. Some advanced runtime bridge tables in the guide (full string/array/map/object lifecycle contracts) are only partially covered.

---

## 5. Pending Roadmap

### 5.1 Remaining Phase 3 Work

1. Implement exception boundary model:
   - handler stack registration
   - runtime throw edge integration
   - catch target resumption semantics
2. Implement cleanup-stack lowering:
   - release-on-return
   - release-on-branch-exit
   - release-on-error/unwind path
3. Extend ownership contracts:
   - owned vs borrowed call convention tracking
   - deterministic retain/release insertion across call boundaries
4. Add stress suites:
   - retain/release race safety tests
   - branch-heavy ARC edge tests
   - exception-path cleanup balance tests

### 5.2 Phase 4+ Preview

1. Complete module init/error rollback guarantees for all runtime failure edges.
2. Extend linkage contracts across runtime/native modules.
3. Add optional compile-on-demand and optimization passes after correctness closure.

---

## 6. Definition of Done (Updated)

`HVMJIT` will be considered fully complete for Phase 3 when all are true:
1. Runtime bridge contracts are enforced for mandatory and declared extended intrinsics.
2. ARC correctness holds on normal + exceptional control flow with ownership-aware lowering.
3. Exception boundary behavior is implemented end-to-end (throw/catch/finally semantics).
4. Unit + integration + stress suites demonstrate stable correctness.
