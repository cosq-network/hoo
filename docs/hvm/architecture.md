# Hooc Virtual Machine (HVM) Architecture

## 1. Introduction

The Hooc Virtual Machine (HVM) is a lightweight, register-based virtual machine designed for the Hooc programming language. It supports static compilation, dynamic linking, JIT compilation (LLVM IR friendly), and C/C++ calling conventions. HVM is optimized for performance, portability, and ease of integration with existing toolchains.

---

## 2. Execution Model

### 2.1 Registers

- **32 general-purpose registers** (`r0` to `r31`), each 64 bits wide.
  - `r0`: Hardwired to zero.
  - `r31`: Stack pointer (SP).
  - `r30`: Frame pointer (FP), optional for debugging.
- Registers can hold integers (64-bit), floating-point numbers (double precision), object references, or SIMD vectors.

### 2.2 Instruction Encoding

HVM uses **32-bit fixed-width instructions** with an **escape prefix** for extended operations:

| Format | Width | Description |
|--------|-------|-------------|
| Standard | 32 bits | Most instructions (opcodes 0x00-0xFF) |
| Extended | 64 bits | Vector/SIMD, Exceptions, Interrupts, FFI, Debug (opcodes 0x100+) |

**Extended Opcode Prefix:** `0x10`
- Prepend `0x10` to create 64-bit extended instruction
- Extended opcode range: `0x100-0x1FF`

**Opcode Map:**
| Range | Category | Width |
|-------|----------|-------|
| 0x00-0x0F | Control/Data movement | 32-bit |
| 0x10-0xF7 | ALU, Memory, etc. | 32-bit |
| 0x100-0x10A | Vector/SIMD | 64-bit |
| 0x110-0x117 | Exception handling | 64-bit |
| 0x118-0x11F | Interrupt handling | 64-bit |
| 0x120-0x12D | FFI instructions | 64-bit |
| 0x130-0x139 | System/Debug | 64-bit |

### 2.3 Memory Model

- **Byte-addressable, little-endian** memory.
- **Stack**: Grows downward, holds activation records.
- **Heap**: Managed by a garbage collector or reference counting.
- **Static data**: For constants, vtables, and module metadata.

### 2.4 Calling Conventions

- **Integer/pointer arguments**: First 8 in `r1`–`r8`.
- **Floating-point arguments**: First 8 in `v0`–`v7` (or reuse `r1`–`r8` for scalar floats).
- **Return value**: In `r1` (or `v0` for FP/vector).
- **Stack alignment**: 8-byte (16-byte recommended).

---

## 3. Object Model

- Every object has a header (size, vtable pointer, lock bits).
- Fields are laid out sequentially.
- Methods are invoked via vtables.
- Interfaces use secondary vtables or interface tables.
- Generics are monomorphized at compile time.

---

## 4. Dynamic Linking & Loading

- Modules are compiled to `.hobj` files containing code, data, and relocation info.
- `IMPORT`/`FROM` generate import stubs.
- The VM resolver binds symbols at load time.
- APIs: `hvm_load_module("path")`, `hvm_resolve("func")`.

---

## 4.1 String Operations

HVM provides native string manipulation instructions for efficient text processing (opcodes 0x84-0xA7).

### 4.1.1 String Creation & Access
- `STRNEW`/`STRNEWB`: Create strings from bytes or capacity
- `STRLEN`: Get string length
- `STRGET`/`STRSET`: Character access by index

### 4.1.2 String Comparison
- `STRCMP`/`STRCMPN`: Compare strings
- `STREQUAL`: Check equality
- `STRSTART`/`STREND`: Prefix/suffix checking

### 4.1.3 String Searching
- `STRCHR`/`STRRCHR`: Find characters
- `STRFIND`/`STRRFIND`: Find substrings
- `STRCONTAINS`: Check substring existence

### 4.1.4 String Manipulation
- `STRSUB`/`STRSLICE`: Extract portions
- `STRJOIN`/`STREPEAT`: Concatenation
- `STRREV`: Reverse string

### 4.1.5 Case & Whitespace
- `STRUPPER`/`STRLOWER`: Change case
- `STRTRIM`/`STRLTRIM`/`STRRTRIM`: Trim whitespace

### 4.1.6 Conversion
- `STRTOI`/`STRTOD`: Parse to numbers
- `ITOSTR`/`DTOSTR`: Convert to strings
- `STRENCODE`/`STRDECODE`: Encoding conversion

---

## 5. Foreign Function Interface (FFI)

HVM provides comprehensive FFI support for calling native code and integrating with external libraries.

### 5.1 Static Runtime Calls

- `CALLHOST` (0x120): Call pre-registered runtime functions (HoocJIT runtime)
- `CALLHOSTV` (0x121): Virtual method calls via host runtime

### 5.2 Native Function Calls

- `CALLNATIVE` (0x122): Call native functions with C ABI
- `PREPCALL` (0x123): Prepare stack frame for native calls
- `FINISHCA` (0x124): Complete native call and retrieve return value

### 5.3 Dynamic Library Loading

- `LOADLIB` (0x125): Load shared libraries (.dll, .so, .dylib)
- `FREELIB` (0x126): Unload dynamic libraries
- `GETSYM` (0x127): Get symbol address from loaded library
- `GETFUNC` (0x128): Resolve function pointer from library

### 5.4 Type Conversion

- `I2PTR` (0x129): Convert integer to pointer
- `PTR2I` (0x12A): Convert pointer to integer
- `REINTERP` (0x12B): Reinterpret pointer type
- `ADDR2FUNC` (0x12C): Convert address to function pointer
- `FUNC2ADDR` (0x12D): Extract address from function pointer

### 5.5 Debugging Support

- `BREAKPOINT` (0x135): Source-level breakpoint using debug info
- `SINGLESTEP` (0x136): Single-step execution
- `GETREGS` (0x137): Read all registers
- `SETREGS` (0x138): Write all registers
- `GETFPOFF` (0x139): Get current frame pointer offset

### 5.6 FFI Calling Convention

When calling native functions (C ABI):

| Category              | Register/Location        | Notes                                    |
|-----------------------|-------------------------|------------------------------------------|
| Integer arguments     | r1 - r8                | First 8 arguments                        |
| Pointer arguments     | r1 - r8                | Treated as integer registers               |
| Floating-point args   | v0 - v7                | First 8 float/double arguments           |
| Return (integer/ptr)  | r1                      | Default return register                   |
| Return (float/double) | v0                      | Vector register for FP return             |
| Stack                 | r31 (sp)               | 8-byte aligned, grows downward            |

---

## 6. JIT & LLVM IR Integration

- HVM bytecode maps well to LLVM IR (SSA form).
- JIT compiler translates HVM instructions to LLVM IR, then to native code.
- Instructions are RISC-like, enabling efficient JIT.

### 6.1 Debug Information

HVM supports DWARF-style debug information for GDB/LLDB compatibility:

- **Line numbers**: `.debug_line` section maps addresses to source lines
- **Variable locations**: `.debug_info` DIEs with location expressions
- **Call frames**: `.debug_frame` for stack unwinding
- **Type info**: `.debug_abbrev` + `.debug_str` for type descriptions

The debug info is preserved through JIT compilation with address remapping.

---

## 7. Exception Handling (0x110-0x117)

HVM provides structured exception handling with try-catch-finally semantics.

- `TRY`/`CATCH`/`FINALLY` (0x113-0x114): instructions for exception boundaries
- `THROW`/`THROWV` (0x111-0x112): throw exceptions
- Exception records contain type, message, and stack trace
- Hardware-assisted exception dispatch

---

## 8. Interrupt Handling (0x118-0x11F)

HVM supports hardware and software interrupts for responsive execution.

- `DI`/`EI` (0x118-0x119): enable/disable interrupts
- `INT` (0x11A): software interrupts
- `SETINT`/`IRET` (0x11C-0x11B): interrupt service routines
- `MASKINT`/`UNMASKINT` (0x11E-0x11F): interrupt masking

---

## 9. Threading

HVM provides native threading support with synchronization primitives.

### 9.1 Thread Management (0xC0-0xC5)
- `THCREATE`/`THJOIN`/`THEXIT` for thread lifecycle
- `THYIELD` for cooperative scheduling
- `THWAIT` for wait with timeout

### 9.2 Synchronization
- Mutexes (0xC6-0xC9): `MUTEXINI`/`MUTEXLCK`/`MUTEXULK`/`MUTEXDL`
- Condition variables (0xCA-0xCE): `CONDNWI`/`CONDSIG`/`CONDBRO`/`CONDWT`/`CONDDST`
- Spinlocks (0xCF-0xD1): `SPININIT`/`SPINLCK`/`SPINULK`
- Barriers (0xD2-0xD3): `BARRSET`/`BARRWT`

### 9.3 Atomic Operations & TLS (0xE0-0xE8)
- `ATOMADD`/`ATOMSUB` for atomic arithmetic
- `ATOMCAS` for compare-and-swap
- `ATOMLD`/`ATOMST` for atomic memory operations
- Thread-local storage: `TLSALLOC`/`TLSGET`/`TLSSET`/`TLSFREE`

---

## 10. Multi-Process

HVM supports multi-process execution with inter-process communication via FFI calls to OS APIs.

### 10.1 Process Management
Multi-process support is provided through native OS calls via `CALLNATIVE`/`SYSCALL`:
- `fork()`/`exec()` for process creation
- `waitpid()` for process lifecycle
- `kill()` for termination signals

### 10.2 Inter-Process Communication
IPC is provided through native OS APIs:
- Pipes: `pipe()` via native calls
- Message passing: OS message queue APIs
- Shared memory: `shmget()`/`shmat()` via native calls

---

## 11. Summary

HVM is designed for:

- High performance and portability.
- Static compilation and dynamic linking.
- Seamless integration with C/C++ and LLVM toolchains.
- Support for modern language features (classes, generics, interfaces).
- Comprehensive FFI support for native code integration.
- Exception handling with try-catch-finally semantics.
- Hardware and software interrupt support.
- Native threading with synchronization primitives.
- SIMD/vector operations for data-parallel workloads.