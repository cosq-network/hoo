# How SimpleASTBuilder Works

**File:** `src/ast/SimpleASTBuilder.h` / `.cpp`

`SimpleASTBuilder` is a single-pass AST builder that consumes ANTLR parse-tree contexts directly. It transforms a flat parse tree into a structured `ASTNode` tree, producing a `std::unique_ptr<ast::CompilationUnit>` as the root.

## Architecture

```cpp
class SimpleASTBuilder {
    std::unique_ptr<ast::CompilationUnit> buildAST(HoocParser::CompilationUnitContext* ctx);
    // ... private build* helpers for declarations, statements, expressions, types
};
```

`SimpleASTBuilder` is a standalone class (it no longer subclasses the ANTLR
visitor). It uses a **context-dispatch model**:

1. `buildAST()` iterates the `CompilationUnitContext` children, dispatching on the context type with `dynamic_cast`.
2. Each `build*` method receives a parser rule context and constructs a typed `ASTNode` (or `ast::*` value).
3. Top-level overloaded functions are grouped into `OverloadList` nodes during the same pass.
4. The finished tree is returned as `std::unique_ptr<ast::CompilationUnit>`.

## Entry point

```cpp
std::unique_ptr<ast::CompilationUnit> buildAST(HoocParser::CompilationUnitContext* ctx);
```

The single public entry point takes a parser `CompilationUnitContext` and returns the fully built AST. All internal methods are private.

## Method categories (~50 methods)

| Category | Examples | Purpose |
|---|---|---|
| Declarations | `buildFunctionDeclaration`, `buildVariableDeclaration`, `buildConstantDeclaration` | Functions, variables, constants |
| Types | `buildPrimitiveType`, `buildOptionalType`, `buildArrayType`, `buildMapType`, `buildDictType`, `buildTensorType`, `buildBaseType` | Type expressions |
| Statements | `buildIfStatement`, `buildWhileStatement`, `buildForInStatement`, `buildForRangeStatement`, `buildTryCatchStatement`, `buildThrowStatement` | Control flow |
| Expressions | `buildAssignmentExpression`, `buildLogicalOrExpression`, `buildAdditiveExpression`, `buildMultiplicativeExpression`, `buildUnaryExpression`, `buildPostfixExpression`, `buildPrimary`, `buildNewExpression` | Value computations |
| Supporting | `buildBlock`, `buildParameter`, `buildArgumentList`, `buildArrayLiteral`, `buildTensorLiteral`, `buildExpressionList` | Structural helpers |
| Imports | `buildImportStatement`, `buildBasicImport`, `buildFromImport`, `buildModulePath` | Module imports |
| Classes | `buildClassDeclaration`, `buildConstructorDeclaration`, `buildClassBody`, `buildClassMember` | Class/struct definitions |

## Literal value helpers

Public helper methods (used in tests and internally):

| Method | Purpose |
|---|---|
| `getIntValue(TerminalNode*)` | Parses `INTEGER_LITERAL` to `int64_t` |
| `getDoubleValue(TerminalNode*)` | Parses `FLOATING_LITERAL` to `double` |
| `getF8Value(TerminalNode*)` | Parses f8 literal, promotes to `double` |
| `getBitValue(TerminalNode*)` | Parses bit literal |
| `getCharValue(TerminalNode*)` | Extracts Unicode codepoint from `CHAR_LITERAL` |
| `getStringValue(TerminalNode*)` | Extracts string content (removes quotes) |
| `getBoolValue(TerminalNode*)` | Returns `true`/`false` from `TRUE`/`FALSE` tokens |
| `getBinaryOperator(TerminalNode*)` | Maps operator token to `BinaryOperator` enum |
| `isInterpolatedString(TerminalNode*)` | Detects `${...}` interpolation syntax |
| `parseInterpolatedString(string)` | Splits template into `InterpolatedString::Part` segments |
| `getPrimitiveTypeKind(string)` | Maps type name string to `PrimitiveTypeKind` |

## Key patterns

- **Overloaded functions** — Abstract functions at the same name (no body) are grouped into an `OverloadList` node. Concrete bodies are compiled individually.
- **Class member overloading** — Class member declarations with the same name are collected into an `ASTNode` with `OverloadList` flag.
- **Serializable/any validation** — `buildClassDeclaration` calls `rejectAnyTypeInPosition` to forbid `any` fields in `serializable` classes. `any` is only allowed as a function return type or inside container type parameters (Map, Dict).
- **Modifiers** — Constructor modifiers (`getClassModifier`) and function modifiers (`getFunctionModifier`) are parsed from the grammar and stored as enum values on the AST node.
- **Single constructor enforcement** — `buildClassDeclaration` validates that each class has at most one constructor.
- **Type inference in variable declarations** — `buildVariableDeclaration` handles both explicit types and `var` inference.

## 'any' type restrictions

The `rejectAnyTypeInPosition` method enforces that `any` cannot be used in:
- Variable declarations
- Parameters
- Constants
- Catch clauses

It is permitted only as a function return type or inside container type parameters.

## Data flow

```
Source text  →  ANTLR Lexer  →  Token stream
                                  ↓
                              ANTLR Parser  →  Parse tree (CST)
                                  ↓
                SimpleASTBuilder (context dispatch)  →  ASTNode tree
                                  ↓
                        HVMCodeGenerator  →  Bytecode
```

## Test patterns

Tests in `tests/parsing/SimpleASTBuilderTest.cpp` use `HooParserWrapper` to parse source code, then call `SimpleASTBuilder::buildAST()` and inspect the resulting `CompilationUnit` via `toString()`:

```cpp
class SimpleASTBuilderTest : public ::testing::Test {
protected:
    std::unique_ptr<HooParserWrapper> parser;
    std::unique_ptr<SimpleASTBuilder> astBuilder;
    
    antlr4::tree::ParseTree* parseCode(const std::string& code) {
        return parser->parseForAST(code);
    }
};

TEST_F(SimpleASTBuilderTest, BuildSingleFunctionDeclaration) {
    std::string code = "func test() { return; }";
    auto ast = astBuilder->buildAST(getCompilationUnit(parseCode(code)));
    ASSERT_NE(ast, nullptr);
    std::string astStr = ast->toString();
    EXPECT_TRUE(astStr.find("declarations=1") != std::string::npos);
}
```
