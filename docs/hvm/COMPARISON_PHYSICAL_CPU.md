# HVM ISA Comparison: Physical Architectures

This document compares the Hooc Virtual Machine (HVM) ISA (Version 1.4, Hardware Ready) with major physical CPU architectures, including Apple Silicon (Arm64), AMD64 (x86-64), and OpenRISC.

## 1. Executive Summary

The HVM is a modern **64-bit RISC architecture** that shares significant design DNA with Arm64 and OpenRISC, while contrasting sharply with the CISC legacy of AMD64. Its primary goal is to provide a "lean and clean" implementation that is equally efficient in physical silicon and software-based JIT environments.

| Feature | HVM (v1.4) | Arm64 / Apple Silicon | AMD64 (x86-64) | OpenRISC (OR1k) |
| :--- | :--- | :--- | :--- | :--- |
| **ISA Class** | RISC | RISC | CISC (Legacy) | RISC (Open) |
| **Registers** | 32 GPRs | 31 GPRs + Zero | 16 GPRs | 32 GPRs |
| **Word Size** | 64-bit | 64-bit | 64-bit | 32/64-bit |
| **Encoding** | Fixed 32-bit* | Fixed 32-bit | Variable (1-15B) | Fixed 32-bit |
| **Model** | Load/Store | Load/Store | Register-Memory | Load/Store |
| **Endianness** | Little | Little (Mostly) | Little | Bi-endian |
| **Target** | Soft-Core / JIT | High-Perf Mobile/Desktop | Server / Desktop | Embedded / Academic |

\* *HVM uses an 8-byte aligned extended format for opcodes >= 0x80.*

---

## 2. Register Models

### HVM vs. Arm64 (Apple Silicon)
HVM and Arm64 are very similar. Both use 32 registers. Arm64 reserves `x31` as a zero register/stack pointer depending on context, whereas HVM hardwires `r0` to zero (matching RISC-V and MIPS) and uses `r31` explicitly for the stack.
*   **HVM Benefit**: Hardwired zero (`r0`) simplifies many instructions (e.g., `NEG rd, rs` is just `SUB rd, r0, rs`).

### HVM vs. AMD64
AMD64 is register-starved by comparison, with only 16 general-purpose registers (RAX, RBX, etc.). This leads to frequent "spilling" to the stack, which HVM avoids by providing 32 registers. HVM's register-based calling convention (`r1-r8`) is similar to the System V ABI used by AMD64 but more consistent.

---

## 3. Instruction Encoding & Decoding

### HVM: Fixed-Width Simplicity
Like Apple Silicon and OpenRISC, HVM instructions are primarily 32-bit. This allows for extremely simple hardware fetch/decode units. A physical HVM CPU can determine the operation and its operands in a single cycle with minimal logic gates.

### AMD64: Variable-Length Complexity
AMD64 instructions can be anywhere from 1 to 15 bytes long. This requires a massive, power-hungry decoder that must "speculate" where instructions begin and end. Apple Silicon's move to Arm64 was largely driven by the power efficiency gains of switching from variable-length (CISC) to fixed-width (RISC) decoding.

---

## 4. Addressing Modes

### HVM: Minimalist (Base + Offset)
HVM (v1.4) follows the "True RISC" philosophy of using only the simplest addressing mode: `Register + Immediate15`. Complex offsets (like array indexing) are lowered by the compiler into explicit arithmetic.

### Arm64 & AMD64: Complex Addressing
*   **Arm64**: Supports "Pre-indexed" and "Post-indexed" addressing, which can update a register while loading memory.
*   **AMD64**: Supports "Scale-Index-Base" (SIB), allowing a single instruction to do `[Base + Index * Scale + Displacement]`.
*   **Trade-off**: HVM is easier to implement in hardware (fewer ALU paths), while Arm64/AMD64 can sometimes achieve higher code density for specific memory patterns.

---

## 5. The "Software-Lowered" Advantage

The most unique aspect of HVM v1.4 compared to these physical giants is its **Purity**.

*   **Standard Physical CPUs**: Often include "helper" instructions for strings, encryption (AES-NI), or complex exceptions (hardware breakpoints).
*   **HVM**: Purges all such complexity. By moving features like `NEW` (object allocation) and `THROW` (exception handling) to a software runtime library, the HVM "Hardware" remains incredibly small. 
*   **Result**: An HVM soft-core can fit into much smaller FPGAs than a full Arm64 or AMD64 implementation, while the Hooc compiler ensures the high-level language performance is not compromised.

---

## 6. Conclusion

HVM is technically a **"Refined RISC"**. It takes the best ideas from **Arm64** (Register count, 64-bit focus) and **OpenRISC** (Open-source simplicity, fixed-width encoding) and combines them with an aggressive **Software-Lowering** strategy. 

While AMD64 remains the king of legacy compatibility, HVM aligns with the modern industry trend—led by Apple Silicon—of using **clean, register-rich RISC architectures** to achieve maximum performance-per-watt and compiler transparency.
