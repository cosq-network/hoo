# HVM Core Instruction Reference

Version: `1.5`
Profile: `hvm64-core-system` (Hardware Ready)
Normative sources:
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm-spec.md`

This reference defines a **64-bit hardware-ready ISA**. HVM is RISC-V-inspired but is not binary-, privilege-, trap-, memory-model-, or ABI-compatible with RISC-V. All high-level VM constructs are handled via software lowering, standard library calls, or profile-gated HVM 1.5 runtime acceleration instructions with software fallback.

## 1. Scope

This reference defines the physical instructions supported by the HVM core and optional HVM 1.5 system profiles. It is sufficient to support the Hoo language through aggressive compiler-level lowering while preserving a 64-bit register and pointer ABI.

## 2. Register Convention Summary

- `r0`: hardwired zero
- `r1`: first argument and return-value register
- `r2..r3`: argument registers
- `r4`: thread pointer (`tp`)
- `r5..r8`: argument registers
- `r9..r15`: caller-saved temporaries
- `r16..r28`: callee-saved
- `r29`: link register (`RET` target)
- `r30`: frame pointer
- `r31`: stack pointer

The argument registers are `r1`, `r2`, `r3`, `r5`, `r6`, `r7`, and `r8`.
`r4` is reserved for the thread pointer and is not an argument register.

These assignments define the HVM calling convention. They are not the RISC-V
LP64 register convention (`x1=ra`, `x2=sp`, `x10-x17=a0-a7`). The LP64
reference is limited to the C/C++ data model and 64-bit pointer representation.

## 3. Instruction Formats

- `R`: `opcode[7] rd[5] rs1[5] rs2[5] func[10]`
- `I`: `opcode[7] rd[5] rs1[5] imm[15]`
- `RI`: `opcode[7] rd[5] rs1[5] rs2[5] imm[10]`
- `B`: `opcode[7] rs1[5] rs2[5] imm[15]`
- `J`: `opcode[7] rd[5] imm[20]`

Opcode-space note:
- `0x00..0x7F` use the base 32-bit encodings directly.
- Opcodes `>= 0x80` use an escape-prefixed extended encoding (`0xFE` prefix).
- In `docs/hvm/hvm_instruction_set.csv`, `Opcode` is the logical opcode value and `Encoding` records whether the row is `base32` or `escape32`.
- Tooling must use `Encoding` to derive the emitted bytes; do not infer the wire format from `Opcode` alone.

Extended opcode wire format (always 8 bytes):
```
byte 0:      0xFE              (escape prefix)
byte 1..:    ULEB128 opcode    (variable bytes until high bit clear)
bytes..3:    0x00              (zero-padding to reach byte 4)
bytes 4..7:  32-bit payload    (same encoding as the base format: R/I/RI/B/J)
```
The payload's opcode field (bits 31:25) is ignored; the opcode is taken from the ULEB128 value.
Tooling must encode and decode instructions using this 8-byte layout for any opcode >= 0x80.
The 8-byte extended instruction fetch begins on a 4-byte-aligned PC and crosses
a 4-byte word boundary. A fetch crossing a page or protection boundary must be
fully mapped and accessible; if either half faults, the whole instruction faults
and is not retired (see `hvm-spec.md` §4.1).

Reserved-field rule: operand fields marked `-` in
`docs/hvm/hvm_instruction_set.csv` (both `Operands` and `Func` columns) are
reserved and must be zero. Encoders must emit zero in every reserved field. A
reserved encoding (nonzero reserved field, or a `func` value not defined for
its opcode) raises the illegal-instruction trap on hardware and must not be
executed.

## 4. Core Instruction Set

### 4.1 Data movement
- `NOP` `MOV` `MOVZ` `LUI` `ADDI`

### 4.2 Integer arithmetic and shifts
- `ADD` `SUB` `MUL` `DIV` `DIVU` `REM`
- `SHL` `SHR` `SAR`

Overflow behavior (signed operations):
- `ADD`, `SUB`, `MUL`: overflow raises a synchronous trap and does not write `rd`.
- `DIV`, `REM`: additionally trap on `INT64_MIN / -1` and `INT64_MIN % -1`.
- `DIV`, `DIVU`, `REM`: trap on a zero divisor.

Synchronous traps are precise: the trapping instruction is not retired, `rd`
is not written, and the trap reports the trapping instruction's PC. Hardware
vectors every synchronous exception to the trap entry point (`stvec.BASE` in
the system profile; see `hvm-spec.md` sections 5.1 and 9.3). A hosted
execution API reports an unhandled trap as `-1` and sets its VM error state.
- `DIV`, `DIVU`, `REM`: trap on division by zero.
- `DIVU` has no overflow path.
- `SHL`, `SHR`, `SAR`: have no overflow path because shift counts are reduced modulo 64.

Hosted interpreter/JIT entry points report an unhandled arithmetic trap as
`-1` and set the VM error state for compatibility with the current API. A
physical processor enters the architectural trap handler instead.

### 4.3 Bitwise/logical
- `AND` `OR` `XOR` `NOT`

### 4.4 Floating point
- `FADD` `FSUB` `FMUL` `FDIV`

NaN canonicalization: every NaN result is the canonical quiet NaN
`0x7ff8'0000'0000'0000`; signaling NaNs are quieted before being written
(`docs/hvm/hvm-spec.md` §5.4). Unordered `FCMP` comparisons return `0`.

### 4.5 Comparisons
- Integer: `CMPEQ` `CMPNE` `CMPLT` `CMPLE` `CMPULT` `CMPULE` (`CMPLT`/`CMPLE` are signed, `CMPULT`/`CMPULE` unsigned)
- Float: `FCMPEQ` `FCMPLT` `FCMPLE`

### 4.5a HVM 1.5 scalar sub-word operations

The HVM 1.5 scalar profile keeps the 64-bit register/ABI model while making
the low-byte operation explicit:

- `ARITH_B` (`0x11`): `ADD.B`, `SUB.B`, `MUL.B`, `DIV.B`, `DIVU.B`, `REM.B`,
  and `REMU.B`.
- `SHIFT_B` (`0x12`): `SHL.B`, `SHR.B`, and `SAR.B`, with shift counts masked
  to three bits and results constrained to the low byte. These are selected
  for both standalone `<<`/`>>` expressions and compound assignments.
- `LOGIC_B` (`0x22`): `BADD` (XOR), `BMUL` (AND), and `BNOT`, normalized to
  bit 0.
- `FLOAT_ARITH_B` (`0x31`): `FADD.B`, `FSUB.B`, `FMUL.B`, and `FDIV.B` over
  canonical E4M3 FP8 bytes.

Inputs are sampled from bits 7:0. Native integer add/subtract/multiply wrap at
8 bits; signed division/remainder use signed int8 values and trap on zero or
the `-128 / -1` overflow case. `SHIFT_B` provides low-byte wrapping left
shifts, logical right shifts, and signed arithmetic right shifts. Codegen
sign-extends `int8` results and zero-extends `byte`/`bit` results. FP8
conversion helpers preserve the existing f64 language ABI when the host lacks
native FP8 instructions.

### 4.6 Branch/jump
- `BEQ` `BNE` `BLT` `BLE`
- `JMP` `JAL` `JALR` `RET`

Jump-target alignment: `JAL`/`JMP`/`JALR` targets must be 4-byte aligned.
A non-4-byte-aligned `JALR` target (`rs + sign_extend(imm15)`) raises an
instruction-address-misaligned trap (scause = 0) before any register update.
Unlike an architecture that silently masks low bits, HVM requires the full
sum to be 4-byte aligned.

### 4.7 Memory (Standard Load/Store)
- Loads: `LD.B` `LD.BU` `LD.H` `LD.HU` `LD.W` `LD.WU` `LD.D`
- Stores: `ST.B` `ST.H` `ST.W` `ST.D`
- Pair operations: `LD.P` `ST.P`
- Address: `LDA`

### 4.8 Atomic memory
- `LR.D` `SC.D`: Load-reserve / store-conditional for atomic synchronisation.

HVM uses an 8-byte reservation granule and defines the pair as acquire-release
for the atomic operation. HVM does not encode RISC-V `aq`/`rl` bits, so this is
not RISC-V LR/SC encoding compatibility. The base HVM memory model is
sequentially consistent; platform-specific device and DMA barriers remain
profile-defined.

### 4.9 Stack/frame
- `PUSH` `POP` `ENTER` `LEAVE` `ADJSP` `FRAME`

### 4.10 Calls/linking
- `CALL` `TAILCALL` (J-format, 20-bit relative offset)

### 4.11 Hardware/System
- `SYSCALL`: Trigger a system call to the runtime.
- `BREAK`: Trap to debugger.

**SYSCALL calling convention**: The immediate field selects the service; arguments are
passed in `r2` (and `r3` for two-argument calls, `r4` for three-argument calls).
The result is written to `rd`. See `docs/hvm/hvm-spec.md` §7 for the full
syscall number table.

### 4.12 System/Trap (system profile; privileged)
- `ECALL`: Trap to supervisor mode. `ECALL` is legal in both U-mode (scause = 8)
  and S-mode (scause = 9); it is illegal in U-mode only when the system profile
  is not active. The syscall number is not encoded in `ECALL` (use `SYSCALL`
  for runtime services). In the hosted HVMJIT profile HVM runs in S-mode with
  no host-side monitor, so `ECALL` surfaces as an unhandled system-profile trap
  rather than performing a privilege transition.
- `TRAPRET`: Return from supervisor trap (S-mode only; illegal trap/scause=2 in U-mode).
  In the hosted profile there is no S-mode handler, so it surfaces as an
  unhandled system-profile trap.
- `CSRRW`: Atomic read-write of a CSR (S-mode only; traps/scause=2 in U-mode). The CSR
  address is the low 12 bits of the HVM `imm15` field; the upper three bits
  must be zero. CSR addresses in the HVM system window are 0x000..0x008
  (`sstatus`, `stvec`, `sepc`, `scause`, `stval`, `satp`, `stime`, `stimecmp`,
  `feature0`); `stime` (0x006) and `feature0` (0x008) are read-only and any
  write reports an illegal-instruction trap (scause=2).
- `SFENCE.VMA`: TLB flush after page-table modification (S-mode only; scause=2 in U-mode).

The hosted `HVMJIT` profile does not emulate physical privilege levels, so
`ECALL` and `TRAPRET` report an unhandled trap there; `SFENCE.VMA` is a no-op.
Physical processors and system simulators must implement the system-profile
behavior in `hvm-spec.md` section 9.

### 4.13 HVM 1.5 runtime and green-compute extensions
- `RETAIN` `RELEASE`: Non-trapping reference-count update helpers for managed Hoo objects.
- `ICACHE.RNG`: Invalidate instruction/JIT cache state for an address range after code generation.
- `LOOP.SET` `LOOP.DECBR`: Optional hardware-loop support for low-power counted loops.
- `PREFETCH.R` `PREFETCH.W` `PREFETCH.NTA` `MEMZERO.HINT`: Advisory memory-traffic hints. Legal implementations may ignore them.
- `ALLOC.BUMP`: Optional thread-local allocation-buffer fast path; zero return means fall back to runtime allocator.
- `RDPROF`: Privileged (S-mode-only) HVM-Prof counter read. `rd = prof_counter(selector, imm15)`;
  executing in U-mode traps (see section 9.7) with `scause = 2`. The hosted HVMJIT
  profile exposes no counters, so `rd = 0` and the selector/`imm15` operands are
  ignored; a platform profile may return nonzero values.
- `CHK.B`: Optional bounds check (HVM-Cap). If `ptr < bound` then `rd = ptr` and
  execution continues; if `ptr >= bound` the instruction traps with `scause = 20`
  (bounds/null fault) and `rd` is not written. No memory is accessed. The trap is
  precise: in the hosted profile the JIT returns a soft error and the interpreter
  records the violation, so `rd` holds its pre-instruction value.
- `LD.D.NZ`: Optional null-checking 64-bit load (HVM-NZ). Computes the effective
  address `addr = rs + imm15`. If `addr == 0` the instruction traps with
  `scause = 20` (null-pointer fault) and `rd` is not written; otherwise the 8-byte
  load proceeds and additionally traps with `scause = 20` if `addr` is not
  8-byte aligned. The trap is precise as for `CHK.B`.
- `BR.HINT`: Optional branch/code-layout hint.
- `DOORBELL`: HVM-A accelerator doorbell dispatch (memory-mapped). Traps with
  `scause = 2` when `feature0.Accel` is clear (HVM-A not implemented), in which
  case no accelerator state is touched. See section 9.7.

### 4.14 HVM-V vector extension
- Configuration: `VSETVL`
- Memory: `VLD.V` `VST.V` `VLDS.V` `VSTS.V` `VLDX.V` `VSTX.V`
- Arithmetic: `VADD.VV` `VADD.VX` `VSUB.VV` `VSUB.VX` `VMUL.VV` `VMUL.VX` `VDIV.VV` `VDIV.VX` `VFMACC.VV` `VFMACC.VF`
- Compare/mask: `VCOMP.VV` `VCOMP.VX` `VMERGE.VVM` `VFIRST.M`
- Reductions: `VREDADD.VS` `VREDMIN.VS` `VREDMAX.VS`
- Bit/shift: `VSLL.VV` `VSLL.VX` `VSRL.VV` `VSRL.VX` `VAND.VV` `VOR.VV` `VXOR.VV`

Vector support is profile-gated. Implementations that expose HVM-V must save/restore vector state according to the ABI profile and feature flags.

## 5. Lowering Rules (Software Implemented)

Operations removed from the ISA and now lowered to the above set:

- **Objects**: `NEW` -> `CALL hoo_alloc`.
- **Arrays**: `NEWA` -> `CALL hoo_alloc`.
- **Field/Element Access**: `LDF`/`STF`/`LDELEM` -> Explicit pointer arithmetic + `LD.D`/`ST.D`.
- **Exceptions**: `TRY`/`THROW` -> `CALL hoo_push_handler` + control flow.

## 6. Canonical Opcode Table

The canonical machine-readable definition is `docs/hvm/hvm_instruction_set.csv`.
That CSV is normative.

**Redundancy notes**:
- `JAL` (base32, 16-bit offset) and `CALL` (escape32, 20-bit offset) are semantically identical. `CALL` provides a larger reachable range; `JAL` saves code space when the offset fits in 16 bits.
- `JMP` (base32, 16-bit offset) and `TAILCALL` (escape32, 20-bit offset) are semantically identical. `TAILCALL` provides a larger range; `JMP` saves code space when the offset fits.

Keep this document synchronized with that CSV and `docs/hvm/hvm-spec.md`.
