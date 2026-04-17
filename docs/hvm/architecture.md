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

### 2.2 Memory Model

- **Byte-addressable, little-endian** memory.
- **Stack**: Grows downward, holds activation records.
- **Heap**: Managed by a garbage collector or reference counting.
- **Static data**: For constants, vtables, and module metadata.

### 2.3 Calling Conventions

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

HVM provides native string manipulation instructions for efficient text processing (opcodes 0x84-0xB0).

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

- `CALLHOST` (0xD0): Call pre-registered runtime functions (HoocJIT runtime)
- `CALLHOSTV` (0xD1): Virtual method calls via host runtime

### 5.2 Native Function Calls

- `CALLNATIVE` (0xD2): Call native functions with C ABI
- `PREPCALL` (0xD3): Prepare stack frame for native calls
- `FINISHCA` (0xD4): Complete native call and retrieve return value

### 5.3 Dynamic Library Loading

- `LOADLIB` (0xD5): Load shared libraries (.dll, .so, .dylib)
- `FREELIB` (0xD6): Unload dynamic libraries
- `GETSYM` (0xD7): Get symbol address from loaded library
- `GETFUNC` (0xD8): Resolve function pointer from library

### 5.4 Type Conversion

- `I2PTR` (0xD9): Convert integer to pointer
- `PTR2I` (0xDA): Convert pointer to integer
- `REINTERP` (0xDB): Reinterpret pointer type
- `ADDR2FUNC` (0xDC): Convert address to function pointer
- `FUNC2ADDR` (0xDD): Extract address from function pointer

### 5.5 FFI Calling Convention

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

---

## 7. Exception Handling

HVM provides structured exception handling with try-catch-finally semantics.

- `TRY`/`CATCH`/`FINALLY` instructions for exception boundaries
- Exception records contain type, message, and stack trace
- Hardware-assisted exception dispatch

---

## 8. Interrupt Handling

HVM supports hardware and software interrupts for responsive execution.

- `DI`/`EI` for enable/disable interrupts
- `INT` for software interrupts
- `SETINT`/`IRET` for interrupt service routines
- Interrupt masking support

---

## 9. Threading

HVM provides native threading support with synchronization primitives.

### 9.1 Thread Management
- `THCREATE`/`THJOIN`/`THEXIT` for thread lifecycle
- `THYIELD` for cooperative scheduling
- Thread-local storage (`TLSALLOC`/`TLSGET`/`TLSSET`)

### 9.2 Synchronization
- Mutexes: `MUTEXINI`/`MUTEXLCK`/`MUTEXULK`
- Condition variables: `CONDNWI`/`CONDSIG`/`CONDBRO`/`CONDWT`
- Spinlocks: `SPININIT`/`SPINLCK`/`SPINULK`
- Barriers: `BARRSET`/`BARRWT`

### 9.3 Atomic Operations
- `ATOMADD`/`ATOMSUB` for atomic arithmetic
- `ATOMCAS` for compare-and-swap
- `ATOMLD`/`ATOMST` for atomic memory operations

---

## 10. Multi-Process

HVM supports multi-process execution with inter-process communication.

### 10.1 Process Management
- `PROCFORK` for creating child processes
- `PROCEXEC` for replacing process image
- `PROCWAIT`/`PROCEXIT` for process lifecycle
- `PROCKILL` for termination signals

### 10.2 Inter-Process Communication
- Pipes: `PIPEOPEN`
- Message passing: `PROCSEND`/`PROCRECV`
- Shared memory: `PROCSHMGET`/`PROCSHMAT`/`PROCSHDT`

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
- Multi-process execution with IPC mechanisms.