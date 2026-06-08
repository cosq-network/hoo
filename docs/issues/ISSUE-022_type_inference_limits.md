# ISSUE-022: Limited Type Inference for `var` Declarations

## 1. Overview
The `var` keyword relies on compile-time type inference from the initializer expression, but the inference only handles a narrow set of cases: primitive literals, `Map.new()`, `Array.new()`, and a few constructor calls. All other cases fall back to the generic typeId 100 (Object), losing type information for method dispatch.

## 2. Technical Analysis

### 2.1 Limited inference scope
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1828-1885 (`getTypeId`)
- **Issue**: The type inference switch only checks `PrimaryExpression` sub-types (integer literal → typeId 1, float literal → typeId 2, string literal → typeId 101, etc.) and specific function-call patterns (`Map.new()` → 103, `Array.new()` → 102, `Character.new()` → 109).

```cpp
// Fallthrough cases not handled:
var a = someFunction();       // typeId = 100 (Object)
var b = obj.method();         // typeId = 100 (Object)
var c = arr[0];               // typeId = 100 (Object)
var d = someComplexExpr;      // typeId = 100 (Object)
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

## 4. Suggested Fix
1. Extend `getTypeId()` to handle more expression types:
   - Function call return types (if the function has a declared return type).
   - Method call return types (if the method has a declared return type).
   - Array subscript access (propagate the array's element type).
2. For `var` declarations on function/method calls, infer the type from the declaration's return type.
3. Propagate element types through array literal construction.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **MEDIUM**
