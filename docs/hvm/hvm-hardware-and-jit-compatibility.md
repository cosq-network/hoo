# HVM Hardware and JIT Compatibility Requirements

Version: `1.0`

This document defines the hardware and SoC enhancements required to turn HVM into a real microprocessor platform for consumer laptops and mobile devices, while preserving compatibility with an enhanced `src/hvm/HVMJIT.cpp`.

The goal is not to redesign HVM into a different ISA. The goal is to keep the current HVM software contract intact and raise the implementation from a runtime-oriented execution model to a hardware-capable architecture that can support:

- a real CPU core
- a real SoC and board/platform stack
- a deterministic compiler and JIT backend
- the current Hooc runtime model

## 1. Design Boundary

The hardware must implement the same architectural rules the JIT already depends on:

- 64-bit little-endian general-purpose execution
- explicit register-based calling convention
- base32 and escape32 instruction encodings
- load/store memory model
- trap and syscall behavior that is predictable and inspectable
- runtime-managed objects for strings, arrays, maps, exceptions, and ARC

The JIT may optimize, inline, or fold sequences, but it must not invent behavior that the hardware does not also provide.

## 2. Architectural Goal

The target is a practical HVM system in three layers:

1. CPU core
2. SoC and board
3. JIT/runtime compatibility layer

This keeps the ISA stable while allowing hardware acceleration and software fallback to coexist.

## 2.1 Hardware and JIT Compatibility Rule

The hardware profile and the JIT must be compatible by construction.

That means:

- the JIT may simulate hardware features, but it must not depend on behavior the hardware cannot implement
- the hardware may accelerate runtime operations, but it must not change the language or ABI contract
- any ISA extension added for hardware must either be implementable in the JIT or have a defined software fallback
- the same instruction meaning must hold across silicon, simulator, and JIT execution
- if the JIT and hardware disagree, the ISA definition is wrong and must be fixed before the design moves forward

Compatibility is the primary rule. Performance is secondary.

## 3. Hardware Enhancements Required

### 3.1 Instruction Fetch and Decode

The hardware must support the canonical HVM encoding rules:

- base32 instructions are 4 bytes
- escape32 instructions are 8 bytes
- the opcode registry is normative, not mnemonic-driven
- the decoder must validate encoding class, opcode, and operand layout

Required enhancements:

- instruction predecode for base32 vs escape32
- aligned fetch and byte lane extraction
- instruction size reporting for PC advancement and branch target calculation
- decode-visible illegal-instruction traps for malformed streams

### 3.2 Core Execution Model

The first hardware core can be in-order, but it must be architecturally complete.

Required capabilities:

- integer ALU
- branch and compare unit
- load/store unit
- stack/frame helpers
- optional FPU
- trap and exception entry

Recommended first silicon model:

- 5-stage in-order pipeline
- static or simple dynamic branch prediction
- one-cycle integer ALU where possible
- multi-cycle divide and floating-point operations if needed

### 3.3 Register and ABI Compatibility

The CPU must preserve the current register convention:

- `r0` is hardwired zero
- `r1..r8` are argument and return registers
- `r29` is link register
- `r30` is frame pointer
- `r31` is stack pointer

Enhancements required:

- precise writeback behavior for link and frame-related instructions
- hardware-visible zero register semantics
- ABI-stable call/return and tail-call behavior
- strict preservation of callee-saved registers across traps and firmware transitions

### 3.4 Memory System

The hardware memory model must remain compatible with the JIT’s expectations.

Required features:

- byte-addressable little-endian memory
- aligned and unaligned access policy defined by ISA or trap behavior
- load/store byte, halfword, word, doubleword
- stack accesses and frame accesses
- address translation support for OS-class deployments

Recommended enhancements:

- L1 I-cache and D-cache
- optional shared L2
- MMU with TLBs
- access permission checks
- page-fault and protection-fault traps

### 3.5 Trap and Debug Model

The JIT already relies on runtime traps and explicit handler routing. Hardware must provide an equivalent model.

Required trap classes:

- illegal instruction
- alignment fault
- page fault
- protection fault
- syscall trap
- breakpoint/debug trap
- external interrupt
- timer interrupt

Required debug behavior:

- halt or single-step support for bring-up
- breakpoint vector entry
- debug lockout for production devices
- firmware-controlled debug unlock policy

### 3.6 Privilege and Firmware

Consumer hardware needs at least three domains:

- user
- supervisor
- firmware or machine mode

Required enhancements:

- privilege transitions
- interrupt masking
- timer programming
- MMIO protection
- boot ROM handoff
- secure monitor or equivalent root-of-trust path

### 3.7 Atomicity and Concurrency

The current JIT and runtime can operate without a full concurrency model, but real hardware should not paint itself into a corner.

Recommended hardware support:

- atomic read-modify-write primitives
- cache coherence for multicore systems
- memory ordering rules that are easy to document
- inter-core interrupt delivery

If atomics are not in the first silicon cut, the ISA and board design should still leave room for them.

### 3.8 Power and Thermal Control

Consumer hardware must support:

- frequency and voltage scaling
- idle states
- wake events
- thermal throttling
- per-core power gating
- suspend and resume

These features are platform requirements, not JIT requirements, but they are necessary for real product deployment.

### 3.9 ISA Expansions Kept for Hardware and JIT Parity

The current core ISA is enough for the existing JIT contract, but real hardware needs a small set of explicit instruction-set extensions to support privilege, virtual memory, multicore execution, and production debug.

This section intentionally keeps only instructions and control operations that can be implemented in hardware and also simulated in a JIT runtime with clear semantics.

These are hardware expansions, not language changes. They should be added in a way that keeps current HVM code valid and keeps the JIT able to target both older and newer cores.

#### 3.9.0 Suggested Instruction Set

The following is the concrete expansion set that should be documented in the ISA if you want a hardware/JIT shared contract:

| Instruction | Must implement in hardware | JIT can simulate | Fallback behavior |
| --- | --- | --- | --- |
| `CSRRD rd, csr` | Yes | Yes | Read from JIT/runtime feature state |
| `CSRWR csr, rs` | Yes | Yes | Update JIT/runtime control state and invalidate dependent caches if needed |
| `FENCE` | Yes | Yes | Emit host compiler/runtime fences |
| `ICACHE.IALL` | Yes | Yes | Flush JIT code cache or patch state |
| `TLBI asid, va` | Yes | Yes | Update runtime page-map metadata and clear translation caches |
| `AMOSWAP rd, rs1, rs2` | Yes | Yes | Use host atomic exchange or a runtime lock primitive |
| `AMOADD rd, rs1, rs2` | Yes | Yes | Use host atomic fetch-add |
| `AMOXOR rd, rs1, rs2` | Yes | Yes | Use host atomic fetch-xor if available, else locked emulation |
| `AMOAND rd, rs1, rs2` | Yes | Yes | Use host atomic fetch-and if available, else locked emulation |
| `AMOOR rd, rs1, rs2` | Yes | Yes | Use host atomic fetch-or if available, else locked emulation |
| `CAS rd, rs1, rs2, rs3` | Yes | Yes | Use host compare-exchange or a runtime critical section |
| `TRAPRET` | Yes | Yes | Restore JIT-visible privilege state and resume at saved PC |
| `WFI` | Yes | Yes | Yield to host runtime scheduler or idle loop |
| `STEP` | Yes | Yes | Run one instruction and stop in the inspector |
| `WATCH` | Yes | Yes | Use JIT instrumentation or host debugger support |

Rules:

- if an instruction is listed here, it belongs in the shared hardware/JIT profile
- if the JIT cannot simulate it, it should not be added to this profile yet
- if hardware cannot implement it cleanly, it should be moved to a later extension document
- the fallback behavior must preserve Hooc semantics even when the extension is absent

#### 3.9.1 Not in the Shared Profile Yet

These ideas may be useful later, but they are intentionally not part of the current shared hardware/JIT contract:

| Candidate extension | Why it is excluded for now | Suggested status |
| --- | --- | --- |
| SIMD/vector execution | Hardware support is useful, but JIT semantics and lowering strategy are not yet fixed | Future extension |
| Wider floating-point formats | Not required by the current Hooc runtime contract | Future extension |
| Bit-manipulation accelerators | Useful for optimization, but not necessary for compatibility | Future extension |
| Cryptographic instructions | Platform-specific and not required for base compatibility | Future extension |
| Prefetch hints | Performance-only, with no required semantic effect | Future extension |
| Saturating/narrowing ops | Not needed for the current ISA contract | Future extension |

Any future extension added here must still satisfy the compatibility rule in section 2.1.

#### 3.9.2 Privileged System Register Access

Required:

- read/write access to machine, supervisor, and user-visible control registers
- status, cause, trap vector, interrupt enable, scratch, and return-PC registers
- timer compare and cycle counters
- MMU control registers, including page table base and address-space ID
- debug control and debug status registers

Suggested ISA shape:

- a small CSR-style instruction family, or
- a dedicated HVM system-register read/write family

Why it is required:

- boot firmware needs to configure the core
- the OS needs to enable interrupts, MMU state, and timers
- the debugger needs a stable entry point
- the JIT needs feature discovery and runtime control hooks

#### 3.9.3 Memory Ordering and Barrier Instructions

Required:

- full fence/barrier instruction for load/store ordering
- instruction-cache synchronization after code generation or patching
- TLB synchronization after page-table or address-space changes

Why it is required:

- multicore ARC and runtime state must remain correct
- generated code and runtime stubs must remain coherent
- the OS must be able to switch memory mappings safely

#### 3.9.4 Atomic Read-Modify-Write Operations

Required for multicore and strongly recommended even for single-core silicon:

- atomic swap
- atomic add
- atomic bitwise operations
- compare-and-swap or load-linked/store-conditional
- atomic exchange for lock-free runtime structures

Why it is required:

- ARC refcount updates must be race-safe
- runtime maps and queues need synchronization
- kernel and firmware code need lock primitives

#### 3.9.5 Trap Return and Low-Power Control

Required:

- explicit trap-return instruction for privilege transitions
- wait-for-interrupt or sleep instruction
- optional yield hint for schedulers and runtime loops

Why it is required:

- firmware and OS trap handlers must return cleanly
- consumer devices need real idle states
- scheduler and timer-driven wakeups need a low-power path

#### 3.9.6 Cache and Translation Maintenance

Required:

- data-cache clean/invalidate operations
- instruction-cache invalidate/synchronize operations
- TLB invalidate by address-space or page-table scope

Why it is required:

- self-modifying code and JIT code patches must execute correctly
- page table edits must become visible deterministically
- firmware recovery and kernel transitions need explicit maintenance controls

#### 3.9.7 Debug and Trace Hooks

Required:

- breakpoint and single-step support
- watchpoint support for bring-up and validation
- optional performance counter access

Why it is required:

- hardware bring-up requires deterministic halting
- kernel debugging and JIT debugging need clean trap behavior
- performance tuning requires visibility into the core

## 4. SoC and Board Enhancements Required

The CPU core alone is not enough for laptop or mobile use.

### 4.1 SoC Blocks

The SoC should integrate:

- memory controller
- cache hierarchy
- PCIe or equivalent high-speed interconnect
- NVMe or storage controller
- USB controller
- display pipeline
- audio subsystem
- GPIO, I2C, SPI, UART, I3C
- camera/sensor interfaces
- secure boot and key storage
- watchdog and timer blocks

### 4.2 Laptop Board Requirements

For consumer laptops, the board should include:

- LPDDR memory
- NVMe storage
- USB-C power and data
- display output
- audio codec
- Wi-Fi and Bluetooth
- battery charger and fuel gauge
- thermal sensors and fan control

### 4.3 Mobile Board Requirements

For handheld or phone-class devices, the mainboard should additionally support:

- tighter power envelope
- MIPI display and camera links
- optional modem integration
- aggressive sleep/wake transitions
- compact PMIC integration
- secure provisioning and manufacturing mode

### 4.4 Firmware and Board Description

Software must be able to discover the board cleanly.

Required platform descriptions:

- device tree
- ACPI-like description where appropriate
- firmware-provided memory map
- interrupt map
- MMIO map
- feature and revision identifiers

## 5. JIT Compatibility Contract

The JIT should remain a valid software target even after hardware exists.

That means:

- every JIT-visible instruction must have a stable hardware meaning
- the JIT and hardware must agree on register semantics
- the JIT and hardware must agree on instruction size and encoding
- the JIT and hardware must agree on trap behavior
- the JIT and hardware must agree on ABI call boundaries
- the JIT and hardware must agree on which instructions are mandatory, optional, or simulated
- the JIT must be able to run the same Hooc program that the hardware runs without changing semantics

### 5.1 Runtime Objects Stay in Software

Do not move language objects into the ISA.

The following remain runtime-managed:

- strings
- characters
- arrays
- maps
- exceptions
- ARC refcounts

Hardware may accelerate memory movement and trap handling, but the object model stays above the ISA.

### 5.2 Syscall and Runtime Hooks Stay Stable

The JIT currently depends on runtime hooks for:

- allocation
- retain/release
- refcount and type queries
- handler push/pop
- throw/rethrow dispatch
- string and character helpers

These hooks should remain stable even if hardware gains richer trap support.

### 5.3 JIT Feature Negotiation

The enhanced JIT should detect whether a target core exposes the new ISA features and use them opportunistically.

Minimum expectations:

- if barriers exist, use them around code generation and runtime synchronization
- if atomics exist, use them for ARC and lock primitives
- if privilege registers exist, use them for timer, interrupt, and MMU setup
- if debug and trace hooks exist, expose them to the runtime inspector and debugger

Fallback behavior:

- if a feature is missing, the JIT must continue to use software fallback paths
- the absence of an extension must not change Hooc language semantics

### 5.4 Excluded From This Compatibility Set

The following are not part of this document's retained ISA expansion set, even though they may be valid future extensions:

- SIMD/vector execution
- fused multiply-add and wider FP forms
- bit-manipulation and population-count helpers
- cryptographic acceleration
- prefetch hints
- saturation and narrowing operations

They can be documented separately if a later hardware profile wants them, but they are outside the current hardware/JIT parity contract.

### 5.5 Control-Flow Compatibility

The JIT expects these control-flow behaviors:

- `BEQ`, `BNE`, `BLT`, `BLE` are relative branches
- `JMP`, `JAL`, `CALL`, `TAILCALL` use word-scaled relative offsets
- `JALR` uses register-plus-immediate control transfer
- `RET` returns through the link register

Hardware must preserve these semantics exactly.

### 5.6 Memory Safety and Alignment Compatibility

The JIT currently enforces alignment in some places and raises errors in others. Hardware should do the same or stricter, but not looser.

Recommended rule:

- aligned accesses succeed normally
- misaligned accesses either trap or are explicitly documented as supported
- the choice must be fixed and visible to the JIT backend

### 5.7 Shared Compatibility Checklist

Before adding a new hardware feature or JIT lowering rule, verify all of the following:

- the instruction exists in the ISA document
- the hardware can implement it with clear semantics
- the JIT can simulate it or provide a software fallback
- the feature does not alter Hooc language semantics
- the feature does not break existing binaries or runtime hooks
- the feature has a documented trap, failure, or fallback behavior

If any item fails, the feature does not belong in the shared hardware/JIT profile yet.

## 6. Required Enhancements to `src/hvm/HVMJIT.cpp`

The JIT should evolve into a hardware-aware backend without changing the language model.

### 6.1 Target Feature Discovery

Add a backend feature model so the JIT can query:

- base32 only vs base32 plus escape32
- cache and MMU availability
- trap/vector support
- atomics availability
- FPU availability
- hardware ARC or syscall acceleration, if any

### 6.2 Strict Encoding Awareness

The JIT should continue to treat the instruction CSV as normative and must not infer encoding from opcode values alone.

Required behavior:

- decode with explicit `Encoding`
- reject malformed escape/base mismatches
- preserve instruction length for PC and relocation math

### 6.3 Hardware-Backed Lowering Paths

The JIT can emit hardware-friendly sequences for:

- loads and stores
- stack frame setup and teardown
- branch and call handling
- trap/syscall entry

But it should keep a software fallback for:

- object allocation
- ARC updates
- exception handler management
- string and character helpers

### 6.4 Trap and Exception Integration

The current JIT already models exception handlers in software. Hardware should expose a compatible trap path, but the JIT should still be able to manage:

- shadow handler stacks
- rethrow routing
- debugger breakpoints
- runtime error propagation

### 6.5 Memory Layout Assumptions

The JIT currently uses a process-local memory image and runtime helper calls.

For hardware compatibility, it should treat the following as explicit configuration:

- stack base and growth direction
- heap base and limits
- page size
- MMIO window
- ROM and firmware mappings

## 7. Recommended Implementation Staging

### Phase 1

- single-core in-order CPU
- base32 and escape32 decode
- software-managed runtime objects
- trap/syscall support
- basic caches

### Phase 2

- MMU and virtual memory
- stronger debug and exception handling
- multicore and coherency
- SoC integration for laptop-class devices

### Phase 3

- mobile SoC variant
- enhanced power management
- richer media and camera blocks
- optional hardware acceleration for runtime-adjacent operations

## 8. Practical Rule Set

If a feature would make the hardware incompatible with `src/hvm/HVMJIT.cpp`, it should be treated as an extension, not a replacement.

If a feature is required by consumer hardware but not by the current JIT, it should be added in a way that keeps the existing software contract stable.

The safe default is:

- keep ISA semantics fixed
- keep runtime objects in software
- add hardware only where it improves performance, power, or platform completeness
- make feature presence discoverable at runtime

## 9. Related Documents

- `docs/hvm/chip/hvm-microprocessor-architecture.md`
- `docs/hvm/chip/hvm-consumer-platform-design.md`
- `docs/hvm/chip/hvm-verilog-implementation-guide.md`
- `docs/hvm/hvm-spec.md`
- `docs/hvm/instructions.md`
