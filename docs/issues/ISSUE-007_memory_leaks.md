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
- **Date**: 2026-06-29
- **Status**: **IMPLEMENTED** (All three remaining gaps closed)
- **Priority**: **HIGH**
- **Audit 2026-06-29**: 
  - **String literal ownership**: Added `isManagedTemporary()` helper to identify freshly allocated temporaries (StringLiteral, InterpolatedString, NewObjectExpression, NewHashMapExpression, ArrayLiteral, TensorLiteral). Expression statements now emit `hoo_release` for these temporaries after evaluation. Binary string concat also releases fresh operand temporaries after `String_concat`.
  - **Reassignment release**: `AssignmentExpression` for local variables now loads and releases the old managed value via `hoo_release` before `ST_D`. Increment/decrement for local variables also releases the old managed value before storing the result.
  - **Abnormal exit cleanup**: Added `emitScopeCleanup(from, to)` helper that walks `scopeStack_` and emits `hoo_release` for managed locals. Called from `visitReturn` (all scopes), `visitBreak`/`visitContinue` (scopes from current depth to loop entry depth), and `beginFunction` (implicit return path). `ControlFlowScope` now tracks `scopeDepth` for precise cleanup.
  - **Type inference**: Added `InterpolatedString` to `inferExpressionTypeId()` returning typeId 101, ensuring expression statement cleanup catches interpolated strings.
- **Test Verification 2026-06-29**: Added 5 new unit tests in `HVMCodeGeneratorTest.cpp`: `StringLiteralExprStmt_EmitsRelease`, `AssignmentToManagedLocal_ReleasesOldValue`, `ReturnFromBlock_CleansUpManagedLocals`, `LoopBreak_CleansUpManagedLocals`, `InterpolatedStringExprStmt_EmitsRelease`. Full codegen test suite passes (138 tests: 18 + 120).
