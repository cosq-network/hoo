# ISSUE-048: String Interpolation Is a Placeholder Only

## 1. Overview
The grammar defines an `interpolatedString` production, but it is explicitly marked as a placeholder. The parser treats interpolated strings as plain string literals, ignoring any `${...}` interpolation syntax. No AST node, codegen pass, or runtime support exists for interpolation.

## 2. Technical Analysis
- **Grammar**: `src/parsing/Hooc.g4:343` — `interpolatedString: STRING_LITERAL; // Placeholder for interpolated strings with ${...}`
- **Impact**: `"Hello, ${name}!"` is treated as the literal text `Hello, ${name}!` with no substitution.

## 3. Impact
- Users must manually concatenate strings to build dynamic content.
- String interpolation is a common language feature that users will expect to work.
- Code examples or documentation referencing interpolation will be misleading.

## 4. Suggested Fix
1. Add an `InterpolatedStringExpr` AST node that stores the template segments.
2. In the parser, scan the string literal for `${...}` patterns and split into segments.
3. In codegen, lower interpolation to a sequence of string concatenation calls using `String.from` and `+`.
4. Add runtime helpers if needed for efficient multi-part concatenation.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **FIXED**
- **Priority**: **MEDIUM**

## 6. Resolution
Interpolation is parsed into text and expression parts by the AST builder and
lowered to managed string construction and concatenation by the HVM code
generator. Parser, AST, codegen, cleanup, and JIT regressions are covered.
