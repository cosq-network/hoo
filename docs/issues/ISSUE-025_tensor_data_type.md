# ISSUE-025: Implementation Plan for `tensor` Data Type

## 1. Overview
This document outlines the detailed implementation plan to introduce a native `tensor` data type to the Hoo language. The `tensor` type will represent 1D, 2D, or 3D fixed-size arrays optimized for AI/ML matrix algebra. It will natively support numerical primitives: `bit`, `int8`, `int64`, `f8` (8-bit floating point), and `f64`.

The implementation requires full-stack integration: from parser grammar and AST nodes, down through the code generator, to the LLVM-based JIT and the C++ runtime library.

## 2. Low-Precision Data Types Specification

### 2.1 `f8` (8-bit Floating Point)
To support modern AI/ML quantization strategies (e.g., OCP FP8), Hoo will implement two variants of the `f8` type:
- **`f8e4m3`**: 1 sign bit, 4 exponent bits, 3 mantissa bits. Optimized for higher precision during inference.
- **`f8e5m2`**: 1 sign bit, 5 exponent bits, 2 mantissa bits. Optimized for wider dynamic range during training (similar to half-precision but smaller).

**Implementation Details**:
- **Storage**: Occupies 1 byte in memory.
- **Arithmetic**: In the HVM core, `f8` values are promoted to `f64` for individual register-based arithmetic. Tensors will use specialized SIMD/Hardware intrinsics for batch `f8` operations.
- **JIT**: Maps to LLVM's `f8e5m2` or `f8e4m3fn` types where available (e.g., for Hopper/H100 architectures).

### 2.2 `bit` (1-bit Numerical Type)
The `bit` type represents a single binary digit (0 or 1). Unlike `bool`, which represents logical truth, `bit` is a numerical primitive intended for hardware-level bitmasking, logical gates, and Binary Neural Networks (BNNs).

**Implementation Details**:
- **Logic**: `0` or `1`. Arithmetic operations (add, mul) follow modular arithmetic or logical gate equivalents.
- **Storage in Scalars**: Occupies 1 byte in a standard register for simplicity during local execution.
- **Storage in Tensors**: **Packed Storage**. A `tensor<bit>` will pack 8 bits into a single byte, providing an 8x reduction in memory footprint compared to `int8` tensors.
- **Conversions**: Explicitly castable to `int64` (0 or 1) and `bool` (0 -> false, 1 -> true).

## 3. Technical Requirements

### 3.1 Grammar Updates (`src/parsing/Hooc.g4`)
- **Type Declaration**: Add `tensor`, `f8`, and `bit` as recognized types.
- **Dimensionality Syntax**: Square bracket notation for shape: `tensor<type>[d1, d2, ...]`.
- **Literal Syntax**: 
  - `0b` or `1b` for `bit` literals.
  - Floating point literals with an `f8` suffix (e.g., `0.5f8`).
  - Tensor literals using nested arrays with a `t` suffix (e.g., `[[1, 0], [1, 1]]t`).
- **Matrix Operators**: Support for `*` (matrix multiply) and `.*` (element-wise multiply).

### 3.2 Detailed Syntax Flavors

#### 3.2.1 Multidimensional Declarations
Tensors support fixed-size shapes of 1, 2, or 3 dimensions:
```hoo
var v: tensor<f64>[10];          // 1D Vector (rank 1)
var m: tensor<f8>[3, 3];         // 2D Matrix (rank 2)
var t: tensor<int8>[4, 4, 4];    // 3D Tensor (rank 3)
```

#### 3.2.2 Literal Notation
Literals use nested square brackets followed by the `t` (tensor) marker. Types are inferred from elements:
```hoo
var v = [1.0, 2.0, 3.0]t;                    // tensor<f64>[3]
var m = [[1, 0], [0, 1]]t;                   // tensor<int64>[2, 2]
var b = [[0b, 1b], [1b, 1b]]t;               // tensor<bit>[2, 2]
var f = [[0.5f8, -1.2f8], [0.0f8, 1.0f8]]t;  // tensor<f8>[2, 2]
```

#### 3.2.3 Algebraic and Matrix Operations
Operators are overloaded for tensor operands. Broadly categorized into **Element-wise** and **Matrix-level**:

| Operator | Name | Semantics |
| :--- | :--- | :--- |
| `a + b` | Addition | Element-wise `a[i] + b[i]` |
| `a - b` | Subtraction | Element-wise `a[i] - b[i]` |
| `a .* b` | E-wise Multiply | Element-wise `a[i] * b[i]` |
| `a ./ b` | E-wise Divide | Element-wise `a[i] / b[i]` |
| `a * b` | Matrix Multiply | Standard dot product / GEMM |
| `a^T` | Transpose | Swap dimensions (e.g., `[i, j]` -> `[j, i]`) |

#### 3.2.4 Comparison Operations (Element-wise)
Comparison operators between two tensors return a `tensor<bit>` of the same shape:
```hoo
var a = [10, 20]t;
var b = [15, 15]t;
var res = a < b; // Result: [1b, 0b]t (tensor<bit>[2])
```
- Supported: `==`, `!=`, `<`, `>`, `<=`, `>=`

#### 3.2.5 Boolean Logic (Element-wise)
Logic operators on `tensor<bit>` allow for fast bitwise masking and gate simulations:
```hoo
var mask1 = [1b, 0b, 1b]t;
var mask2 = [0b, 1b, 1b]t;
var combined = mask1 && mask2; // Result: [0b, 0b, 1b]t
var inverted = !mask1;         // Result: [0b, 1b, 0b]t
```
- Supported: `&&` (AND), `||` (OR), `!` (NOT)

### 3.3 AST Updates (`src/ast/`)
- **Type Nodes**: Update `PrimitiveTypeKind` to include `F8` and `BIT`. Create a `TensorType` node storing element type and shape.
- **Expression Nodes**: Add `BitLiteral` and `TensorLiteral`.
- **AST Builder**: Update `SimpleASTBuilder.cpp` to handle new literals and type declarations.

### 3.4 Code Generation (`src/codegen/HVMCodeGenerator.cpp`)
- **Type IDs**: Assign unique IDs: `bit` (8), `f8` (9), `tensor` (104).
- **Allocation**: Lower `tensor` declarations to `_F_hoo_tensor_alloc`. For `tensor<bit>`, calculate packed buffer size (`(dims + 7) / 8`).
- **Operator Lowering**: 
  - Overload `+`, `-`, `*` for tensors.
  - Implement **Type Promotion Rules**: Tensors of different precision (e.g., `tensor<f8> * tensor<f64>`) should promote to the higher precision or follow explicit quantization rules.
- **Indexing**: 
  - Standard types: `base + header + (i * stride_i + j * stride_j) * size`.
  - `bit` types: `base + header + (offset / 8)`, followed by bit-masking `(1 << (offset % 8))`.

### 3.5 JIT Integration (`src/hvm/HVMJIT.cpp`)
- **Wrappers**: Register JIT function wrappers for tensor math.
- **Low-Precision Backend**: 
  - Use LLVM `half` or `float` as intermediates for `f8` if native `f8` is missing.
  - Use bitwise IR operations for `tensor<bit>` acceleration.

### 3.6 Runtime Library (`src/runtime/lib/`)
- **`HooTensor`**: Extend struct to handle `bit` packing flags.
- **Math Routines**: 
  - `f8` GEMM (General Matrix Multiplication) using optimized kernels.
  - `bit` Popcount/XNOR-based convolution for BNN support.
  - Element-wise ReLU/Sigmoid optimized for `f8`.

## 4. Implementation Phases

### Phase 1: Foundation (f8, bit & Grammar)
1. Add `f8` and `bit` to the compiler stack (Grammar, AST, Codegen).
2. Update `Hooc.g4` for `tensor` syntax and new literals.
3. Regenerate ANTLR C++ parser.

### Phase 2: Runtime Tensor Storage
1. Implement `HooTensor` with bit-packing support for `tensor<bit>`.
2. Implement reference counting and bounds-checked multi-dimensional indexing.

### Phase 3: Matrix Algebra & Codegen
1. Implement `f8` and `bit` optimized math kernels in `hoort`.
2. Update `HVMCodeGenerator` to lower tensor expressions to these kernels.
3. Bind kernels in `HVMJIT.cpp`.

### Phase 4: AI/ML Specialized Operations
1. Implement high-level tensor operations: `reshape`, `transpose`, `softmax`.
2. Verification with 1D/2D/3D test cases.


## 5. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: **HIGH** (for AI/ML target workloads)