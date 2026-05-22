# HVM Core Instruction Reference

Version: `1.3`  
Profile: `core-minimalest`  
Normative sources:
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/HVM_SPEC.md`

This document intentionally covers only the current core profile. Optional/extended families are documented separately in `docs/hvm/HVM_EXTENSIONS.md`.

## 1. Scope

This reference matches the minimal ISA needed for the current grammar in `src/parsing/Hooc.g4`:
- expressions and assignments
- control flow
- memory and stack/frame
- object/array operations
- calls, exceptions, and FFI bridge

## 2. Register Convention Summary

- `r0`: hardwired zero
- `r1..r8`: argument registers (`r1` also return value)
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`RET` target)
- `r30`: frame pointer
- `r31`: stack pointer

## 3. Instruction Formats

- `R`: `opcode[7] rd[5] rs1[5] rs2[5] func[10]`
- `I`: `opcode[7] rd[5] rs1[5] imm[15]`
- `RI`: `opcode[7] rd[5] rs1[5] rs2[5] imm[10]`
- `B`: `opcode[7] rs1[5] rs2[5] imm[15]`
- `J`: `opcode[7] rd[5] imm[20]`

Opcode-space note:
- `0x00..0x7F` use the base 32-bit encodings directly.
- Core opcodes `>= 0x100` (exception/FFI entries such as `TRY`, `THROW`, `CALLHOST`) are represented via an escape-prefixed extended encoding as defined by `HVM_SPEC.md`.

Immediate and branch semantics follow `docs/hvm/HVM_SPEC.md`.

## 4. Core Instruction Set

### 4.1 Data movement

- `NOP` `MOV` `MOVZ` `LUI` `ADDI`

### 4.2 Integer arithmetic and shifts

- `ADD` `SUB` `MUL` `DIV` `DIVU` `REM`
- `SHL` `SHR` `SAR`

### 4.3 Bitwise/logical

- `AND` `OR` `XOR` `NOT`

### 4.4 Floating point

- `FADD` `FSUB` `FMUL` `FDIV`

### 4.5 Comparisons

- Integer: `CMPEQ` `CMPNE` `CMPLT` `CMPLE`
- Float: `FCMPEQ` `FCMPLT` `FCMPLE`

### 4.6 Branch/jump

- `BEQ` `BNE` `BLT` `BLE`
- `JMP` `JAL` `JALR` `RET`

### 4.7 Memory

- Loads: `LD.B` `LD.BU` `LD.H` `LD.HU` `LD.W` `LD.WU` `LD.D`
- Stores: `ST.B` `ST.H` `ST.W` `ST.D`
- Address: `LDA`

### 4.8 Stack/frame

- `PUSH` `POP` `ENTER` `LEAVE` `ADJSP` `FRAME`

### 4.9 Objects/arrays

- `NEW` `NEWA`
- `LDF` `STF`
- `LDELEM` `STELEM`
- `ARRAYLEN`

### 4.10 Calls/linking

- `CALL` `CALLI` `TAILCALL`

### 4.11 Exceptions

- `TRY` `THROW` `CATCH` `FINALLY` `RETHROW` `ENDFIN`

### 4.12 FFI/runtime bridge

- `CALLHOST` `CALLNATIVE` `LOADLIB` `GETSYM`

## 5. Derived Operations (Not Separate Opcodes)

Lowering rules:

- `SUBI rd, rs, imm` -> `ADDI rd, rs, -imm`
- `NEG rd, rs` -> `SUB rd, r0, rs`
- `CMPGT rd, a, b` -> `CMPLT rd, b, a`
- `CMPGE rd, a, b` -> `CMPLE rd, b, a`
- `BGT a, b, off` -> `BLT b, a, off`
- `BGE a, b, off` -> `BLE b, a, off`
- `MOVI` omitted; build constants via `MOVZ`/`LUI` (+ arithmetic/logic as needed)

## 6. Canonical Opcode Table

The canonical machine-readable definition is `docs/hvm/hvm_instruction_set.csv`.
Keep this document synchronized with that CSV and `docs/hvm/HVM_SPEC.md`.
