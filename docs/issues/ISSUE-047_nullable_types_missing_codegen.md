# ISSUE-047: Nullable/Optional Types Defined in Grammar and AST But Not Implemented in Codegen

## 1. Overview
The grammar and AST define `OptionalType` (nullable types using `?` suffix, e.g., `int64?`, `string?`), and the parser builds OptionalType AST nodes. However, the code generator returns a hard-coded type ID of `100` (generic object) for all optional types without any null-safety semantics.

## 2. Technical Analysis
- **Grammar**: `src/parsing/Hooc.g4` line 196 — `optionalType: arrayType QUESTION?;`
- **AST**: `src/ast/Type.h:109-123` — `OptionalType` class
- **Codegen**: `src/codegen/HVMCodeGenerator.cpp:3459` — `if (dynamic_cast<const ast::OptionalType*>(type)) return 100;`
- **Missing**:
  - Null-check code generation before dereference
  - Null-safety validation in assignments and calls
  - Proper type tracking for nullable vs non-nullable variants
  - Assignment compatibility checking between `T` and `T?`

## 3. Impact
- Using `?` syntax produces incorrect behavior (no null checks emitted).
- Type safety is silently bypassed — a null value can be assigned to a non-nullable `int64` without any check.
- The language advertises nullable types but they are non-functional.

## 4. Suggested Fix
1. Add a pass to validate that non-nullable types do not receive null values at compile time where possible.
2. Emit null-check branches before dereferencing optional values.
3. Ensure `T?` is tracked as a distinct type through the type system and semantics, not reduced to generic `any`.

## 5. Status
- **Date**: 2026-06-23
- **Status**: **PROPOSED**
- **Priority**: **HIGH**
