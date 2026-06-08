# Hooc Virtual Machine (HVM) Specification

This is the **minimal core HVM profile** required to implement the current Hooc grammar in `src/parsing/Hooc.g4`. This profile is designed for **physical hardware compatibility**, strictly using low-level RISC instructions.

## 1. Scope

This profile is limited to the physical ISA required to support Hooc. All high-level language constructs (objects, arrays, exceptions) are lowered to standard memory operations and function calls to a runtime library.

Anything not required by a pure RISC core (SIMD, threading, interrupts, specialized VM opcodes) is excluded or handled via system calls.

## 2. Execution Model

- 64-bit register machine
- byte-addressable little-endian memory
- downward-growing stack
- flat physical memory model (paged/protected via MMU if present)
- 32-bit base instruction words with escape-prefixed extended opcodes for ops >= 0x80
- Extended (>= 0x80) opcodes use an 8-byte encoding: escape byte `0xFE`, ULEB128-encoded opcode,
  zero-padding to offset 4, then the 32-bit base-format payload (R/I/RI/B/J). The payload
  encoding is identical to base-format instructions but the opcode field in the payload is ignored
  (the opcode is taken from the ULEB128 value).

## 3. Register Set

The core profile uses 32 general-purpose 64-bit registers (`r0..r31`):
- `r0`: hardwired zero
- `r1..r8`: argument registers, `r1` is return-value register
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`lr`, return address)
- `r30`: frame pointer (`fp`)
- `r31`: stack pointer (`sp`)

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
- opcodes `>= 0x80` use an escape-prefixed encoding (`0xFE` prefix)
- branch/jump immediates are instruction offsets: `pc += sign_extend(imm) * 4`

## 5. Minimal Instruction Set

The normative list is `docs/hvm/hvm_instruction_set.csv`.

### 5.1 Required Families (`core-minimalest`)

- Data movement: `NOP`, `MOV`, `MOVZ`, `LUI`, `ADDI`
- Integer arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`, `SHL`, `SHR`, `SAR`
- Bitwise/logical: `AND`, `OR`, `XOR`, `NOT`
- Floating-point: `FADD`, `FSUB`, `FMUL`, `FDIV`
- Comparisons: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
- Branch/jump: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- Memory: `LD.B`, `LD.BU`, `LD.H`, `LD.HU`, `LD.W`, `LD.WU`, `LD.D`, `ST.B`, `ST.H`, `ST.W`, `ST.D`, `LDA`
- Stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- Calls/linking: `CALL`, `TAILCALL`
- Hardware/System: `SYSCALL`, `BREAK`

### 5.2 Explicitly Excluded from Core (Lowered to Software)

The following are **NOT** in the ISA and must be lowered by the compiler:
- `NEW`/`NEWA`: Lowered to `CALL hoo_alloc`.
- `LDF`/`STF`: Lowered to `ADDI` + `LD.D`/`ST.D`.
- `LDELEM`/`STELEM`: Lowered to pointer arithmetic + `LD.D`/`ST.D`.
- `TRY`/`CATCH`/`THROW`: Lowered to control flow + `CALL hoo_push_handler`.

## 6. Mapping to Current Grammar

- `newExpression` -> `CALL hoo_alloc` + `CALL` constructor.
- member/index access -> explicit pointer arithmetic + `LD.D`/`ST.D`.
- `try/catch/finally` -> `CALL hoo_push_handler` + conditional control flow.

## 7. System Call (SYSCALL) Interface

`SYSCALL` dispatches to internal runtime services via the immediate field `imm15`:

| imm15 | Name           | Operation                              | Reads Register | Writes Register |
|-------|----------------|----------------------------------------|----------------|-----------------|
| 1     | `kSysAlloc`    | `rd = hoo_alloc(r2, r3)`              | r2 (size), r3 (typeId) | rd |
| 2     | `kSysRetain`   | `rd = hoo_retain(r2)`                 | r2 (object)   | rd |
| 3     | `kSysRelease`  | `rd = hoo_release(r2)`                | r2 (object)   | rd |
| 4     | `kSysRefcount` | `rd = hoo_refcount(r2)`               | r2 (object)   | rd |
| 5     | `kSysTypeId`   | `rd = hoo_typeid(r2)`                 | r2 (object)   | rd |
| 6     | `kSysException`| `rd = hoo_exception_runtime(0)`       | —             | rd |
| 7     | `kSysPushHandler`   | `rd = hoo_push_handler(r2)`      | r2 (handler PC) | rd |
| 8     | `kSysPopHandler`    | `rd = hoo_pop_handler()`          | —             | rd |
| 9     | `kSysThrowToHandler`| `rd = hoo_throw_handler(r2)`     | r2 (exception) | rd |
| 10    | `kSysRethrowToHandler`| `rd = hoo_rethrow_handler()`   | —             | rd |
| 11    | `kSysStringData`| `rd = hoo_string_data(r2)`            | r2 (string)   | rd |

Arguments are passed in registers `r2` and `r3`; the result is written to `rd` (the `rd` field of the I-format instruction).

## 8. Notes

- This spec is a **pure hardware profile**, suitable for physical CPU design.
- The HVM backend now performs aggressive lowering to maintain this purity.
- **RET implementation note**: The architectural semantics of `RET` are `pc = r29` (branch to link register). In the interpreter and JIT backends, `RET` is implemented via native C++ function return (`return r1`); this is equivalent because `CALL` stores the return address (`pc+4`) in `r29` before transferring control via a C++ function call. A physical hardware implementation must execute `pc = r29` directly.
- **JAL / CALL redundancy**: `JAL` (base32, 16-bit offset) and `CALL` (escape32, 20-bit offset) are semantically identical — both set `rd = pc+4; pc += offset`. `CALL` provides a larger reachable range; `JAL` saves code space when the offset fits in 16 bits.
- **JMP / TAILCALL redundancy**: `JMP` (base32, 16-bit offset) and `TAILCALL` (escape32, 20-bit offset) are semantically identical — both perform `pc += offset` without saving a return address. `TAILCALL` provides a larger range; `JMP` saves code space when the offset fits.
