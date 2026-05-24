# ISSUE-005: Unimplemented Class Modifiers

## 1. Overview
The Hooc grammar (`src/parsing/Hooc.g4`) defines eight class modifiers:
- `singleton`
- `immutable`
- `factory`
- `observable`
- `service`
- `strategy`
- `actor`
- `final`

While the parser and AST (`src/ast/ClassDeclaration.h`) successfully capture these modifiers, the backend (`HVMCodeGenerator.cpp`) completely ignores them during code generation.

## 2. Technical Analysis
Currently, `HVMCodeGenerator::generateModule` iterates through class declarations but only computes field offsets for basic layout. It does not check `classDecl->hasModifier(...)`.

The only subsystem that currently acknowledges these modifiers is `SymbolMangler`, which encodes them into function/method names (e.g., `_F_MyClass_N_CT` for a singleton constructor).

## 3. Requirements & Lowering Suggestions
To achieve "Hardware Purity" and full language support, the code generator must implement specific behaviors for these modifiers:

*   **`singleton`**: The generated constructor (`CT`) should likely be restricted, or a static instance pointer should be implicitly generated in the `.bss` or `.data` section to hold the single instance.
*   **`immutable`**: The codegen should enforce that `ST.D` (store) instructions cannot target the field offsets of an immutable class after initialization.
*   **`final`**: Ensure that no other class can extend this class (this is likely a semantic check needed before codegen, but the codegen could also emit metadata for the JIT).
*   **`actor` / `observable` / `service` / `strategy` / `factory`**: These high-level architectural modifiers likely require emitting specific `SHT_TYPE` metadata or registering the class with runtime dispatchers/schedulers in the JIT/Runtime environment.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: Medium
