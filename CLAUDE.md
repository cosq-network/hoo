# Hooc Project Guide for Claude

This document provides a comprehensive guide for AI assistants (like Claude) working on the Hooc compiler project. It covers the architecture, conventions, and best practices for contributing to the codebase.

## Project Overview

Hooc is a modern, statically-typed programming language compiler that compiles to LLVM IR and provides JIT execution. The project is written in C++17 and uses ANTLR4 for parsing.

**Key Technologies:**
- **ANTLR4**: Parser generation (grammar in `src/Hooc.g4`)
- **LLVM**: Code generation, optimization, and JIT execution
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
[LLVMCodeGenerator] → LLVM IR Module
    ↓
[LLVM Optimizer] → Optimized IR
    ↓
[LLVM OrcJIT] → Machine Code (JIT)
```

### Core Components

1. **HooCompiler** (`src/HooCompiler.h/cpp`)
   - Main compiler interface
   - Orchestrates the entire compilation pipeline
   - Entry point: `compile(moduleName, sourceCode)`

2. **ProcessIsolatedParser** (`src/ProcessIsolatedParser.h/cpp`)
   - Wraps ANTLR4 parser to isolate parsing errors
   - Converts source code to ANTLR4 parse tree

3. **SimpleASTBuilder** (`src/SimpleASTBuilder.h/cpp`)
   - Visitor pattern implementation for ANTLR4 parse tree
   - Converts parse tree to typed AST nodes
   - Located in `src/ast/` directory

4. **LLVMCodeGenerator** (`src/LLVMCodeGenerator.h/cpp`)
   - Generates LLVM IR from AST
   - Handles type checking, name mangling, and code emission

5. **ModuleSystem** (`src/ModuleSystem.h/cpp`)
   - Manages module imports and exports
   - Resolves qualified names (e.g., `hoo.String`)
   - Provides standard library integration

6. **Runtime Library** (`src/rt/`)
   - **hoo_runtime.c**: Reference counting and memory management
   - **hoo_string.cpp**: UTF-8 string implementation
   - **hoo_generic_array.cpp**: Generic array using `std::any`
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
- Runtime files: `src/rt/` (C files use `.c`, C++ use `.cpp`)

### Naming Conventions

- **Classes**: PascalCase (e.g., `HooCompiler`, `LLVMCodeGenerator`)
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

`src/Hooc.g4` - Main grammar definition

### Regenerating Parser

```bash
cd build
make clean_generated  # Clean old files
make generate_parser  # Generate new parser files
make                  # Rebuild project
```

### Adding New Syntax

1. Update `src/Hooc.g4` with new grammar rules
2. Regenerate parser
3. Add AST node type in `src/ast/`
4. Implement visitor method in `SimpleASTBuilder`
5. Add code generation in `LLVMCodeGenerator`
6. Add comprehensive tests

### Parser Rules Naming

- Lexer rules: UPPERCASE (e.g., `FUNC`, `IDENTIFIER`)
- Parser rules: camelCase (e.g., `functionDeclaration`, `expression`)

## Working with LLVM

### Context and Module Management

- Each compilation creates a fresh `LLVMContext`
- Each source file generates one `llvm::Module`
- Functions and types are registered in the module

### Type Mapping

Hooc Type → LLVM Type:
- `int64` → `i64`
- `double` → `double`
- `bool` → `i1`
- `char` → `i8`
- `string` → `i8*` (pointer to HooString)
- `T[]` → `i8*` (pointer to HooArray)
- `T?` → `i8*` (pointer, null allowed)
- Classes → `i8*` (opaque pointer to object)

### Name Mangling

- Member functions: `ClassName_methodName`

### Code Generation Patterns

**Variable Declaration:**
```cpp
llvm::AllocaInst* alloca = builder->CreateAlloca(type, nullptr, varName);
llvm::Value* initValue = /* generate initializer */;
builder->CreateStore(initValue, alloca);
```

**Function Call:**
```cpp
std::vector<llvm::Value*> args = { /* arguments */ };
llvm::Function* func = module->getFunction(funcName);
llvm::Value* result = builder->CreateCall(func, args);
```

**Reference Counting:**
```cpp
// Retain: increment refcount
llvm::Function* retainFunc = module->getFunction("hoo_retain");
builder->CreateCall(retainFunc, {objPtr});

// Release: decrement refcount
llvm::Function* releaseFunc = module->getFunction("hoo_release");
builder->CreateCall(releaseFunc, {objPtr});
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
- **Code generation tests**: Verify LLVM IR generation
- **Integration tests**: End-to-end compilation and execution
- **Runtime tests**: Test runtime library functions

### Running Tests

```bash
cd build
./hoo-tests                    # Run all tests
./hoo-tests --gtest_filter=ClassName.TestName  # Run specific test
ctest --verbose                # Run via CTest
```

## Common Tasks

### Adding a New Language Feature

1. **Update Grammar** (`src/Hooc.g4`)
   ```antlr
   myNewStatement: MY_KEYWORD expression SEMICOLON;
   ```

2. **Add AST Node** (`src/ast/MyNewStatement.h`)
   ```cpp
   class MyNewStatement : public Statement {
   public:
       MyNewStatement(std::unique_ptr<Expression> expr);
       // Accessors...
   };
   ```

3. **Update AST Builder** (`src/SimpleASTBuilder.cpp`)
   ```cpp
   std::any SimpleASTBuilder::visitMyNewStatement(
       HoocParser::MyNewStatementContext* ctx) {
       // Build AST node from parse tree
   }
   ```

4. **Add Code Generation** (`src/LLVMCodeGenerator.cpp`)
   ```cpp
   llvm::Value* LLVMCodeGenerator::generateMyNewStatement(
       const MyNewStatement* stmt) {
       // Generate LLVM IR
   }
   ```

5. **Add Tests** (`tests/MyNewStatementTest.cpp`)
   ```cpp
   TEST_F(MyFeatureTest, BasicMyNewStatement) {
       // Test cases
   }
   ```

### Adding Runtime Functions

1. Declare in header (`src/rt/hoo_myfeature.h`)
2. Implement in C/C++ (`src/rt/hoo_myfeature.cpp`)
3. Register with LLVM in code generator
4. Add to CMakeLists.txt if new file

### Debugging Tips

**Print LLVM IR:**
```cpp
module->print(llvm::errs(), nullptr);
```

**Check function exists:**
```cpp
llvm::Function* func = module->getFunction("functionName");
assert(func != nullptr && "Function not found");
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
5. **Verify LLVM IR** - Print and inspect generated IR for correctness

### When Fixing Bugs

1. **Add regression test** - Create test that reproduces the bug
2. **Identify the layer** - Is it parsing, AST building, or code generation?
3. **Check ANTLR4 output** - Use ANTLR4 visualizer if needed
4. **Verify LLVM IR** - Ensure generated IR is valid

### When Refactoring

1. **Run tests frequently** - Ensure no regressions
2. **Maintain API compatibility** - Don't break existing tests
3. **Update documentation** - Keep this file and others in sync
4. **Use small commits** - Incremental changes are easier to debug

### Common Pitfalls

- **LLVM ownership**: Don't delete LLVM objects manually
- **String literals**: Use `builder->CreateGlobalStringPtr()` for constants
- **Type checking**: Always verify types before code generation
- **Memory leaks**: Test with memory debugging enabled

## Resources

- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual.html)
- [ANTLR4 Documentation](https://github.com/antlr/antlr4/blob/master/doc/index.md)
- [GoogleTest Primer](https://google.github.io/googletest/primer.html)
- Grammar: `src/Hooc.g4`
- Tests: `tests/` directory (comprehensive examples)

## Questions?

When working on this project:
1. Read existing tests to understand feature behavior
2. Check the grammar file for syntax rules
3. Look at similar features for implementation patterns
4. Run tests after every change
5. Ask for clarification when needed

This is a living document. Update it as the project evolves.
