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

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Verified `LogicalAnd` and `LogicalOr` still evaluate both operands before emitting a `LOGIC` instruction or tensor helper call; no branch-based short-circuit lowering is present.
