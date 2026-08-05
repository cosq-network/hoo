# FAQ

## General

**Q: How do I add a new keyword to the language?**

A: Add the token and production rules in `src/parsing/Hooc.g4`, regenerate the parser, then add a `build*` or `visit*` method in `SimpleASTBuilder`. Finally, add codegen support in `HVMCodeGenerator`.

**Q: Where does type checking happen?**

A: Mostly in `HVMCodeGenerator`. The AST builder (`SimpleASTBuilder`) does minimal validation — primarily `rejectAnyTypeInPosition` to forbid `any` in certain contexts and single-constructor enforcement. The codegen performs type inference via `inferExpressionTypeId()`, checks argument counts via parameter type lists, validates modifiers (immutable field reassignment, singleton instantiation, etc.), and enforces serializable class constraints.

**Q: How do I debug bytecode output?**

A: Use the pattern from `HVMCodeGeneratorTest.cpp` — decode instructions from the `.text` section and inspect opcodes/mnemonics:

```cpp
auto insts = module->decodeInstructions(
    module->getSection(".text")->data);
for (size_t i = 0; i < insts.size(); ++i) {
    std::cout << "  [" << i << "] mnemonic=" << insts[i].getMnemonic()
              << " opcode=" << (int)insts[i].getOpcode() << std::endl;
}
```

You can also dump raw bytes with `std::hex` for manual inspection, or add logging to `HVMCodeGenerator` (it emits to `errors_`).

**Q: What's the difference between `src/tests/` and `tests/`?**

A: Tests are in `tests/`. There is no `src/tests/`. The test directory mirrors `src/`:
- `tests/parsing/` — Parser and SimpleASTBuilder tests (26 files)
- `tests/codegen/` — HVMCodeGenerator tests (2 files)
- `tests/core/` — SymbolMangler, compiler, and CLI tests (4 files)
- `tests/jit/` — JIT execution tests (28 files)
- `tests/hvm/`, `tests/ast/`, `tests/runtime/`, `tests/repl/`, `tests/examples/` — Other test categories

## Mangling

**Q: Why did my symbol mangling test fail?**

A: Likely because the mangler and demangler are out of sync. Run a round-trip test:
```cpp
std::string mangled = SymbolMangler::mangleFunctionName(params);
DemangledSymbol demangled = SymbolMangler::demangleSymbol(mangled);
// Verify fields match
```
Verify you added the new type code to both `getTypeCodeMap()` (mangling) and the `demangleType` recursive parser (demangling). Also check `getModifierCodeMap()` and `getFunctionModifierCodeMap()` for modifier changes.

**Q: How do I add a new primitive type?**

A: Add the mapping in `getTypeCodeMap()` in `src/core/SymbolMangler.cpp`, add the demangling case in `demangleType()`, update `typeIdToMangleType()` in `HVMCodeGenerator.cpp`, and add tests for both mangle and demangle round-trips.

**Q: Why is my mangled name not unique for overloaded functions?**

A: The mangler includes parameter types in the output. Verify that each overload has distinct parameter types. Use `EXPECT_NE(m1, m2)` in tests to confirm.

**Q: What happens when `mangleType` encounters an unrecognized type?**

A: It falls back to hex encoding: `"Q" + toHex(normalized) + "Z"`. `demangleType` can still decode this. The fallback code `"o"` is only returned for empty input.

## Code generation

**Q: How does ARC work in generated code?**

A: The codegen inserts `retain` calls after object allocations and `release` calls at the end of scopes for reference-typed locals. The JIT provides `jit_hoo_retain` / `jit_hoo_release` runtime helpers. Register allocation reserves r4 as the thread pointer (not available for args).

**Q: How are registers allocated?**

A: The `argReg()` helper maps argument indices to register numbers, skipping r4 (reserved for thread pointer). Temporary registers (r9-r20) are managed with a bitmask in `usedRegs_[32]`, allocated via `allocateRegister()` and freed via `freeRegister()`. Loop and switch control-flow scopes checkpoint and restore that mask around `break`, `continue`, and join points.

**Q: Can I add a new section type to HOModule?**

A: Yes. Add a new `SHT_*` constant, add serialization/deserialization logic in `HOModule::serialize` / `deserialize`, and update the section dispatch loop.

## Codebase

**Q: How do I find the runtime type ID for a given type?**

A: Use `typeIdFromDeclaredType()` for static type declarations or `inferExpressionTypeId()` for dynamic expression types. Built-in types have fixed IDs: `String`=101, `Array`=102, `Map`=103, `Random`=105, `Character`=109, `Args`=110, `Compression`=111, `Csv`=112, `Buffer`=113, `URL`=114, `HttpClient`=115, `HttpResponse`=116, `HashMap`=117, `AnyArray`=118, `DateTime`=119, `Regex`=120, `Mutex`=121, `Uuid`=122.

**Q: How do I add support for a new built-in class?**

A: Add it to `classToPrefix()` in `HVMCodeGenerator.cpp`, assign a type ID in `builtinConstructedTypeId()`, add the JIT symbols in `HVMJIT.cpp`, add free function dispatch if needed (with `is*FreeFunction()` and `*ReturnTypeId()`), and write parsing + JIT tests.

**Q: How do I diagnose a JIT symbol resolution failure?**

A: Check that the symbol is registered in `HVMJIT.cpp` as a `jit_hoo_*` wrapper. Verify the mangled name matches what the codegen emits (the `SymbolFixup` list in `symbolFixups_` shows what the codegen expects). Check that `LLVMOrcLookupSymbol` can find the symbol.

**Q: What header should I include for a given component?**

A:
| Component | Header |
|---|---|
| SymbolMangler | `src/core/SymbolMangler.h` |
| HVMCodeGenerator | `src/codegen/HVMCodeGenerator.h` |
| SimpleASTBuilder | `src/ast/SimpleASTBuilder.h` |
| HOModule | `src/hvm/HOModule.h` |
| HooCompiler | `src/core/HooCompiler.h` |
| AST types | `src/ast/AST.h` |
| Parser wrapper | `src/parsing/HooParserWrapper.h` |
| Generated parser | `HoocParser.h` |

**Q: How do I set up a test that needs the full compile pipeline?**

A: Use `HooCompiler` directly:
```cpp
auto compiler = std::make_unique<HooCompiler>();
auto module = compiler->compile("test", sourceCode);
```
For parser-only tests, use `HooParserWrapper`:
```cpp
auto parser = std::make_unique<HooParserWrapper>();
auto* tree = parser->parseForAST(code);
auto* ctx = dynamic_cast<HoocParser::CompilationUnitContext*>(tree);
```
