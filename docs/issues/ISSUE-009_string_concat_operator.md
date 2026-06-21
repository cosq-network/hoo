# ISSUE-009: `+` on String Operands Produces Pointer Arithmetic, Not Concatenation

## 1. Overview
The `+` operator between string values compiles to an `ARITH ADD` instruction on the opaque handle values. This performs integer addition on the string pointers instead of calling the runtime string concatenation function.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1363-1391
- **Issue**: The `BinaryOperator::PLUS` case emits an `ARITH` instruction with func=0 (ADD) unconditionally, regardless of operand types. There is no type-driven dispatch to call `hoo_string_concat` for string operands.

```cpp
case BinaryOperator::PLUS: {
    emit(Opcode::ARITH, OperandsR{dest, leftReg, rightReg, 0}); // always integer add
    break;
}
```

## 3. Impact
- `"hello" + " world"` compiles to a nonsensical pointer addition.
- Any program using `+` for string concatenation produces wrong results silently.
- This also affects string + number concatenation in interpolated contexts.

## 4. Suggested Fix
Add a type check before the ARITH emission. If both operands are Strings (typeId 101) or one is a String, emit a call to `_F_hoo_string_concat_s_s` instead of `ARITH ADD`.

```cpp
case BinaryOperator::PLUS: {
    if (bothOperandsAreString || oneOperandIsString) {
        emitCall(Opcode::CALL, "_F_hoo_string_concat_v_p_p");
    } else {
        emit(Opcode::ARITH, OperandsR{dest, leftReg, rightReg, 0});
    }
    break;
}
```

Requires either runtime type checks or compile-time type inference to determine operand types at the `+` point.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Verified binary `+` still lowers through arithmetic opcodes without a string-type branch. String concatenation support remains limited to the interpolated-string helper path.
