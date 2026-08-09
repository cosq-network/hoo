# Hoo Virtual Machine (HVM) Specification

Version: `1.5`

This document is the **normative architectural contract** for a physical HVM
microprocessor, a cycle-accurate simulator, the interpreter, and the LLVM JIT.
HVM is a standalone 64-bit load/store ISA. It is RISC-V-inspired but is not
binary-, privilege-, trap-, memory-model-, or ABI-compatible with RISC-V,
AMD64, or ARM64. HVM 1.5 adds profile-gated extensions without changing the
64-bit register machine or public ABI.

## 1. Scope

This profile defines the minimum architectural state, encodings, retirement
semantics, memory behavior, traps, ABI, and system facilities needed to build a
conforming processor. All high-level language constructs (objects, arrays,
exceptions) are lowered to standard memory operations and calls to a runtime
library.

Anything not required by the mandatory core remains profile-gated. Optional
extensions must be discoverable through feature flags and must have software
fallbacks unless the platform profile explicitly requires them. An
implementation must not change the architectural behavior of a supported
instruction when an extension is present.

## 2. Execution Model

- 64-bit register machine with a byte-addressable little-endian memory system.
- 64-bit pointers and an LP64-compatible C/C++ data model. The HVM calling
  convention is defined in section 3 and is not a host-platform ABI.
- `pc` is a byte address. Instructions are 4-byte aligned. The architectural
  reset `pc` is supplied by the platform profile.
- The stack grows toward lower addresses. `r31` is the architectural stack
  pointer; its alignment requirement is supplied by the ABI profile and is 16
  bytes for the standard HVM ABI.
- The mandatory core uses a flat physical address space. The system profile
  adds HVM-39 translation; an implementation without an MMU must run in Bare
  mode and must reject non-Bare translation modes.
- An instruction retires atomically: either its architectural effects commit,
  or a synchronous exception is taken and no later instruction is observed.
  Memory operations may have microarchitectural speculation, but retirement
  must preserve this architectural rule.
- Base instructions are 32 bits. Extended instructions use an 8-byte encoding:
  escape byte `0xFE`, ULEB128 logical opcode, zero padding through byte 3, and
  a 32-bit R/I/RI/B/J payload. The payload opcode field is ignored.

### 2.1 Architectural state

Each hart has the following state:

- `pc`: 64-bit byte address.
- `r0..r31`: 64-bit integer registers, with `r0` permanently reading as zero
  and writes to it discarded.
- Optional floating-point status/control state; the HVM 1.5 core does not
  expose accrued FP exception flags through the public ABI.
- `privilege`: `S` or `U` in the system profile.
- `trap_state`: `cause`, `fault_pc`, `fault_address`, `bad_instruction`, and
  `pending` fields as defined in section 9.
- `reservation`: implementation-defined LR/SC reservation state as defined in
  section 9.6.
- Optional HVM-V vector state: vector registers `v0..v31` (each `VLEN` bits),
  `vl`, and `vtype`, present only when `feature0.Vector` is set (section 5.5.1).

The `pc`, privilege state, trap state, and reservation state are not directly
addressable through general-purpose registers.

### 2.2 Reset and illegal state

Reset clears all general-purpose registers, clears pending traps and the LR/SC
reservation, enters system-profile S-mode, and loads the platform reset `pc`.
The reset value of memory, CSRs, timers, and devices is platform-defined and
must be documented by the board profile. An unsupported opcode, malformed
extended encoding, misaligned instruction fetch, or reserved encoding raises
an illegal-instruction trap; it must never execute as an implicit no-op.

## 3. Register Set

The core profile uses 32 general-purpose 64-bit registers (`r0..r31`):
- `r0`: hardwired zero
- `r1`: first argument and return-value register
- `r2..r3`: argument registers
- `r4`: thread pointer (`tp`)
- `r5..r8`: argument registers
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`lr`, return address)
- `r30`: frame pointer (`fp`)
- `r31`: stack pointer (`sp`)

(See `docs/hvm/hvm_register_set.csv`.)

### 3.1 Standard HVM calling convention

- `r1`, `r2`, `r3`, `r5`, `r6`, `r7`, and `r8` carry the first seven scalar or
  pointer arguments; `r1` also carries the scalar or pointer return value.
  `r4` is reserved for the thread pointer and is never an argument register.
- Additional arguments are passed in ascending 8-byte stack slots at the
  caller-owned outgoing argument area.
- `r9..r15` are caller-saved. `r16..r28`, `r30`, and `r31` are callee-saved;
  `r29` is the link register and is saved by a callee when it makes a nested
  call.
- A callee must preserve the incoming stack pointer and restore it before
  returning. The standard stack pointer alignment is 16 bytes at every public
  call boundary.
- Integer and pointer values are returned in `r1`. A 64-bit floating-point
  value is returned as its IEEE-754 binary64 bit pattern in `r1`.
- HVM object references are native 64-bit pointers in the public ABI. Compact
  object references are permitted only inside a platform/runtime profile that
  explicitly defines their conversion at ABI boundaries.
- The ABI does not define C++ name mangling, exception unwinding, or object
  layout. Those are compiler/runtime contracts layered above this ISA.

## 4. Encoding

Base instruction formats are little-endian 32-bit words. Bit 0 is the least
significant bit of the first byte in memory:
- `R`: `opcode[7] rd[5] rs1[5] rs2[5] func[10]`
- `I`: `opcode[7] rd[5] rs1[5] imm[15]`
- `RI`: `opcode[7] rd[5] rs1[5] rs2[5] imm[10]`
- `B`: `opcode[7] rs1[5] rs2[5] imm[15]`
- `J`: `opcode[7] rd[5] imm[20]`

Extended opcode encoding:
- opcodes `0x00..0x7F` use the base 32-bit formats above
- opcodes `>= 0x80` use an escape-prefixed encoding (`0xFE` prefix)
- The extended ULEB128 opcode occupies bytes 1 through 3 and must terminate
  within those three bytes. Nonzero padding bytes, an unterminated ULEB128
  value, or an opcode not present in the active feature set is invalid.
- branch/jump immediates are signed instruction offsets: `target_pc = pc +
  sign_extend(imm) * 4`. The offset is relative to the address of the current
  instruction, not the next instruction.
- A target outside the implemented address space raises an instruction-access
  or address-misalignment trap; it is not silently truncated.

Reserved encoding fields:
- Every base format above uses all 32 bit positions, so an instruction word's
  fields are always well-formed with respect to the format. The `func` field
  is only meaningful for instructions that define it in the CSV.
- The `hvm_instruction_set.csv` `Operands` column marks unused operand fields
  with `-` (for example, `RET` has `rd`, `rs1`, and `rs2` marked `-`), and the
  `Func` column marks unused func with `-`. Such fields are reserved for that
  instruction.
- Reserved fields must be zero. An instruction whose reserved fields are not
  all zero is a reserved encoding and raises the illegal-instruction trap
  (scause = 2) rather than being executed. Encoders must emit zero in every
  reserved field.
- An instruction whose opcode is defined but whose `func` value is not listed
  for that opcode in the CSV is likewise a reserved encoding and must raise
  the illegal-instruction trap.

### 4.1 Memory access rules

- Instruction fetches are 4-byte aligned. `NOP` and all base 32-bit instructions
  occupy one 4-byte line starting at a 4-byte-aligned PC.
- Extended (`escape32`) instructions occupy an 8-byte line that begins on a
  4-byte-aligned PC (the escape prefix `0xFE` is at offset 0; the 32-bit
  payload is at offset 4). Such a fetch crosses a 4-byte word boundary. A
  fetch that crosses a page or protection boundary must be fully mapped and
  accessible; if either half faults, the entire extended instruction raises the
  corresponding instruction-access fault and no part is retired. The lower
  addressed word is fetched and checked first; a fault on the second word does
  not retire the first.
- `LD.H`/`ST.H` addresses are 2-byte aligned, `LD.W`/`ST.W` addresses are
  4-byte aligned, and `LD.D`/`ST.D`, `LR.D`/`SC.D`, and pair operations are
  8-byte aligned. Misalignment raises the corresponding address-misaligned
  trap; the core profile does not perform invisible split accesses.
- `LD.B`/`ST.B` and `LDA` have no alignment requirement.
- Jump target alignment: `JAL` and `JMP` targets must be 4-byte aligned; a
  non-4-byte-aligned target raises an instruction-address-misaligned trap
  (scause = 0) before any register update. `JALR` computes its target as
  `rs + sign_extend(imm15)`; unlike an architecture that silently masks low
  bits, HVM requires the resulting target to be 4-byte aligned, otherwise an
  instruction-address-misaligned trap (scause = 0) is raised before any
  register update. The check is on the full sum `rs + sign_extend(imm15)`.
- `LD.B`/`ST.B` and `LDA` have no alignment requirement.
- All multi-byte values use little-endian byte order.
- A memory access that crosses an unmapped, inaccessible, or device boundary
  is faulting and has no partial architectural write. Pair stores are atomic
  with respect to the pair only when the platform profile explicitly provides
  that guarantee; otherwise each word follows the documented precise-fault
  order.

## 5. Minimal Instruction Set

The normative list is `docs/hvm/hvm_instruction_set.csv`.

### 5.1 Required Families (`hvm64-core-system`)

- Data movement: `NOP`, `MOV`, `MOVZ`, `LUI`, `ADDI`
  - `LUI` places its 15-bit immediate at bit shift 49 (`hvm::kLuiImmediateShift`); the shared constant is the implementation/spec synchronization point.
- Integer arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `DIVU`, `REM`, `SHL`, `SHR`, `SAR`
  - Signed integer overflow (`ADD`, `SUB`, `MUL`) raises an arithmetic-overflow trap and does not write `rd`.
  - `DIV` and `REM` raise an arithmetic-overflow trap on `INT64_MIN / -1` and `INT64_MIN % -1`.
  - `DIV`, `DIVU`, and `REM` raise a division-by-zero trap when the divisor is zero.
  - `DIVU` has no overflow path. Shift counts are reduced modulo 64.
- Bitwise/logical: `AND`, `OR`, `XOR`, `NOT`
- Floating-point: `FADD`, `FSUB`, `FMUL`, `FDIV`
- Comparisons: `CMPEQ`, `CMPNE`, `CMPLT`, `CMPLE`, `CMPULT`, `CMPULE`, `FCMPEQ`, `FCMPLT`, `FCMPLE`
  - `CMPLT`/`CMPLE` compare signed 64-bit values; `CMPULT`/`CMPULE` compare the same bits as unsigned 64-bit values. The result is 1 or 0 in `rd`.
- Branch/jump: `BEQ`, `BNE`, `BLT`, `BLE`, `JMP`, `JAL`, `JALR`, `RET`
- Memory: `LD.B`, `LD.BU`, `LD.H`, `LD.HU`, `LD.W`, `LD.WU`, `LD.D`, `ST.B`, `ST.H`, `ST.W`, `ST.D`, `LDA`
- Atomic memory: `LR.D`, `SC.D`
- Stack/frame: `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`
- Calls/linking: `CALL`, `TAILCALL`
- Hardware/System: `SYSCALL`, `BREAK`

Arithmetic traps are architectural synchronous exceptions. They are precise:
the trapping instruction is not retired, its destination register `rd` is not
written, and the exception records the PC of the trapping instruction. Every
other synchronous exception in this specification — misaligned or faulting
memory access, reserved or illegal encoding (scause = 2), `BREAK`, and
`SYSCALL` — follows the same precise-trap rule.

Vectoring: hardware must take every synchronous exception to a trap entry
point. The trap-entry mechanism is required core behavior, because the core
instruction set itself raises synchronous exceptions (`SYSCALL`, `BREAK`,
arithmetic traps, address faults). In a system-profile implementation the
entry point is `stvec.BASE` and the exception state is recorded in
`sepc`/`scause`/`stval`/`bad_instruction` (section 9.3). A core-only
implementation without the system profile must document its trap entry in its
platform profile (section 8.2): either a minimal trap entry point with the
same exception-state contract, or a deterministic halt that exposes the
exception cause. It may not skip the trapping instruction, continue with an
undefined result, or treat a synchronous exception as non-precise.

A hosted HVM execution API reports an unhandled synchronous trap as `-1` and
sets its VM error state for compatibility with the current interpreter/JIT
API.

### 5.2 HVM 1.5 Required Green-Compute Core Extensions

HVM 1.5 promotes the following low-complexity extensions into the standard HVM CPU profile for documented mobile, desktop, server, and robotics systems:

- Runtime atomics: `RETAIN`, `RELEASE`
- JIT cache coherency: `ICACHE.RNG`
- Pair memory operations: `LD.P`, `ST.P`

These instructions remain 64-bit operations. They do not change pointer width, register width, stack slot width, or the public ABI.

### 5.3 HVM 1.5 scalar sub-word profile

The HVM 1.5 scalar profile adds five base-encoded instruction families while
retaining the 64-bit register machine:

- `ARITH_B` (`0x11`) samples operands from bits 7:0 and provides wrapping
  `ADD.B`/`SUB.B`/`MUL.B`, signed and unsigned division, and signed and
  unsigned remainder.
- `SHIFT_B` (`0x12`) samples the low byte, masks the shift count to three
  bits, and provides wrapping `SHL.B`, logical `SHR.B`, and signed `SAR.B`.
  The language frontend exposes these through `<<`, `>>`, `<<=`, and `>>=`.
- `LOGIC_B` (`0x22`) applies XOR, AND, and NOT to bit 0 and returns a
  normalized bit value.
- `FLOAT_ARITH_B` (`0x31`) performs arithmetic on canonical E4M3 FP8 byte
  encodings. The interpreter and LLVM JIT use a software-compatible shim when
  host FP8 instructions are unavailable.
- `CMP_B` (`0x42`) compares the low 8 bits of two registers. Functions 0/1 are
  equality/inequality, 2/3 are signed less-than/less-than-or-equal, and 4/5
  are unsigned less-than/less-than-or-equal. Results are normalized to 0 or 1.

Integer sub-word instructions write a low-byte result. The compiler applies
sign extension for `int8` and zero extension for `byte`/`bit` when values
return to the normal 64-bit language ABI. FP8 codegen encodes and decodes at
the existing f64 language boundary, preserving mixed-precision compatibility.

### 5.4 Floating-point state and semantics

HVM has no separate floating-point register file in version 1.5. `FADD`,
`FSUB`, `FMUL`, `FDIV`, and floating comparisons interpret the 64-bit contents
of the referenced general-purpose registers as IEEE-754 binary64 values and
write their result bit pattern to a general-purpose register.

- Operations use round-to-nearest, ties-to-even.
- Normal IEEE-754 finite, infinite, subnormal, and signed-zero behavior is
  required.
- Division by zero, invalid operations, overflow, underflow, and inexact
  results do not raise HVM traps in the core profile; results follow IEEE-754
  binary64 rules.
- NaN results are canonicalized to a single quiet-NaN encoding. Signaling NaNs
  are quieted before the result is written. The canonical quiet-NaN payload is
  implementation-defined; the hosted HVMJIT profile and this core
  specification use `0x7ff8'0000'0000'0000` (the canonical quiet NaN). A
  platform profile may only narrow, never widen, the set of NaN bit patterns
  accepted as inputs; every NaN result produced by the implementation must be
  bit-identical to this canonical value.
- Floating comparisons return integer `0` or `1` in `rd`. Unordered NaN
  comparisons return `0` for equality and less-than/less-than-or-equal.
- The core does not expose accrued FP exception flags. A future platform
  profile may expose them through a system-profile CSR, but flags must not
  alter integer or control-flow semantics.

The interpreter and JIT must operate on the same 64-bit bit patterns and
canonicalize NaNs identically; host floating-point environment differences
must not change architectural results.

### 5.5 Optional Profile-Gated Extensions

The following extensions are optional unless required by a specific HVM platform profile:

- HVM-L hardware loops: `LOOP.SET`, `LOOP.DECBR`
- HVM-MEM advisory memory hints: `PREFETCH.R`, `PREFETCH.W`, `PREFETCH.NTA`, `MEMZERO.HINT`
- HVM-V vector operations: `VSETVL`, vector load/store, arithmetic, compare, merge, reduction, shift, bitwise, and `VFIRST.M`
- HVM-A accelerator dispatch: `DOORBELL`
- HVM-Alloc: `ALLOC.BUMP`
- HVM-ObjRef: compact managed object references represented by runtime metadata, not native ABI pointers
- HVM-Prof: `RDPROF`
- HVM-Cap: `CHK.B`
- HVM-NZ: `LD.D.NZ`
- Branch/code layout hints: `BR.HINT`

Advisory instructions may be implemented as no-ops. Runtime-specific instructions must have software fallback unless the platform profile says otherwise.

**HVM-Prof (`RDPROF`).** `RDPROF` is a privileged, S-mode-only system-profile
instruction. It reads a profiling/performance counter addressed by a `selector`
(`rs1`) and an `imm15` operand; in the hosted HVMJIT profile no profiling
counters are exposed, so both the interpreter and JIT return `rd = 0` without
trapping, and any `selector`/`imm15` is ignored. A platform profile that
implements HVM-Prof may return nonzero counter values (and may expose a
`profsel` selector register); executing `RDPROF` in U-mode is a mode violation
that traps with `scause = 2`.

**HVM-Cap (`CHK.B`).** The hosted profile implements the minimal bounds-check
form only: it compares the raw unsigned 64-bit values in `rs1` (`ptr`) and
`rs2` (`bound`), writes `ptr` to `rd` when `ptr < bound`, and traps with
`scause = 20` otherwise. It does not strip or interpret upper-bit pointer tags,
consult allocation metadata, validate capability provenance, or perform a
memory access. A physical HVM-Cap profile may add tagged-pointer and provenance
checks, but it must preserve the successful `rd = ptr` result and the bounds
fault contract; the hosted profile must not be described as providing those
stronger guarantees.

### 5.5.1 HVM-V vector state and encoding

HVM-V adds the following architectural state (present only when
`feature0.Vector` is set):
- `v0..v31`: 32 vector registers, each `VLEN` bits wide.
- `vl`: active element count (number of elements operated on by a vector
  instruction).
- `vtype`: current element-type/layout encoding consumed by `VSETVL` and
  vector arithmetic.
- `vregs[32][VLMAX]`: internal backing storage indexed by register and
  element index `0..vl-1`.

`VSETVL rd, rs1, rs2` sets the vector length and type: `vtype = rs2` and
`vl = min(as_uint(rs1), VLMAX)`, then writes `vl` to `rd`. `VLMAX` is a
platform-defined maximum; the hosted HVMJIT profile fixes `VLEN` at 64 bits
(one 8-byte element), so `VLMAX = 8` regardless of `vtype`. HVM does **not**
define a `vstart` register — there is no fault-only-first or element-resume
state. Vector memory instructions (`VLD.V`, `VST.V`, ...) operate over elements
`0..vl-1` of the configured registers using the active element width.

`vtype` is carried verbatim from `rs2` into the register. The hosted profile
uses the following element-type encodings consumed by vector arithmetic and
memory operations:

| `vtype` | element type |
| --- | --- |
| `0`    | signed/unsigned integer, 64-bit (`i64`/`u64`) |
| `2`    | IEEE-754 binary64 floating point (`f64`) |
| `9`    | IEEE-754 binary64 floating point (`f64`) (alias of `2`) |

A `vtype` value not listed above is reserved; the hosted profile treats every
reserved value as `0` (integer). Other bits in `vtype` are currently unused and
ignored, so the effective element width is always 64 bits in this profile —
element widths narrower than 64 bits (e.g. the 8/16/32-bit SEW values used by
other platform profiles) are not supported by the hosted HVMJIT interpreter and
JIT and select integer semantics.

Element encodings for `vtype` beyond this core contract are defined per
platform profile; the core encoding contract is `vl = min(avl, VLMAX)` and
`vtype` is carried verbatim.

### 5.6 Explicitly Excluded from Core (Lowered to Software)

The following are **NOT** in the ISA and must be lowered by the compiler:
- `NEW`/`NEWA`: Lowered to `CALL hoo_alloc`.
- `LDF`/`STF`: Lowered to `ADDI` + `LD.D`/`ST.D`.
- `LDELEM`/`STELEM`: Lowered to pointer arithmetic + `LD.D`/`ST.D`.
- `TRY`/`CATCH`/`THROW`: Lowered to control flow + `CALL hoo_push_handler`.

### 5.7 Privileged Instructions (HVM System Profile)

The following are **only** required for the system-level profile (e.g., running a kernel). Minimal embedded HVM64 implementations may omit them when they do not run a protected OS:

- Supervisor traps: `ECALL`, `TRAPRET`
- System register access: `CSRRW`
- TLB management: `SFENCE.VMA`

Omitting the system profile omits privilege-level semantics (U-mode/S-mode
distinction), the CSR-based exception state, and these instructions. It does
not remove the requirement to report the synchronous exceptions that the core
instruction set itself can raise (`SYSCALL`, `BREAK`, arithmetic traps,
address faults); see section 5.1 for the core-only trap-entry contract.

## 6. Mapping to Current Grammar

- `newExpression` -> `CALL hoo_alloc` + `CALL` constructor.
- member/index access -> explicit pointer arithmetic + `LD.D`/`ST.D`.
- `try/catch/finally` -> `CALL hoo_push_handler` + conditional control flow.

## 7. System Call (SYSCALL) Interface

`SYSCALL` dispatches to internal runtime services via the immediate field `imm15`:

In a hosted interpreter or JIT, the service table is a direct runtime bridge.
On physical hardware, `SYSCALL` is a synchronous trap with cause `16` from
U-mode or `17` from S-mode; the platform syscall handler implements the same
register contract. On trap entry, `stval` is set to the `imm15` syscall number
and `bad_instruction` holds the faulting `SYSCALL` (section 9.3), so the
handler can dispatch without re-reading memory. A hardware implementation must
not depend on C++ symbols or the presence of `hoort` in order to decode or
retire the instruction.

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
| 12    | `kSysThreadCreate`   | `rd = thread_create(r2, r3)`     | r2 (entry), r3 (arg) | rd (TID) |
| 13    | `kSysThreadExit`     | `thread_exit(r2)`                 | r2 (retval)   | — |
| 14    | `kSysFutex`          | `rd = futex(r2, r3, r4)`          | r2 (uaddr), r3 (op), r4 (val) | rd |
| 15    | `kSysGetTid`         | `rd = get_tid()`                   | —             | rd |
| 16    | `kSysOpen`           | `rd = open(r2, r3, r4)`            | r2 (path), r3 (flags), r4 (mode) | rd (fd) |
| 17    | `kSysRead`           | `rd = read(r2, r3, r4)`            | r2 (fd), r3 (buf), r4 (count) | rd (bytes) |
| 18    | `kSysWrite`          | `rd = write(r2, r3, r4)`           | r2 (fd), r3 (buf), r4 (count) | rd (bytes) |
| 19    | `kSysClose`          | `rd = close(r2)`                   | r2 (fd)       | rd |
| 20    | `kSysLseek`          | `rd = lseek(r2, r3, r4)`           | r2 (fd), r3 (offset), r4 (whence) | rd (pos) |
| 21    | `kSysFstat`          | `rd = fstat(r2, r3)`               | r2 (fd), r3 (buf) | rd |
| 22    | `kSysClockGetTime`   | `rd = clock_gettime(r2, r3)`       | r2 (clk_id), r3 (ts_ptr) | rd |
| 23    | `kSysGetRandom`      | `rd = getrandom(r2, r3)`           | r2 (buf), r3 (len) | rd (bytes) |

Arguments are passed in registers `r2`, `r3`, and `r4` (for three-argument calls); the result is written to `rd`. Syscalls 1–11 are runtime-internal services. Syscalls 12–23 are platform OS services (threading, file I/O, clock); their presence depends on the host environment.

## 8. Notes

- This spec is a **pure hardware profile**, suitable for physical CPU design.
- The HVM backend now performs aggressive lowering to maintain this purity.
- HVM 1.5 remains a **64-bit architecture**. Compact object references are an optional managed-runtime representation and are not native pointers at C/C++ ABI boundaries.
- **RET implementation note**: The architectural semantics of `RET` are `pc = r29` (branch to link register). In the interpreter and JIT backends, `RET` is implemented via native C++ function return (`return r1`); this is equivalent because `CALL` stores the return address (`pc+4`) in `r29` before transferring control via a C++ function call. A physical hardware implementation must execute `pc = r29` directly.
- **JAL / CALL redundancy**: `JAL` (base32, 16-bit offset) and `CALL` (escape32, 20-bit offset) are semantically identical — both set `rd = pc+4; pc += offset`. `CALL` provides a larger reachable range; `JAL` saves code space when the offset fits in 16 bits.
- **JMP / TAILCALL redundancy**: `JMP` (base32, 16-bit offset) and `TAILCALL` (escape32, 20-bit offset) are semantically identical — both perform `pc += offset` without saving a return address. `TAILCALL` provides a larger range; `JMP` saves code space when the offset fits.

### 8.1 Interpreter and JIT conformance

The interpreter and JIT are execution engines for the same architecture; they
are not separate ABIs. Both must:

- decode the same base and extended instruction bytes;
- implement the same register-0, PC, alignment, arithmetic, floating-point,
  memory, atomic, and trap semantics;
- commit architectural state in the same order for every instruction;
- expose the same syscall table and feature flags; and
- return the same hosted execution result for normal completion, unhandled
  traps, and explicit termination.

The JIT may translate multiple instructions, use host registers, inline runtime
calls, or fall back to the interpreter. It must preserve the architectural
state at every externally visible call, trap, breakpoint, stop request, and
memory access. A hardware implementation may pipeline or speculate, but its
retirement behavior must match the same contract.

### 8.2 Platform profile requirements

A processor or SoC claiming HVM conformance must publish a platform profile
that defines:

- reset address, physical address width, RAM and MMIO ranges;
- implemented optional-extension feature bits;
- interrupt sources, priority, routing, and timer frequency;
- boot firmware entry state and device-discovery mechanism;
- cache-coherency and DMA rules;
- reservation granule and atomicity domain;
- debug, halt, single-step, and external-observation behavior; and
- synchronous-exception and trap-entry contract (vector target, or the
  deterministic halt and cause-exposure mechanism) for implementations that
  omit the system profile (see section 5.1).

The core specification deliberately does not assign device addresses or
pretend that the hosted Hoo runtime is present on bare hardware. A bare-metal
program must provide the syscall and allocation services required by its ABI,
or the platform must document those services as unavailable.

### 8.3 Hosted HVMJIT profile

The current `HVMJIT` hosted profile executes the Bare/flat-memory core and the
Hoo runtime extensions. It does not emulate a physical MMU, interrupt
controller, timer device, DMA engine, or S/U privilege transition. In that
profile:

- `CSRRW` exposes the eight-entry HVM CSR window and is stateful per HVM hart.
- `SFENCE.VMA` is a no-op because the hosted profile has no TLB.
- `ECALL` and `TRAPRET` report an unhandled system-profile trap rather than
  attempting to transition a host process between privilege modes. HVM resets
  into S-mode (see section 9.2), so `ECALL` is a legal system-profile trap
  (`scause = 9`) that the hosted profile cannot service (no host-side S-mode
  monitor), and `TRAPRET` has no S-mode handler to return from (treated as a
  trap). Both surface as the soft-trap error reported by the interpreter;
  they do not perform any host privilege transition.
- HVM-39, interrupts, device memory, and physical boot behavior are implemented
  only by a conforming hardware or system simulator profile.

A module requiring those facilities must declare the corresponding platform
profile. Hosted JIT execution of ordinary Hoo modules remains compatible with
the core ISA and runtime ABI.

## 9. Privileged Architecture (HVM System Profile)

This is an HVM-defined system profile with RISC-V-inspired terminology. It is
not an implementation of the RISC-V privileged architecture. In particular,
HVM currently defines no M-mode, delegation CSRs, or RISC-V supervisor ABI.
Implementations targeting RISC-V hardware must provide an explicit translation
layer for these differences.

This section defines the system-level extensions required to run a general-purpose HVM OS. These extensions are orthogonal to the core profile — an implementation may omit them for embedded/RTOS use. A Linux port would require an HVM-specific port or translation layer; this profile is not sufficient for an unmodified RISC-V Linux environment.

### 9.1 Privilege Levels

Two HVM privilege modes:
- **S-mode** (Supervisor, 1): kernel, can execute all instructions and access all HVM-defined CSRs.
- **U-mode** (User, 0): applications, restricted to non-privileged instructions. Traps on attempted privileged operation.

The current privilege level is stored in `sstatus.SPP`. Hardware resets into
S-mode. This reset and two-mode model are HVM choices; RISC-V systems normally
reset in M-mode and transition to S-mode or U-mode through machine firmware.

### 9.2 Control and Status Registers (CSRs)

CSRs are addressed by a 12-bit immediate field in the `CSRRW` instruction.

| Address | Name       | Description                                                        |
|---------|------------|--------------------------------------------------------------------|
| 0x000   | `sstatus`  | Supervisor status (SPP, SIE, SPIE, UPIE)                           |
| 0x001   | `stvec`    | Trap handler PC (4-byte aligned, mode bits in low 2 bits)          |
| 0x002   | `sepc`     | Exception program counter (address of trapping instruction)        |
| 0x003   | `scause`   | Exception cause (bit 63 = interrupt flag, low bits = cause code)   |
| 0x004   | `stval`    | Trap value (faulting address for page faults)                     |
| 0x005   | `satp`     | Supervisor address translation (mode + ASID + page-table PPN)      |
| 0x006   | `stime`    | Cycle counter (read-only, 64-bit, increments every cycle)          |
| 0x007   | `stimecmp` | Timer compare value; when `stime >= stimecmp`, a timer interrupt fires |
| 0x008   | `feature0` | Read-only implemented-feature bit register (see layout below)     |

**`feature0` field layout** (64-bit, read-only; writes are ignored):
- Bit 0: `BaseCore`    — `hvm64-core-system` minimal instruction set
- Bit 1: `GreenCompute` — RETAIN/RELEASE/ICACHE.RNG/LD.P/ST.P
- Bit 2: `SubWord`     — HVM 1.5 scalar sub-word profile
- Bit 3: `Vector`      — HVM-V
- Bit 4: `HardwareLoop` — HVM-L
- Bit 5: `Advisory`    — PREFETCH.*/MEMZERO.HINT/BR.HINT
- Bit 6: `Alloc`       — HVM-Alloc
- Bit 7: `Prof`        — HVM-Prof
- Bit 8: `Cap`         — HVM-Cap
- Bit 9: `Nz`          — HVM-NZ
- Bit 10: `Accel`      — HVM-A (implementations not supporting HVM-A leave this 0)
- Bits 11–63: reserved, read as 0

Software that needs a feature must first read `feature0` and check the
corresponding bit. Bits 11–63 are reserved; software must treat them as
reserved (ignored for behavior, always read as 0).

**`sstatus` field layout** (64-bit):
- Bit 1: `SIE`   — Supervisor Interrupt Enable (0 = masked, 1 = enabled)
- Bit 5: `SPIE`  — Previous SIE value (saved/restored on trap/return)
- Bit 8: `SPP`   — Previous privilege mode (0 = U, 1 = S)
- All other bits reserved (read as 0, ignore writes).

**`scause` encoding**:
- Bit 63: interrupt flag (1 = interrupt, 0 = exception)
- Bits 62–0: cause code

| scause      | Description                          |
|-------------|--------------------------------------|
| 2           | Illegal instruction                  |
| 3           | Breakpoint                           |
| 8           | Environment call from U-mode (ECALL) |
| 9           | Environment call from S-mode (ECALL) |
| 12          | Instruction page fault               |
| 13          | Load page fault                      |
| 15          | Store/AMO page fault                 |
| 0x8000_0000_0000_0000 | Supervisor timer interrupt  |

Additional HVM synchronous causes are:

| scause | Description |
|--------|-------------|
| 0 | Instruction-address misaligned |
| 1 | Instruction access fault |
| 4 | Load-address misaligned |
| 5 | Load access fault |
| 6 | Store/AMO address misaligned |
| 7 | Store/AMO access fault |
| 16 | HVM `SYSCALL` from U-mode |
| 17 | HVM `SYSCALL` from S-mode |
| 18 | Arithmetic overflow |
| 19 | Division by zero |
| 20 | Null-pointer or bounds fault |

**`stvec` encoding**:
- Bits 1–0: mode (0 = direct, all traps jump to BASE)
- Bits 63–2: BASE (trap handler PC, must be 4-byte aligned)

**`satp` encoding** (64-bit):
- Bits 63–60: MODE (0 = Bare, no translation; 8 = HVM-39)
- Bits 59–44: ASID (16-bit address-space identifier)
- Bits 43–0: PPN (physical page number of root page table, shifted right by 12)

**Reset values** (after a hart-level reset, or the equivalent of a TRAPRET
to the reset vector):
- `sstatus` = 0 (SPP = S since hardware resets into S-mode; SIE = 0).
- `stvec` = 0 (direct mode, BASE = 0).
- `sepc`, `scause`, `stval`, `bad_instruction` = 0.
- `satp` = 0 (MODE = Bare, no address translation; see section 9.5).
- `stime` = 0 (or a platform-defined monotonic starting offset; see section
  9.5). `stimecmp` = 0.
- `feature0` = the implemented feature set (read-only). See section 9.2.

All CSRs and GPRs are 0 unless otherwise specified. `r0` reads as 0 and writes
are discarded. A platform profile may not redefine these reset values; it may
only constrain them further (for example, fixing the `stime` epoch).

### 9.3 Trap and Interrupt Handling

**Trap entry** (hardware on ECALL, BREAK, page fault, `SYSCALL`, or interrupt):
1. `sepc` = PC of trapping instruction (for interrupts: PC of interrupted instruction)
2. `scause` = cause code (bit 63 set for interrupts)
3. `stval` = fault address, bad operand, or zero when the cause has no value.
   For `SYSCALL`, `stval` = the `imm15` syscall number.
4. `bad_instruction` = complete faulting instruction for any synchronous
   exception whose instruction can be reconstructed (including `SYSCALL`),
   otherwise unchanged
5. `sstatus.SPIE` = `sstatus.SIE`; `sstatus.SIE` = 0 (disable interrupts)
6. `sstatus.SPP` = current privilege level (0 for U-mode traps)
7. Switch to S-mode
8. If `stvec.mode == 0` (direct), or if the trap is a synchronous exception: `PC = stvec.BASE`
9. If `stvec.mode == 1` (vectored) and the trap is an interrupt: `PC = stvec.BASE + (scause & 0x7FFF_FFFF_FFFF_FFFF) * 4`

In vectored mode the vector offset uses the low cause code (the interrupt bit in
bit 63 is excluded), so each distinct interrupt cause gets its own 4-byte slot
(`cause * 4`) within the trap vector. Synchronous exceptions always use direct
entry at `stvec.BASE` regardless of mode. `stvec.BASE` (and thus the vectored
target) must be 4-byte aligned; an unaligned BASE is a reserved
`stvec` encoding (see section 8.2).

Synchronous exceptions always vector to `stvec.BASE`. The handler may inspect
`scause`, `sepc`, `stval`, and `bad_instruction` to distinguish the fault. For
a `SYSCALL` trap, the handler reads the syscall number from `stval` (or decodes
it from `bad_instruction`) and uses the register contract in section 7. A
trap taken while already in S-mode remains in S-mode and records `SPP=1`; a
trap from U-mode records `SPP=0`. Trap entry does not modify general-purpose
registers.

When more than one fault is possible, the processor reports the first fault in
architectural access order: instruction fetch, instruction decode, register
and arithmetic evaluation, then memory access. For `LD.P`/`ST.P`, the lower
addressed 64-bit word is checked and committed before the higher word; a fault
on the second word leaves the first word's documented effect visible. A
platform profile may provide a stronger pair-atomic guarantee, but must state
it explicitly.

**Trap return** (`TRAPRET`):
1. `SIE` = `sstatus.SPIE`
2. Privilege = `sstatus.SPP`
3. `PC` = `sepc`

`TRAPRET` is legal only in S-mode. Software is responsible for advancing
`sepc` before return when the faulting instruction is restartable, such as an
`ECALL`; faulting loads, stores, and arithmetic instructions normally resume
only after the cause has been handled or the process is terminated.

Interrupts are taken when `sstatus.SIE == 1` and `sstatus.SPP == 0` (U-mode). In S-mode, interrupts are taken when `sstatus.SIE == 1`.

### 9.4 Timer

`CSRRW` with CSR address 0x006 reads `stime` (read-only, any write is ignored).
`CSRRW` with CSR address 0x007 reads/writes `stimecmp`.

`stime` counts host cycles and is only directly readable by S-mode via
`CSRRW` (U-mode `CSRRW` traps as scause = 2). A U-mode program that needs wall
time must obtain it through a `SYSCALL` runtime service (see section 7) or by
trapping to S-mode first; HVM does not define a U-mode-mapped time counter.
`stimecmp` is read/write only by S-mode; writes from U-mode trap.

When `stime >= stimecmp`, a timer interrupt (scause = 0x8000_0000_0000_0000) is pending. It fires when interrupts are enabled and not masked.

### 9.5 Supervisor Address Translation (HVM-39)

HVM-39 provides a 39-bit virtual address space with 4 KiB pages using a 3-level radix tree.

**Address format:**
- Virtual address `VA[38:0]` is translated; `VA[63:39]` must equal `VA[38]` (canonical sign-extension).
- A 39-bit VA is split: `VPN[2]` (bits 38–30), `VPN[1]` (bits 29–21), `VPN[0]` (bits 20–12), `offset` (bits 11–0).

**Page table entry** (8 bytes, 64-bit):
- Bit 0: `V`    — Valid
- Bit 1: `R`    — Readable
- Bit 2: `W`    — Writable
- Bit 3: `X`    — Executable
- Bit 4: `U`    — User (accessible from U-mode)
- Bit 5: `G`    — Global (not flushed on ASID switch)
- Bit 6: `A`    — Accessed (set by hardware)
- Bit 7: `D`    — Dirty (set by hardware on write)
- Bits 9–8: `RSW` — Reserved for supervisor software
- Bits 53–10: `PPN` — Physical page number (44 bits, shifted right by 12)
- Bits 63–54: reserved

**Translation walk:**
1. If `satp.MODE == 0` (Bare), VA is used directly as PA.
2. Root = `satp.PPN * 4096` (physical address of root page table).
3. For each level `i` from 2 down to 0:
   a. PTE address = root + `VPN[i] * 8`
   b. Load PTE. If `PTE.V == 0`, raise page fault.
   c. If `PTE.R == 0` and `PTE.X == 0`, go to next level (non-leaf PTE).
   d. Otherwise (leaf PTE): check permissions. If violation, raise page fault.
   e. HVM-39 permits leaf PTEs only at level 0. A leaf at level 1 or 2 is an
      invalid PTE and raises a page fault; superpages are reserved for a future
      profile.
   f. PA = `PTE.PPN * 4096 + offset`.

A non-leaf PTE must have `V=1`, `R=0`, `W=0`, and `X=0`. A PTE with `W=1`
and `R=0`, or with reserved bits set, is invalid. Leaf PTEs must have at least
one of `R,W,X` set. The `U` bit must be set for U-mode access and must be clear
for supervisor-only mappings. `G` is ignored when ASIDs are disabled.

On every successful access, hardware sets `A=1`; on a successful store,
hardware sets `D=1`. If the implementation cannot update these bits atomically,
it raises the corresponding page fault instead. A stale TLB entry must never
permit access after a permission-changing page-table write followed by
`SFENCE.VMA`.

**Page fault causes:**
- Instruction fetch: `scause=12`; checks X, U
- Load: `scause=13`; checks R, U
- Store: `scause=15`; checks W, U

`stval` is set to the faulting virtual address.

### 9.6 Atomic Memory Operations

`LR.D` (load-reserve) and `SC.D` (store-conditional) provide the primitives for implementing atomics and spinlocks. HVM defines these as a custom pair and does not encode RISC-V's `aq`/`rl` instruction bits.

**Reservation set:**
- `LR.D` creates a reservation on the 8-byte reservation granule containing the loaded address. A future platform may use a larger granule only if it publishes that profile and preserves all stated atomicity guarantees.
- `LR.D` and `SC.D` require 8-byte-aligned addresses. A misaligned `LR.D`
  raises a load-address-misaligned trap (`scause = 4`); a misaligned `SC.D`
  raises a store/AMO-address-misaligned trap (`scause = 6`).
- `SC.D` succeeds only if the reservation is still valid (no intervening store to the same reservation granule by any hart).
- The reservation is invalidated by:
  - Any store to the reserved granule (by any hart)
  - Another `LR.D` by the same hart (new reservation replaces old)
  - Context switch or trap
- `SC.D` writes `0` to `rd` on success, a nonzero value on failure.
- `LR.D`/`SC.D` ordering is acquire-release and is part of the SC total order of the
  base memory model (section 9.8); no separate fence is required. A platform must
  not be documented as using a weaker ordering than this. (HVM does not encode
  RISC-V `aq`/`rl` bits; relaxed/acquire-only/release-only orderings are reserved
  for a future weak-memory extension, not the base profile.)
- ACPI-style spinlock: `loop: LR.D rd, (rs1); SC.D rd, rs1, rs2; bnez rd, loop`

The hosted HVMJIT profile has one reservation state per executing HVM state. Its
ordinary byte/halfword/word/doubleword and pair stores invalidate an overlapping
reservation, so same-state intervening stores have the required behavior. A
multi-hart implementation must extend that invalidation to all harts sharing
the reservation domain.

### 9.7 CSR Access

`CSRRW rd, rs, csr12`: Atomically reads the CSR addressed by the low 12 bits
of the HVM `imm15` field into `rd`, then writes the value of `rs` to the CSR.
The upper three immediate bits must be zero. If `rs = r0`, the write is
suppressed (behaves as a read-only access).

Attempted write to a read-only CSR (e.g., `stime`, `feature0`) is ignored by
the hosted profile; a physical system-profile implementation raises the
illegal-instruction trap for such a write (scause = 2). Access to an undefined
CSR address (an address outside the implemented CSR window, or with an
unimplemented feature bit set) raises the illegal-instruction trap
(scause = 2).

**Privilege and mode violations.** The following are synchronous exceptions
and trap with the given `scause` values:

| Condition | scause |
|-----------|--------|
| Any privileged instruction (`ECALL`, `TRAPRET`, `CSRRW`, `SFENCE.VMA`, `RDPROF`) executed in U-mode | 2 (illegal instruction) |
| `ECALL` executed in S-mode | 9 |
| `ECALL` executed in U-mode | 8 |
| `TRAPRET` executed in U-mode | 2 |
| CSR address outside the implemented window (`imm15 >=` implemented CSR count) | 2 |
| Upper three CSR-immediate bits nonzero | 2 |
| Optional instruction executed when its feature bit is clear (e.g. `DOORBELL` with `feature0.Accel` = 0) | 2 |

`ECALL` is legal in both U-mode (scause = 8) and S-mode (scause = 9); the
claim in `instructions.md` (version 1.4) that it is "illegal in S-mode" is
incorrect and superseded by this table.

### 9.8 Memory Model

The mandatory HVM core uses a **sequentially consistent (SC) memory model**
for normal cacheable memory. Every hart observes one total order of retired
loads, stores, and atomic operations that respects each hart's program order.
This deliberately favors a simple, portable hardware contract over the weaker
memory models used by modern desktop ISAs.

- A normal load observes the latest preceding store to the same address in the
  global order.
- A normal store becomes globally visible when the instruction retires.
- `LR.D`/`SC.D` are atomic in the reservation domain and participate in the
  same total order. They provide acquire-release behavior without encoded
  `aq`/`rl` bits.
- A failed `SC.D` performs no store and writes `1` to `rd`; success writes
  zero.
- Device/MMIO accesses are strongly ordered with respect to other accesses to
  the same device region. The platform profile must define DMA visibility and
  cache-coherency rules.
- `SFENCE.VMA` ensures all preceding page-table writes are visible to
  subsequent address translation and invalidates matching translation entries.
- No general `FENCE` instruction is required by the core profile. A future weak
  memory extension may add one, but it must not weaken the mandatory SC
  behavior of the base profile.

The interpreter must serialize memory operations according to this model. A
JIT may use host atomics and fences, but it must not expose host memory-ordering
behavior that is weaker than HVM's SC contract.
