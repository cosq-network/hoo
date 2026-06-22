# How Unit Tests Are Integrated

Unit tests are built as a normal CMake target, not through a custom in-tree test framework.

## Test target

When `HOO_BUILD_TESTS=ON`, the build creates the `hoo-tests` executable.

The target is assembled from:

- `tests/test_main.cpp` as the entry point
- C++ test files under `tests/ast/`, `tests/codegen/`, `tests/core/`, `tests/hvm/`, `tests/jit/`, `tests/parsing/`, `tests/repl/`, and `tests/runtime/`
- Google Test, linked as `GTest::gtest` and `GTest::gtest_main`
- Project libraries such as `hoo-core` and `hoorepl`

`tests/examples/` is kept as standalone example source and is not part of the `hoo-tests` binary.

## Main function

`tests/test_main.cpp` is responsible for global initialization before any test runs:

```cpp
int main(int argc, char **argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    std::quick_exit(result);
}
```

LLVM initialization is required because the JIT and codegen tests create LLVM-backed objects during test execution.

## Include paths available to tests

`hoo-tests` is configured with these include directories:

- repository root
- `src/`
- LLVM include directories
- ANTLR runtime include directories
- `${CMAKE_BINARY_DIR}/generated/antlr4`

That is why tests can include both project headers such as `src/parsing/HooParserWrapper.h` and generated headers such as `HoocParser.h`.

## Parser generation dependency

The test target depends on the parser generation step indirectly through the main build. The generated C++ ANTLR sources live under:

```text
${CMAKE_BINARY_DIR}/generated/antlr4
```

The generated parser files are produced from `src/parsing/Hooc.g4` and include:

- `HoocLexer.cpp` / `HoocLexer.h`
- `HoocParser.cpp` / `HoocParser.h`
- `HoocVisitor.cpp` / `HoocVisitor.h`
- `HoocBaseVisitor.cpp` / `HoocBaseVisitor.h`
- `HoocListener.cpp` / `HoocListener.h`
- `HoocBaseListener.cpp` / `HoocBaseListener.h`

## Running the suite

The recommended flow is:

```sh
cmake -DHOO_BUILD_TESTS=ON -S . -B build
cmake --build build --target hoo-tests
ctest --test-dir build --output-on-failure
```

The `ctest` invocation runs the `HooUnitTests` CTest entry that executes `hoo-tests`.

## What the test files cover

The suite is organized by responsibility:

- parser and AST shape checks in `tests/parsing/`
- code generation checks in `tests/codegen/`
- runtime library checks in `tests/runtime/`
- JIT lifecycle and execution checks in `tests/jit/`
- compiler, CLI, and mangling checks in `tests/core/`
- module and instruction checks in `tests/hvm/`
- AST node behavior in `tests/ast/`
- REPL behavior in `tests/repl/`

## Writing new integrated tests

To add a new test file, place it in the appropriate directory and add it to the `hoo-tests` source list in `CMakeLists.txt`. The usual pattern is:

1. Use `gtest` macros.
2. Include only the headers you need.
3. Keep the test focused on one layer of the pipeline.
4. If the test needs a parser context, use `HooParserWrapper`.
5. If the test needs LLVM, make sure it runs under `tests/test_main.cpp` and does not duplicate initialization.

## Notes on platform setup

The test target has some platform-specific linking and runtime setup, especially on Windows, where the build exports runtime symbols needed by the executable. That is handled in `CMakeLists.txt`; test authors normally do not need to touch it unless they add a new dependency that changes the link surface.
