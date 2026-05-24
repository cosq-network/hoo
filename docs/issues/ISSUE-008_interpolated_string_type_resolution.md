# ISSUE-008: Interpolated String Lowering Misclassifies Non-Literal Values

## 1. Overview
Interpolated string lowering currently infers runtime type IDs only from literal AST nodes. When interpolation uses variables, parameters, or nested expressions, the value is treated as `HOO_TYPE_OBJECT` by default even when it is a primitive.

## 2. Technical Analysis
The lowering logic in `src/codegen/HVMCodeGenerator.cpp` checks the AST shape of each interpolated part:

- `IntegerLiteral` -> `HOO_TYPE_INT64`
- `FloatingLiteral` -> `HOO_TYPE_FLOAT64`
- `BooleanLiteral` -> `HOO_TYPE_BOOL`
- `StringLiteral` -> `HOO_TYPE_STRING`
- `CharacterLiteral` -> `HOO_TYPE_CHARACTER`

That works for inline literals, but it does not inspect the actual static type or runtime value for:

- local variables
- parameters
- function return values
- compound expressions

As a result, code such as `var n = 42; return "${n}";` can be lowered as an object conversion instead of an integer conversion.

## 3. Impact
- Primitive values embedded through variables stringify incorrectly.
- Numeric interpolations may produce pointer-like output rather than human-readable values.
- The feature appears correct for literal tests but fails for common real-world cases.

## 4. Suggested Fixes
- Carry type information into interpolation lowering instead of relying on AST node kind alone.
- Reuse existing semantic/type metadata from the frontend when available.
- If static type resolution is unavailable, emit a runtime type query before calling `hoo_string_from_any()`.
- Add tests that cover:
  - integer variables
  - floating-point variables
  - boolean variables
  - function parameters
  - nested expressions such as `${a + 1}`

## 5. Status
- **Date**: 2026-05-25
- **Status**: **TODO (REGRESSION)**
- **Priority**: High
