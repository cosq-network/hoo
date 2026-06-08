# ISSUE-020: Removed Class Modifiers and Legacy Grammar Dead Code

## 1. Overview
The grammar defines eight class modifiers (`singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`, `final`) but four of these have been removed from the language (`factory`, `observable`, `strategy`, `actor`). The grammar still parses and accepts them, which is misleading.

## 2. Technical Analysis

### 2.1 Removed modifiers still in grammar
- **Location**: `src/parsing/Hooc.g4`
- **Issue**: The `classModifier` rule still includes `factory`, `observable`, `strategy`, `actor` even though they were removed from the language (per ISSUE-005 implementation notes, commit `360f682`).

```
classModifier
    : SINGLETON | IMMUTABLE | FACTORY | OBSERVABLE
    | SERVICE | STRATEGY | ACTOR | FINAL;
```

- `FACTORY`, `OBSERVABLE`, `STRATEGY`, `ACTOR` are still in the grammar `SINGLETON IMMUTABLE FACTORY OBSERVABLE SERVICE STRATEGY ACTOR FINAL` rule.
- These keywords remain reserved but produce no codegen effect.

### 2.2 Dead `interpolatedString` grammar rule
- **Location**: `src/parsing/Hooc.g4` line 325
- **Issue**: The rule `interpolatedString: STRING_LITERAL;` is never referenced by any parser rule. String interpolation is handled entirely through textual scanning for `${` patterns in the AST builder.

### 2.3 `getBinaryOperator()` declared but never defined
- **Location**: `src/ast/SimpleASTBuilder.h` line 103
- **Issue**: Method is declared `public:` in the header but has no implementation. Calling it causes a linker error.

### 2.4 `getBoolValue()` defined but never called
- **Location**: `src/ast/SimpleASTBuilder.cpp` lines 1074-1077
- **Issue**: Fully implemented but no caller exists. Boolean literal handling is done inline.

## 3. Impact
- Confusing grammar that accepts keywords with no effect.
- Dead code paths that may mislead developers during maintenance.
- `getBinaryOperator()` will cause a linker error if any code tries to call it.

## 4. Suggested Fixes
1. Remove `FACTORY`, `OBSERVABLE`, `STRATEGY`, `ACTOR` from the `classModifier` grammar rule.
2. Remove the dead `interpolatedString` rule from `Hooc.g4`.
3. Either implement `getBinaryOperator()` or remove its declaration.
4. Either use `getBoolValue()` in the AST builder or remove it.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **LOW**
