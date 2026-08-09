# ISSUE-031: Advanced Return Type Inference for Method Chains

## 1. Overview
Hoo's type inference system now recursively preserves type metadata for
supported method chains (for example, `obj.getMap().length()` and
`makeArray()[0]`). This issue now covers advanced chains whose intermediate
types are dynamic, generic, overloaded with conversions, or supplied by
external signatures without metadata.

## 2. Technical Analysis
`HVMCodeGenerator.cpp` now recursively evaluates the metadata of `MemberAccess`, `FunctionCall`, `ArrayAccess`, `await`, and collection literals. This allows a known intermediate result to become the receiver type of the next operation, even when that result is not stored in a local variable. For example, `obj.getMap().length()` can use the return metadata for `getMap()` and then resolve `length()` against `Map`.

The remaining boundary is intentional: dynamic/unknown values, generic results without concrete metadata, external signatures that omit return metadata, and overloads requiring implicit-conversion ranking still fall back to the established conservative behavior.

## 3. Requirements
- **Recursive Inference**: Implement a robust `inferType(Expression*)` method that recursively traverses the expression tree, calculating the resulting typeId and class name at each node.
- **Metadata Persistence**: Ensure `ClassLayout` and `FunctionDeclaration` metadata are accessible during this recursive pass.

## 4. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED — dynamic `any`-return receivers remain conservative by design**
- **Priority**: Medium (Affects code ergonomics and dispatch safety)
- **Audit 2026-08-05**: Recursive receiver inference, field/method return metadata,
  collection element propagation, and exact overload return selection are
  implemented and covered by JIT regressions. Generic/dynamic chains and
  external signatures without metadata remain future work.
- **Fix 2026-08-09**: Inference now also propagates metadata through unary,
  logical, binary, await, and Hoo free-function expressions, including result
  class names for subsequent receiver dispatch. Unknown receivers remain
  conservative and are rejected when method-name candidates are ambiguous.
- **Fix 2026-08-09 (external signatures)**: Archive-local imports now preserve
  external module path, return type, and parameter metadata. Chained calls use
  that metadata, and overload selection ranks exact matches before safe numeric
  widening and generic/object fallbacks.
- **Fix 2026-08-09 (generic container propagation)**: `any`-array literals whose
  elements share a single inferred type (`["ab", "cd"]any`, `[10, 20]any`)
  propagate that element type through `ArrayAccess` and subsequent chained
  method dispatch, so `values[0].length()` resolves against `String` and scalar
  elements type as `int64`. Mixed (`["ab", 42]any`) and empty (`[]any`)
  literals keep a dynamic element type and continue to reject ambiguous method
  chains. Function-returned `:any` receivers remain conservative compile-time
  errors per the roadmap until runtime type dispatch is designed.
