# Hooc Development Roadmap

Last Updated: 2026-05-22

This roadmap reflects the current HVM **core-minimalest** direction and current grammar surface.

## 1. Strategic Direction

Primary near-term goals:

1. Keep language/compiler path stable and testable
2. Keep HVM core minimal and grammar-driven
3. Complete practical `.hoo -> .ho -> run` workflows
4. Grow standard library/runtime through modules and FFI without inflating core ISA

## 2. Current Baseline

- Grammar and parser are active (`src/parsing/Hooc.g4`)
- AST builder and LLVM codegen are active
- HVM documentation has been consolidated around:
  - `docs/hvm/HVM_SPEC.md`
  - `docs/hvm/hvm_instruction_set.csv`
  - `docs/hvm/hvm_register_set.csv`
  - `docs/hvm/instructions.md`
  - `docs/hvm/HO_FILE_FORMAT.md`
- HVM optional families are now explicitly separated into extension profiles

## 3. Phase Plan

## Phase A: Stability and Consistency (Now)

- keep grammar/AST/codegen/docs synchronized
- enforce doc consistency sweeps across HVM docs
- lock core ISA minimality (no implicit family expansion)

Exit criteria:

- no internal contradictions across HVM spec/CSV/reference docs
- regression tests green for parser/AST/codegen/HVM module subsystems

## Phase B: AOT Pipeline Completion

- make `.hoo -> .ho` build flow fully operational
- make `.ho` execution path first-class and documented
- tighten module loading/linking behavior for multi-module runs

Exit criteria:

- end-to-end AOT scenarios covered by integration tests
- predictable CLI UX for build vs run modes

## Phase C: Runtime and Standard Library Expansion

- deepen `hoo.io` file/directory/path features
- mature `hoo.math` and `hoo.net` APIs
- continue runtime-backed functionality (string-heavy behavior, host/native bridges)

Exit criteria:

- documented, tested APIs with practical examples
- stable behavior across supported platforms

## Phase D: Optional HVM Extension Profiles

- add non-core families only as gated profiles
- document each extension profile and compatibility constraints
- add profile-specific tests and capability detection

Exit criteria:

- core remains minimal and unchanged
- extensions are opt-in and versioned

## 4. Non-Goals for Core HVM

The following are not part of core unless grammar evolution mandates them:

- SIMD/vector opcode families
- threading/atomic/TLS opcode families
- interrupt/system/debug opcode families
- string-specialized opcode families

## 5. Contributor Priorities

High-value contribution areas:

1. AOT pipeline and module-linking integration tests
2. Standard-library module implementations (`hoo.io`, `hoo.math`, `hoo.net`)
3. Documentation consistency and tooling checks
4. Parser/AST/codegen gap tests for grammar evolution

## 6. Success Metrics

- stable core ISA boundary over time
- reduction in doc drift regressions
- reliable end-to-end build/run workflows for both JIT and AOT paths
- increased integration-test coverage for module and FFI flows
