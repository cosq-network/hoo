# Hooc Virtual Machine (HVM) Architecture

This document describes the current HVM architecture as defined by:

- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`

The active profile is **core-minimalest** (Hardware Ready).

## 1. Architectural Scope

HVM is a pure register-based RISC architecture. It is intentionally limited to physical CPU instructions. High-level language constructs are lowered by the compiler to standard memory operations and function calls.

Supported language surface (via lowering):

- declarations (`func`, `class`, `var`, `const`)
- expressions (arithmetic, logical, relational, assignment)
- control flow (`if`, `while`, `for in .. by`, `break`, `continue`, `return`)
- object/array operations (lowered to `hoo_malloc` + pointer arithmetic)
- exceptions (lowered to `hoo_push_handler` + control flow)
- FFI declarations (lowered to standard `CALL`s to external symbols)

## 2. Execution Model

- 64-bit register machine
- byte-addressable little-endian memory
- downward-growing stack
- physical memory model (software-managed heap)
- 32-bit base instruction words plus escape-prefixed extended opcode space for ops >= 0x80

## 3. Register Architecture

32 general-purpose 64-bit registers (`r0..r31`):

- `r0`: hardwired zero
- `r1..r8`: argument registers (`r1` also return value)
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`RET` jumps to `r29`)
- `r30`: frame pointer
- `r31`: stack pointer

## 4. Instruction Encoding Model

Base formats:

- `R`: `opcode[7] rd[5] rs1[5] rs2[5] func[10]`
- `I`: `opcode[7] rd[5] rs1[5] imm[15]`
- `RI`: `opcode[7] rd[5] rs1[5] rs2[5] imm[10]`
- `B`: `opcode[7] rs1[5] rs2[5] imm[15]`
- `J`: `opcode[7] rd[5] imm[20]`

Opcode-space behavior:

- `0x00..0x7F`: base 32-bit format path
- `>= 0x80`: escape-prefixed extended path (`0xFE` prefix)

Immediates/branches:

- immediates are sign-extended unless documented otherwise
- branch/jump offsets are instruction offsets (`pc += sign_extend(imm) * 4`)

## 5. Core Instruction Families

The canonical full opcode list is `docs/hvm/hvm_instruction_set.csv`.

Family summary:

- Data movement: `NOP`, `MOV`, `MOVZ`, `LUI`, `ADDI`
- Integer arithmetic/shift: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`, `SHL`, `SHR`, `SAR`
- Bitwise/logical: `AND`, `OR`, `XOR`, `NOT`
- Floating-point: `FADD`, `FSUB`, `FMUL`, `FDIV`
- Comparisons: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
- Control transfer: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- Memory: `LD.*`, `ST.*`, `LDA`
- Stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- Calls: `CALL`, `CALLI`, `TAILCALL`
- Hardware/System: `SYSCALL`, `BREAK`

## 6. Minimality Rules (Lowering)

Operations that are **NOT** first-class opcodes:

- `SUBI` -> `ADDI rd, rs, -imm`
- `NEG` -> `SUB rd, r0, rs`
- `NEW/NEWA` -> `CALL hoo_malloc`
- `LDF/STF` -> Pointer arithmetic + `LD.D/ST.D`
- `TRY/THROW` -> `CALL hoo_push_handler` + control flow

## 7. Runtime and OS Boundary

HVM relies on a software runtime library for:

- Memory allocation (`hoo_malloc`)
- Exception management (`hoo_push_handler`, `hoo_throw`)
- String operations (`hoo_string_*`)
- System interaction (`SYSCALL`)

## 8. Relationship to Module Format

HVM module/container details are defined separately in `docs/hvm/HO_FILE_FORMAT.md`.

## 9. Evolution Policy

If new language features require hardware support:

1. Prioritize software lowering to existing RISC instructions.
2. Add optional profiles in `docs/hvm/HVM_EXTENSIONS.md` only if performance or security necessitates silicon-level implementation.
