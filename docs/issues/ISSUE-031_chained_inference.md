# ISSUE-031: Missing Return Type Inference for Method Chains

## 1. Overview
Hoo's type inference system (`getTypeId`) has been significantly improved for simple method calls (e.g., `var x = obj.method()`). However, it fails to infer types for "chained" method calls (e.g., `var x = obj.getMap().length()`), reverting to `typeId 100` (Object) for the entire chain.

## 2. Technical Analysis
In `HVMCodeGenerator.cpp`, the `inferExpressionTypeId` logic for `MemberAccess` and `FunctionCall` only looks one level deep. It can resolve the return type of a method if the object's type is known in the local scope. However, for a chain:
1. `obj` is found in scope -> `Type A`.
2. `obj.getMap()` return type is looked up from `Type A`'s metadata -> `Map`.
3. The result of `obj.getMap()` is an intermediate expression that is **not** in the local variable scope.
4. `(obj.getMap()).length()` fails because the inference engine doesn't know the type of the intermediate result of `getMap()`.

## 3. Requirements
- **Recursive Inference**: Implement a robust `inferType(Expression*)` method that recursively traverses the expression tree, calculating the resulting typeId and class name at each node.
- **Metadata Persistence**: Ensure `ClassLayout` and `FunctionDeclaration` metadata are accessible during this recursive pass.

## 4. Status
- **Date**: 2026-06-16
- **Status**: **PARTIALLY IMPLEMENTED**
- **Priority**: Medium (Affects code ergonomics and dispatch safety)
- **Audit 2026-06-21**: Recursive receiver inference and method return maps now cover basic chained calls, but dispatch is still limited by method-name-only class lookup and lack of overload-aware signatures.
