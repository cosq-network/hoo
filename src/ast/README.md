# AST Subsystem (`src/ast`)

This directory contains the Typed Abstract Syntax Tree (AST) which serves as the core semantic model of the Hooc compiler. It bridge the gap between the ANTLR4 parse tree and the aggressive lowering backends (HVM and LLVM).

## 1. The Compilation Pipeline

```text
Source (.hoo)
  -> ANTLR4 Parse Tree
  -> SimpleASTBuilder
  -> Typed AST (Semantic Model)
  -> HVM/LLVM Backend (Aggressive Lowering)
```

## 2. Core Responsibilities

The AST is not just a structural mirror of the code; it is an enriched semantic graph responsible for:
- **Type Resolution**: Explicitly tagging every expression and declaration node with its resolved type.
- **Lowering Metadata**: Providing the backend with the information needed for physical hardware mapping, such as class field offsets and type IDs.
- **Modifier Enforcement**: Capturing structural constraints like `SINGLETON`, `IMMUTABLE`, and `ACTOR` for specialized JIT dispatch.
- **FFI Mapping**: Maintaining complex native signatures for cross-platform linking.

## 3. Architecture & Ownership

- **Ownership**: The tree is constructed using `std::unique_ptr` and `std::vector<std::unique_ptr<T>>`, ensuring clear ownership hierarchies and automatic memory management.
- **Visitors**: The backends utilize the **Visitor Pattern** (implemented in `HVMCodeGenerator` and `LLVMCodeGenerator`) to perform recursive translation and lowering.

## 4. Main Files

| File | Description |
| :--- | :--- |
| `CompilationUnit.h` | The root node of a module, containing all top-level declarations. |
| `Declaration.h` | Nodes for `func`, `var`, `const`, and `class`. |
| `ClassDeclaration.h`| Specialized node for classes, including modifiers and inheritance. |
| `Expression.h` | Nodes for all operations (Arith, Logic, Calls, Member Access). |
| `Statement.h` | Control flow nodes (`if`, `while`, `for`) and Exception blocks. |
| `Type.h` | Normative model for Hooc types (Primitive, Array, Map, Optional). |
| `SimpleASTBuilder.cpp`| The primary engine for converting ANTLR artifacts into the Typed AST. |

## 5. Synchronizing with HVM v1.4

The AST must preserve metadata required for the **v1.4 "Hardware Ready"** profile:
- **`__hoo_init`**: The AST captures global initializers to ensure the `_F_module_init_v` function is correctly emitted.
- **Shadow Stack**: Exception nodes are structured to support the lowering to `hoo_push_handler` and `hoo_throw` runtime calls.
- **Physical Layouts**: Class nodes provide the field ordering needed for the backend to calculate precise memory offsets for `LD.D`/`ST.D`.

## 6. Development Checklist

When extending the Hooc language:
1. Update `src/parsing/Hooc.g4` with the new syntax.
2. Define the corresponding node in `src/ast/`.
3. Update `SimpleASTBuilder` to populate the new node.
4. Update `src/codegen/HVMCodeGenerator.cpp` to implement the lowering rule for the new node.
5. Add comprehensive unit tests in `tests/ast/` and `tests/codegen/`.
