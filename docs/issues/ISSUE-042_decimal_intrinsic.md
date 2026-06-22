# ISSUE-042 Decimal Intrinsic Data Type

## Goal
Introduce a fixed-precision decimal intrinsic type family, `Decimal<P, S>`, where `P` is the total precision and `S` is the scale. The type must be first-class in Hoo, support arithmetic and comparison, and remain distinct from the existing `float`, `double`/`f64`, and `f8` numeric types.

This proposal uses a dedicated suffix-based decimal numeric literal so ordinary floating-point literals continue to belong to `double` and `f8`.

---

## Design Summary

`Decimal<P, S>` is a built-in managed intrinsic type, similar in surface shape to `Character`, `Buffer`, `DateTime`, and `Uuid`, but parameterized by precision and scale.

- `P` is the total precision: the maximum number of significant base-10 digits the value can store.
- `S` is the scale: the number of digits kept to the right of the decimal point.
- `0 <= S <= P`.
- `Decimal<P, S>` and `Decimal<P2, S2>` are distinct types.
- No implicit promotion occurs between `Decimal` and `double`/`f64`/`f8`.
- Arithmetic and comparison are only defined for decimal-compatible operands with matching type parameters unless the source expression is explicitly converted.

The compiler may lower decimal operations to runtime helpers through `CALL`, but it must not reuse the existing `d` mangling code, because that already denotes `double`/`f64`.

---

## Source-Level Syntax

### Decimal type annotation
Decimal precision and scale are part of the type syntax:

```hoo
Decimal<38, 2> price;
Decimal<18, 6> rate;
```

The syntax is type-level only. It does not change how bare numeric tokens are lexed.

### Decimal literal initialization
Decimal values are initialized from a suffix-based literal:

```hoo
Decimal<38, 2> price = 19.99m;
Decimal<38, 2> tax = 8m;
Decimal<38, 2> ratio = 0.08m;
```

### Function declarations
Use the current Hoo function syntax, where the return type precedes the function name:

```hoo
func: Decimal<38, 2> calculate_bmi(
    weight: Decimal<38, 2>,
    height: Decimal<38, 2>
) {
    return weight / (height * height);
}
```

### Example usage
```hoo
Decimal<38, 2> price = 19.99m;
Decimal<38, 2> taxRate = 0.08m;
Decimal<38, 2> tax = price * taxRate;
Decimal<38, 2> total = price + tax;
print(total.toString());
```

```hoo
Decimal<38, 2> balance = 100.00m;
Decimal<38, 2> withdrawal = 150.00m;

if (withdrawal > balance) {
    print("Insufficient funds");
} else {
    Decimal<38, 2> remaining = balance - withdrawal;
    print(remaining.toString());
}
```

---

## Semantics

### Precision and scale
- `P` and `S` are compile-time constants and are part of the type identity.
- Values are normalized to the declared scale.
- Assignment between different decimal types requires an explicit conversion.
- If a result cannot be represented within the declared precision, the operation throws a decimal overflow error.

### Arithmetic
Supported operators:
- `+`
- `-`
- `*`
- `/`
- `%`
- unary `-`

Rules:
- Both operands must be the same `Decimal<P, S>` type, or one operand must be explicitly converted to that type first.
- Mixed arithmetic with `double`, `f64`, or `f8` is not implicit.
- Integer operands are not implicitly widened into decimal in binary expressions.

### Comparison
Supported comparisons:
- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`

Comparisons are value-based after canonical normalization to the declared scale.

### Non-goals
- No logical operators on decimal values.
- No implicit conversion from decimal to `bool`.
- No replacement for existing `double` or `f8` syntax.
- No change to the lexical meaning of existing floating-point literals.

---

## Grammar And Parsing

### 1. Type syntax
Add a parameterized decimal type production to the type grammar. The new production should parse `Decimal<precision, scale>` in type positions only.

Illustrative shape:

```antlr4
decimalType
    : 'Decimal' '<' INTEGER_LITERAL ',' INTEGER_LITERAL '>'
    ;
```

If the project prefers reusing a shared type-argument rule, `Decimal` should be added there instead of introducing a separate literal token.

### 2. Decimal numeric literal
Add a suffix-based `DECIMAL_LITERAL` lexer rule that stays visually distinct from existing floating-point literals:

```antlr4
DECIMAL_LITERAL: ([0-9]+'.'[0-9]* | '.' [0-9]+ | [0-9]+) [mM];
```

This does not conflict with `FLOATING_LITERAL` or `F8_LITERAL`, because the decimal literal requires a suffix.

If the language later adds another decimal literal form, it must remain non-overlapping with:
- `FLOATING_LITERAL`
- `F8_LITERAL`
- `INTEGER_LITERAL`

### 3. Primary expressions
Decimal construction should be represented as a dedicated decimal literal category, not as a string constructor.

---

## AST And Type System

### AST
Add a decimal type node that preserves `precision` and `scale`, plus a `DecimalLiteral` node for suffix-based decimal literals such as `19.99m`.

### Type model
Extend `PrimitiveTypeKind` or the equivalent type representation only if the compiler keeps `Decimal` in the primitive/type-system layer. If `Decimal` is handled as a managed intrinsic class, keep the type parameters on the type node and resolve the runtime class name separately.

Either way:
- `Decimal<38, 2>` must not collapse to the same type ID or mangled code as `double`.
- `Decimal<38, 2>` and `Decimal<18, 6>` must remain distinct at compile time.

---

## Name Mangling

Decimal needs its own mangling representation.

- Do not use `d`, because that already denotes `double`/`f64`.
- Encode the precision and scale in the mangled signature.
- Type-parameterized decimal forms must round-trip through demangling.

Example shape:

```text
Decimal<38,2> -> unique decimal code + encoded precision/scale
```

The exact short code can be chosen by the implementation, but it must be distinct from existing `double`, `f64`, and `f8` encodings.

---

## Runtime Representation

Decimal should remain an intrinsic managed object if the implementation wants pointer-sized values at the HVM boundary.

Recommended payload:
- scaled integer mantissa
- declared precision
- declared scale
- refcount or managed header semantics, consistent with the rest of the runtime

Implementation requirements:
- construction from decimal literal
- construction from int64
- optional explicit construction from `double`/`f8` with documented rounding behavior
- addition, subtraction, multiplication, division, modulus, negation
- equality and relational comparisons
- string conversion

Implementation constraints:
- overflow must be checked
- division by zero must raise a decimal-specific runtime error
- conversion from binary floating-point must be explicit and documented
- internal constants must be representable in the chosen C++ types; do not write `10^28` as a `ULL` literal if it does not fit

---

## Codegen And JIT

### Code generation
`Decimal` operations should lower to runtime helpers through `CALL`, following the same high-level pattern used by other intrinsic runtime services.

Required behavior:
- type-check operands before lowering
- reject implicit mixing with `double` and `f8`
- emit decimal-specific helper calls for arithmetic and comparisons
- preserve the declared precision and scale in any literal materialization path

#### `src/codegen/HVMCodeGenerator.cpp`

Add decimal type recognition in `typeIdFromDeclaredType()` and preserve the type parameters in the AST/type metadata path:

```cpp
uint32_t HVMCodeGenerator::typeIdFromDeclaredType(const ast::Type* type, std::string* outClassName) const {
    if (auto bt = dynamic_cast<const ast::BaseType*>(type)) {
        if (bt->isPrimitive()) {
            switch (bt->getPrimitiveType()->getKind()) {
                case ast::PrimitiveTypeKind::INT64: return 1;
                case ast::PrimitiveTypeKind::FLOAT:
                case ast::PrimitiveTypeKind::DOUBLE:
                case ast::PrimitiveTypeKind::F64:   return 2;
                case ast::PrimitiveTypeKind::F8:     return 9;
                case ast::PrimitiveTypeKind::BOOL:   return 3;
                case ast::PrimitiveTypeKind::VOID:   return 4;
                case ast::PrimitiveTypeKind::INT8:   return 5;
                case ast::PrimitiveTypeKind::BYTE:   return 6;
                case ast::PrimitiveTypeKind::CHAR:   return 7;
                case ast::PrimitiveTypeKind::BUFFER: return 113;
                default: break;
            }
        }

        const std::string name = bt->getIdentifier();
        if (name == "Decimal") {
            if (outClassName) *outClassName = name;
            return builtinConstructedTypeId(name);
        }
    }
    return 100;
}
```

Parse suffix-based decimal literals and materialize them as decimal constants:

```cpp
uint8_t HVMCodeGenerator::emitDecimalLiteral(const ast::DecimalLiteral& lit,
                                             int32_t precision,
                                             int32_t scale) {
    const auto [scaledValue, ok] = parseDecimalLiteral(lit.getText(), precision, scale);
    if (!ok) {
        addError("Invalid decimal literal for the declared Decimal<P, S> type");
        return 0;
    }
    return emitDecimalConstant(scaledValue, precision, scale);
}
```

Reject implicit mixing with `double` and `f8` before lowering a binary expression:

```cpp
uint8_t HVMCodeGenerator::visitExpression(const ast::Expression& expr) {
    if (const auto* binary = dynamic_cast<const ast::BinaryExpression*>(&expr)) {
        const uint32_t leftType = inferExpressionTypeId(binary->getLeft());
        const uint32_t rightType = inferExpressionTypeId(binary->getRight());

        const bool leftIsDecimal = leftType == HOO_TYPE_DECIMAL;
        const bool rightIsDecimal = rightType == HOO_TYPE_DECIMAL;

        if (leftIsDecimal || rightIsDecimal) {
            if (leftType != rightType) {
                addError("Decimal operands must have matching precision and scale");
                return 0;
            }
            return emitDecimalBinaryOp(*binary);
        }
    }
    // Existing non-decimal lowering continues here.
}
```

Lower decimal arithmetic through runtime symbols:

```cpp
uint8_t HVMCodeGenerator::emitDecimalBinaryOp(const ast::BinaryExpression& binary) {
    const uint8_t lhs = visitExpression(binary.getLeft());
    const uint8_t rhs = visitExpression(binary.getRight());

    const char* symbol = nullptr;
    switch (binary.getOperator()) {
        case ast::BinaryOperator::ADD:       symbol = "_F_hoo_Decimal_add_p_p_p"; break;
        case ast::BinaryOperator::SUBTRACT:  symbol = "_F_hoo_Decimal_sub_p_p_p"; break;
        case ast::BinaryOperator::MULTIPLY:  symbol = "_F_hoo_Decimal_mul_p_p_p"; break;
        case ast::BinaryOperator::DIVIDE:    symbol = "_F_hoo_Decimal_div_p_p_p"; break;
        case ast::BinaryOperator::MODULUS:   symbol = "_F_hoo_Decimal_mod_p_p_p"; break;
        default: return 0;
    }

    emit(Opcode::MOV, OperandsR{1, lhs, 0, 0});
    emit(Opcode::MOV, OperandsR{2, rhs, 0, 0});
    emitCall(Opcode::CALL, symbol);
    const uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    return dest;
}
```

### JIT bindings
Add JIT wrappers for the decimal runtime helpers using the same `extern "C"` bridging style as other runtime-managed intrinsics.

The wrappers must:
- pass pointer handles correctly
- return newly allocated decimal handles where appropriate
- propagate runtime exceptions for divide-by-zero and overflow

#### `src/hvm/HVMJIT.cpp`

Add decimal runtime bridges alongside the existing managed-object wrappers:

```cpp
#include "runtime/lib/hoo_decimal.h"

extern "C" uint64_t jit_hoo_decimal_add(void* state_ptr) {
    auto* state = static_cast<HVMJIT::HVMState*>(state_ptr);
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(
        hoo_decimal_add(reinterpret_cast<HooDecimal>(state->regs[1]),
                        reinterpret_cast<HooDecimal>(state->regs[2]))));
}

extern "C" uint64_t jit_hoo_decimal_eq(void* state_ptr) {
    auto* state = static_cast<HVMJIT::HVMState*>(state_ptr);
    return static_cast<uint64_t>(
        hoo_decimal_eq(reinterpret_cast<HooDecimal>(state->regs[1]),
                       reinterpret_cast<HooDecimal>(state->regs[2])));
}
```

---

## Files To Update

| File | Change |
|------|--------|
| `src/parsing/Hooc.g4` | Add `Decimal<precision, scale>` type syntax in type positions and a suffix-based `DECIMAL_LITERAL` rule. |
| `src/ast/Type.h` | Add decimal type metadata if the type system tracks it directly. |
| `src/ast/ASTImpl.cpp` | Render decimal types including precision and scale. |
| `src/parsing/SimpleASTBuilder.cpp` | Build decimal type nodes and decimal literal nodes, validating `precision >= scale >= 0`. |
| `src/core/SymbolMangler.cpp` | Add a unique decimal encoding and include precision/scale in mangled forms. |
| `src/codegen/HVMCodeGenerator.cpp` | Resolve decimal types, reject implicit mixing with `double`/`f8`, and lower decimal ops to runtime helpers. |
| `src/runtime/lib/hoo_runtime.h` | Register the decimal intrinsic type and decimal-specific runtime error identifiers. |
| `src/runtime/lib/hoo_decimal.h` | New runtime API for decimal arithmetic, comparison, constant materialization, and formatting. |
| `src/runtime/lib/hoo_decimal.cpp` | New decimal runtime implementation. |
| `src/hvm/HVMJIT.cpp` | Add JIT entry points for decimal arithmetic and comparison helpers. |
| `tests/parsing/*` | Add decimal type parsing coverage. |
| `tests/runtime/*` | Add decimal construction, arithmetic, comparison, and overflow tests. |
| `tests/jit/*` | Add end-to-end decimal lowering tests. |

## Unit Test Plan

Add focused unit tests that verify the decimal feature end to end:

### Parsing and AST
- `DecimalTypeParsingTest.ParsesPrecisionAndScale`
- `DecimalTypeParsingTest.RejectsMissingScale`
- `DecimalTypeParsingTest.RejectsScaleGreaterThanPrecision`
- `DecimalTypeParsingTest.RejectsZeroPrecision`
- `DecimalLiteralParsingTest.ParsesSuffixLiteral`
- `DecimalLiteralParsingTest.RejectsUnsignedFloatLiteralAsDecimal`
- `DecimalLiteralParsingTest.KeepsFloatAndF8LiteralsDistinct`
- `SimpleASTBuilderTest.BuildsDecimalTypeNode`
- `SimpleASTBuilderTest.BuildsDecimalLiteralNode`

### Type resolution and mangling
- `DecimalTypeResolutionTest.ResolvesDecimalDeclaredType`
- `DecimalTypeResolutionTest.KeepsDecimalDistinctFromDouble`
- `DecimalTypeResolutionTest.KeepsDecimalDistinctFromF8`
- `SymbolManglerTest.ManglesDecimalWithUniqueCode`
- `SymbolManglerTest.DemanglesDecimalTypeRoundTrip`
- `SymbolManglerTest.DoesNotReuseDoubleCodeForDecimal`

### Code generation
- `HVMCodeGeneratorDecimalTest.InfersDecimalLiteralType`
- `HVMCodeGeneratorDecimalTest.RejectsMixedDecimalAndDoubleArithmetic`
- `HVMCodeGeneratorDecimalTest.RejectsMixedDecimalAndF8Arithmetic`
- `HVMCodeGeneratorDecimalTest.EmitsDecimalAddCall`
- `HVMCodeGeneratorDecimalTest.EmitsDecimalSubCall`
- `HVMCodeGeneratorDecimalTest.EmitsDecimalMulCall`
- `HVMCodeGeneratorDecimalTest.EmitsDecimalDivCall`
- `HVMCodeGeneratorDecimalTest.EmitsDecimalComparisonCall`

### Runtime arithmetic
- `HooDecimalTest.FromLiteralText`
- `HooDecimalTest.FromInt64`
- `HooDecimalTest.Add`
- `HooDecimalTest.Sub`
- `HooDecimalTest.Mul`
- `HooDecimalTest.Div`
- `HooDecimalTest.Mod`
- `HooDecimalTest.Neg`
- `HooDecimalTest.Eq`
- `HooDecimalTest.Neq`
- `HooDecimalTest.LtLe`
- `HooDecimalTest.GtGe`
- `HooDecimalTest.NormalizesScaleForEquality`
- `HooDecimalTest.TrimsTrailingZerosInToString`

### Runtime errors and boundaries
- `HooDecimalTest.DivByZero`
- `HooDecimalTest.ModByZero`
- `HooDecimalTest.OverflowOnAdd`
- `HooDecimalTest.OverflowOnMul`
- `HooDecimalTest.RejectsInvalidLiteralSuffix`
- `HooDecimalTest.RejectsLiteralExceedingPrecision`

### JIT integration
- `HooDecimalJitTest.NewDecimalLiteralLowering`
- `HooDecimalJitTest.AddLowering`
- `HooDecimalJitTest.ComparisonLowering`
- `HooDecimalJitTest.DivideByZeroPropagatesException`
- `HooDecimalJitTest.KeepsDecimalHandlesAliveAcrossCallBoundary`

### `src/parsing/SimpleASTBuilder.cpp`

Parse the decimal type arguments and validate the declared bounds:

```cpp
std::unique_ptr<ast::Type> SimpleASTBuilder::buildType(HooParser::TypeContext* ctx) {
    if (ctx->decimalType()) {
        const auto* dec = ctx->decimalType();
        const int64_t precision = std::stoll(dec->INTEGER_LITERAL(0)->getText());
        const int64_t scale = std::stoll(dec->INTEGER_LITERAL(1)->getText());

        if (precision < 1 || scale < 0 || scale > precision) {
            throw std::runtime_error("Invalid Decimal<P, S>: require P >= 1 and 0 <= S <= P");
        }

        return std::make_unique<ast::DecimalType>(precision, scale);
    }

    return buildExistingType(ctx);
}
```

Add a decimal literal branch in `buildPrimary()`:

```cpp
if (ctx->DECIMAL_LITERAL()) {
    return std::make_unique<DecimalLiteral>(
        buildToken(ctx->DECIMAL_LITERAL()->getSymbol()),
        ctx->DECIMAL_LITERAL()->getText());
}
```

If the implementation stores decimal as a managed intrinsic class instead of a dedicated AST type, the same parse step should still preserve both integers on the type node so codegen and mangling can see them later.

---

## Acceptance Criteria

The design is acceptable only if all of the following are true:

1. `Decimal<P, S>` is defined in grammar syntax.
2. Ordinary floating literals still parse as `double`, while decimal literals require a distinct suffix such as `m`.
3. `f8` and `double` mangling remain unchanged.
4. Decimal mangling does not reuse `d`.
5. Decimal arithmetic does not implicitly accept `double` or `f8` operands.
6. Precision and scale are validated and preserved at compile time.
7. Runtime overflow and divide-by-zero behavior is explicitly defined.

---

## Status
- **Date**: 2026-06-22
- **Status**: **REVISED DESIGN REQUIRED**
- **Priority**: Medium
- **Audit 2026-06-22**: The previous draft conflicted with existing numeric literal handling, reused the `d` mangling code already assigned to `double`/`f64`, and treated decimal as if it should overlap with `f8` and `double`. This version makes precision and scale part of the type syntax and keeps decimal separate from the existing floating-point types.
