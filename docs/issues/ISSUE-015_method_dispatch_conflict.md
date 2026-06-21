# ISSUE-015: Method Name Resolution Uses Single-Class Map, Causing Wrong Dispatch

## 1. Overview
The `methodNameToClass_` map records only one class per method name. If two unrelated classes define methods with the same name (e.g., `String.length()` and `Map.length()`), the map retains whichever class was processed last in module order. Instance method calls on `var` variables may dispatch to the wrong class.

## 2. Technical Analysis

### 2.1 Single-entry map
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 247-252
- **Issue**: For each function declaration, the code records `methodNameToClass_[fn->getName()] = layout.name`. If a second class defines a method with the same name, it silently overwrites the first entry.

```cpp
// Line 250: Overwrites any previous class with the same method name
methodNameToClass_[fn->getName()] = layout.name;
```

### 2.2 Lookup uses the overwritten entry
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1041-1060
- **Issue**: When resolving a method call like `obj.method()`, the code iterates `methodNameToClass_` to find the class associated with `methodName`. If the wrong class was recorded, the method dispatches to the wrong runtime function.

### 2.3 Impact on `var` variables
When `var obj = ...` is used (type inference falls back to typeId 100 = Object), method dispatch relies entirely on `methodNameToClass_`. Two classes with a `length()` method (String, Array, Map) would all resolve to whichever was defined last.

## 3. Impact
- `someArray.length()` may call `String.length()` if String is defined after Array.
- `someString.contains(x)` may call `Map.contains()` if Map is defined after String.
- Silent data corruption with no compiler error.

## 4. Suggested Fix
Replace the single-class map with a multi-class index:

```cpp
// Instead of:
std::unordered_map<std::string, std::string> methodNameToClass_;

// Use:
std::unordered_map<std::string, std::vector<std::string>> methodNameToClasses_;
```

When resolving, if a method name has multiple candidate classes, emit a runtime type check (load the object's `type_id` from offset -8 of the ARC header) to select the correct dispatch target.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **PARTIALLY MITIGATED**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Verified method-return inference reduces some ambiguity, but `methodNameToClass_` remains keyed only by method name, so same-name methods across classes can still collide.
- **Update 2026-06-11**: Type inference improvements (see ISSUE-022) now resolve many `var` declarations to precise typeIds (int64, double, string, etc.) instead of falling back to 100 (Object). This reduces the impact of the `methodNameToClass_` conflict for variables with inferrable types. 
- **Update 2026-06-17**: Major inference expansion for `f8`, `bit`, and `tensor` types further reduces the vulnerability by ensuring that arithmetic and logic results are precisely typed. The remaining risk for chained method calls is now tracked in ISSUE-031. The multi-class index fix in §4 remains the correct long-term architectural solution for non-inferrable types.
