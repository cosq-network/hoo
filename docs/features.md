# Hooc Language Features Guide

This guide describes the current Hooc language surface and how it maps to the active HVM core profile.

Normative references:

- Grammar: `src/parsing/Hooc.g4`
- HVM core profile: `docs/hvm/hvm-spec.md`
- HVM opcode list: `docs/hvm/hvm_instruction_set.csv`

## 1. Core Language Features

### 1.1 Types

Primitive types:

- `int8`, `byte`, `int64`
- `float`, `double`, `f64`
- `bool`, `char`, `string`, `void`

Composite and advanced:

- arrays: `T[]`, `T[][]`, ...
- nullable types: `T?`
- map types: `map[keyType, valueType]` with key in `{byte, int8, int64, char, string}`
- qualified identifiers for type names (`module.Type`)

### 1.2 Declarations

- functions (`func`)
- classes (`class`, `extends`, `constructor`)
- variables (`var`)
- constants (`const`)
- imports (`import`, `from ... import ... as ...`)

### 1.3 Expressions and Operators

Supported expression families:

- arithmetic: `+ - * / %`
- relational: `== != < <= > >=`
- logical: `&& || !`
- assignment: `=`
- compound assignment: `+= -= *= /= %= <<= >>=`
- postfix: member access (`.`), indexing (`[]`), calls (`(...)`), `++`, `--`
- literals: integer, float, string, multiline string, char, bool, null, array literals
- object construction: `new QualifiedType(args...)`

### 1.4 Control Flow and Exceptions

- `if` / `else`
- `while`
- `for` in-expression, with optional range (`..`) and optional `by`
- `break`, `continue`, `return`
- `scope` block
- exceptions: `try`, `catch`, `finally`, `throw`, `rethrow`

### 1.5 FFI Surface

- library declaration: `library "..."`
- dynamic link declaration: `link dynamic modulePath ...`
- native/extern declarations: `native`, `extern native`
- FFI types: primitive, qualified type, pointer, fixed-size array, function type

## 2. HVM Mapping (Current Core Profile)

The language is mapped to **core-minimalest** HVM.

### 2.1 Included Core Families (Hardware Ready)

- data movement: `NOP`, `MOV`, `MOVZ`, `LUI`, `ADDI`
- integer arithmetic/shift: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`, `SHL`, `SHR`, `SAR`
- bitwise/logical: `AND`, `OR`, `XOR`, `NOT`
- floating-point: `FADD`, `FSUB`, `FMUL`, `FDIV`
- comparisons: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
- branches/jumps: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- memory: `LD.*`, `ST.*`, `LDA`
- stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- calls: `CALL`, `CALLI`, `TAILCALL`
- hardware/system: `SYSCALL`, `BREAK`

### 2.2 Lowering Rules (Compiler Implemented)

The following high-level language constructs are lowered to the core ISA:

- **Objects**: `new Point()` -> `CALL hoo_malloc` + `CALL Point_init`.
- **Arrays**: `[1, 2]` -> `CALL hoo_malloc` + `ST.D` sequence.
- **Member/Index Access**: `p.x` or `arr[i]` -> pointer arithmetic + `LD.D`/`ST.D`.
- **Exceptions**: `try/catch` -> `CALL hoo_push_handler` + control flow.
- **Strings**: `"..."` -> `CALL hoo_string_from_cstr`.

### 2.3 Not in Core

Intentionally excluded from the current HVM core profile:

- SIMD/vector families
- threading/atomics/TLS families
- interrupt/system/debug families
- string-specialized opcode family

String-heavy behavior is runtime/library driven in core.

## 3. Register and Calling Conventions (HVM Core)

- `r0`: constant zero
- `r1..r8`: argument registers (`r1` is also return value)
- `r9..r15`: caller-saved
- `r16..r28`: callee-saved
- `r29`: link register used by `RET`
- `r30`: frame pointer
- `r31`: stack pointer

## 4. Status Notes

- Language features listed here are grammar-level supported syntax.
- Backend/runtime completeness can vary by execution path.
- For current HVM scope decisions, always defer to `docs/hvm/hvm-spec.md`.
