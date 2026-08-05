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
- **Date**: 2026-08-05
- **Status**: **PARTIALLY IMPLEMENTED — advanced/dynamic cases remain**
- **Priority**: Medium (Affects code ergonomics and dispatch safety)
- **Audit 2026-08-05**: Recursive receiver inference, field/method return metadata,
  collection element propagation, and exact overload return selection are
  implemented and covered by JIT regressions. Generic/dynamic chains and
  conversion-ranked overloads remain future work.
