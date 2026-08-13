# How ANTLR4 Generates Parser

ANTLR4 takes a grammar file (`.g4`) and generates a lexer, parser, and visitor/listener base classes.

## What The Generator Produces

For this repo, ANTLR4 generates the following C++ artifacts from `Hooc.g4`:

- `HoocLexer.h` / `HoocLexer.cpp`
- `HoocParser.h` / `HoocParser.cpp`
- `HoocVisitor.h` / `HoocVisitor.cpp`
- `HoocBaseVisitor.h` / `HoocBaseVisitor.cpp`
- `HoocListener.h` / `HoocListener.cpp`
- `HoocBaseListener.h` / `HoocBaseListener.cpp`

It also writes token and interpreter sidecar files used by the runtime parser tooling:

- `Hooc.tokens`
- `HoocLexer.tokens`
- `Hooc.interp`
- `HoocLexer.interp`

## Grammar files

The current Hoo grammar lives in a single file:

- **`src/parsing/Hooc.g4`** — token definitions and production rules for the full Hoo language.

## Generation step

`CMakeLists.txt` drives generation automatically through the `generate_parser` target. The relevant build rule runs ANTLR in `src/parsing/` with the `hooc` package name and writes output to `build/generated/antlr4/`.

If you want to regenerate manually, use the same inputs the build does:

```bash
cd src/parsing
java -jar /path/to/antlr-4.13.2-complete.jar -Dlanguage=Cpp -visitor -listener -o /path/to/build/generated/antlr4 -package hooc Hooc.g4
```

The generated files are written under the build tree:

```
build/generated/antlr4/
  HoocLexer.h / HoocLexer.cpp
  HoocParser.h / HoocParser.cpp
  HoocVisitor.h / HoocVisitor.cpp
  HoocBaseVisitor.h / HoocBaseVisitor.cpp
  HoocLexer.tokens
  HoocLexer.interp
```

The build target also adds the generated directory to the parser library include path, so `HooParserWrapper` and `SimpleASTBuilder` can include `HoocParser.h` and `HoocLexer.h` directly.

## Parsing Pipeline

The runtime parse flow is:

1. `HooParserWrapper` creates an `ANTLRInputStream` from source text.
2. `HoocLexer` tokenizes the stream.
3. `CommonTokenStream` buffers the tokens.
4. `HoocParser` consumes the tokens and builds parse-tree contexts.
5. `SimpleASTBuilder` walks those contexts and produces typed AST nodes.

That split keeps the parser layer and the AST layer separate. The generated parser knows grammar structure; the builder knows language semantics and object construction.

## How SimpleASTBuilder uses it

`SimpleASTBuilder` is a standalone class (it no longer subclasses the ANTLR visitor). It walks the parser's concrete syntax tree (CST) directly: `buildAST()` accepts a `HoocParser::CompilationUnitContext`, iterates its children with `dynamic_cast`, and dispatches to dedicated `build*` helpers that construct typed `ASTNode` objects.

The builder is responsible for:

- turning parser contexts into AST nodes
- validating grammar-local invariants
- mapping terminal tokens into typed literals
- translating grammar constructs into Hoo AST classes

The builder is not responsible for lexical analysis or tokenization. Those are handled entirely by the generated lexer and parser.

> **Convention:** Every grammar rule `someRule` has a corresponding `build*` helper in `SimpleASTBuilder` (e.g. `buildFunctionDeclaration`, `buildType`, `buildExpression`). Keep these in sync.

## Practical Notes

- `HoocLexer` tokenizes keywords, operators, literals, delimiters, identifiers, and comments.
- `HoocParser` consumes those tokens and builds parse-tree contexts such as `CompilationUnitContext`, `TypeContext`, and `ExpressionContext`.
- `HooParserWrapper` is the repo’s thin helper around the generated classes. It owns the lexer, token stream, and parser so tests and `SimpleASTBuilder` can parse source without repeating setup code.
- `parseForAST()` parses a full compilation unit and returns `HoocParser::CompilationUnitContext*`.
- `parseExpression()` parses a single expression and returns `HoocParser::ExpressionContext*`.
- `wasSuccessful()` reports whether the last parse completed without syntax errors.
- `getLastError()` carries either a syntax error summary or an exception message from the parser setup.

## Using It In Tests

Parser-focused tests usually follow this pattern:

```cpp
HooParserWrapper parser;
auto* tree = parser.parseForAST(source);
ASSERT_NE(nullptr, tree);
ASSERT_TRUE(parser.wasSuccessful()) << parser.getLastError();

SimpleASTBuilder builder;
auto ast = builder.buildAST(tree);
ASSERT_NE(nullptr, ast);
```

Expression-only tests can use `parseExpression()` when they do not need a full compilation unit.

## Troubleshooting

- If `HoocParser.h` or `HoocLexer.h` are missing, the parser generation target has not run or the generated include directory is not on the include path.
- If you change grammar rules but the builder stops compiling, check whether `SimpleASTBuilder` is missing a corresponding `build*` update.
- If the parser reports syntax errors on valid code, inspect the token stream first. Most mistakes come from lexer rule ordering, missing keywords, or punctuation changes in `Hooc.g4`.
- If the generated classes appear stale, clean the build tree or rerun the `generate_parser` target so CMake rebuilds the ANTLR outputs.
