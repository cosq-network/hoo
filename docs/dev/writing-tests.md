# How To Write New Unit Tests

The project uses **Google Test** (`gtest`) for unit tests. The C++ test binary is `hoo-tests`, built from the sources listed in `CMakeLists.txt` under `tests/`.

## Test layout

The test tree is organized by subsystem, not by `src/` mirroring:

```text
tests/
  ast/        - AST node tests
  codegen/    - Bytecode generation tests
  core/       - Compiler, CLI, and symbol mangling tests
  hvm/        - HVM and module format tests
  jit/        - End-to-end JIT tests
  parsing/    - Parser and AST builder tests
  repl/       - REPL tests
  runtime/    - Runtime library tests
  test_main.cpp
```

`tests/examples/` contains standalone example source files. They are useful as fixtures or manual smoke tests, but they are not compiled into `hoo-tests`.

## Test entry point

`tests/test_main.cpp` is the single entry point for the test binary. It initializes LLVM before running the suite:

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

That setup matters for any test that touches `HVMJIT`, code generation, or LLVM-backed loading.

## Build and run

Configure tests with:

```sh
cmake -DHOO_BUILD_TESTS=ON -S . -B build
cmake --build build --target hoo-tests
ctest --test-dir build --output-on-failure
```

You can also run the binary directly:

```sh
./build/hoo-tests
```

## Common test patterns

### Parser tests

Parser tests usually exercise `HooParserWrapper` and verify that the parse tree is non-null and error-free.

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;

class HooParserWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<HooParserWrapper>();
    }

    std::unique_ptr<HooParserWrapper> parser;
};

TEST_F(HooParserWrapperTest, ParseValidFunction) {
    auto *parseTree = parser->parseForAST("func test() { return; }");
    ASSERT_NE(parseTree, nullptr);
    EXPECT_TRUE(parser->getLastError().empty());
}
```

If you need to inspect expressions rather than full compilation units, use `parseExpression()` instead of `parseForAST()`.

### AST builder tests

`SimpleASTBuilder` consumes `HoocParser::CompilationUnitContext` and produces AST nodes:

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "src/ast/SimpleASTBuilder.h"
#include "src/parsing/HooParserWrapper.h"
#include "HoocParser.h"

using namespace hooc;
using namespace hooc::ast;

TEST(SimpleASTBuilder, BuildSingleFunctionDeclaration) {
    HooParserWrapper parser;
    SimpleASTBuilder builder;

    auto *parseTree = parser.parseForAST("func test() { return; }");
    ASSERT_NE(parseTree, nullptr);

    auto *ctx = dynamic_cast<HoocParser::CompilationUnitContext *>(parseTree);
    ASSERT_NE(ctx, nullptr);

    auto ast = builder.buildAST(ctx);
    ASSERT_NE(ast, nullptr);
    EXPECT_NE(ast->toString().find("CompilationUnit"), std::string::npos);
}
```

That is the same shape used by `tests/parsing/SimpleASTBuilderTest.cpp`.

### Code generation tests

Codegen tests typically compile Hoo source through `HooCompiler`, then inspect the resulting module or encoded instructions.

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "src/core/HooCompiler.h"

using namespace hooc;

class HVMCodeGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler_;
};

TEST_F(HVMCodeGeneratorTest, CompileSimpleFunction) {
    auto module = compiler_->compile("test", R"(
        func:int64 add(a: int64, b: int64) {
            return a + b;
        }
    )");

    ASSERT_NE(module, nullptr);
    ASSERT_NE(module->getSection(".text"), nullptr);
}
```

### JIT tests

JIT tests use `HVMJIT` with a `DefaultIOProvider` and usually load source code directly:

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "src/core/DefaultIOProvider.h"
#include "src/hvm/HVMJIT.h"

using namespace hooc;

TEST(HVMJITLifecycleTest, LoadSourceCodeThenDestroy) {
    auto io = std::make_unique<DefaultIOProvider>();
    auto jit = std::make_unique<HVMJIT>(*io);

    ASSERT_TRUE(jit->loadSourceCode("test", "func :int64 test() { return 42; }"))
        << jit->getLastError();
}
```

The full JIT suites in `tests/jit/` follow the same pattern, but extend it with function lookup and execution checks.

## Adding a new test

1. Put the new test file under the appropriate `tests/<area>/` directory.
2. Use `gtest` macros such as `TEST()` or `TEST_F()`.
3. Include headers from `src/` directly, plus any generated parser headers such as `HoocParser.h` when needed.
4. Add the file to the `hoo-tests` source list in `CMakeLists.txt`.
5. Reconfigure and rebuild `hoo-tests`.

## Writing good tests

- Prefer one behavior per test when possible.
- Use raw string literals for multi-line Hoo source.
- Check both success and failure paths.
- Keep parser tests focused on the parse tree, AST tests focused on AST shape, and JIT tests focused on runtime behavior.
- For mangling and demangling, verify round trips and collisions, not only a single expected string.
- Avoid relying on brittle formatting unless the formatting is the thing under test.
