# ISSUE-027: HVM 1.5 Sub-Word Precision Extension Implementation

## 1. Overview
HVM 1.5 introduces native support for sub-word precision operations, specifically targeting 8-bit and 1-bit data types. While HVM remains a 64-bit register machine, these new instructions operate explicitly on the lower 8 bits of the existing `r0..r31` registers. This extension eliminates the software overhead of manual masking and sign extension, provides a direct path for AI-specific FP8 hardware acceleration, and improves JIT vectorization efficiency.

## 2. Architectural Specification

### 2.1 Register Semantics
- **Inputs**: `rs1` and `rs2` are read as standard 64-bit registers. The instruction only uses bits `[7:0]`.
- **Outputs**: `rd` is written with the 8-bit result in bits `[7:0]`.
- **Extension Rule**:
    - **Unsigned (`byte`, `bit`)**: Bits `[63:8]` are zero-filled.
    - **Signed (`int8`)**: Bits `[63:8]` are filled with the value of bit `7` (Sign Extension).
    - **Floating Point (`f8`)**: Follows the precision-specific extension rules (promotion to `f64` for scalar fallback).

### 2.2 New Instruction Families

#### Family 1: Integer Sub-word Arithmetic (`ARITH_B`, Opcode 0x11)
Standard 32-bit `R` format.
- `ADD.B` (func 0): 8-bit addition.
- `SUB.B` (func 1): 8-bit subtraction.
- `MUL.B` (func 2): 8-bit multiplication.
- `DIV.B` (func 5): 8-bit signed division.
- `DIVU.B` (func 6): 8-bit unsigned division.
- `REM.B` (func 7): 8-bit remainder.

#### Family 2: Low-Precision Floating Point (`FLOAT_ARITH_B`, Opcode 0x31)
Native 8-bit floating-point math (E4M3/E5M2).
- `FADD.B` (func 0)
- `FSUB.B` (func 1)
- `FMUL.B` (func 2)
- `FDIV.B` (func 3)

#### Family 3: Binary Bit Logic (`LOGIC_B`, Opcode 0x22)
Numerical logic on bit 0.
- `BADD` (func 0): Bitwise XOR (Boolean addition).
- `BMUL` (func 1): Bitwise AND (Boolean multiplication).
- `BNOT` (func 2): Flips bit 0.

## 3. Technical Requirements

### 3.1 HVM Core (`src/hvm/`)
- **`Opcode` Enum**: Add `ARITH_B (0x11)`, `LOGIC_B (0x22)`, and `FLOAT_ARITH_B (0x31)`.
- **Encoding/Decoding**: Update `HVMInstruction::decode32` and `encode32` to correctly pack/unpack these opcodes.
- **Validation**: Ensure `isValid()` checks the correct `func` range for the new families.

### 3.2 HVM JIT Translator (`src/hvm/HVMJIT.cpp`)
- **Integer Implementation**: Use LLVM's `Trunc` to `i8`, perform arithmetic, and `SExt`/`ZExt` back to `i64`.
- **FP8 Implementation**: 
    - **Targeted**: If host supports native FP8 (LLVM 16+), map directly to `f8e5m2` or `f8e4m3fn`.
    - **Fallback**: Implement a "Fast Promotion" routine that expands `f8` bits to `f64` bits via bit-shifting before computation.
- **Bitwise Logic**: Implement using specialized LLVM IR for XNOR/Popcount patterns.

### 3.3 Compiler Codegen (`src/codegen/HVMCodeGenerator.cpp`)
- **Type-Aware Dispatch**: Update `visitBinaryExpression` to check the `inferExpressionTypeId()` result.
- **Opcode Selection**:
    - If operands are `int8`/`byte`, select `Opcode::ARITH_B`.
    - If operands are `f8`, select `Opcode::FLOAT_ARITH_B`.
    - If operands are `bit`, select `Opcode::LOGIC_B`.
- **Implicit Semantics**: Rely on HVM 1.5's auto-extension rules to avoid emitting manual masking instructions.

## 4. Efficiency Analysis

### 4.1 Instruction Count
| Operation | HVM 1.4 (Simulated) | HVM 1.5 (Native) | Reduction |
| :--- | :--- | :--- | :--- |
| `int8` Add | `ADD` + `AND 0xFF` + `SEXT` | `ADD.B` | 66% |
| `f8` Mul | `CALL promote` + `FMUL` + `CALL quantize` | `FMUL.B` | ~90% (cycle count) |

### 4.2 JIT Optimization (Vectorization)
By using 8-bit opcodes, the JIT can inform LLVM's **SLP Vectorizer** that it is safe to pack 16 operations into a single 128-bit XMM/NEON register. In HVM 1.4, LLVM would often assume 64-bit alignment and only pack 2 operations.

## 5. Implementation Phases

### Phase 1: ISA Foundation
- Update `HVMInstruction.h` and `HVMInstruction.cpp` with new opcodes.
- Add unit tests for 8-bit instruction encoding/decoding.

### Phase 2: JIT Sub-word Core
- Implement `ARITH_B` and `LOGIC_B` in `HVMJIT.cpp` using LLVM `i8` types.
- Verify correctness of 8-bit signed overflow and zero-extension.

### Phase 3: FP8 & AI Acceleration
- Implement `FLOAT_ARITH_B`.
- Add support for LLVM's native 8-bit float types.
- Implement software fallback for architectures lacking native FP8.

### Phase 4: Compiler Hardening
- Update `HVMCodeGenerator` to emit the new opcodes based on type inference.
- Remove redundant masking instructions from the generator.

## 6. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: **HIGH** (Prerequisite for high-performance AI/ML support)
