# HVM QEMU System Simulator Design

Version: `1.0`

This document describes how to build a QEMU-based system simulator for HVM hardware profiles, including a typical HVM CPU, a laptop-class SoC, and a mobile-class SoC/board.

The goal is to simulate a real machine, not just execute instructions. That means the simulator must model:

- the HVM CPU ISA
- memory and address translation
- interrupts and traps
- timers and debug entry
- device I/O and MMIO
- boot firmware and board discovery
- the same software contract used by `src/hvm/HVMJIT.cpp`

The simulator should be useful for:

- ISA bring-up
- firmware development
- OS kernel validation
- board-level device modeling
- runtime and JIT compatibility testing
- hardware/prototype co-design

## 1. What "QEMU-Based" Means Here

There are two practical ways to use QEMU for HVM:

1. **QEMU as a full system emulator**  
   QEMU owns the board, SoC, devices, interrupts, memory map, and CPU execution model.

2. **QEMU as a platform harness around an HVM execution engine**  
   QEMU owns the machine and devices, while the HVM CPU execution is delegated to a custom CPU model or translation backend.

For HVM, the first option is the right architecture. HVM is a real ISA-style machine model, so it should behave like a normal system target rather than a user-mode translator.

The cleanest design is:

- add an HVM CPU target to QEMU
- add one or more HVM machine types
- add device models for a baseline HVM SoC
- keep the HVM JIT as a separate runtime/backend that shares the same ISA rules

## 2. High-Level Design Goals

The QEMU simulator should:

- execute HVM binaries and firmware images
- expose the same instruction semantics as the hardware profile
- model the board and SoC structure explicitly
- preserve the HVM ABI and register conventions
- allow deterministic debugging and tracing
- validate behavior against the JIT and future RTL

The simulator should not:

- invent new ISA semantics
- hide board behavior behind unrealistic magic
- depend on compiler-specific shortcuts
- diverge from the documented HVM encoding contract

## 3. Recommended QEMU Architecture

Use a normal QEMU system-emulation layout:

- **CPU target**: HVM guest CPU definition
- **Machine**: board-specific wiring for HVM SoC variants
- **Memory**: RAM, ROM, MMIO, flash, and optional secure storage
- **Interrupt controller**: core-local and global interrupt routing
- **Timer block**: cycle/timer/compare support
- **Debug unit**: breakpoint, watchpoint, single-step, halt
- **Bus fabric**: simple interconnect or memory-mapped router
- **Peripheral devices**: UART, block, net, storage, display, input, I2C, SPI, GPIO, etc.

The first version should be small and honest:

- no unnecessary devices
- no guest-visible emulation gaps hidden as success
- a small number of well-defined board variants

## 4. Core CPU Model in QEMU

The HVM CPU in QEMU should model the architectural state only.

### 4.1 Architectural State

At minimum the CPU model needs:

- 32 general-purpose 64-bit registers
- program counter
- condition/status fields if the ISA requires them
- privilege level
- trap cause and trap return metadata
- interrupt pending/enable state
- timer/cycle counters
- MMU state if enabled
- debug state

### 4.2 Register Convention

The QEMU CPU model must preserve the HVM convention:

- `r0` is hardwired zero
- `r1..r8` are arguments and returns
- `r29` is link register
- `r30` is frame pointer
- `r31` is stack pointer

### 4.3 Instruction Decode

The decoder must support:

- base32 instructions
- escape32 instructions
- opcode/func based decode
- explicit instruction length
- illegal instruction traps for malformed encodings

The decoder must be driven by the normative instruction set documentation, not mnemonic assumptions.

### 4.4 Execution Model

The initial QEMU CPU can be:

- direct TCG translation for each HVM instruction
- precise architectural state updates after each instruction
- in-order semantics
- trap-aware, interrupt-aware control flow

It should model:

- arithmetic
- loads/stores
- branches and calls
- trap/syscall handling
- debug entry
- privilege transitions
- return from trap

### 4.5 JIT Compatibility

The QEMU CPU model must remain consistent with `src/hvm/HVMJIT.cpp`.

That means:

- identical register semantics
- identical instruction length handling
- identical branch offset rules
- identical trap/syscall meaning
- compatible ABI and runtime calls

If QEMU and the JIT disagree, the ISA specification is the source of truth, not the emulator.

## 5. Memory Map and Addressing

A good HVM machine should use an explicit, documented physical map.

### 5.1 Suggested Baseline Map

Example layout:

- `0x0000_0000_0000_0000` - ROM / reset vector / boot stub
- `0x0000_0000_0010_0000` - RAM start
- `0x0000_0000_1000_0000` - MMIO window
- `0x0000_0000_2000_0000` - flash / NVRAM / secure storage
- `0x0000_0000_3000_0000` - device-specific windows

This is only a proposal; the real map should be fixed in the machine document and firmware table.

### 5.2 Memory Model Rules

The simulator should support:

- little-endian memory access
- aligned byte/halfword/word/doubleword accesses
- optional faulting on misalignment
- page table translation if MMU is enabled
- MMIO read/write callbacks

### 5.3 JIT Alignment with Memory

The JIT and QEMU should agree on:

- address size
- alignment behavior
- sign/zero extension rules
- fault behavior for illegal addresses
- stack and heap growth semantics

## 6. Privilege, Trap, and Interrupt Model

The simulator must expose a real system-level control model.

### 6.1 Privilege Levels

Recommended modes:

- user
- supervisor
- machine or firmware

QEMU should treat privilege as architectural state, not as an implementation detail.

### 6.2 Traps

The machine should support:

- illegal instruction
- syscall trap
- breakpoint trap
- alignment fault
- page fault
- protection fault
- debug halt
- external interrupt
- timer interrupt

### 6.3 Trap Entry/Return

The simulator should model:

- trap vector selection
- cause register updates
- saved PC/return PC capture
- trap return instruction
- nested trap behavior if the architecture requires it

### 6.4 Interrupts

At minimum:

- timer interrupt
- software interrupt
- external interrupt line(s)
- debug interrupt or halt request

For mobile/laptop-class machines, these must be wired through a proper interrupt controller rather than ad hoc callbacks.

## 7. ISA Extensions to Support in QEMU

The shared hardware/JIT profile already identifies the extensions that belong in the common contract.

QEMU should simulate the following directly:

- `CSRRD`
- `CSRWR`
- `FENCE`
- `ICACHE.IALL`
- `TLBI`
- `AMOSWAP`
- `AMOADD`
- `AMOXOR`
- `AMOAND`
- `AMOOR`
- `CAS`
- `TRAPRET`
- `WFI`
- `STEP`
- `WATCH`

### 7.1 How QEMU Should Model Them

#### `CSRRD` / `CSRWR`

Use architectural state fields in the CPU model.

Examples:

- status register
- trap vector register
- interrupt mask
- timer compare register
- MMU base register
- debug control register

#### `FENCE`

Model as a memory-ordering barrier.

In a single-threaded emulator this may be lightweight, but it must still exist as an architectural event.

#### `ICACHE.IALL`

Invalidate the simulated instruction cache and any translated-block state affected by guest code patching.

#### `TLBI`

Invalidate translation caches for the selected page tables or ASID range.

#### Atomics

Use QEMU-supported atomic primitives or guest-visible lock emulation.

The result must be architecturally atomic even if the host implementation uses helper functions.

#### `TRAPRET`

Restore saved privilege state and resume from the trap return PC.

#### `WFI`

Enter a simulated low-power wait state until an interrupt becomes pending.

#### `STEP`

Allow single-step execution from the debug layer.

#### `WATCH`

Implement watchpoints using QEMU memory watch support or a guest-side debug trigger abstraction.

## 8. Board Variants to Model

Do not start with one giant board. Model a small family of boards.

### 8.1 HVM Dev Board

Purpose:

- bring-up
- firmware testing
- runtime/JIT validation

Suggested components:

- 1 HVM core
- RAM
- UART
- timer
- interrupt controller
- simple flash ROM
- optional virtio block and net devices

### 8.2 HVM Laptop Board

Purpose:

- consumer laptop-class validation
- OS boot
- device-driver work

Suggested components:

- 2 to 8 HVM cores
- RAM
- shared cache model or cache-aware abstraction
- UART debug console
- NVMe or virtio-blk
- USB controller
- display pipeline
- input devices
- audio codec or audio path
- RTC / timer / watchdog
- interrupt controller
- secure boot ROM

### 8.3 HVM Mobile Board

Purpose:

- low-power mobile-like platform testing
- suspend/resume
- compact device topology

Suggested components:

- 2 to 4 HVM cores or a small efficiency cluster
- LPDDR-style memory model
- timer and power controller
- display and touch input
- storage controller
- sensor buses
- optional modem abstraction
- aggressive idle and wake behavior

## 9. Device Model Strategy

### 9.1 Start with a Small Device Set

The first QEMU machine should only include the devices required to boot and run tests.

Recommended minimum:

- UART
- timer
- interrupt controller
- RAM
- ROM
- block device
- optional network device

### 9.2 Add Devices in Layers

Layer 1:

- console
- storage
- boot

Layer 2:

- networking
- input
- display

Layer 3:

- power management
- secure storage
- sensors
- camera
- audio

### 9.3 Prefer Reusable QEMU Devices Where Possible

When a generic QEMU device is enough, use it rather than inventing a custom device:

- `virtio-blk` for block storage
- `virtio-net` for networking
- `pl011`-style or equivalent UART model
- generic timer and interrupt controller

Use custom devices only when HVM needs unique semantics:

- HVM CSR block
- HVM trap/debug block
- HVM MMU or TLB maintenance block
- HVM-specific power controller

## 10. Firmware and Boot Flow

### 10.1 Boot Flow

The machine should boot in this order:

1. reset vector executes from ROM
2. machine firmware initializes RAM and devices
3. firmware configures privilege, timers, and MMU if present
4. firmware loads the kernel or runtime
5. control transfers to the OS or HVM runtime

### 10.2 Firmware Options

Possible firmware styles:

- simple custom boot ROM
- minimal monitor
- U-Boot-like boot chain
- OpenSBI-like supervisor handoff model
- ACPI or device-tree based handoff

For HVM, device tree style descriptions are the simplest start.

### 10.3 Board Description

The simulator should provide machine description data:

- RAM layout
- MMIO map
- interrupt map
- timer configuration
- CPU count and feature flags
- device model versioning

This can be exposed via:

- device tree blobs
- firmware tables
- memory-mapped configuration registers

## 11. QEMU Code Organization

A clean QEMU port should follow a structure like this:

```text
target/hvm/
  cpu.h
  cpu.c
  translate.c
  op_helper.c
  helper.h
  disas.c
  gdbstub.c

hw/hvm/
  hvm-machine.c
  hvm-soc.c
  hvm-irq.c
  hvm-timer.c
  hvm-debug.c
  hvm-csr.c
  hvm-bootrom.c

include/hw/hvm/
  hvm-machine.h
  hvm-soc.h
  hvm-irq.h
```

### 11.1 CPU Target Files

- `cpu.c`: CPU state, reset, realize, interrupts, feature flags
- `translate.c`: HVM instruction translation to TCG ops
- `op_helper.c`: helper calls for traps, atomics, memory special cases
- `disas.c`: disassembly for trace and debugging
- `gdbstub.c`: register mapping and debugger support

### 11.2 Machine Files

- `hvm-machine.c`: board creation and wiring
- `hvm-soc.c`: SoC instantiation
- `hvm-irq.c`: interrupt controller wiring
- `hvm-timer.c`: timer block
- `hvm-debug.c`: debug and watchpoint infrastructure
- `hvm-csr.c`: machine-level control registers
- `hvm-bootrom.c`: reset ROM image and boot handoff

## 12. How the HVM CPU Should Be Translated in QEMU

### 12.1 Direct Translation

Each HVM instruction should be translated into TCG operations or equivalent helper calls.

Examples:

- `ADD` -> integer add
- `LD.D` -> translated memory load with alignment check
- `ST.D` -> translated store with ARC or memory hooks if required
- `BEQ` -> compare and conditional branch
- `CALL` -> relative branch plus link register update
- `TRAP` -> helper that updates trap state and jumps to vector

### 12.2 Translation Granularity

Use reasonable translation blocks:

- single basic block or instruction sequence
- stop on control flow boundaries
- stop on traps, syscalls, and privilege transitions
- stop when self-modifying code or debug state requires revalidation

### 12.3 Host/Guest Boundary

The simulator should not inline semantics that belong to the guest ISA.

Keep the following in guest-visible helper logic:

- traps
- interrupts
- MMU faults
- watchpoints
- atomic edge cases
- page table maintenance

## 13. Memory and Device Modeling for Typical HVM Boards

### 13.1 CPU-Centric Board Design

The HVM CPU should not be coupled directly to device models.

Instead, design an SoC abstraction:

- CPU cluster
- shared interconnect
- interrupt controller
- timer block
- debug block
- MMIO region
- memory controller

### 13.2 Typical Laptop SoC

Recommended blocks:

- 4-core HVM cluster
- L2 cache abstraction
- LPDDR controller
- PCIe root complex
- NVMe
- USB controller
- display controller
- audio
- keyboard/touchpad input
- UART debug
- RTC and watchdog
- secure boot storage

### 13.3 Typical Mobile SoC

Recommended blocks:

- 2-core or 4-core HVM cluster
- LPDDR controller
- low-power interconnect
- display and touch
- audio
- storage
- sensor hub
- modem abstraction
- aggressive power controller
- secure boot and key storage

## 14. Debugging and Trace Support

### 14.1 Debug Features

The simulator should expose:

- breakpoints
- watchpoints
- single-step
- stop reason reporting
- register inspection
- memory inspection
- trap cause inspection

### 14.2 GDB Integration

If possible, map HVM architectural registers to GDB:

- general-purpose registers
- PC
- privilege state
- trap cause
- status registers

This allows standard debugger use while still supporting HVM-specific trace features.

### 14.3 Execution Trace

The simulator should support:

- instruction trace logging
- trap trace logging
- MMIO trace logging
- interrupt trace logging
- optional cycle counting

Trace must be deterministic enough for regression testing.

## 15. Testing Strategy

### 15.1 CPU Tests

Test:

- arithmetic
- branches
- calls and returns
- loads and stores
- stack frames
- trap handling
- CSR access
- atomic operations
- WFI behavior

### 15.2 Board Tests

Test:

- reset and boot
- firmware handoff
- interrupt routing
- timer interrupts
- UART console
- block storage access
- MMIO reads/writes
- debug breakpoints

### 15.3 JIT Compatibility Tests

Compare QEMU and JIT behavior for:

- instruction decode
- instruction size
- branch offsets
- trap causes
- stack behavior
- runtime hooks
- atomic and barrier semantics

The same HVM program should behave the same way under both systems.

## 16. Implementation Phases

### Phase 1: CPU and Console

- HVM CPU target
- ROM
- RAM
- UART
- timer
- basic interrupts
- simple boot path

### Phase 2: System Expansion

- MMU
- TLB maintenance
- debug/watchpoint support
- storage and network devices
- board description tables

### Phase 3: Laptop Board

- display pipeline
- input devices
- USB
- power management
- secure boot flow

### Phase 4: Mobile Board

- low-power tuning
- camera/sensor bus
- modem abstraction
- suspend/resume
- battery-aware behavior

## 17. Practical Recommendations

If you want the shortest path to a useful HVM QEMU simulator:

1. implement a minimal HVM CPU target
2. add a dev board with UART, timer, RAM, ROM, and interrupt controller
3. get boot firmware running
4. add device tree or equivalent board description
5. validate against the JIT
6. then expand to laptop/mobile SoCs

Do not start with a large consumer board. Start with a small dev machine and grow the platform in layers.

## 18. Compatibility Rule

The simulator must stay compatible with the hardware profile and the JIT profile at the same time.

That means:

- the simulator is allowed to emulate missing hardware
- the simulator is not allowed to redefine HVM semantics
- the simulator is not allowed to accept instructions that the ISA rejects
- the simulator must use the same instruction length, branch, trap, and ABI rules as the JIT and the hardware profile

If the simulator can run a program that hardware cannot or vice versa, the shared contract is broken and must be corrected.

## 19. Getting Started Guide

This section gives a practical path to get a minimal HVM/QEMU simulator running.

The goal of the first milestone is not full device completeness. The first milestone is:

- a buildable QEMU fork or out-of-tree target
- one HVM CPU definition
- one minimal HVM board
- ROM + RAM + UART + timer
- a boot stub that reaches a visible console message

### 19.1 Recommended Development Approach

Use an incremental bring-up strategy:

1. add the HVM CPU target
2. add a minimal machine type
3. boot from ROM into a serial console
4. add instruction decode and arithmetic
5. add branches and loads/stores
6. add traps and timer interrupts
7. add MMIO and device tree
8. compare the simulator against the JIT

Do not start with a complete laptop SoC.

### 19.2 Repository Setup

Recommended layout if you are keeping the QEMU integration inside this repository:

```text
third_party/qemu/
  target/hvm/
  hw/hvm/
  include/hw/hvm/
  docs/hvm/

docs/hvm/
  hvm-qemu-system-simulator.md
```

Recommended layout if you are maintaining a separate QEMU fork:

```text
qemu-hvm/
  target/hvm/
  hw/hvm/
  include/hw/hvm/
  tests/
  docs/
```

For the first pass, keep the HVM-specific code isolated under `target/hvm` and `hw/hvm`.

### 19.3 Required Toolchain

You typically need:

- a C compiler toolchain
- a C++ compiler toolchain if you add host-side utilities
- `git`
- `python3`
- `meson`
- `ninja`
- `pkg-config`
- `make`

Depending on the host and QEMU build configuration, you will also need development headers and libraries for:

- `glib`
- `pixman`
- `zlib`
- `libfdt`
- optional `capstone`
- optional `libslirp`

If you enable debugger or graphics support, you may also need:

- SDL or GTK development packages
- GDB support headers
- optional SPICE support

### 19.4 Development Environment

Use a normal local development environment with:

- a recent Linux or macOS host
- enough disk space for a QEMU build tree
- a clean out-of-tree build directory
- the ability to run QEMU system emulation locally

Recommended local setup:

- separate source and build directories
- one debug build
- one optimized build
- one test run directory for firmware and disk images

Example structure:

```text
workspace/
  qemu/
  qemu-build-debug/
  qemu-build-release/
  hvm-images/
  hvm-firmware/
```

### 19.5 Minimal External Dependencies

For the first boot path, keep dependencies minimal:

- ROM image or firmware blob
- RAM image or bootloader image
- serial console output
- timer interrupt source
- simple device tree blob

Avoid depending on:

- full graphics stack
- complex storage stacks
- networking
- advanced ACPI generation

Those can be added later.

### 19.6 Initial Bootstrap Plan

The smallest useful boot path is:

1. QEMU resets into an HVM ROM vector
2. ROM initializes stack, privilege, and UART
3. ROM prints a boot banner
4. ROM copies or loads a payload into RAM
5. ROM branches to the payload
6. payload prints a second message
7. payload halts via `BREAK`, trap, or a machine stop request

This gives you a visible end-to-end signal before you add the rest of the SoC.

### 19.7 Minimal CPU State Skeleton

The HVM CPU state in QEMU should begin as a small struct.

```c
typedef struct HVMCPUState {
    uint64_t gpr[32];
    uint64_t pc;
    uint64_t next_pc;
    uint64_t status;
    uint64_t trap_vector;
    uint64_t trap_cause;
    uint64_t trap_epc;
    uint64_t int_enable;
    uint64_t int_pending;
    uint64_t timer_compare;
    uint64_t cycle_counter;
    uint32_t priv;
    uint32_t flags;
} HVMCPUState;
```

This is only a starting point. Expand it only when the ISA or machine model requires it.

### 19.8 Minimal Machine Initialization Skeleton

The board initialization should be explicit and boring.

```c
static void hvm_machine_init(MachineState *machine)
{
    HVMState *s = HVM_MACHINE(machine);

    memory_region_init_ram(&s->ram, NULL, "hvm.ram", machine->ram_size, &error_fatal);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->ram);

    hvm_cpu_create(s);
    hvm_timer_create(s);
    hvm_uart_create(s);
    hvm_irq_create(s);

    hvm_load_rom(s, machine->firmware);
    hvm_install_device_tree(s);
}
```

The first version should wire only the devices needed for console boot.

### 19.9 Minimal Boot ROM Skeleton

The ROM should do four things:

- set up the stack
- initialize UART
- print a banner
- jump to payload code

Pseudo-code:

```asm
reset:
    csrwr sp, rom_stack_top
    call uart_init
    call uart_puts, "HVM booting..."
    call load_payload
    jmp payload_entry
```

If you want to keep the ROM extremely small, it can simply jump into a fixed RAM address where a test payload is already loaded by QEMU.

### 19.10 Minimal UART Driver Skeleton

For first boot, a memory-mapped UART is enough.

```c
static inline void hvm_uart_putc(uint64_t base, char c)
{
    volatile uint32_t *tx = (volatile uint32_t *)(uintptr_t)(base + 0x00);
    *tx = (uint32_t)c;
}

static void hvm_uart_puts(uint64_t base, const char *s)
{
    while (*s) {
        hvm_uart_putc(base, *s++);
    }
}
```

This is enough to prove the board boots and the MMIO map works.

### 19.11 Minimal HVM Payload Skeleton

Once the ROM and UART work, test a tiny HVM payload that:

- writes a register
- performs a load or store
- calls a helper
- triggers a trap or breakpoint

Example shape:

```text
    MOVI r1, 42
    ADDI r2, r1, 1
    ST.D [sp+0], r2
    SYSCALL 0
    BREAK
```

The exact syntax depends on the assembler, but the test should exercise:

- register writes
- memory access
- trap handling
- console output

### 19.12 Build and Run Loop

The development loop should look like this:

1. build the QEMU target
2. boot the ROM image
3. observe UART output
4. fix decode or device wiring issues
5. run the same payload under the JIT
6. compare register state and trap results

Keep logs for:

- instruction trace
- MMIO trace
- interrupt trace
- trap trace

### 19.13 First Validation Checklist

The first milestone is complete when all of these are true:

- the HVM CPU target compiles
- the machine boots to ROM
- the ROM prints a message over UART
- the timer can generate an interrupt
- `BREAK` or trap entry is visible in the debugger
- the same minimal program behaves the same under the JIT and QEMU

### 19.14 When to Add More Complexity

Add more features only after the previous layer is stable:

- add MMU only after the console boot path is stable
- add storage only after UART and timer are stable
- add display only after the board can boot unattended
- add laptop/mobile peripherals only after the dev board is reliable

This keeps the simulator maintainable and makes JIT parity easier to preserve.

## 20. First QEMU Files To Create

This section is a practical source-tree checklist for the first HVM QEMU bring-up.

The goal is to create a small, coherent set of files that compile together and make the machine bootable.

### 20.1 CPU Target Files

Create these first:

- `target/hvm/cpu.h`
- `target/hvm/cpu.c`
- `target/hvm/translate.c`
- `target/hvm/op_helper.c`
- `target/hvm/helper.h`
- `target/hvm/disas.c`
- `target/hvm/gdbstub.c`

What each file should contain:

- `cpu.h`: HVMCPU state, feature flags, register layout, and declarations
- `cpu.c`: reset, realize, interrupt entry, and CPU lifecycle code
- `translate.c`: instruction decode and TCG lowering
- `op_helper.c`: helpers for traps, MMU faults, atomics, and special instructions
- `helper.h`: helper prototypes used by TCG-generated code
- `disas.c`: instruction disassembly for logging and debugging
- `gdbstub.c`: register mapping for debugger attachment

### 20.2 Machine and SoC Files

Create these next:

- `hw/hvm/hvm-machine.c`
- `hw/hvm/hvm-soc.c`
- `hw/hvm/hvm-irq.c`
- `hw/hvm/hvm-timer.c`
- `hw/hvm/hvm-debug.c`
- `hw/hvm/hvm-csr.c`
- `hw/hvm/hvm-bootrom.c`

What each file should contain:

- `hvm-machine.c`: machine type registration and board wiring
- `hvm-soc.c`: SoC container, CPU cluster hookup, and MMIO layout
- `hvm-irq.c`: interrupt controller model and routing helpers
- `hvm-timer.c`: timer and compare register logic
- `hvm-debug.c`: breakpoint, watchpoint, and single-step plumbing
- `hvm-csr.c`: machine-visible control register model
- `hvm-bootrom.c`: ROM image loading and reset-vector setup

### 20.3 Public Headers

Create these headers early so the source files stay consistent:

- `include/hw/hvm/hvm-machine.h`
- `include/hw/hvm/hvm-soc.h`
- `include/hw/hvm/hvm-irq.h`
- `include/hw/hvm/hvm-timer.h`
- `include/hw/hvm/hvm-csr.h`
- `include/hw/hvm/hvm-debug.h`

These should define:

- machine and SoC structs
- device state structs
- constants for MMIO offsets and interrupt lines
- reset and realize entrypoints

### 20.4 Build System Files

Update the build system with:

- `target/hvm/meson.build`
- `hw/hvm/meson.build`
- `include/hw/hvm/meson.build` if needed

Also update the global QEMU target and machine registration lists.

### 20.5 Minimal First Commit Order

The first commit set should be small and ordered:

1. add the HVM CPU type
2. add the machine type stub
3. add ROM and RAM wiring
4. add UART output
5. add timer interrupts
6. add instruction decode for a tiny subset
7. add trap and breakpoint handling
8. add disassembler and debugger register mapping

This order keeps the tree compilable at each stage.

### 20.6 Minimal Instruction Subset To Implement First

The first executable subset should include:

- `NOP`
- `ADD`
- `SUB`
- `MOV`
- `ADDI`
- `LD.D`
- `ST.D`
- `BEQ`
- `BNE`
- `JMP`
- `JAL`
- `JALR`
- `RET`
- `BREAK`
- `SYSCALL`

Optional for the same first pass:

- `LDA`
- `PUSH`
- `POP`
- `ENTER`
- `LEAVE`
- `ADJSP`

Do not start by implementing the entire ISA. Reach boot first.

### 20.7 Suggested First Machine Description

The first machine should be very small:

- 1 HVM core
- 64 MB or 128 MB RAM
- ROM at reset vector
- UART at a fixed MMIO address
- timer block
- interrupt controller
- optional block device later

This is enough for firmware bring-up and JIT parity testing.

### 20.8 First Files Checklist

If you want a literal checklist, start with these files:

- `target/hvm/cpu.c`
- `target/hvm/cpu.h`
- `target/hvm/translate.c`
- `target/hvm/op_helper.c`
- `hw/hvm/hvm-machine.c`
- `hw/hvm/hvm-soc.c`
- `hw/hvm/hvm-irq.c`
- `hw/hvm/hvm-timer.c`
- `hw/hvm/hvm-bootrom.c`
- `hw/hvm/hvm-debug.c`
- `include/hw/hvm/hvm-machine.h`
- `include/hw/hvm/hvm-soc.h`

That set is enough to get to a bootable skeleton.

### 20.9 What Not To Create First

Do not start with:

- GPU support
- full storage stack
- networking
- audio
- camera
- modem integration
- ACPI generation
- complex power management

Those belong after the CPU, boot flow, and console path are stable.

## 21. Implementation Plan and Exit Criteria

This section turns the simulator work into a sequence of checkpoints.

### Milestone 1: Bootable CPU Skeleton

Goal:

- the HVM CPU target compiles
- the machine type exists
- ROM and RAM are wired
- UART output works
- a tiny payload can run

Exit criteria:

- QEMU starts with the HVM machine selected
- the reset vector executes
- the UART prints a banner
- the CPU register file is visible in the debugger
- `BREAK` or a trap reaches the host cleanly

### Milestone 2: Instruction Coverage for Core ISA

Goal:

- implement the minimal executable instruction subset
- verify branches, calls, loads, stores, and return behavior
- verify base32 and escape32 decoding

Exit criteria:

- small HVM test programs run correctly
- `CALL`/`RET` and `JALR` work
- invalid instruction encodings trap correctly
- the simulator and JIT agree on instruction length and branch targets

### Milestone 3: Traps, Timer, and Interrupts

Goal:

- model trap entry and return
- model timer interrupts
- expose basic debug stops and watchpoints

Exit criteria:

- timer interrupts fire and are acknowledged
- breakpoints stop execution predictably
- syscall and trap causes are visible
- privilege transitions are preserved across trap entry and return

### Milestone 4: MMU and Maintenance Instructions

Goal:

- add MMU/TLB support
- add CSR-style control registers
- add barrier and cache-maintenance instructions

Exit criteria:

- page translation works
- TLB invalidation works
- instruction-cache invalidation works after code changes
- the JIT and simulator still agree on traps and memory ordering

### Milestone 5: Board-Realistic Device Model

Goal:

- expand from dev board to laptop/mobile board abstractions
- add storage, input, display, and power-related models as needed

Exit criteria:

- firmware can enumerate the board
- the machine description is stable
- a realistic boot flow exists for laptop and mobile variants
- device behavior is deterministic enough for regression tests

### Milestone 6: Parity Validation Against JIT

Goal:

- compare simulator and JIT behavior under identical inputs
- ensure the same Hoo program behaves the same way

Exit criteria:

- instruction decode matches
- register results match
- memory results match
- trap and interrupt results match
- runtime hook behavior matches

### Final Definition of Done

The QEMU-based HVM simulator is considered ready when:

- it boots the minimal dev board successfully
- it runs the core HVM ISA correctly
- it models traps, interrupts, and timer events
- it supports the shared hardware/JIT extension set
- it can execute the same test corpus as the HVM JIT with matching results
- it can be extended toward laptop and mobile boards without changing ISA semantics

## 22. Roadmap Table

| Phase | Scope | Main Deliverable | Exit Criteria |
| --- | --- | --- | --- |
| 1 | CPU skeleton + console | Minimal HVM CPU and dev board | Boots ROM, prints UART banner, trap/debug works |
| 2 | Core ISA coverage | Arithmetic, control flow, loads/stores | Core test programs run and match JIT |
| 3 | Traps and interrupts | Timer, syscall, breakpoint, watchpoints | Interrupts and trap return are stable |
| 4 | MMU and maintenance | CSR, TLB, cache maintenance | Translation and invalidation work correctly |
| 5 | Laptop/mobile board modeling | Storage, input, display, power | Firmware enumerates realistic board variants |
| 6 | Parity validation | QEMU vs JIT comparison | Same Hoo programs behave identically |

## 23. Task Checklist

### 23.1 First Sprint

- create `target/hvm/cpu.h`
- create `target/hvm/cpu.c`
- create `target/hvm/translate.c`
- create `hw/hvm/hvm-machine.c`
- create `hw/hvm/hvm-soc.c`
- create `hw/hvm/hvm-irq.c`
- create `hw/hvm/hvm-timer.c`
- create `hw/hvm/hvm-bootrom.c`
- create `hw/hvm/hvm-debug.c`
- wire RAM, ROM, UART, timer, and interrupt controller
- boot a ROM that prints a banner

### 23.2 Second Sprint

- implement decode for base32 and escape32 instructions
- implement `ADD`, `SUB`, `MOV`, `ADDI`
- implement `LD.D`, `ST.D`
- implement `BEQ`, `BNE`, `JMP`, `JAL`, `JALR`, `RET`
- implement `BREAK` and `SYSCALL`
- verify branch and link semantics against the JIT

### 23.3 Third Sprint

- add trap entry and return
- add timer interrupts
- add `CSRRD` and `CSRWR`
- add `FENCE`, `ICACHE.IALL`, and `TLBI`
- add `WFI`
- add debugger stop reasons and single-step

### 23.4 Fourth Sprint

- add atomics: `AMOSWAP`, `AMOADD`, `AMOXOR`, `AMOAND`, `AMOOR`, `CAS`
- add MMU/TLB behavior
- add device tree or equivalent board description
- add storage and optional network device models

### 23.5 Validation Checklist

- QEMU boots the dev board
- UART output is visible
- timer interrupts fire
- trap causes are reported
- invalid instructions trap
- misaligned accesses behave as documented
- the JIT and QEMU agree on instruction length and branch targets
- runtime hooks behave identically in both environments
