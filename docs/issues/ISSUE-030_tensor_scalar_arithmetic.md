# ISSUE-030: Tensor-Scalar Mixed Arithmetic Support

## 1. Overview
The implementation plan for the `tensor` data type (ISSUE-025) focuses on tensor-tensor operations (e.g., matrix multiplication, element-wise addition). However, many AI/ML algorithms require mixed-mode arithmetic between tensors and scalars (e.g., scaling a weight matrix by a learning rate, or adding a scalar bias).

## 2. Technical Analysis
Current `HVMCodeGenerator::visitBinaryExpression` handles tensors by checking if *both* operands are tensors (typeId 104). If one is a scalar and the other is a tensor, it currently falls back to standard `Opcode::ARITH`, which will attempt to perform math on the opaque tensor handle (a pointer) and the scalar value, leading to memory corruption or crashes.

## 3. Requirements
- **AST Support**: Ensure the type system allows `tensor * f64` or `f64 * tensor`.
- **Codegen Lowering**: Detect "Broadcasting" cases where one operand is a scalar. 
- **Runtime Intrinsics**: Implement specialized kernels in `hoort`:
    - `_F_hoo_Tensor_scale_p_p_d` (Tensor * Double)
    - `_F_hoo_Tensor_add_scalar_p_p_d` (Tensor + Double)
- **Vectorization**: The JIT should optimize these by loading the scalar into a SIMD register once and performing a vectorized operation against the tensor buffer.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PROPOSED**
- **Priority**: Medium (Essential for training loops/optimizers)
- **Audit 2026-06-21**: Tensor binary expressions still route to tensor-vector/tensor-binary helper paths when either operand is tensor; scalar broadcasting helpers are not implemented.
