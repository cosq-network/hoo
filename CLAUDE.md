# Hooc Project Guide for Claude

This document provides a comprehensive guide for AI assistants (like Claude) working on the Hooc compiler project. It covers the architecture, conventions, and best practices for contributing to the codebase.

## Project Overview

Hooc is a modern, statically-typed programming language compiler that compiles to HVM bytecode with JIT execution via LLVM ORC JIT. The project is written in C++17 and uses ANTLR4 for parsing.

**Key Technologies:**
- **ANTLR4**: Parser generation (grammar in `src/parsing/Hooc.g4`)
- **LLVM**: ORC JIT backend for HVM bytecode execution
- **HVM**: Homebrew Virtual Machine — custom instruction set, module format, and bytecode
- **C++17**: Implementation language
- **GoogleTest**: Testing framework
- **CMake**: Build system

## Architecture Overview

### Compilation Pipeline

```
Source Code (.hoo)
    ↓
[ANTLR4 Lexer/Parser] → Parse Tree
    ↓
[SimpleASTBuilder] → Abstract Syntax Tree (AST)
    ↓
[HVMCodeGenerator] → HVM Bytecode
    ↓
[HVMJIT / LLVM ORC JIT] → Machine Code (JIT)
    ↓  (or)
[HOModule] → .ho Binary Module (AOT)
```

### Core Components

1. **HooCompiler** (`src/core/HooCompiler.h/cpp`)
   - Main compiler interface
   - Orchestrates the entire compilation pipeline
   - Entry point: `compile(moduleName, sourceCode)`

2. **ProcessIsolatedParser** (`src/parsing/ProcessIsolatedParser.h/cpp`)
   - Wraps ANTLR4 parser to isolate parsing errors
   - Converts source code to ANTLR4 parse tree

3. **SimpleASTBuilder** (`src/ast/SimpleASTBuilder.h/cpp`)
   - Visitor pattern implementation for ANTLR4 parse tree
   - Converts parse tree to typed AST nodes

4. **HVMCodeGenerator** (`src/codegen/HVMCodeGenerator.h/cpp`)
   - Generates HVM bytecode from AST
   - Handles type checking, name mangling, and code emission

5. **HVMJIT** (`src/hvm/HVMJIT.h/cpp`)
   - JIT compiles HVM bytecode to machine code via LLVM ORC JIT
   - Handles module loading, linking, and symbol resolution

6. **HOModule / HVMModuleBundle** (`src/hvm/`)
   - `.ho` binary module format (serialization/deserialization)
   - Module bundle management with dependency resolution

7. **ModuleSystem** (`src/modules/ModuleSystem.h/cpp`)
   - Manages module imports and exports
   - Resolves qualified names (e.g., `hoo.String`)
   - Provides standard library integration

8. **Runtime Library** (`src/runtime/lib/`)
   - **hoo_runtime.c**: Reference counting and memory management
   - **hoo_string.cpp**: UTF-8 string implementation
   - **hoo_generic_array.cpp**: Generic array implementation
   - **hoo_io.c**: I/O operations
   - **hoo_math.c**: Math functions
   - **Runtime Registry**: Dynamic class registration for FFI

### AST Structure

All AST nodes inherit from `ASTNode` and are defined in `src/ast/`:

```
ASTNode (base)
├── Declaration
│   ├── FunctionDeclaration
│   ├── ClassDeclaration
│   └── VariableDeclaration
├── Statement
│   ├── Block
│   ├── IfStatement
│   ├── WhileStatement
│   ├── ForStatement
│   ├── ReturnStatement
│   └── ExpressionStatement
├── Expression
│   ├── BinaryExpression
│   ├── UnaryExpression
│   ├── CallExpression
│   ├── MemberAccessExpression
│   ├── NewExpression
│   └── Primary (literals, identifiers)
└── Type
    ├── PrimitiveType
    ├── ArrayType
    └── OptionalType
```

## Coding Conventions

### File Organization

- Header files: `.h` extension
- Implementation files: `.cpp` extension
- Test files: `*Test.cpp` in `tests/` directory
- Runtime files: `src/runtime/lib/`

### Naming Conventions

- **Classes**: PascalCase (e.g., `HooCompiler`, `HVMCodeGenerator`)
- **Functions/Methods**: camelCase (e.g., `generateCode`, `buildAST`)
- **Variables**: camelCase with trailing underscore for private members (e.g., `context_`, `lastError_`)
- **Constants**: UPPER_SNAKE_CASE
- **Namespaces**: lowercase (e.g., `hooc`, `hooc::ast`)

### Memory Management

- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` sparingly
- Runtime objects use manual reference counting via `hoo_retain`/`hoo_release`
- LLVM objects follow LLVM ownership conventions (see LLVM documentation)

### Error Handling

- Compilation errors stored in `HooCompiler::lastError_`
- Return `nullptr` for failed operations
- Use GoogleTest assertions (`ASSERT_*`, `EXPECT_*`) in tests
- Runtime errors should be descriptive

## Working with ANTLR4

### Grammar File Location

`src/parsing/Hooc.g4` - Main grammar definition

### Regenerating Parser

```bash
cmake --build build --target clean_generated
cmake --build build --target generate_parser
cmake --build build
```

ANTLR4-generated sources are now placed under `build/<preset>/generated/antlr4/` (not committed to the repository).

### Adding New Syntax

1. Update `src/parsing/Hooc.g4` with new grammar rules
2. Regenerate parser
3. Add AST node type in `src/ast/`
4. Implement visitor method in `SimpleASTBuilder`
5. Add code generation in `HVMCodeGenerator`
6. Add comprehensive tests

### Parser Rules Naming

- Lexer rules: UPPERCASE (e.g., `FUNC`, `IDENTIFIER`)
- Parser rules: camelCase (e.g., `functionDeclaration`, `expression`)

## Working with HVM

### HVM Bytecode

The HVM is a custom 32-bit RISC instruction set. See `docs/hvm/hvm-spec.md` and `docs/hvm/instructions.md` for the full ISA reference.

Key properties:
- 32 GPRs (`r0`–`r31`), `r1` is return value, `r29` is link register
- Little-endian, 64-bit register machine
- Instruction formats: I-type, B-type, J-type, plus escape prefix for extended opcodes
- Symbol relocation via `SymbolFixup` for forward/recursive calls

### Name Mangling

Member functions: `ClassName_methodName`  
See `src/core/SymbolMangler.h` for full mangling/demangling scheme.

### Code Generation Patterns

**Variable Declaration:**
```cpp
// HVM: generate a store from a register to a frame offset
codegen.emitStore(reg, frameOffset);
```

**Function Call:**
```cpp
// HVM: CALL (immediate target) or CALLI (indirect)
std::vector<uint8_t> regs = {argReg1, argReg2};
codegen.emitCall(targetLabel, regs);
```

**Reference Counting:**
```cpp
// HVM: SYSCALL_RETAIN / SYSCALL_RELEASE via the SYSCALL instruction
codegen.emitSyscall(SyscallOp::RETAIN_FIELD, {objReg});
codegen.emitSyscall(SyscallOp::RELEASE, {objReg});
```

## Testing Guidelines

### Test Structure

All tests follow GoogleTest conventions:

```cpp
#include <gtest/gtest.h>
#include "../src/HooCompiler.h"

class MyFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler_ = std::make_unique<HooCompiler>();
    }

    std::unique_ptr<HooCompiler> compiler_;
};

TEST_F(MyFeatureTest, TestCase) {
    std::string code = R"(
        func test() -> int64 {
            return 42;
        }
    )";

    auto module = compiler_->compile("test", code);
    ASSERT_NE(module, nullptr);
    // More assertions...
}
```

### Test Categories

- **Parsing tests**: Verify correct parse tree construction
- **AST tests**: Verify AST building from parse tree
- **Code generation tests**: Verify HVM bytecode generation
- **Integration tests**: End-to-end compilation and execution
- **Runtime tests**: Test runtime library functions

### Running Tests

```bash
cd build/<preset>
./hoo-tests                    # Run all tests
./hoo-tests --gtest_filter=ClassName.TestName  # Run specific test
ctest --preset <preset>        # Run via CTest
```

## Common Tasks

### Adding a New Language Feature

1. **Update Grammar** (`src/parsing/Hooc.g4`)
   ```antlr
   myNewStatement: MY_KEYWORD expression SEMICOLON;
   ```

2. **Regenerate Parser** (`cmake --build build --target generate_parser`)

3. **Add AST Node** (`src/ast/MyNewStatement.h`)
   ```cpp
   class MyNewStatement : public Statement {
   public:
       MyNewStatement(std::unique_ptr<Expression> expr);
       // Accessors...
   };
   ```

4. **Update AST Builder** (`src/ast/SimpleASTBuilder.cpp`)
   ```cpp
   std::any SimpleASTBuilder::visitMyNewStatement(
       HoocParser::MyNewStatementContext* ctx) {
       // Build AST node from parse tree
   }
   ```

5. **Add Code Generation** (`src/codegen/HVMCodeGenerator.cpp`)
   ```cpp
   void HVMCodeGenerator::generateMyNewStatement(
       const MyNewStatement* stmt) {
       // Generate HVM bytecode
   }
   ```

6. **Add Tests** (`tests/MyNewStatementTest.cpp`)
   ```cpp
   TEST_F(MyFeatureTest, BasicMyNewStatement) {
       // Test cases
   }
   ```

### Adding Runtime Functions

1. Declare in header (`src/runtime/lib/hoo_myfeature.h`)
2. Implement in C/C++ (`src/runtime/lib/hoo_myfeature.c` or `.cpp`)
3. Register with HVMJIT for symbol export
4. Add to `src/runtime/lib/CMakeLists.txt` if new file

### Debugging Tips

**Print HVM bytecode:**
```cpp
module->printAssembly();
```

**Memory debugging:**
```cpp
// In Debug builds, enable memory tracking
hoo_print_memory_stats();
```

**AST debugging:**
```cpp
// Add debug output in visitor methods
std::cout << "Visiting node: " << typeid(*node).name() << std::endl;
```

## Current Implementation Status

### Fully Implemented
- ✅ Basic types (int64, double, bool, char, string, void)
- ✅ Variables and assignments
- ✅ Arithmetic and logical operations
- ✅ Functions with parameters and return values
- ✅ If/else statements
- ✅ While loops
- ✅ Classes with constructors
- ✅ Object creation (new expressions)
- ✅ Member access and method calls
- ✅ Arrays
- ✅ Nullable types
- ✅ Reference counting (ARC)
- ✅ String library
- ✅ Module system basics
- ✅ Exception handling (try-catch-finally, throw, rethrow)

### Partially Implemented
- 🟡 For loops (range syntax needs work)
- 🟡 Import statements (parsing done, full resolution WIP)

### Not Yet Implemented

## Important Notes for AI Assistants

### When Adding Features

1. **Always add tests first** - Write failing tests, then implement
2. **Follow existing patterns** - Study similar features before implementing
3. **Update all layers** - Grammar → AST → Code generation → Tests
4. **Check memory management** - Ensure proper retain/release for objects
5. **Verify HVM bytecode** - Print and inspect generated bytecode for correctness
6. **Check ANTLR4 version compatibility** - The project targets ANTLR 4.13.2; regenerated sources must match

### Build Targets

The project defines these primary CMake targets:

| Target | Type | Description |
|--------|------|-------------|
| `hooc` | Executable | CLI entry point (`src/core/main.cpp`) |
| `hoo-core` | Static library | Core compiler + HVM + JIT (merged from `hvm` + `hoo-compiler`) |
| `hoo-parser` | Static library | ANTLR4-generated parser |
| `hoort` | Static/shared library | Runtime library (C/C++, no LLVM dependency) |
| `hoo-tests` | Executable | Unit tests (GoogleTest, when `HOOC_BUILD_TESTS=ON`) |

See `docs/build-targets.md` for the full target reference.

### When Refactoring

1. **Run tests frequently** - Ensure no regressions
2. **Maintain API compatibility** - Don't break existing tests
3. **Update documentation** - Keep this file and others in sync
4. **Use small commits** - Incremental changes are easier to debug

### Common Pitfalls

- **LLVM ownership**: Don't delete LLVM objects manually (used internally by HVMJIT)
- **HVM register pressure**: Ensure register allocation doesn't exceed r0..r31
- **Type checking**: Always verify types before code generation
- **Memory leaks**: Test with memory debugging enabled

## Resources

- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual.html)
- [ANTLR4 Documentation](https://github.com/antlr/antlr4/blob/master/doc/index.md)
- [GoogleTest Primer](https://google.github.io/googletest/primer.html)
- HVM Spec: `docs/hvm/hvm-spec.md`
- Grammar: `src/parsing/Hooc.g4`
- Tests: `tests/` directory (comprehensive examples)

## Questions?

When working on this project:
1. Read existing tests to understand feature behavior
2. Check the grammar file for syntax rules
3. Look at similar features for implementation patterns
4. Run tests after every change
5. Ask for clarification when needed

This is a living document. Update it as the project evolves.
