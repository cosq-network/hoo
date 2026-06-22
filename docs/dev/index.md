# Hoo Developer Guide

## Topics

1. [ANTLR4 Parser Generation](antlr-parser.md) — How the lexer/parser are generated from `.g4` grammar files.
2. [SimpleASTBuilder](simple-ast-builder.md) — The stack-based ANTLR visitor that converts parse trees to ASTs.
3. [HVMCodeGenerator](hvm-code-generator.md) — The AST-to-bytecode compiler (~3000+ lines).
4. [HOModule Binary Layout](ho-module.md) — The binary container format (8 sections, LE encoding).
5. [SymbolMangler](symbol-mangler.md) — Name mangling and demangling for linker-visible symbols.
6. [Unit Test Integration](unit-test-integration.md) — How the test framework is structured and run.
7. [Writing New Unit Tests](writing-tests.md) — How to add tests to existing suites or create new ones.
8. [HVMJIT](hvm-jit.md) — The LLVM ORC-based JIT execution engine (~8224 lines).
9. [Modifying the Core Pipeline](modifying-pipeline.md) — How to change SimpleASTBuilder, HVMCodeGenerator, and HVMJIT.
10. [Dos and Don'ts](dos-and-donts.md) — Conventions and pitfalls to avoid.
11. [FAQ](faq.md) — Frequently asked questions.
12. [Terms & Glossary](glossary.md) — Definitions of key terms.
