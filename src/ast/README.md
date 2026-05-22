# AST Subsystem (`src/ast`)

This directory contains the typed Abstract Syntax Tree used between parsing (`Hooc.g4`) and code generation.

## 1. Pipeline Position

```
Source (.hoo)
  -> ANTLR parse tree
  -> SimpleASTBuilder
  -> Typed AST
  -> code generation / lowering
```

## 2. Core Responsibilities

- represent grammar constructs as typed nodes
- preserve semantic distinctions needed by downstream codegen/lowering
- provide stable structure for analysis and tests

## 3. Main Files

- `AST.h`: umbrella include
- `ASTNode.h`: base node abstraction
- `Declaration.h`: declarations (functions, variables/constants, classes, FFI declarations)
- `Statement.h`: statements (block/control flow/exception/scope/etc.)
- `Expression.h`: expression tree nodes
- `Type.h`: type nodes (primitive/array/optional/map/qualified)
- `ImportStatement.h`: import AST forms
- `CompilationUnit.h`: root node
- `SimpleASTBuilder.h/.cpp`: parse-tree to AST conversion
- `ASTImpl.cpp`: string/utility implementations for node display

## 4. Grammar Coverage

`SimpleASTBuilder` is intended to track `src/parsing/Hooc.g4` closely:

- imports and module paths
- top-level declarations
- class members/modifiers/constructors
- variable/constant declarations
- type forms (primitive, arrays, optional, map, qualified identifiers)
- statements including exception constructs
- expression precedence and postfix forms
- FFI declaration/type forms

When grammar changes, builder and tests should be updated in the same change.

## 5. Design Notes

- Nodes use ownership via `std::unique_ptr`
- Parent ownership is explicit through child containers/fields
- AST should avoid parser-specific details that are not semantically meaningful

## 6. Change Checklist

When adding or changing a grammar feature:

1. update `Hooc.g4`
2. add/update AST node types if needed
3. update `SimpleASTBuilder` mappings
4. update codegen/lowering behavior
5. add/adjust parser/AST/codegen tests

Keep this subsystem synchronized with:

- `docs/grammar.md`
- `docs/features.md`
- `docs/hvm/HVM_SPEC.md` (for VM-facing lowering assumptions)
