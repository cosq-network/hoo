# Hooc Architecture

This document summarizes the current compiler/runtime/VM architecture after the HVM core-profile reduction in `docs/hvm/HVM_SPEC.md`.

## 1. System Overview

Hooc has a single execution path:

- Source (`.hoo`) -> ANTLR parser -> typed AST -> HVM bytecode -> JIT execution (via LLVM ORC JIT)
- AOT path: `.hoo` -> HVM bytecode -> `.ho` binary module -> load and run via HVMJIT loader

## 2. Compiler Pipeline (Current)

Core components:

- Parser: `src/parsing/Hooc.g4`, generated parser in `build/<preset>/generated/antlr4/`
- AST builder: `src/ast/SimpleASTBuilder.*`
- Code generation: `src/codegen/HVMCodeGenerator.*`
- Compiler orchestration: `src/core/HooCompiler.*`
- JIT: `src/hvm/HVMJIT.*`

Pipeline:

1. Parse source with ANTLR (`ProcessIsolatedParser` in `src/parsing/`)
2. Build typed AST (`SimpleASTBuilder`)
3. Generate HVM bytecode (`HVMCodeGenerator`)
4. JIT compile and execute via LLVM ORC JIT (`HVMJIT`)
5. Alternatively serialize to `.ho` module format (`HOModule`) for AOT

## 3. HVM Architectural Profile (Current Core)

The active HVM profile is `core-minimalest`:

- 64-bit register machine
- 32 GPRs (`r0..r31`)
- little-endian memory model
- 32-bit base instruction formats plus escape-prefixed extended opcode space for opcodes `>= 0x100`
- Symbol relocation via `SymbolFixup` for forward-referenced `CALL` targets.

Register convention:

- `r0` zero
- `r1..r8` args (`r1` return value)
- `r9..r15` caller-saved
- `r16..r28` callee-saved
- `r29` link register (`RET` target)
- `r30` frame pointer
- `r31` stack pointer

Normative sources:

- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`
- `docs/hvm/instructions.md`

## 4. Grammar-Driven VM Surface

The ISA surface is intentionally limited to language constructs present in `src/parsing/Hooc.g4`:

- declarations: `func`, `class`, `var`, `const`
- expressions: arithmetic/logical/relational/assignment
- control flow: `if`, `while`, `for in .. range .. by`, `break`, `continue`, `return`
- object/array usage: `new`, member/index access
- exceptions: `try/catch/finally`, `throw`, `rethrow`
- FFI declarations: `library`, `link dynamic`, `native`, `extern`

Not in core:

- SIMD/vector instruction families
- threading/atomics/TLS instruction families
- interrupt/system/debug instruction families
- string-specialized opcode family (strings handled via runtime/library bridge)

## 5. HVM Module Format Boundary

Binary module handling is defined by:

- `src/hvm/HOModule.h`
- `src/hvm/HOModule.cpp`
- `docs/hvm/HO_FILE_FORMAT.md`

Key points:

- `.ho` format version `1.3`
- little-endian only
- fixed 64-byte header
- 40-byte section entries
- metadata sections serialized from structured module state (symbol/import/export/reloc/funcmeta)

## 6. Runtime Boundary

Runtime responsibilities split into:

1. Core runtime services (memory/object/array/string/host runtime functions)
2. FFI bridge via SYSCALL opcode with runtime-resolved function addresses

The core ISA intentionally relies on runtime/library functions instead of embedding many specialized instruction families.

## 7. Validation Strategy

The AST construction phase (`SimpleASTBuilder`) implements strict validation. Rather than using silent fallbacks or logging to `stderr`, the builder now throws `std::runtime_error` for any structural or literal anomalies:

- **Literal Parsing**: Integer/Floating/Character literals must conform strictly to expected formats; parsing failures trigger immediate exceptions. Integers are handled as `int64_t` to ensure high precision across backends.
- **Type/Modifier Safety**: Unrecognized primitive types, class modifiers, or function modifiers result in hard failures.
- **Structural Integrity**: Malformed control flow constructs (e.g., loops missing required components) are caught during AST building.

This strategy ensures that errors are caught early and never propagate silently into the code generation or execution phases.

## 8. Lowering Rules and Minimality

Some operations are intentionally absent as dedicated opcodes and are lowered:

- `SUBI` -> `ADDI` with negated immediate
- `NEG` -> `SUB rd, r0, rs`
- `CMPGT`/`CMPGE` -> operand-swapped `CMPLT`/`CMPLE`
- `BGT`/`BGE` -> operand-swapped `BLT`/`BLE`
- `MOVI` synthesized via `MOVZ`/`LUI` (or `.rodata` spill + `LD.D` for >15 bit values)

This keeps the ISA minimal while preserving full grammar coverage.

## 9. Change Control

When grammar evolves, update in this order:

1. `src/parsing/Hooc.g4`
2. AST (`src/ast/*`)
3. lowering/codegen
4. `docs/hvm/HVM_SPEC.md`
5. `docs/hvm/hvm_instruction_set.csv`
6. `docs/hvm/instructions.md`
7. `docs/hvm/HO_FILE_FORMAT.md` and `src/hvm/HOModule*` if binary/module semantics change

Optional families should go to `docs/hvm/HVM_EXTENSIONS.md`, not the core profile.
