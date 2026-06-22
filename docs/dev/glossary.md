# Terms & Glossary

## A

| Term | Definition |
|---|---|
| **ANTLR** | ANother Tool for Language Recognition — the parser generator used to produce the Hoo lexer/parser from `.g4` grammar files. |
| **ARC** | Automatic Reference Counting — memory management strategy for reference types. The codegen inserts `retain`/`release` calls around reference-typed values. |
| **AST** | Abstract Syntax Tree — the tree representation of source code produced by `SimpleASTBuilder`. Root is `ast::CompilationUnit`. |
| **argReg** | Helper function in `HVMCodeGenerator` that maps argument indices to register numbers, skipping r4 (reserved thread pointer). |

## B

| Term | Definition |
|---|---|
| **BasicImport** | An import statement of the form `import module.path;`. |
| **BaseType** | A type that is either a primitive or a qualified identifier (e.g., `int64`, `foo.bar.User`). |

## C

| Term | Definition |
|---|---|
| **ClassLayout** | Internal `HVMCodeGenerator` struct tracking a class's fields (offsets), methods (return types, private flags), base class, and modifier flags. |
| **Compressed instruction** | A 16-bit HVM instruction format. Emitted via `emitCompressed(opcode4, rd, rs1, imm4)`. |
| **CompilationUnit** | The root AST node produced by `SimpleASTBuilder::buildAST()`. Contains all imports and top-level declarations. |
| **CST** | Concrete Syntax Tree — the raw parse tree from ANTLR, containing every token including whitespace and punctuation. Distinguished from AST which is a simplified, typed representation. |

## D

| Term | Definition |
|---|---|
| **Demangling** | Reversing mangled names back to human-readable form via `SymbolMangler::demangle()` or `SymbolMangler::demangleSymbol()`. |
| **DemangledSymbol** | Struct returned by `demangleSymbol()` containing module path, class name, function name, base class, modifiers, return type, and parameter types. |

## E

| Term | Definition |
|---|---|
| **encodeComponent** | Function that hex-encodes special characters in identifiers, wrapping the result in `E...E` delimiters. |
| **encodeString** | Low-level hex encoding: each non-alphanumeric character becomes `_<hex><hex>_`. |

## F

| Term | Definition |
|---|---|
| **Free function** | A top-level function that maps to a JIT runtime symbol, not a class method. Categories: JSON, buffer, CSV, filesystem, datetime, encoding, math, hashing, system, process, regex, thread, UUID, character, path. |
| **FromImport** | An import statement of the form `from module import item, ...`. |
| **FunctionPrologueInfo** | Struct returned by `beginFunction()` containing the entry instruction index, byte offset, and mangled name for function prologue/epilogue pairing. |

## H

| Term | Definition |
|---|---|
| **HOModule** | Binary container format for compiled Hoo bytecode. Contains 8 section types (`.text`, `.data`, `.rodata`, `.symtab`, `.reloc`, `.export`, `.import`, `.funcmeta`). Supports `serialize()`/`deserialize()` with LE encoding. |
| **HooCompiler** | Top-level compiler driver (`src/core/HooCompiler.h`) that orchestrates parsing → AST building → codegen → module output. Used directly in tests. |
| **HVM** | Hoo Virtual Machine — the target runtime for compiled bytecode. |
| **HVMCodeGenerator** | The AST-to-bytecode compiler (~4492 lines). Walks `ASTNode` tree, allocates registers, manages scopes, emits instructions, handles built-in class dispatch. |
| **HVMJIT** | LLVM ORC-based JIT execution engine. Registers ~100+ `jit_hoo_*` runtime wrappers, handles trampoline dispatch, and provides `jit_hoo_retain`/`jit_hoo_release` for ARC. |
| **HooParserWrapper** | Parser wrapper used in tests (`src/parsing/HooParserWrapper.h`). Provides `parseForAST()` to get a parse tree from source string. |

## I

| Term | Definition |
|---|---|
| **ImportItem** | A single item in a `from` import, with optional alias (`name as alias`). |

## J

| Term | Definition |
|---|---|
| **JIT** | Just-In-Time compilation — HVMJIT uses LLVM ORC to compile bytecode to native code at runtime. Tested extensively in `tests/jit/` (28 test files). |

## L

| Term | Definition |
|---|---|
| **Label** | Control flow target in `HVMCodeGenerator` with a `targetByteOffset` and a list of `Fixup` entries for patching forward references. |
| **LE** | Little-Endian byte encoding for all binary module data in HOModule. |
| **Local** | Struct in `HVMCodeGenerator` tracking a local variable's stack offset, type ID, class name, element type ID (for arrays), and key type ID (for HashMaps). |

## M

| Term | Definition |
|---|---|
| **MangledFunctionParams** | Input struct for `SymbolMangler::mangleFunctionName()` containing module path, class name, base class, modifiers, function name, return type, and parameter types. |
| **Mangling** | Encoding function/type names into flat linker-visible strings. Uses `_F_` prefix for functions and `_H_` prefix for module symbols. |
| **Modifier flags** | Bitmap constants (singleton, immutable, final, serializable, sealed, borrowing, virtual) stored on `ASTNode`. In the codegen, tracked as boolean fields on `ClassLayout`. |
| **ModulePath** | Represents a dotted module path (e.g., `hoo.collections.List`). Used in imports and mangling. |

## O

| Term | Definition |
|---|---|
| **Opcode** | A single bytecode instruction in the HVM instruction set. |
| **OverloadList** | An AST node that groups multiple function signatures sharing the same name. Created when multiple abstract functions (no body) share the same name. |

## Q

| Term | Definition |
|---|---|
| **QualifiedIdentifier** | A dotted name path (e.g., `foo.bar.User`). Used in type references and import paths. |

## R

| Term | Definition |
|---|---|
| **Register allocation** | Managed by `HVMCodeGenerator` via `usedRegs_[32]` bitmask. r4 reserved for thread pointer. r9-r20 available for temporaries. `argReg()` skips r4 when mapping argument indices to registers. |

## S

| Term | Definition |
|---|---|
| **Section** | A named region in an `HOModule` (text, data, symbol table, etc.). Each section has a type constant (`SHT_*`), size, and payload. |
| **SimpleASTBuilder** | The ANTLR visitor that converts parse trees to ASTs. Uses a stack-based evaluation model with ~50 methods organized by category (declarations, types, statements, expressions, supporting, imports, classes). |
| **Symbol fixup** | A deferred symbol reference in `HVMCodeGenerator`. Stored in `symbolFixups_` as `SymbolFixup` structs with symbol name, instruction index, and byte offset. Resolved at module finalization. |
| **SymbolMangler** | Utility for name mangling and demangling (~744 lines). Provides `mangleFunctionName()`, `mangleModuleSymbol()`, `demangleSymbol()`, `demangle()`, `mangleType()`, and `demangleType()`. |

## T

| Term | Definition |
|---|---|
| **Type ID** | A `uint32_t` runtime identifier for types. Used throughout `HVMCodeGenerator` for register allocation, method dispatch, mangling, and serialization. Built-in types have fixed IDs (String=101, Array=102, etc.). |

## U

| Term | Definition |
|---|---|
| **unknown** | The fallback string returned by `demangleType()` and `codeToTypeName()` when a mangled type code is not recognized. |
