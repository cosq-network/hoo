# ISSUE-021: Missing Statement Types in Grammar and Codegen

## 1. Overview
The Hoo language grammar and codegen are missing several common statement types and have incomplete implementations for others. These gaps limit expressiveness and cause compiler crashes for valid Hoo programs.

## 2. Issues

### 2.1 No `do-while` loop
- **Grammar**: `src/parsing/Hoo.g4`
- **Codegen**: `src/codegen/HVMCodeGenerator.cpp`
- **Issue**: The language has `while` and `for` but no `do-while` loop. The grammar has no corresponding rule.

### 2.2 No `switch/case` statement
- **Grammar**: `src/parsing/Hoo.g4`
- **Issue**: No `switch` expression or multi-branch conditional statement exists beyond `if/else`.

### 2.3 No `scope` block (documented)
- **Documentation**: `docs/issues/ISSUE-003_scope_statement.md`
- **Status**: Already documented. The AST node exists but `visitStatement` dispatch is missing.
- **Issue**: The `scope { }` block for explicit lifetime management is not implemented.

### 2.4 `for-in` only supports arrays
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 568-622
- **Issue**: The `for-in` loop lowering hardcodes array layout assumptions. It does not support:
  - Iterating over strings (character-by-character)
  - Iterating over maps (key-value pairs)
  - Iterating over ranges as objects
  - Custom iterable classes

```cpp
// Hardcoded array layout:
// length at offset 0, capacity at offset 8
// elements start at offset 32
```

### 2.5 `try-catch-finally` has no guarantee for finally
- **Documentation**: `docs/issues/ISSUE-012_finally_block_safety.md`
- **Issue**: The finally block is not guaranteed to execute when exceptions propagate.

## 3. Impact
- Programs needing `do-while` or `switch` must use equivalent `while`/`if` chains.
- `for (c in "hello")` produces nonsensical bytecode.
- `for (k, v in map)` fails to compile or crashes.

## 4. Suggested Priority Order
1. Add `for-in` support for strings (needed for character iteration).
2. Add `scope` block dispatch in `visitStatement` (minimal effort, short-term path).
3. Add `do-while` loop to grammar and codegen.
4. Add `switch/case` statement to grammar and codegen.
5. Add `for-in` support for maps.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **MEDIUM**
- **Audit 2026-06-21**: Grammar still lacks `do`, `switch`, and explicit `scope` statements. `for-in` remains array/helper-oriented and `finally` execution remains unsafe under ISSUE-012.
