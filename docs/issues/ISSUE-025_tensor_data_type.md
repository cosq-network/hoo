# ISSUE-025: Implementation Plan for `tensor` Data Type

## 1. Overview
This document outlines the detailed implementation plan to introduce a native `tensor` data type to the Hoo language. The `tensor` type will represent 1D, 2D, or 3D fixed-size arrays optimized for AI/ML matrix algebra. It will natively support numerical primitives: `int8`, `int64`, `f8` (8-bit floating point, to be implemented), and `f64`.

The implementation requires full-stack integration: from parser grammar and AST nodes, down through the code generator, to the LLVM-based JIT and the C++ runtime library.

## 2. Technical Requirements

### 2.1 Grammar Updates (`src/parsing/Hooc.g4`)
- **Type Declaration**: Add `tensor` as a recognized primitive/complex type.
- **Dimensionality Syntax**: Define syntax for shape declarations (e.g., `tensor<f64>[10, 10]`).
- **Literal Syntax**: Add syntax for tensor literals (e.g., `[[1.0, 2.0], [3.0, 4.0]]t` or similar matrix notation).
- **Matrix Operators**: Ensure matrix multiplication (`*`), element-wise multiplication (`.*`), and transposition operators (`^T` or method calls) are supported by the grammar.

### 2.2 AST Updates (`src/ast/`)
- **Type Nodes**: Create a `TensorType` node extending `Type` that stores the underlying element type (`int8`, `int64`, `f8`, `f64`) and the dimensionality/shape.
- **Expression Nodes**: Create a `TensorLiteral` expression node to capture multi-dimensional literal definitions.
- **AST Builder**: Update `SimpleASTBuilder.cpp` to map the new grammar rules to the new AST nodes.

### 2.3 Code Generation (`src/codegen/HVMCodeGenerator.cpp`)
- **Type ID**: Assign a unique type ID for tensors (e.g., `104` to follow Arrays `102` and Maps `103`). Update `getTypeId()` logic.
- **Allocation**: Lower tensor declarations to a new runtime allocation intrinsic (`_F_hoo_tensor_alloc...`).
- **Operator Lowering**: 
  - Overload binary operators (`+`, `-`, `*`) in `visitBinaryExpression`.
  - When both operands are tensors, lower the operation to a JIT/Runtime intrinsic (e.g., `_F_hoo_tensor_matmul`) instead of standard `Opcode::ARITH`.
- **Indexing**: Lower multi-dimensional subscript access (`t[i, j]`) into flat memory offset calculations with bounds checking.

### 2.4 JIT Integration (`src/hvm/HVMJIT.cpp`)
- **Wrappers**: Register JIT function wrappers for all tensor runtime functions.
- **f8 Support**: The JIT must be updated to handle `f8` types via LLVM's `f8e5m2` or `f8e4m3fn` if hardware support is desired, or soft-float emulation if not.
- **Optimizations**: Potentially map matrix operations to accelerated BLAS/LAPACK or specialized hardware intrinsics during the LLVM IR translation phase.

### 2.5 Runtime Library (`src/runtime/lib/`)
- **Memory Model**: Create `hoo_tensor.h` and `hoo_tensor.cpp`. Define a `HooTensor` struct with an ARC-compliant 16-byte header, shape metadata (stride, dimensions), and a contiguous data buffer.
- **Element Types**: Implement strict checks restricting elements to `int8`, `int64`, `f8`, and `f64`.
- **Math Routines**: Implement high-performance C++ routines for:
  - Element-wise operations (add, sub, mul, div, ReLU, Sigmoid).
  - Matrix multiplication (Dot product, GEMM).
  - Transposition and reshaping.

## 3. Implementation Phases

### Phase 1: Foundation (f8 & Grammar)
1. Add the `f8` primitive type to the compiler stack (Grammar, AST, Codegen).
2. Update the Hoo grammar (`Hooc.g4`) to support the `tensor` keyword, shape declarations, and tensor literals.
3. Generate the ANTLR C++ parser files.

### Phase 2: Runtime Library (`hoort`)
1. Define the `HooTensor` ARC-managed struct.
2. Implement memory allocation, reference counting (`retain`/`release`), and bounds-checked read/write access.
3. Implement basic matrix algebra C functions (addition, element-wise multiplication, dot product).

### Phase 3: Codegen & JIT Binding
1. Update `HVMCodeGenerator` to construct `TensorType` and `TensorLiteral` AST nodes.
2. Implement lowering rules to translate tensor operations into runtime function calls.
3. Register the C functions in `HVMJIT.cpp` so they are resolvable by the bytecode.

### Phase 4: Verification & Optimization
1. Add extensive unit tests in `tests/runtime/` and `tests/jit/` for memory safety, bounds checking, and math accuracy.
2. Optimize the C++ runtime implementations (e.g., using SIMD intrinsics or linking a BLAS library).

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: **HIGH** (for AI/ML target workloads)