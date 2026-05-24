# HVMJIT Technical Implementation Plan (Current Status)

This document tracks the **actual implementation status** of `HVMJIT` in `src/hvm/HVMJIT.cpp` and the remaining roadmap.

---

## 1. Current Delivery Snapshot

- **Status**: Phase 1 through Phase 4 are implemented and test-verified in this workspace. Phase 5 is in progress with meaningful delivered slices.
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

### 2.3 Runtime Bridge & Intrinsics (Phase 3)

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

### 2.4 Module Bootstrap & Initialization (Phase 4)

Implemented:
1. Post-load initializer flow that executes module initialization in dependency order.
2. Per-module once-only guards (`std::call_once`) for module init and vtable init paths.
3. Vtable initializer execution ordering with base-before-derived traversal support.
4. Static section mapping for `.text`, `.rodata`, `.data`, and `.bss` with zero-init semantics for writable virtual memory sections.
5. Rollback cleanup for module/vtable init state on load failure.

### 2.5 FFI & Multi-Binary Linkage (Phase 5, partial)

Implemented:
1. Native import detection and resolution (`IT_NATIVE`, process-symbol and shared-library symbol paths).
2. Dynamic native library preloading from import declarations.
3. Per-module process symbol generator wiring in ORC `JITDylib` setup.
4. State-ABI runtime bridge invocation for imported/runtime symbols.
5. `hoo_string_data` bridge support via runtime symbol export and syscall/IR path integration.
6. Inbound callback trampoline scaffolding:
   - slot-based stable C trampoline entry points
   - module/function callback target registration
   - callback dispatch into loaded HVM functions.

---

## 3. Test Verification Status

Current suites and outcomes:
1. `HVMJITLoaderTest` and `HVMJITInstructionSemanticsTest` are active and passing.
2. Full project test target `hoo-tests` passes in direct execution.
3. `ctest --preset macos-homebrew-ninja` passes.

Coverage currently includes:
1. Loader/validation/rollback/dependency/cycle behavior.
2. Core instruction semantics for implemented subset.
3. Runtime syscall bridge for alloc/retain/release/refcount/type-id and exception object creation.
4. ARC-sensitive `ST.D` managed object behavior.
5. Phase 4 bootstrap ordering/once-only/static-memory scenarios.
6. Phase 5 native import success/failure resolution.
7. Phase 5 inbound callback trampoline dispatch behavior.
8. `hoo_string_data` bridge-path behavior.

---

## 4. Known Limitations (Important)

Not yet complete for full guide-level Phase 5+ semantics:
1. No full ABI-specific trampoline lowering (complete SysV/Win64 register-class mapping and Windows shadow-space implementation details in generated call stubs).
2. Callback trampoline layer currently provides foundational single-argument slot-dispatch scaffolding; generalized signature marshalling is still pending.
3. No full structured try/catch/finally IR unwind model (`hoo_push_handler` + resume semantics).
4. No complete lexical ownership dataflow with cleanup-stack release insertion across all CFG exits.
5. Runtime concurrency stress/audit/debug modes from guide sections 7.17+ are not fully implemented.

---

## 5. Pending Roadmap

### 5.1 Remaining Phase 5 Work

1. Implement full ABI trampoline generation:
   - SysV/Win64 integer + FP register mapping
   - explicit Windows shadow-space handling
2. Expand callback marshalling:
   - multi-argument callback signatures
   - return-type and ownership-aware callback bridges
3. Add FFI boundary hardening:
   - explicit native guard policies
   - richer diagnostic surfaces for bridge failures

### 5.2 Phase 6+ Preview

1. Exception boundary and cleanup-stack completeness.
2. ARC ownership dataflow completeness.
3. Optimization/hardening and tooling/debugging milestones.

---

## 6. Definition of Done (Updated)

`HVMJIT` will be considered fully complete for Phase 5 when all are true:
1. Native import/library linkage behavior is deterministic and covered by unit/integration tests.
2. ABI trampoline generation is complete for supported host ABIs (including FP register classes).
3. Callback inbound trampoline behavior supports declared FFI callback signatures with validated marshalling.
4. Runtime string and core bridge paths are validated under unit + integration coverage.
