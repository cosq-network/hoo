# ISSUE-022: Limited Type Inference for `var` Declarations

## 1. Overview
The `var` keyword relies on compile-time type inference from the initializer expression, but the inference only handles a narrow set of cases: primitive literals, `Map.new()`, `Array.new()`, and a few constructor calls. All other cases fall back to the generic typeId 100 (Object), losing type information for method dispatch.

## 2. Technical Analysis

### 2.1 Limited inference scope
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1828-1885 (`getTypeId`)
- **Issue**: The type inference switch checks `PrimaryExpression` sub-types (integer literal → typeId 1, float literal → typeId 2, string literal → typeId 101, etc.) and specific function-call patterns (`Map.new()` → 103, `Array.new()` → 102, `Character.new()` → 109) and instance method return types on known built-in types (Args, Character, Map).

```cpp
// Extended but still incomplete:
var a = someFunction();       // typeId = 100 (Object) — not inferred
var b = obj.method();         // typeId = 100 (Object) — not inferred
var c = arr[0];               // typeId = 100 (Object) — not inferred
var d = someComplexExpr;      // typeId = 100 (Object) — not inferred

// These ARE inferred (added since initial issue filing):
var e = ch.codepoint();       // typeId = 1 (int64) — Character method
var f = m.getInt64String(k);  // typeId = 101 (String) — Map method  
var g = m.getInt64Double(k);  // typeId = 2 (double) — Map method
var h = m.containsInt64(k);   // typeId = 1 (int64) — Map method
var i = args.count();         // typeId = 1 (int64) — Args method
```

### 2.2 Impact on method dispatch
When a `var` variable has typeId 100 (Object), instance method calls on it use `methodNameToClass_` for dispatch. As ISSUE-015 documents, this single-class map dispatches to the wrong class when multiple classes define the same method name.

### 2.3 Hardcoded element type for array literals
- **Location**: `src/codegen/HVMCodeGenerator.cpp` line 766
- **Issue**: Array literal elements are hardcoded to typeId 100 (Object). No inference is done from the actual element values.

## 3. Impact
- Method calls on `var` variables return wrong results when multiple classes share method names.
- Array literals always create "array of Object" regardless of actual element types.
- No type-driven optimizations possible for most `var` variables.

## 4. Fixes Applied
1. ✅ Extended `getTypeId()` to handle:
   - Function call return types (looked up from `functionReturnTypes_`).
   - Method call return types (looked up from `ClassLayout::methodReturnTypes`).
   - Array subscript access (looked up from `Local::elementTypeId`).
2. ✅ `var` declarations on function/method calls now infer the type from the declaration's return type (including user-defined class names via `Local::className`).
3. ✅ Array literal element types inferred from uniform literal elements and stored in header (offset 16).

### Remaining work
- Method-level function return type info (no separate map; currently relies on `FunctionDeclaration` AST during `visitFunction`).

## 5. Status
- **Date**: 2026-06-08
- **Status**: **PARTIALLY FIXED**
- **Priority**: **MEDIUM**
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
- **Update 2026-06-17 (e)**: Remaining work on method chains is now tracked separately in ISSUE-031.
