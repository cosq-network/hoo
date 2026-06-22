# How Unit Tests Are Integrated

The project uses a custom test framework (`src/tests/`) that integrates with the build system.

## Test structure

```
src/tests/
  test_runner.cpp          — Main entry point, registers all test suites
  test_ast.cpp             — AST construction and manipulation tests
  test_codegen.cpp         — Bytecode generation tests
  test_module.cpp          — Module serialization tests
  test_mangler.cpp         — Symbol mangling/demangling tests
  test_parser.cpp          — Parser-level tests
  test_ir.cpp              — IR-level tests
  fixtures/                — Sample .ho source files used by tests
  utils.h/cpp              — Shared test utilities
```

## Test registration

Each test file defines a suite using a macro system:

```cpp
// test_mangler.cpp

TEST_SUITE(Mangler) {
    TEST(MangleSimpleFunction) {
        SymbolMangler mangler;
        std::string mangled = mangler.mangle("foo", {Type::Int, Type::String});
        ASSERT_EQ(mangled, "_F_fooi_s_");
        return true;
    }

    TEST(DemangleRoundTrip) {
        // ...
        return true;
    }
}
```

## Running tests

Tests are built as a separate target and run via:

```sh
cmake --build . --target hoo_tests && ./hoo_tests
```

## Test utilities

`src/tests/utils.h` provides helpers such as:

- `compileString(src)` — Compile a string of Hoo source and return the `HOModule`.
- `executeModule(mod)` — JIT-compile and run the module, returning exit code.
- `expectError(code)` — Assert that compilation fails with a specific error code.
