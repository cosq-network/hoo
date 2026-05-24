# HVM Verilog Implementation Guide

Version: `1.0`

This document gives a minimal RTL plan for implementing the HVM microprocessor in Verilog.

The intent is not to define a final commercial product. The goal is to provide a clean, small, synthesizable baseline that can grow into a real processor core.

## 1. Design Style

- synchronous, positive-edge clocked RTL
- simple reset path
- clear separation between datapath and control
- parameterized widths where practical
- no hidden state outside documented registers

## 2. Top-Level Module

Suggested top-level module:

`hvm_core`

### 2.1 Main Ports

- `clk`
- `rst_n`
- instruction bus interface
- data bus interface
- interrupt inputs
- debug inputs
- trace outputs

### 2.2 Internal Blocks

- program counter unit
- instruction fetch unit
- instruction decode unit
- register file
- integer ALU
- floating-point unit
- branch unit
- load/store unit
- trap and syscall unit
- control/state machine
- optional MMU/TLB wrapper

## 3. Module Breakdown

### 3.1 `pc_unit`

Responsibilities:

- hold current PC
- apply branch/jump targets
- handle trap vectors
- sequence normal fetches

### 3.2 `ifetch`

Responsibilities:

- fetch instruction words
- detect escape-prefixed extended instructions
- handle instruction cache hits and misses

### 3.3 `decoder`

Responsibilities:

- decode opcode, func, and format
- extract register operands and immediates
- classify instruction type
- emit control signals for downstream units

### 3.4 `regfile`

Responsibilities:

- 32 x 64-bit registers
- hardwire `r0` to zero
- support 2 read ports and 1 write port as a baseline
- optionally support extra read ports for call-heavy pipelines

### 3.5 `alu_int`

Responsibilities:

- integer arithmetic and logic
- comparison flags
- shift operations
- multiply/divide/remainder

### 3.6 `fpu`

Responsibilities:

- 64-bit floating-point math
- float compare
- optional multi-cycle execution

### 3.7 `lsu`

Responsibilities:

- byte/halfword/word/doubleword memory access
- alignment checks
- sign/zero extension
- stack access support

### 3.8 `branch_unit`

Responsibilities:

- branch condition evaluation
- direct and indirect control transfer
- link register writeback

### 3.9 `trap_unit`

Responsibilities:

- syscall entry
- breakpoint trap
- illegal instruction trap
- fault reporting
- exception return path

## 4. Pipeline Behavior

### 4.1 Baseline Pipeline

A clean first version should use a 5-stage pipeline:

- IF
- ID
- EX
- MEM
- WB

### 4.2 Hazards

Required handling:

- register data hazards
- load-use hazards
- branch hazards
- call/return hazards

Baseline implementation can use:

- stall insertion
- forwarding paths
- branch flush logic

## 5. Encoding Support

The RTL must understand the current HVM instruction contract:

- base32 instructions are 4 bytes
- escape32 instructions are 8 bytes
- `0xFE` indicates extended encoding
- opcode values above `0x7F` are escape32

The decoder must not guess the encoding from the mnemonic.

## 6. Execution Semantics

### 6.1 Arithmetic

Supported operations:

- `ADD`, `SUB`
- `MUL`, `DIV`, `DIVU`, `REM`
- `SHL`, `SHR`, `SAR`
- `AND`, `OR`, `XOR`, `NOT`

### 6.2 Control Flow

Supported operations:

- `BEQ`, `BNE`, `BLT`, `BLE`
- `JMP`, `JAL`, `JALR`, `RET`
- `CALL`, `TAILCALL`

### 6.3 Memory

Supported operations:

- `LD.B`, `LD.BU`, `LD.H`, `LD.HU`, `LD.W`, `LD.WU`, `LD.D`
- `ST.B`, `ST.H`, `ST.W`, `ST.D`
- `LDA`
- `PUSH`, `POP`, `ENTER`, `LEAVE`, `ADJSP`, `FRAME`

### 6.4 System

Supported operations:

- `SYSCALL`
- `BREAK`

## 7. Suggested Verilog Interfaces

### 7.1 Instruction Bus

- `instr_valid`
- `instr_ready`
- `instr_addr`
- `instr_data`
- `instr_size`

### 7.2 Data Bus

- `mem_valid`
- `mem_ready`
- `mem_addr`
- `mem_wdata`
- `mem_rdata`
- `mem_we`
- `mem_be`

### 7.3 Trap and Debug

- `trap_valid`
- `trap_cause`
- `trap_pc`
- `debug_halt`

## 8. Reset and Boot

On reset:

- PC loads from reset vector
- registers clear to known state
- trap state clears
- pipeline flushes

Boot should enter:

- ROM code
- firmware init
- runtime handoff

## 9. Minimal RTL Feature Set

The first implementation should include:

- integer core
- register file
- branches and calls
- basic load/store
- trap handling
- syscall entry
- instruction decode for base32 and escape32

Optional in the first revision:

- FPU
- MMU
- cache subsystem
- branch prediction
- out-of-order execution

## 10. Verification Strategy

Recommended verification layers:

- unit testbench per module
- ISA instruction tests
- pipeline hazard tests
- memory alignment tests
- trap and syscall tests
- formal assertions for critical invariants

## 11. Invariants Worth Asserting

- `r0` always reads as zero
- illegal opcodes trap
- base32 instructions never decode as escaped
- escaped instructions never decode as base32
- stack pointer updates are monotonic per instruction semantics
- branch targets are computed from documented offsets

## 12. Example Implementation Order

1. register file
2. ALU
3. decoder
4. PC and branch unit
5. load/store unit
6. trap unit
7. integration testbench
8. cache and MMU wrappers
9. floating-point unit

## 13. Synthesis Notes

- keep the first version small and readable
- prefer inferred RAM only where the target toolchain supports it
- keep reset behavior deterministic
- avoid unnecessary combinational feedback
- isolate platform-specific memory and bus adapters from the core

## 14. Deliverable Set

For a usable project handoff, the RTL package should include:

- `hvm_core.v`
- `hvm_regfile.v`
- `hvm_decoder.v`
- `hvm_alu.v`
- `hvm_lsu.v`
- `hvm_branch.v`
- `hvm_trap.v`
- `hvm_fpu.v` optionally
- `tb_hvm_core.v`
- design notes and ISA mapping tables

