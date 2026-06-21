# ISSUE-007: ARC Memory Leaks in Generated Code

## 1. Overview
The HVMCodeGenerator emits `hoo_retain`/`hoo_release` calls for string interpolation intermediates but never releases string literals, new-expression results, or function return values. This causes heap-allocated objects to leak in all but trivial programs.

## 2. Technical Analysis

### 2.1 String literal leaks
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 781-805
- **Issue**: `fromCStr_static` creates a heap-allocated String object, but `hoo_release` is never called on the result.

### 2.2 New-expression leaks
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 910-975
- **Issue**: `new` expressions call `hoo_alloc` for user-defined class instances but never emit a matching `hoo_release`. The caller is expected to release, but there is no ownership tracking in the generated code.

### 2.3 Interpolated string inconsistency
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 821-823 vs 781-805
- **Issue**: Interpolated strings call `hoo_release` on intermediate concat results, but the final string value is never released by the caller. Plain string literals never call `hoo_release` at all.

## 3. Impact
- Every string literal in user code leaks.
- Every `new T()` leaks unless the result is manually released via an intrinsic.
- Long-running programs will exhaust memory.

## 4. Suggested Fix
- Emit `hoo_release` calls for string literals at the end of their use scope.
- Implement a scope-based ownership tracking system in `HVMCodeGenerator` that emits `hoo_release` for all allocated expressions before function return or block exit.
- Use the existing `scopeStack_` to track variable lifetimes and emit releases on scope exit.

## 5. Related Issues
- ISSUE-022: No ARC/release for new object expressions (merged into this issue)

## 6. Status
- **Date**: 2026-06-08
- **Status**: **PARTIALLY IMPLEMENTED**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Scope release, return-value retain, and new-object retain paths now exist. String literal ownership, reassignment release of old managed values, and abnormal exits through `return`/`break`/`continue`/exceptions still leave ARC incomplete.
