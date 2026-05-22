# Hooc Virtual Machine (HVM) Specification

This is the **minimal core HVM profile** required to implement the current Hooc grammar in `src/parsing/Hooc.g4`.

## 1. Scope

This profile is intentionally limited to language features that are currently present in the grammar:
- declarations (`func`, `class`, `var`, `const`)
- expressions (arithmetic, logical, relational, assignment)
- control flow (`if`, `while`, `for in .. by`, `break`, `continue`, `return`)
- object/array usage (`new`, member/index access)
- exceptions (`try/catch/finally`, `throw`, `rethrow`)
- FFI declarations (`library`, `link dynamic`, `native`, `extern`)

Anything not required by the grammar (SIMD, threading, interrupts, debugging opcodes, etc.) is excluded from this core profile.

## 2. Execution Model

- 64-bit register machine
- byte-addressable little-endian memory
- downward-growing stack
- object/array heap managed by runtime
- 32-bit base instruction words with escape-prefixed extended opcodes

## 3. Register Set

The core profile uses 32 general-purpose 64-bit registers (`r0..r31`):
- `r0`: hardwired zero
- `r1..r8`: argument registers, `r1` is return-value register
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`lr`, return address)
- `r30`: frame pointer
- `r31`: stack pointer

(See `docs/hvm/hvm_register_set.csv`.)

## 4. Encoding

Base instruction formats:
- `R`: `opcode[7] rd[5] rs1[5] rs2[5] func[10]`
- `I`: `opcode[7] rd[5] rs1[5] imm[15]`
- `RI`: `opcode[7] rd[5] rs1[5] rs2[5] imm[10]`
- `B`: `opcode[7] rs1[5] rs2[5] imm[15]`
- `J`: `opcode[7] rd[5] imm[20]`

Extended opcode encoding:
- opcodes `0x00..0x7F` use the base 32-bit formats above
- opcodes `>= 0x100` (for example `TRY`, `THROW`, `CALLHOST`) use an escape-prefixed encoding in the instruction stream
- this keeps the core register/immediate semantics while allowing the extended numeric opcode space used by `hvm_instruction_set.csv`

Immediate rules:
- immediates are sign-extended unless documented otherwise (`MOVZ`)
- branch/jump immediates are instruction offsets: `pc += sign_extend(imm) * 4`

## 5. Minimal Instruction Set

The normative list is `docs/hvm/hvm_instruction_set.csv`.

### 5.1 Required Families (`core-minimalest`)

- Data movement: `NOP`, `MOV`, `MOVZ`, `LUI`, `ADDI`
- Integer arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`, `SHL`, `SHR`, `SAR`
- Bitwise/logical: `AND`, `OR`, `XOR`, `NOT`
- Floating-point (required by grammar numeric types): `FADD`, `FSUB`, `FMUL`, `FDIV`
- Comparisons: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
- Branch/jump: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- Memory: `LD.B`, `LD.BU`, `LD.H`, `LD.HU`, `LD.W`, `LD.WU`, `LD.D`, `ST.B`, `ST.H`, `ST.W`, `ST.D`, `LDA`
- Stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- Objects/arrays: `NEW`, `NEWA`, `LDF`, `STF`, `LDELEM`, `STELEM`, `ARRAYLEN`
- Calls/linking: `CALL`, `CALLI`, `TAILCALL`
- Exceptions: `TRY`, `THROW`, `CATCH`, `FINALLY`, `RETHROW`, `ENDFIN`
- FFI/runtime bridge: `CALLHOST`, `CALLNATIVE`, `LOADLIB`, `GETSYM`

### 5.2 Not In Core

The following are intentionally excluded from the minimal grammar-driven profile:
- SIMD/vector instructions
- thread/atomic/TLS instructions
- interrupt/system/debug instructions
- extended string-specialized opcode family

String operations are supported via runtime/library calls in this core profile, not dedicated string opcodes.

### 5.3 Derived Operations (Lowering Rules)

- `SUBI rd, rs, imm` -> `ADDI rd, rs, -imm`
- `NEG rd, rs` -> `SUB rd, r0, rs`
- `CMPGT rd, a, b` -> `CMPLT rd, b, a`
- `CMPGE rd, a, b` -> `CMPLE rd, b, a`
- `BGT a, b, off` -> `BLT b, a, off`
- `BGE a, b, off` -> `BLE b, a, off`
- `MOVI` is intentionally omitted; immediates are formed with `MOVZ`/`LUI` plus arithmetic/logic as needed.

## 6. Mapping to Current Grammar

- `newExpression` -> `NEW`/`CALL` constructor path
- member/index access -> `LDF`/`STF`, `LDELEM`/`STELEM`
- `for ... in ... range ... by ...` -> compare + branch + add/sub in lowered IR
- compound assignments -> arithmetic + store
- `try/catch/finally` and `throw/rethrow` -> exception family (`TRY`..`ENDFIN`)
- FFI grammar constructs -> loader/symbol/runtime bridge (`LOADLIB`, `GETSYM`, `CALLNATIVE`, `CALLHOST`)

## 7. Notes

- This spec is a **core profile**, not a superset architecture catalog.
- If grammar evolves (for example native threading syntax or SIMD syntax), new instruction families can be added as optional profiles.
