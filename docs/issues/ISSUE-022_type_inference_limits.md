# ISSUE-022: Type Inference for `var` Declarations

## 1. Overview
The `var` keyword uses compile-time expression inference. The inference engine
now recursively carries runtime type ID, user-defined class name, collection
element type, and map key type through supported expressions, preserving type
information for method dispatch and typed lowering.

## 2. Technical Analysis

### 2.1 Limited inference scope
- **Location**: `src/codegen/HVMCodeGenerator.cpp` (`inferExpressionTypeInfo` and `getTypeId`)
- **Implementation**: Inference handles literals, operators, function returns,
  method returns, fields, array access, arrays, tensors, maps, futures, and
  user-defined class metadata. Unknown or dynamic expressions intentionally
  fall back to type ID 100 (`Object`).

```cpp
// Extended but still incomplete:
var a = someFunction();       // inferred from the declared return type
var b = obj.method();         // inferred from class method metadata
var c = arr[0];               // inferred from the array element type
var d = someComplexExpr;      // Object only when the expression is unknown

// These ARE inferred (added since initial issue filing):
var e = ch.codepoint();       // typeId = 1 (int64) — Character method
var f = m.getInt64String(k);  // typeId = 101 (String) — Map method  
var g = m.getInt64Double(k);  // typeId = 2 (double) — Map method
var h = m.containsInt64(k);   // typeId = 1 (int64) — Map method
var i = args.count();         // typeId = 1 (int64) — Args method
```

### 2.2 Impact on method dispatch
When a `var` variable has typeId 100 (Object), instance method calls on it use `methodNameToClass_` for dispatch. As ISSUE-015 documents, this single-class map dispatches to the wrong class when multiple classes define the same method name.

### 2.3 Array literal element type
Array literals infer a common element type recursively. Homogeneous literals
such as `[1, 2, 3]` retain `int64` element metadata; heterogeneous or dynamic
elements conservatively use `Object`.

## 3. Impact
- Method calls on `var` variables return wrong results when multiple classes share method names.
- Heterogeneous or dynamic expressions may still require `Object` dispatch.
- No type-driven optimizations possible for most `var` variables.

## 4. Fixes Applied
1. ✅ Extended `getTypeId()` to handle:
   - Function call return types (looked up from `functionReturnTypes_`).
   - Method call return types (looked up from `ClassLayout::methodReturnTypes`).
   - Array subscript access (looked up from `Local::elementTypeId`).
2. ✅ `var` declarations on function/method calls now infer the type from the declaration's return type (including user-defined class names via `Local::className`).
3. ✅ Array literal element types inferred from uniform literal elements and stored in header (offset 16).
4. ✅ Added recursive expression metadata for chained function/method calls,
   field access, array access, futures, and collection values.
5. ✅ Added method return-class metadata and overload return selection by
   inferred parameter types.
6. ✅ Receiver inference now drives method and field dispatch before the
   method-name fallback index, while preserving private-access enforcement.

### Type-system boundaries
- `any`, unknown external signatures, and incompatible heterogeneous values
  intentionally remain dynamic (`Object`) and require runtime handling.
- Overload selection uses exact inferred parameter IDs with a conservative
  fallback; implicit conversion ranking remains a separate overload-system
  concern.

## 5. Status
- **Date**: 2026-08-05
- **Status**: **IMPLEMENTED for the supported compile-time type model**
- **Priority**: **MEDIUM**
- **Audit 2026-08-05**: Recursive receiver and result metadata is implemented and verified for built-in, user-defined, collection, array-access, and overload return paths. Dynamic/unknown expressions remain intentionally conservative.
- **Update 2026-06-11 (a)**: Inference extended to cover instance method return types on `Args` (typeId 110), `Character` (typeId 109), and `Map` (typeId 103) objects. Method calls on these types now correctly infer int64, double, and String return types.
- **Update 2026-06-11 (b)**: Major inference expansion:
  - **Direct function calls** (`var x = someFunction()`): Now looks up the function's declared return type from `functionReturnTypes_` map. Works for all return types including user-defined classes (with class name propagation via `functionReturnClass_`).
  - **User-defined class method calls** (`var y = obj.method()`): Now looks up the method's return type from `ClassLayout::methodReturnTypes`, populated during class method indexing. Class name is tracked per local variable via `Local::className` and propagated from declared types and inferred return types.
  - **Parameter type tracking**: Parameter types now also track className for method inference on parameters.
  - **Changes**: Added `methodReturnTypes` to `ClassLayout`, `functionReturnTypes_`/`functionReturnClass_` maps, `className` field to `Local`, and `typeIdFromDeclaredType()` helper. Extended `getTypeId()` signature with `outClassName` parameter.
- **Update 2026-06-11 (c)**: Array and collection inference:
  - **Array subscript access** (`var x = arr[0]`): Now looks up the array's `elementTypeId` from the local variable's scope. Works for typed arrays (`Array<Int64>`) and array literals with uniform element types.
  - **Array literal element type inference**: When constructing `[1, 2, 3]`, the common element type is inferred from the literal elements and stored in the header (offset 16).
  - **For-in loop variable** (`for x in arr`): Now infers the element type from the iterable's `elementTypeId`.
  - **Changes**: Added `elementTypeId` to `Local` struct, `getLocalElementTypeId()` helper, and array literal element type inference in `visitExpression`.
- **Update 2026-06-17 (d)**: Major expansion of `inferExpressionTypeId` to support low-precision and tensor types:
  - **Scalars**: Explicit support for `F8Literal` (typeId 9) and `BitLiteral` (typeId 8).
  - **Operators**: Logical NOT now correctly returns `bool` (3) or `tensor<bit>` (104). Logical AND/OR now correctly infer `bit` (8) when both operands are bits, otherwise `bool` (3).
  - **Relational Ops**: Binary relational operators now correctly infer `bool` (3) or `tensor<bit>` (104) based on operand types.
  - **Tensors**: Added `typeId 104` for tensors. Arithmetic operations on tensors now correctly propagate the tensor type.
  - **Nesting**: Added recursive handling for `ParenthesizedExpression`.
  - **Function Returns**: Inference now consults the `functionReturnTypes_` map for all function calls.
- **Update 2026-08-05**: Basic chained inference is complete and covered by JIT tests for map-to-string, function-to-array, array access, and user-defined method-to-array dispatch. ISSUE-031 now tracks only broader generic/dynamic chain semantics beyond the supported metadata model.

## 6. Verification

- JIT regression coverage verifies chained map/string dispatch, function-return
  array dispatch, chained array element inference, and user-defined method
  return dispatch.
- Existing tests cover literals, operators, tensors, decimals, futures,
  collections, and access-control behavior.
- Full build and CTest pass with 0 failures; the test binary registers 2,044
  GoogleTest cases.
