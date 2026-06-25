# ISSUE-023: Missing Return Value Validation for Non-Void Functions

## 1. Overview
Non-void functions that lack a `return` statement still compile successfully. The function epilogue returns whatever value happens to be in register r1 at the end of the function body, producing silent garbage.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 390-393
- **Issue**: After visiting the function body, the code generator always emits `LEAVE; RET`. There is no check that a `return` statement was actually emitted for non-void functions.

```cpp
// Function epilogue — always emitted:
emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
emit(Opcode::RET, OperandsR{0, 0, 0, 0});
```

The `return` statement would set r1 to the return value. Without a return, r1 contains whatever value was last computed (a loop variable, an unused expression result, or garbage from register initialization).

## 3. Impact
- Functions with non-void return types that forget to `return` compile without error.
- Return values are garbage that depends on the preceding code flow.
- Hard-to-debug bugs that appear/disappear with unrelated code changes.

## 4. Suggested Fix
Track whether a `return` statement was visited in the current function. In the epilogue, if the function is non-void and no `return` was encountered, emit a compile error.

```cpp
struct FunctionPrologueInfo {
    // ... existing fields ...
    bool hasReturn_ = false; // NEW
};

// In visitReturnStatement:
info.hasReturn_ = true;

// In endFunction:
if (!info.isVoid && !info.hasReturn_) {
    addError("Non-void function '" + fnName + "' has no return statement");
}
```

Alternatively, for runtime safety, emit r1 = 0 before `RET`:

```cpp
emit(Opcode::MOVZ, OperandsI{1, 0, 0}); // r1 = 0 (safety zero)
emit(Opcode::LEAVE, ...);
emit(Opcode::RET, ...);
```

## 5. Status
- **Date**: 2026-06-08
- **Status**: **FIXED**
- **Priority**: **MEDIUM**
- **Audit 2026-06-24**: Missing return check implemented. `currentFunctionHasReturn_` flag tracked in `visitStatement(ReturnStatement)`. Error emitted in `beginFunction` for non-void functions lacking a return statement. Verified by `NonVoidFunctionMissingReturn` and `NonVoidFunctionWithReturn` tests.
- **Fix 2026-06-24**: Added `currentFunctionHasReturn_` member to `HVMCodeGenerator`, tracked across return statements, and enforced in `beginFunction` via `addError()` for non-void functions missing a return. Tested with `NonVoidFunctionMissingReturn`, `NonVoidFunctionWithReturn`, and `VoidFunctionWithoutReturn`.
