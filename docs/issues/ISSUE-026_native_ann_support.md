# ISSUE-026: Implementation Plan for Native Artificial Neural Network (ANN) Support

## 1. Overview
This document defines the architecture for adding first-class Artificial Neural Network (ANN) support to the Hoo ecosystem. Following the "Language-First" philosophy, this implementation maximizes compiler-level transformations and minimizes C++ runtime dependencies. The goal is to allow developers to build, train, and deploy high-performance models using pure Hoo code, leveraging the LLVM JIT for hardware-level optimization.

## 2. Architectural Philosophy: Language-First AI
Most AI frameworks (PyTorch, TensorFlow) are C++ libraries with language wrappers. Hoo will instead treat neural networks as **standard Hoo code** that the compiler understands and optimizes.

| Feature | Implementation Strategy |
| :--- | :--- |
| **Matrix Math** | Lowered to nested HVM loops; vectorized by LLVM JIT. |
| **Autograd** | Source-to-Source Automatic Differentiation at compile-time. |
| **NN Layers** | Written in pure Hoo classes (e.g., `class Linear`). |
| **Optimization** | LLVM SLP Vectorizer and Loop-Unrolling. |
| **Low-Precision** | Native `bit` and `f8` support in the codegen pipeline. |

## 3. Technical Requirements

### 3.1 First-Class Automatic Differentiation (`grad`)
Hoo will introduce a `grad` keyword (or intrinsic) that performs **Reverse-Mode Automatic Differentiation** by transforming the AST.

- **Syntax**: `var d_loss = grad(my_model_forward);`
- **Compiler Action**: 
    1. The `HVMCodeGenerator` identifies the target function.
    2. It generates a new "adjoint" function that computes the partial derivatives of the output with respect to all input parameters.
    3. It applies the **Chain Rule** by traversing the forward-pass AST and synthesizing a backward-pass AST.
- **Benefit**: Zero runtime graph-building overhead. Training loops are just native bytecode.

### 3.2 Native Tensor Lowering (No Runtime Math)
To maintain "Hardware Purity," the compiler will not call a C++ `matmul` library.

- **Loop Synthesis**: The expression `C = A * B` (where A, B are tensors) is lowered into a triple-nested HVM loop of scalar multiplications and additions.
- **LLVM Vectorization**: The HVM JIT uses LLVM's `LoopVectorize` and `SLPVectorizer` passes. Because the loops are in native IR, LLVM can automatically target the host's SIMD (SSE/AVX/NEON) or even GPU instructions without needing custom kernels.
- **Memory**: Tensors are simple managed buffers with shape metadata. The runtime only handles `alloc` and `ARC`.

### 3.3 The `hoo.nn` Standard Library
A core library written entirely in `.hoo` to provide high-level abstractions.

```hoo
import hoo.math;

class Linear {
    public var weight: tensor<f8>[input_dim, output_dim];
    public var bias: tensor<f8>[output_dim];

    func :tensor<f8>[output_dim] forward(x: tensor<f8>[input_dim]) {
        return (x * this.weight) + this.bias;
    }
}
```

### 3.4 Hardware-Accelerated JIT
The JIT translator (`HVMJIT`) will be updated to:
- Detect specific HVM loop patterns corresponding to matrix math.
- Annotate LLVM IR with `!llvm.loop.vectorize.enable` and `!tbaa` (Type-Based Alias Analysis) metadata to ensure maximum optimization.
- Intercept large tensor operations for dispatch to **LLVM AMX/Hopper** intrinsics if available.

## 4. Implementation Phases

### Phase 1: Autograd Prototype (Scalars)
1. Implement scalar-level `grad()` in the compiler.
2. Verify by calculating derivatives of simple mathematical functions.
3. Implement `optimizer.step()` as a Hoo function.

### Phase 2: Tensor Algebra Lowering
1. Update `HVMCodeGenerator` to lower tensor operations (`+`, `-`, `*`) into HVM loop primitives.
2. Implement shape inference in the AST to ensure tensor compatibility at compile-time.
3. Validate that LLVM JIT vectorizes these loops effectively.

### Phase 3: The `hoo.nn` Bootstrap
1. Write `hoo.nn.Module`, `hoo.nn.Linear`, and `hoo.nn.Conv2D` in pure Hoo.
2. Implement activation functions (`relu`, `sigmoid`) as Hoo functions.
3. Demonstrate a "Hello World" MNIST-style network training entirely in Hoo.

### Phase 4: Binary Neural Networks (BNNs)
1. Implement specialized lowering for `tensor<bit>`.
2. Map `tensor<bit> * tensor<bit>` to XNOR + Popcount HVM instructions.
3. Optimize the JIT to emit native `vpopcnt` (AVX-512) or similar hardware instructions.

## 5. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: **CRITICAL** (Core differentiator for the Hoo ecosystem)
- **Audit 2026-06-21**: No `grad`, `hoo.nn`, model/layer runtime, or autograd implementation was found. This remains a proposal layered on top of the tensor core.
