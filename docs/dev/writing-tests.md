# How to Write New Unit Tests

The project uses **Google Test** (gtest) as the test framework. Tests are located in `tests/` with subdirectories mirroring `src/`:

```
tests/
  ast/               — AST node tests
  codegen/           — Bytecode generation tests (HVMCodeGeneratorTest.cpp, ...ComprehensiveTest.cpp)
  core/              — Core tests (SymbolManglerTest.cpp, HooCompilerTest.cpp, HooCLITest.cpp)
  examples/          — End-to-end example tests
  hvm/               — HVM runtime tests
  jit/               — JIT execution tests (28 test files: math, string, array, class, etc.)
  parsing/           — Parser & AST builder tests (26 test files)
  repl/              — REPL tests
  runtime/           — Runtime library tests
  test_main.cpp      — Main entry (LLVM init + gtest runner)
```

## Test framework setup

`tests/test_main.cpp` initializes LLVM targets before running tests:

```cpp
int main(int argc, char **argv) {
    if (llvm::InitializeNativeTarget()) {
        std::cerr << "ERROR: Failed to initialize native target" << std::endl;
    }
    if (llvm::InitializeNativeTargetAsmPrinter()) { ... }
    if (llvm::InitializeNativeTargetAsmParser()) { ... }
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    std::quick_exit(result);
}
```

## Adding a test to an existing suite

### SymbolMangler tests (gtest class style)

```cpp
#include <gtest/gtest.h>
#include "src/core/SymbolMangler.h"

class SymbolManglerTest : public ::testing::Test {
protected:
    MangledFunctionParams makeParams() {
        MangledFunctionParams p;
        p.className = ""; p.functionName = "";
        p.returnType = ""; p.parameterTypes = {};
        p.isConstructor = false; p.isDestructor = false;
        p.isStatic = false; p.isVirtual = false;
        return p;
    }
};

TEST_F(SymbolManglerTest, SimpleFunctionMangling) {
    auto params = makeParams();
    params.functionName = "foo";
    params.returnType = "int64";
    params.parameterTypes = {"int64"};
    std::string mangled = SymbolMangler::mangleFunctionName(params);
    EXPECT_EQ(mangled, "_F_foo_i8_i8");
}
```

### Codegen tests (compile + inspect bytecode)

```cpp
#include <gtest/gtest.h>
#include "core/HooCompiler.h"
#include "codegen/HVMCodeGenerator.h"
#include "hvm/HOModule.h"

class HVMCodeGeneratorTest : public ::testing::Test {
protected:
    std::unique_ptr<HooCompiler> compiler_;
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }
};

TEST_F(HVMCodeGeneratorTest, CompileSimpleFunction) {
    std::string code = R"(
        func:int64 add(a: int64, b: int64) {
            return a + b;
        }
    )";
    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    auto insts = module->decodeInstructions(
        module->getSection(".text")->data);
    ASSERT_GE(insts.size(), 4);
}
```

### JIT execution tests (full pipeline)

Files in `tests/jit/` test the full compile→JIT→execute pipeline:

```cpp
// Pattern used in tests/jit/HooMathJitTest.cpp, HooStringJitTest.cpp, etc.
// (Each JIT test file compiles Hoo source, JIT-compiles it, calls the function,
//  and asserts return values.)
```

## Creating a new test suite

1. Create a new `.cpp` file in the appropriate `tests/` subdirectory.
2. Include the necessary headers.
3. Define a test fixture or use free-standing `TEST()` macros.
4. Add the file to `CMakeLists.txt` in the tests directory.
5. Rebuild and run.

## Testing the full pipeline

```cpp
TEST(CompileAndRun) {
    auto module = compiler_->compile("test", R"(
        func:int64 main() {
            return 42;
        }
    )");
    ASSERT_NE(module, nullptr);
    
    // Option 1: Decode and inspect bytecode
    auto insts = module->decodeInstructions(
        module->getSection(".text")->data);
    
    // Option 2: JIT-compile and execute (see JIT tests)
    // (HVMJIT lifecycle tests show this pattern)
}
```

## JIT test pattern

JIT tests (in `tests/jit/`) typically:
1. Compile Hoo source via `HooCompiler`
2. Set up an `HVMJIT` instance
3. Load the compiled module
4. Look up a function symbol
5. Call it via the JIT trampoline
6. Assert return values

```cpp
// Approximate pattern from HVMJITLifecycleTest.cpp / HooMathJitTest.cpp
TEST_F(HooMathJitTest, Sqrt) {
    auto module = compileModule(R"(
        func:double sqrtTest(v: double) {
            return Math.sqrt(v);
        }
    )");
    jit_->loadModule(*module);
    auto func = jit_->findFunction("sqrtTest");
    double result = func.call<double(double)>(4.0);
    EXPECT_DOUBLE_EQ(result, 2.0);
}
```

## Best practices

- **One assertion per test** where possible — makes failures easier to diagnose.
- **Arrange / Act / Assert** — Keep the structure clear.
- **Test error paths** — Compile invalid code and check `hasErrors()` or inspect error messages.
- **Round-trip tests** — Mangle then demangle; serialize then deserialize; compile + decode.
- **Edge cases** — Empty strings, large values, deeply nested types, unicode, special characters.
- **Use raw string literals** (`R"(...)"`) for multi-line Hoo source code.
- **Fixture `SetUp()`** — Use `SetUp()` to initialize shared resources (compiler, JIT, parser).
- **Unique mangling verification** — When testing overloads, assert `EXPECT_NE(m1, m2)` to confirm distinct mangled names.
- **Whitespace independence** — Test that mangled types are identical with and without whitespace (verified via `mangleType`).
- **Invalid input handling** — Test that malformed inputs produce expected fallbacks (e.g., `demangleType("M")` returns `"unknown"`).
