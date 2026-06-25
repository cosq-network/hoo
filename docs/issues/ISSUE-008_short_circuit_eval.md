# ISSUE-008: Missing Short-Circuit Evaluation for `&&` and `||`

## 1. Overview
The logical AND (`&&`) and logical OR (`||`) operators are compiled to unconditional `LOGIC` instructions that evaluate both operands. The Hoo language requires short-circuit semantics: `a && b` must not evaluate `b` if `a` is false.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1393-1410
- **Issue**: `LogicalAnd` and `LogicalOr` are lowered to `ARITH` instructions with bitwise AND/OR. Both operands are evaluated unconditionally via `visitExpression`.

```cpp
// Current (broken) lowering for &&:
uint8_t leftReg = visitExpression(expr.getLeft());
uint8_t rightReg = visitExpression(expr.getRight());
emit(Opcode::ARITH, OperandsR{dest, leftReg, rightReg, 2}); // bitwise AND
```

## 3. Impact
- **Null safety broken**: `a != null && a.foo()` crashes because `a.foo()` is evaluated even when `a` is null.
- **Redundant side effects**: `f() && g()` always calls both functions even when `f()` returns false.
- **Incorrect semantics**: Any program depending on short-circuit behavior will produce wrong results.

## 4. Suggested Fix
Replace the unconditional evaluation with a conditional branch pattern:
```
  evaluate left -> r1
  if op == AND: BEQ r1, 0, falseLabel
  if op == OR:  BNE r1, 0, trueLabel
  evaluate right -> r1
falseLabel/trueLabel:
  mov dest, r1
```

## 5. Resolution
Short-circuit evaluation for `&&` and `||` is now implemented in `HVMCodeGenerator.cpp` (lines 2840–2901) using conditional `BEQ`/`BNE` branches that skip the right operand evaluation based on the truthiness of the left operand.

## 6. Status
- **Date**: 2026-06-08
- **Status**: **IMPLEMENTED**
- **Priority**: **HIGH**
- **Audit 2026-06-24**: Short-circuit evaluation confirmed working. `LogicalAnd` emits `BEQ left, 0, skipLabel` to skip right evaluation when left is falsy; `LogicalOr` emits `BNE left, 0, skipLabel` to skip right evaluation when left is truthy.
