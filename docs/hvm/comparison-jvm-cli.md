# HVM ISA Comparison: Java Bytecode vs. .NET CLI

This document provides a technical comparison between the Hooc Virtual Machine (HVM) ISA (Version 1.4, Hardware Ready) and the two industry-standard virtual machines: the Java Virtual Machine (JVM) and the .NET Common Language Infrastructure (CLI).

## 1. Executive Summary

The HVM represents a fundamental shift in philosophy compared to the JVM and .NET CLI. While the latter are **high-level, stack-based virtual machines** designed primarily for emulation and JIT compilation, the HVM is a **low-level, register-based RISC architecture** designed for physical hardware implementation.

| Feature | HVM (v1.4) | Java Bytecode (JVM) | .NET CLI (CIL) |
| :--- | :--- | :--- | :--- |
| **Model** | Register-based (32 GPRs) | Stack-based | Stack-based |
| **Abstraction Level** | Physical RISC (Low) | High-Level Semantic | High-Level Semantic |
| **Instruction Format** | Fixed 32-bit (Bit-packed) | Variable-length Byte-stream | Variable-length Byte-stream |
| **Memory Access** | Explicit Load/Store | Managed References | Managed References / Typed Refs |
| **Object Creation** | Lowered (Software CALL) | Native Opcode (`new`) | Native Opcode (`newobj`) |
| **Exceptions** | Lowered (Shadow Stack) | Native Tables (`athrow`) | Native Blocks (`throw`) |
| **Hardware Ready** | Yes (Physical Silicon) | No (Requires VM/JIT) | No (Requires VM/JIT) |

---

## 2. Register vs. Stack Architecture

### HVM: Register-Based
HVM uses 32 general-purpose 64-bit registers. This mimics physical CPUs (like ARM64 or RISC-V).
*   **Pros**: Reduced instruction count for expressions, easier to map to physical hardware, explicit control over register allocation.
*   **Cons**: Requires a register allocator in the compiler.

### JVM / .NET: Stack-Based
Both JVM and .NET use an evaluation stack. Operands are pushed onto the stack, and instructions operate on the top values.
*   **Pros**: Highly compact bytecode, simpler compiler backend (no register allocation needed).
*   **Cons**: High "stack traffic" (frequent push/pop), complex to optimize for physical CPUs without an expensive JIT-to-register pass.

---

## 3. Instruction Semantics: RISC vs. CISC-VM

### HVM: Pure RISC
HVM (v1.4) has purged all "semantic" instructions. It does not "know" what an object is.
*   **Lowering**: The compiler calculates that `obj.field` is at `base_address + 16` and emits a standard `LD.D` (Load Double).
*   **Purity**: No opcodes for memory allocation, string manipulation, or exception unwinding exist in the ISA. These are all handled by software calls (`CALL`) to a runtime library.

### JVM / .NET: High-Level CISC
JVM and .NET instructions are highly semantic. 
*   **Opcodes**: Instructions like `invokevirtual` (JVM) or `ldfld` (.NET) perform complex logic internally: they look up class metadata, verify types, check for nulls, and resolve offsets at runtime.
*   **Verification**: The VM performs intensive bytecode verification to ensure type safety before execution.

---

## 4. Encoding and Density

### HVM: Bit-Packed Fixed-Width
HVM uses a fixed 32-bit instruction word (R, I, B, J formats).
*   **Decoding**: extremely fast and simple logic, suitable for a physical 5-stage pipeline.
*   **Alignment**: Instructions are always 4-byte aligned (8-byte for extended ops).

### JVM / .NET: Variable-Length Stream
Bytecodes are essentially a stream of 1-byte opcodes followed by variable-sized arguments.
*   **Density**: Extremely high code density (small binary size).
*   **Decoding**: Requires a sequential fetch-and-decode loop, making high-performance superscalar hardware execution much more complex.

---

## 5. Exception Handling

### HVM: Control-Flow Lowering
Exceptions in HVM are lowered to standard control flow. The compiler emits calls to `hoo_push_handler` and `hoo_pop_handler`. If a `SYSCALL` or standard `CALL` triggers an error, the runtime library manages the jump to the saved PC. This is identical to how C++ handles exceptions on physical hardware (e.g., using DWARF tables or shadow stacks).

### JVM / .NET: Managed Exception Blocks
The bytecode includes "exception tables" or "protected blocks" defined in the metadata. The VM itself monitors every instruction. When an error occurs, the VM stops execution, scans its internal tables, and automatically redirects the instruction pointer.

---

## 6. Target Deployment

### HVM: The "Universal Target"
Because HVM is hardware-ready, a single binary (`.ho`) can:
1.  Run on an **HVM physical processor** (FPGA or ASIC).
2.  Run on an **HVM high-performance interpreter**.
3.  Be **JIT-compiled** to x86/ARM by LLVM.

### JVM / .NET: The "Abstract Target"
JVM and .NET are designed to be platform-independent but **emulation-dependent**. They cannot run without a massive host environment (the JRE or .NET Runtime) providing metadata resolution, garbage collection, and stack management.

---

## 7. Conclusion

HVM is essentially **"Hooc-flavored RISC-V."** It provides the portability of a virtual machine but the performance profile and implementation simplicity of physical hardware. Unlike the JVM or .NET, which prioritize **managed safety and metadata**, the HVM prioritizes **execution predictability and hardware-level transparency**.
