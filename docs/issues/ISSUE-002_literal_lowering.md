# ISSUE-002: Missing Literal Lowering (Float/Char/Interpolated)

## 1. Overview
The `HVMCodeGenerator` only supports `IntegerLiteral`, `StringLiteral`, `BooleanLiteral`, and `NullLiteral`. Several high-level literal types defined in the AST are unreachable in the backend.

## 2. Technical Analysis
- **FloatingLiteral**: HVM supports 64-bit doubles. These need to be encoded as 8-byte bit-patterns in the `.rodata` section, then loaded into a register via `LD.D`.
- **CharacterLiteral**: DONE. Lowered to heap-allocated `Character` runtime type.
- **InterpolatedString**: Currently ignored. Requires lowering to a sequence of `hoo_String_from_cstr` and `hoo_String_concat` runtime calls.

## 3. Requirements & Lowering Suggestions
- Implement `dynamic_cast` branches for `ast::FloatingLiteral` and `ast::CharacterLiteral` in `visitExpression`. [DONE]
- For `ast::InterpolatedString`, use a recursive concatenation strategy using the `_F_hoo_String_concat_p_p_p` runtime bridge.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **PARTIALLY IMPLEMENTED (Floating/Character DONE)**
- **Priority**: Medium
