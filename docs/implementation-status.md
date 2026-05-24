# Hooc Implementation Status

This document tracks implementation status with emphasis on current HVM alignment.

Last Updated: 2026-05-24

## 1. High-Level Status

| Area | Status | Notes |
|---|---|---|
| Grammar (`src/parsing/Hooc.g4`) | Active | Current language source of truth |
| AST Builder | Active | Tracks grammar-level constructs; handles int64_t |
| HVM code generation path | Active | Primary executable path; pure RISC ISA |
| LLVM (JIT engine) | Active | Used only as JIT backend for HVM bytecode execution |
| HVM core spec/docs | Active | Hardware-ready profile documented (v1.4) |
| HVM module format (`.ho`) | Active | `HOModule` and `HO_FILE_FORMAT.md` aligned |
| HVM optional extensions | Documented | Non-core profiles in `HVM_EXTENSIONS.md` |
| HVMJIT Phase 4 bootstrap/init | Active | Implemented and unit-tested |
| HVMJIT Phase 5 FFI/linkage | Active (partial) | Native linkage + callback trampoline scaffolding implemented |

## 2. HVM Profile Status

Current HVM mode is **core-minimalest**.

### 2.1 Implemented as Core Contract

- Core register model (`r0..r31`, `r29` link register, `r1` return value)
- Base instruction formats + escape-prefixed extended opcode space
- Core instruction families documented in:
  - `docs/hvm/hvm-spec.md`
  - `docs/hvm/hvm_instruction_set.csv`
  - `docs/hvm/instructions.md`
- `.ho` binary format version `1.3` and parser/serializer constraints
- Symbol relocation via `SymbolFixup` for forward/recursive calls

### 2.2 Explicitly Excluded from Core

- SIMD/vector opcode families
- threading/atomic/TLS opcode families
- interrupt/system/debug opcode families
- string-specialized opcode families

These are optional-extension territory only.

### 2.3 Derived-Operation Policy (Active)

These remain lowering rules, not dedicated core opcodes:

- `SUBI`, `NEG`
- `CMPGT`, `CMPGE`
- `BGT`, `BGE`
- `MOVI` direct opcode form

## 3. Grammar-to-HVM Coverage Status

The current core profile is sufficient for grammar-defined semantics in:

- expressions and assignments (including 64-bit integer support)
- control flow (`if`, `while`, `for ... range ... by ...`, `break`, `continue`, `return`)
- object/array operations (including constructor calls and multi-dimensional arrays)
- exception handling (`try/catch/finally`, `throw`, `rethrow`)
- FFI declarations (`library`, `link dynamic`, `native`, `extern`) via runtime bridge opcodes

## 4. Runtime/OS Boundary Status

Current design uses a software runtime library for:

- heap management: `hoo_malloc`
- exception handling: `hoo_push_handler`, `hoo_throw`
- string operations: `hoo_string_*`
- system interaction: `SYSCALL` instruction

String-heavy behavior remains runtime-driven in core, not string-opcode driven.

## 5. Module Format Status

`HOModule` and `HO_FILE_FORMAT.md` are aligned on:

- 64-byte header
- 40-byte section entries
- little-endian requirement
- structured metadata sections (`symtab`, `reloc`, `export`, `import`, `funcmeta`, `strtab`)
- robust parse-time bounds/overflow validation

## 6. Known Gaps / Future Work

### 6.1 Core HVM

- Keep core stable; avoid opcode-set expansion without grammar-driven necessity.

### 6.2 Optional Profiles

- Any SIMD/threading/interrupt/debug growth should be staged as optional capability profiles with explicit gating and tests.

### 6.3 Tooling and Verification

- Maintain consistency checks across:
  - `HVM_SPEC.md`
  - `hvm_instruction_set.csv`
  - `hvm_register_set.csv`
  - instructions.md
  - grammar (`Hooc.g4`)

  ### 6.4 Register Management
  - Implement stack-based register spilling for extremely complex expression trees.
  - Add robust name mangling for cross-module symbol resolution.

### 6.5 HVMJIT Phase Status

- Phase 4: completed in current workspace, including post-load module init ordering, vtable init ordering, static section mapping, and once-only guards.
- Phase 5 (partial): implemented native/process symbol import resolution, dynamic library preload path, state-ABI runtime bridge dispatch for imported symbols, inbound callback trampoline slot dispatcher, and `hoo_string_data` bridge path.
- Remaining Phase 5 work: full ABI trampoline specialization (SysV/Win64 register-class mapping incl. floating-point/XMM and Windows shadow space), richer marshalling contracts, and hardened FFI safety guards.

## 7. Source-of-Truth Index

- Grammar: `src/parsing/Hooc.g4`
- HVM spec: `docs/hvm/hvm-spec.md`
- Opcode list: `docs/hvm/hvm_instruction_set.csv`
- Register set: `docs/hvm/hvm_register_set.csv`
- Instruction reference: `docs/hvm/instructions.md`
- Optional profiles: `docs/hvm/hvm-extensions.md`
- Module format: `docs/hvm/ho-file-format.md`
