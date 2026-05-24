# HVM Microprocessor Architecture

Version: `1.0`

This document defines a minimal but complete 64-bit HVM microprocessor architecture suitable for RTL implementation and later scaling to laptop- and mobile-class SoCs.

The design is based on the current HVM ISA profile:

- 64-bit general-purpose registers
- little-endian byte-addressable memory
- load/store execution model
- explicit branches and calls
- HVM instructions as defined in `docs/hvm/hvm_instruction_set.csv`

## 1. Design Goals

- Keep the core small enough for a clean Verilog implementation.
- Preserve HVM hardware semantics exactly.
- Support a modern compiler backend without hidden VM behavior.
- Make the design scalable from a single core to a multicore SoC.
- Keep the architecture understandable for firmware, OS, and compiler teams.

## 2. Architectural Positioning

The processor is an ARM64-class 64-bit RISC core in spirit, but it is not ARM-compatible.

It should provide:

- a fixed-width core instruction path for fast decode
- a simple register file model
- explicit integer, floating-point, branch, and memory units
- a clean trap and syscall path
- a predictable ABI for compiled Hooc code

## 3. Programmer Model

### 3.1 Registers

- 32 general-purpose 64-bit registers
- `r0` is hardwired zero
- `r1..r8` are argument and return registers
- `r9..r15` are caller-saved temporaries
- `r16..r28` are callee-saved
- `r29` is link register
- `r30` is frame pointer
- `r31` is stack pointer

### 3.2 Data Types

- integer: 8, 16, 32, 64 bits
- floating point: 64-bit IEEE-754
- pointers: 64-bit
- booleans: integer-backed, usually `0` or `1`
- characters: Unicode code points carried as integer values in software-visible runtime objects

### 3.3 Memory Model

- byte-addressed little-endian memory
- flat physical address space
- optional MMU and privilege separation for consumer products
- alignment enforced by hardware or trapping policy

## 4. Execution Model

The core uses a classic 5-stage pipeline:

1. Fetch
2. Decode
3. Execute
4. Memory
5. Writeback

This is the simplest practical baseline for HVM because it matches the ISA style and keeps timing behavior easy to reason about.

### 4.1 Decode Strategy

The decode unit should recognize:

- base 32-bit opcodes
- escape-prefixed extended opcodes
- R, I, B, J, and RI encodings

The decoder should not infer behavior from the mnemonic. It should decode from opcode, func, and encoding class.

### 4.2 Control Flow

Required control-flow instructions:

- `BEQ`, `BNE`, `BLT`, `BLE`
- `JMP`, `JAL`, `JALR`, `RET`
- `CALL`, `TAILCALL`

The core should support:

- direct relative jumps
- indirect register jumps
- function call and return linkage
- tail-call optimization in hardware or in the compiler

## 5. Functional Units

### 5.1 Integer ALU

Supports:

- `ADD`, `SUB`
- `MUL`, `DIV`, `DIVU`, `REM`
- `SHL`, `SHR`, `SAR`
- `AND`, `OR`, `XOR`, `NOT`
- compare operations

### 5.2 Floating-Point Unit

Supports:

- `FADD`, `FSUB`, `FMUL`, `FDIV`
- floating comparisons

### 5.3 Load/Store Unit

Supports:

- byte, halfword, word, and doubleword loads/stores
- address generation via `LDA`
- stack operations via `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`

### 5.4 Trap and System Unit

Supports:

- `SYSCALL`
- `BREAK`
- faults for illegal instructions, alignment errors, protection errors, and page faults

## 6. Privilege Model

A practical consumer implementation should include:

- user mode
- supervisor mode
- machine or firmware mode

Minimum privileged features:

- trap entry and return
- interrupt masking
- timer interrupt
- external interrupt
- access control for MMIO and privileged registers

## 7. Cache and Memory Hierarchy

For a practical laptop/mobile core, the architecture should allow:

- L1 instruction cache
- L1 data cache
- shared L2 cache
- optional last-level cache in the SoC

Recommended baseline:

- 32 KB I-cache
- 32 KB D-cache
- private L2 per core or shared L2 cluster

## 8. TLB and MMU

The core should support:

- virtual memory translation
- page table walking
- instruction and data TLBs
- optional huge pages

This is required for modern operating systems and browser-class workloads.

## 9. Interrupts and Exceptions

The architecture should expose:

- synchronous exceptions
- asynchronous interrupts
- debug breakpoint trap
- syscall trap

The compiler/runtime can lower high-level exceptions to software, but the CPU still needs a real trap path for robust system software.

## 10. Security Features

Recommended baseline features:

- secure boot chain
- memory protection
- privileged register fencing
- optional pointer authentication or control-flow hardening hooks
- debug lockout for production devices

## 11. Performance Targets

For a modern consumer implementation:

- out-of-order execution is optional for first silicon, not required for the ISA
- a simple in-order core is acceptable for v1
- later revisions can add speculation, wider issue, and better branch prediction

Suggested first target:

- 2 to 4 instructions per cycle sustained on common integer workloads
- predictable memory ordering
- low-power idle states

## 12. Implementation Strategy

Phase 1:

- single in-order core
- private L1 caches
- simple MMU
- trap and syscall support

Phase 2:

- multicore cluster
- shared cache
- coherent interconnect
- stronger power management

Phase 3:

- media accelerators
- security enclave
- higher-performance memory subsystem

## 13. Firmware and Software Expectations

The platform should ship with:

- boot ROM
- minimal secure monitor
- device tree or ACPI-style hardware description
- kernel support for interrupts, timers, MMIO, and power states

## 14. Verification Plan

The architecture should be verified with:

- ISA compliance tests
- microarchitecture stress tests
- memory and alignment tests
- trap and interrupt tests
- compiler-generated code tests

