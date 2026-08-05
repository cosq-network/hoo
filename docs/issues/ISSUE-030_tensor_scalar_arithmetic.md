# ISSUE-030: Tensor-Scalar Mixed Arithmetic Support

## 1. Overview
This issue completes mixed-mode arithmetic between tensors and scalars. Many
AI/ML algorithms require it for scaling a weight matrix by a learning rate or
adding a scalar bias.

## 2. Technical Analysis
The original implementation entered the tensor path when either operand had
type ID 104, then treated both values as tensor pointers. That unsafe path has
been replaced with explicit tensor-tensor versus tensor-scalar dispatch.

## 3. Requirements
- **AST/type support**: Existing expression and type inference preserve the
  tensor result for mixed arithmetic while codegen identifies the scalar side.
- **Codegen lowering**: `+`, `-`, `*`/`.*`, and `/`/`./` support both operand
  orders. Matrix multiplication remains tensor-tensor-only.
- **Runtime kernels**: Add/subtract/scale/divide broadcast over every tensor
  element, with scalar bits plus scalar type ID at the JIT ABI boundary.
- **Semantics**: f64 scalars promote integer/FP8 tensors to f64; f8 scalars
  preserve FP8 when possible; integer and bit scalars preserve native tensor
  storage semantics.
- **JIT**: Wide and low-precision operations use registered scalar-broadcast
  runtime contracts, avoiding unsafe raw pointer arithmetic on scalar operands.

## 4. Implemented ABI

The runtime exposes typed scalar-bit entry points in `hoo_tensor.h`:

- `hoo_tensor_add_scalar_bits`
- `hoo_tensor_sub_scalar_bits`
- `hoo_tensor_sub_scalar_left_bits`
- `hoo_tensor_scale_scalar_bits`
- `hoo_tensor_div_scalar_bits`
- `hoo_tensor_div_scalar_left_bits`

Each receives `(tensor, scalar_bits, scalar_type)`. Floating-point values are
passed using their f64 ABI bits; integer, byte, and bit values are decoded from
the scalar type ID. Double-specific convenience APIs are also available for
native callers, including `hoo_tensor_scale_scalar` and
`hoo_tensor_add_scalar`.

The JIT registers corresponding `_F_hoo_Tensor_*` contracts for both the typed
scalar-bit ABI and the requested double ABI, including:

```text
_F_hoo_Tensor_add_scalar_p_p_d
_F_hoo_Tensor_sub_scalar_p_p_d
_F_hoo_Tensor_scale_p_p_d
_F_hoo_Tensor_div_scalar_p_p_d
```

## 5. Semantics and Boundaries

- Results preserve the tensor shape and are newly allocated managed tensors.
- `tensor * scalar` and `scalar * tensor` are scaling operations; `.*` is an
  equivalent element-wise spelling.
- Division by zero produces zero, matching tensor-tensor element division.
- `f64` promotion produces an f64 tensor. An f8 scalar preserves f8 when the
  tensor is not already f64. Integer and bit scalars preserve the tensor's
  native storage type where valid.
- Matrix multiplication, tensor comparisons against scalars, and tensor logic
  against scalars remain outside this issue and are rejected by codegen rather
  than being treated as tensor pointers.

## 6. Verification

Coverage is provided by:

- `tests/runtime/HooTensorTest.cpp`: floating-point broadcasting, operand
  ordering, integer-width behavior, FP8 preservation, null handling, and shape
  preservation.
- `tests/jit/HooTensorJitTest.cpp`: integer, f64, low-precision, and both-order
  source-level expressions executed through the HVM JIT.

The full RelWithDebInfo suite passes with 2,059 tests and 2 disabled tests.

## 7. Status
- **Date**: 2026-08-05
- **Status**: **IMPLEMENTED**
- **Priority**: Medium (Essential for training loops/optimizers)
- **Implementation audit 2026-08-05**: Runtime kernels, JIT wrappers and
  symbols, codegen dispatch, both operand orders, native integer/FP8 behavior,
  and runtime/JIT regression tests are complete.
