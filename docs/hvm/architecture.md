# Hooc Virtual Machine (HVM) Architecture

This document describes the current HVM architecture as defined by:

- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`

The active profile is **core-minimalest**.

## 1. Architectural Scope

HVM is a register-based VM profile intentionally limited to what the current grammar requires.

Supported language surface:

- declarations (`func`, `class`, `var`, `const`)
- expressions (arithmetic, logical, relational, assignment)
- control flow (`if`, `while`, `for in .. by`, `break`, `continue`, `return`)
- object/array operations (`new`, member/index access)
- exceptions (`try/catch/finally`, `throw`, `rethrow`)
- FFI declarations (`library`, `link dynamic`, `native`, `extern`)

Explicitly out of core:

- SIMD/vector families
- threading/atomic/TLS families
- interrupt/system/debug families
- dedicated string opcode families

## 2. Execution Model

- 64-bit register machine
- byte-addressable little-endian memory
- downward-growing stack
- runtime-managed object/array heap
- 32-bit base instruction words plus escape-prefixed extended opcode space

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
- `>= 0x100`: escape-prefixed extended path (used in core for exception/FFI opcodes)

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
- Objects/arrays: `NEW`, `NEWA`, `LDF`, `STF`, `LDELEM`, `STELEM`, `ARRAYLEN`
- Calls/linking: `CALL`, `CALLI`, `TAILCALL`
- Exceptions: `TRY`, `THROW`, `CATCH`, `FINALLY`, `RETHROW`, `ENDFIN`
- FFI/runtime bridge: `CALLHOST`, `CALLNATIVE`, `LOADLIB`, `GETSYM`

## 6. Minimality Rules (Derived Operations)

Certain operations are not first-class opcodes and must be lowered:

- `SUBI` -> `ADDI` with negated immediate
- `NEG` -> `SUB rd, r0, rs`
- `CMPGT/CMPGE` -> swapped `CMPLT/CMPLE`
- `BGT/BGE` -> swapped `BLT/BLE`
- `MOVI` synthesized via `MOVZ`/`LUI` (+ arithmetic/logic)

## 7. Runtime and FFI Boundary

Core profile deliberately pushes non-essential specialization to runtime/library calls:

- string-heavy behavior via runtime helpers, not string-specific opcodes
- host/native interop via `CALLHOST`, `CALLNATIVE`, `LOADLIB`, `GETSYM`

This keeps ISA small while preserving language coverage.

## 8. Relationship to Module Format

HVM module/container details are defined separately:

- `docs/hvm/HO_FILE_FORMAT.md`
- `src/hvm/HoModule.h`
- `src/hvm/HoModule.cpp`

Architecture and module format must evolve together when opcode-space, metadata, or calling-convention semantics change.

## 9. Evolution Policy

If new grammar features require new instruction families:

1. keep core unchanged unless required
2. add optional profile in `docs/hvm/HVM_EXTENSIONS.md`
3. version/gate capability explicitly
4. add compatibility tests before promoting to core
