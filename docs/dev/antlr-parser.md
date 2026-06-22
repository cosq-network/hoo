# How ANTLR4 Generates Parser

ANTLR4 takes grammar files (`.g4`) and generates a lexer, parser, and visitor/listener base classes.

## Grammar files

The grammar is split across two files:

- **`src/parser/hoo_lexer.g4`** — token definitions (keywords, operators, literals, punctuation).
- **`src/parser/hoo_parser.g4`** — production rules for the full Hoo language.

## Generation step

Edit `CMakeLists.txt` or run the `antlr4` tool directly. The output is written alongside the grammar files:

```
src/parser/
  ho_parser.h / ho_parser.cpp
  ho_lexer.h / ho_lexer.cpp
  ho_parser_visitor.h / ho_parser_visitor.cpp
  ho_parser_base_visitor.h / ho_parser_base_visitor.cpp
```

## How SimpleASTBuilder uses it

`SimpleASTBuilder` extends `ho_parser_base_visitor` and overrides every `visit*` method. Each method returns an `AntlrRef` (a visitor-defined node wrapper). The parser produces a concrete syntax tree (CST); the visitor converts it to an AST by walking the tree and building `ASTNode` objects.

> **Convention:** Every grammar rule `someRule` has a corresponding `visitSomeRule` override in `SimpleASTBuilder`. Keep these in sync.
