# HVM Core Instruction Reference

Version: `1.4`  
Profile: `core-minimalest` (Hardware Ready)  
Normative sources:
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/HVM_SPEC.md`

This reference defines a **pure hardware-ready ISA**. All high-level VM constructs have been purged and are now handled via software lowering or standard library calls.

## 1. Scope

This reference defines the physical instructions supported by the HVM core. It is sufficient to support the Hooc language through aggressive compiler-level lowering.

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
- Opcodes `>= 0x80` use an escape-prefixed extended encoding (`0xFE` prefix).

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

### 4.7 Memory (Standard Load/Store)
- Loads: `LD.B` `LD.BU` `LD.H` `LD.HU` `LD.W` `LD.WU` `LD.D`
- Stores: `ST.B` `ST.H` `ST.W` `ST.D`
- Address: `LDA`

### 4.8 Stack/frame
- `PUSH` `POP` `ENTER` `LEAVE` `ADJSP` `FRAME`

### 4.9 Calls/linking
- `CALL` `TAILCALL` (J-format, 20-bit relative offset)

### 4.10 Hardware/System
- `SYSCALL`: Trigger a system call to the OS/Runtime.
- `BREAK`: Trap to debugger.

## 5. Lowering Rules (Software Implemented)

Operations removed from the ISA and now lowered to the above set:

- **Objects**: `NEW` -> `CALL hoo_malloc`.
- **Arrays**: `NEWA` -> `CALL hoo_malloc`.
- **Field/Element Access**: `LDF`/`STF`/`LDELEM` -> Explicit pointer arithmetic + `LD.D`/`ST.D`.
- **Exceptions**: `TRY`/`THROW` -> `CALL hoo_push_handler` + control flow.
- **FFI**: `CALLHOST` -> Standard `CALL` to linked symbol.

## 6. Canonical Opcode Table

The canonical machine-readable definition is `docs/hvm/hvm_instruction_set.csv`.
Keep this document synchronized with that CSV and `docs/hvm/HVM_SPEC.md`.
