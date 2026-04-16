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

## 5. JIT & LLVM IR Integration

- HVM bytecode maps well to LLVM IR (SSA form).
- JIT compiler translates HVM instructions to LLVM IR, then to native code.
- Instructions are RISC-like, enabling efficient JIT.

---

## 6. Summary

HVM is designed for:

- High performance and portability.
- Static compilation and dynamic linking.
- Seamless integration with C/C++ and LLVM toolchains.
- Support for modern language features (classes, generics, interfaces).