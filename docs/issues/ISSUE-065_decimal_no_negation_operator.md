# ISSUE-065: Decimal Unary Negation Operator Not Implemented

## Status
- **Date**: 2026-07-19
- **Status**: **OPEN**
- **Priority**: 🟢 **P3 - LOW** (Backlog - nice-to-have feature)
- **Sprint**: Week 4 (Day 2)
- **Estimate**: 0.5 day
- **Related**: ISSUE-059
- **Workaround**: Use `0m - decimal`

---

## 1. Overview
The Decimal intrinsic type supports binary arithmetic (+, -, *, /, %) and comparison operators, but the unary negation operator (`-decimal`) is not implemented. Users must use `0m - decimal` as a workaround.

## 2. Technical Analysis

### Current Grammar (Hooc.g4)
```antlr4
unaryExpression
    : (MINUS | NOT)? postfixExpression
    ;
```

The grammar allows unary minus, but the codegen doesn't handle it for Decimal types.

### Current Codegen (HVMCodeGenerator.cpp:3026-3029)
```cpp
if (leftIsDecimal || rightIsDecimal) {
    if (leftType != rightType) {
        addError("Decimal operands must both be Decimal types");
        return 0;
    }
    return emitDecimalBinaryOp(*binary);
}
```

Only binary operations are handled; no unary decimal handling exists.

### Expected Behavior
```hoo
Decimal<38,2> price = 19.99m;
Decimal<38,2> negative = -price;  // Should work: -19.99m
```

## 3. Requirements
1. Add unary negation support in codegen for Decimal types
2. Add `hoo_decimal_neg()` runtime function
3. Update grammar if needed (already supports unary minus)
4. Add test coverage

## 4. Implementation Notes

### Runtime Function
```c
extern "C" HooDecimal hoo_decimal_neg(HooDecimal d) {
    if (!d) return nullptr;
    const auto* impl = toImpl(d);
    return hoo_decimal_new(-impl->mantissa, impl->precision, impl->scale);
}
```

### Codegen Update
```cpp
if (auto unary = dynamic_cast<const ast::UnaryExpression*>(&expr)) {
    if (unary->getOperator() == ast::UnaryOperator::MINUS) {
        uint32_t operandType = inferExpressionTypeId(unary->getOperand());
        if (operandType == 125) { // HOO_TYPE_DECIMAL
            return emitDecimalUnaryOp(*unary);
        }
    }
}
```

## 5. Impact
- Current: Users must use `0m - decimal` workaround
- After fix: Natural `-decimal` syntax works

## 6. Test Coverage
- Test basic negation: `-19.99m == -19.99m`
- Test double negation: `--19.99m == 19.99m`
- Test negation in expressions: `-(a + b)`
- Test zero: `-0.00m == 0.00m`
