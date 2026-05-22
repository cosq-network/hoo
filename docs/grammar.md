# Hooc Grammar Specification

This document summarizes the active grammar in `src/parsing/Hooc.g4` and its relationship to the current HVM core profile.

Normative grammar source:

- `src/parsing/Hooc.g4`

Normative HVM sources:

- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`

## 1. Lexical Overview

### 1.1 Core Keywords

- Declarations: `func`, `class`, `constructor`, `var`, `const`
- Control flow: `if`, `else`, `for`, `while`, `in`, `by`, `break`, `continue`, `return`, `scope`
- OOP/modifiers: `extends`, `public`, `private`, `async`, `final`, `singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`
- Module/import: `import`, `from`, `as`
- Exceptions: `try`, `catch`, `finally`, `throw`, `rethrow`
- FFI: `native`, `extern`, `pointer`, `array`, `library`, `link`, `dynamic`, `at`, `function`
- Built-in literals: `true`, `false`, `null`
- Reserved internal token: `__hoo_init` (`HOO_INIT`)

### 1.2 Primitive Types

- `int8`, `byte`, `int64`
- `float`, `double`, `f64`
- `bool`, `char`, `string`, `void`

### 1.3 Operators

- Arithmetic: `+ - * / %`
- Assignment: `= += -= *= /= %= <<= >>=`
- Increment/decrement: `++ --`
- Relational: `== != < <= > >=`
- Logical: `&& || !`
- Misc: `? -> .. .`

## 2. Top-Level Structure

`compilationUnit` supports:

1. zero or more `importStatement`
2. mixed stream of:
  - declarations (`functionDeclaration`, `classDeclaration`, `variableDeclaration`, `constantDeclaration`)
  - FFI declarations (`ffiDeclaration`)
3. EOF

## 3. Type Grammar

- `type`: optional type or map type
- optional type: `arrayType QUESTION?`
- arrays: repeated `[]` suffix
- base type: primitive or qualified identifier
- map type: `map[keyType, valueType]`
- key type restricted to: `byte | int8 | int64 | char | string`

## 4. Statement Grammar

Supported statements:

- block
- variable declaration statement
- expression statement
- return statement
- `if`
- `while`
- `for`
- `scope`
- `break`
- `continue`
- `try/catch/finally` (simplified rule)
- `throw` / `rethrow`

## 5. Expression Grammar

Precedence chain:

1. assignment (`=` and compound assignment forms)
2. logical OR (`||`)
3. logical AND (`&&`)
4. relational/equality (`== != < <= > >=`)
5. additive (`+ -`)
6. multiplicative (`* / %`)
7. unary (`- !`)
8. postfix/member/index/call plus `++/--`
9. primary literals/identifiers/new/array literal/parenthesized

## 6. FFI Grammar

Top-level FFI declarations:

- library import declaration
- dynamic link declaration
- native function declaration
- native variable declaration

FFI type grammar includes:

- primitive and qualified types
- pointer types
- fixed-size array parameter types
- function types

## 7. HVM Core Alignment

The current grammar aligns with the HVM `core-minimalest` profile:

- arithmetic/logical/relational expressions
- control-flow forms
- objects/arrays
- exceptions
- FFI bridge

This grammar does not require core inclusion of:

- SIMD/vector opcode families
- threading/atomic/TLS opcode families
- interrupt/system/debug opcode families

Those remain outside core and belong to optional extension profiles.

## 8. Notes

- Any grammar extension that introduces genuinely new runtime semantics should trigger synchronized updates to:
  - AST builder
  - lowering/codegen
  - `HVM_SPEC.md`
  - `hvm_instruction_set.csv`
  - `instructions.md`
