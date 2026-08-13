# Dos and Don'ts

## Do

- **Keep SimpleASTBuilder stack-based** — Do not add ad-hoc state that breaks the visitor pattern.
- **Use the `ASTNode` flag system** for node classification (kinds: `FunctionDecl`, `VarDecl`, etc.).
- **Check `SymbolMangler` test coverage** when adding new type codes.
- **Validate serializable classes** in the codegen, not the parser.
- **Use `LE` encoding** for all binary module data.
- **Run tests** before committing: `cmake --build --preset ninja-relwithdebinfo-tests && ctest --preset ninja-relwithdebinfo` (or `cmake --build . --target hoo-tests && ./hoo-tests`).
- **Update mangle/demangle together** — they must stay symmetric.

## Don't

- **Don't modify generated parser files** (anything under `build/generated/antlr4/`, i.e. `Hooc*.cpp/h`). Edit `src/parsing/Hooc.g4` and regenerate.
- **Don't bypass the AST** — do not generate bytecode directly from the parse tree.
- **Don't hardcode offsets** in `HOModule` — always use section-relative addressing.
- **Don't add `any` fields to `serializable` classes** — the codegen will reject these.
- **Don't push values onto the SimpleASTBuilder stack without popping them** in the caller's visitor — stack leaks cause subtle bugs.
