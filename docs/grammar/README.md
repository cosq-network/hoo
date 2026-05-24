# hoo Language Grammar Reference

This directory contains the normative documentation for the hoo language grammar, as defined in `src/parsing/hoo.g4`. hoo is a high-performance, statically-typed object-oriented language designed for the HVM (hoo Virtual Machine).

## Documentation Index

1.  **[Lexical Structure](lexical-structure.md)**
    *   Keywords, identifiers, literals (string, char, numeric), and comments.
2.  **[Type System](types.md)**
    *   Primitive types, arrays, maps, and nullable (optional) types.
3.  **[Module System](modules.md)**
    *   Python-style imports, module paths, and compilation units.
4.  **[Functions & Methods](functions.md)**
    *   Function declarations, parameters, return types, and modifiers.
5.  **[Classes & Inheritance](classes.md)**
    *   Class declarations, advanced modifiers, inheritance, and constructors.
6.  **[Statements & Control Flow](statements.md)**
    *   Blocks, conditionals, loops, scopes, and exception handling.
7.  **[Expressions & Operators](expressions.md)**
    *   Operator precedence, assignment, logical, relational, and arithmetic operations.

## Technical Foundation

The hoo compiler uses **ANTLR4** for parsing. The grammar is designed to support an aggressive lowering pipeline that translates these high-level constructs into a pure 64-bit RISC ISA (`.ho` bytecode) for physical hardware compatibility. 

When executed, the `HVMJIT` dynamically translates this bytecode into host-native LLVM IR via LLVM ORC v2, providing zero-abstraction execution while bridging high-level operations via the `SYSCALL` interface.

For details on how the HVM dynamic translator and host environment execute these constructs, see the **[hoo Runtime Library Reference](../runtime/README.md)**.
