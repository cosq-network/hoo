# Hooc Language Elements and HVM Mapping

This document lists Hooc language elements by compilation granularity and maps them to the current HVM core profile.

Normative references:

- `src/parsing/Hooc.g4`
- `docs/hvm/hvm-spec.md`
- `docs/hvm/hvm_instruction_set.csv`

## 1. Compilation Tiers

## Tier 1: Atomic Expressions

- integer/float/string/multiline string/char/bool/null literals
- identifiers

Typical HVM mapping:

- literal materialization via `MOVZ`/`LUI`/`ADDI` patterns
- identifier loads via `LD.*` or frame/global addressing

## Tier 2: Operator Expressions

- unary: `-`, `!`
- binary arithmetic/logical/relational

Typical HVM mapping:

- arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`
- bit/logical: `AND`, `OR`, `XOR`, `NOT`
- compare: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
- derived comparisons lowered (no dedicated `CMPGT/CMPGE`)

## Tier 3: Postfix and Access Expressions

- member access, index access, function/method call
- array literals
- `new` expressions

Typical HVM mapping:

- object/array: `NEW`, `NEWA`, `LDF`, `STF`, `LDELEM`, `STELEM`, `ARRAYLEN`
- calls: `CALL`, `CALLI`, `TAILCALL`

## Tier 4: Statement-Level Constructs

- variable/constant declarations
- assignment and compound assignment
- expression statements
- `return`, `break`, `continue`
- `if`, `while`, `for`, `scope`
- `try/catch/finally`, `throw`, `rethrow`

Typical HVM mapping:

- control flow: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- exceptions: `TRY`, `THROW`, `CATCH`, `FINALLY`, `RETHROW`, `ENDFIN`

## Tier 5: Top-Level Module Elements

- imports
- function declarations
- class declarations (with modifiers, constructor, methods)
- module-level variables/constants
- FFI declarations

Typical HVM/runtime mapping:

- import semantics resolved through module/runtime linker path
- FFI declarations mapped through `LOADLIB`, `GETSYM`, `CALLNATIVE`, `CALLHOST`

## 2. HVM Mapping Matrix (Core-Minimalest)

Legend:

- `Yes`: directly covered by core opcodes + standard lowering
- `Runtime`: requires runtime/library behavior in addition to opcodes
- `Metadata`: structural/declarative element; not itself executable opcode stream

| Element Group | HVM Core Coverage | Notes |
|---|---|---|
| Literals and arithmetic expressions | Yes | Uses core ALU + immediate construction |
| Relational/logical expressions | Yes | Uses compare/branch core forms |
| Compound assignments | Yes | Lowered through arithmetic + store |
| Postfix `++/--` | Yes | Lowered increment/decrement sequence |
| Function calls | Yes | `CALL`/`CALLI`/`TAILCALL` |
| Objects and arrays | Yes | `NEW`/`NEWA`/field/element ops |
| Control flow | Yes | branch/jump core set |
| Exceptions | Yes | exception core family present |
| FFI declarations | Runtime | bridged by core FFI opcodes + runtime |
| Imports and module structure | Metadata/Runtime | resolved at compile/link/load boundaries |

## 3. Derived Operations Policy

These operations are intentionally absent as dedicated core opcodes and must be lowered:

- `SUBI` -> `ADDI rd, rs, -imm`
- `NEG` -> `SUB rd, r0, rs`
- `CMPGT/CMPGE` -> swapped `CMPLT/CMPLE`
- `BGT/BGE` -> swapped `BLT/BLE`
- `MOVI` synthesized via `MOVZ`/`LUI` (+ arithmetic/logic)

## 4. Explicitly Out of Core

Not required for the current grammar and excluded from core ISA:

- SIMD/vector opcode families
- threading/atomic/TLS opcode families
- interrupt/system/debug opcode families
- dedicated string opcode families

If needed later, these should be introduced as optional profiles, not added implicitly to core.
