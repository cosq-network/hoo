# ISSUE-011: Missing Runtime Bounds Checking for Array Access

## 1. Overview
The `arr[index]` subscript operator compiles to a direct address calculation and load with no bounds check against the array's stored length. Out-of-bounds access causes silent memory corruption instead of a trapped error.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1602-1628
- **Issue**: The array indexing lowering reads the element pointer at `base + 8 + index * 8` and loads from it, without first comparing `index` against the length stored at `base + 0`.

```cpp
// Current lowering (no bounds check):
// r1 = arrayObj
// r2 = index
// r3 = *(r1 + 0)  ← length, read but NOT compared
// r1 = r1 + 8 + r2 * 8  ← address of element
// r1 = *(r1)  ← load element (no bounds guard)
```

## 3. Impact
- Array out-of-bounds access silently reads/writes adjacent memory.
- Memory corruption, data leaks, and security vulnerabilities.
- No alignment with the ARC memory model — overwriting beyond array bounds corrupts adjacent object headers.

## 4. Suggested Fix
Insert a comparison and conditional branch after computing the element address:

```cpp
emit(Opcode::LD, OperandsR{lengthReg, arrayReg, 0, 0}); // load length
emit(Opcode::CMP, OperandsR{tmpReg, indexReg, lengthReg, 1}); // BLT: tmp = (index < length)
emit(Opcode::BEQ, OperandsB{tmpReg, 0, trapLabel}); // if not <, trap
// ... load element ...
```

The trap label should invoke `_F_hoo_exception_throw_v_p` with a bounds-check error object, or set a trap flag in the HVM state.

## 5. Status
- **Date**: 2026-06-08 (opened), 2026-06-10 (fixed)
- **Status**: **FIXED**
- **Priority**: **HIGH**
- **Update 2026-06-10**: Array subscript (`arr[idx]`) now emits bounds check (CMP idx < len, BEQ to trap) before element access. Offset formula corrected from `base + 8 + idx*8` to `base + 32 + idx*8` (ARRAY_HEADER_WORDS=4). For-in loop offset fix applied identically. OOB trap creates runtime exception via `jit_hoo_exception_runtime` and throws via `hoo_throw`.
- **Update 2026-06-17**: Native bounds checking has been implemented in the `tensor` runtime (`src/runtime/lib/hoo_tensor.cpp`). All tensor indexing operations (`get_numeric`, `hoo_tensor_set_value`) validate indices against the tensor's rank and dimensions, returning `nullptr` or zero for out-of-bounds access. The compiler's tensor subscript lowering leverages these runtime safety checks.
