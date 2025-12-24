# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**hooc** is a compiler for the **hoo** programming language - a modern, safe, and expressive language that evolves C's philosophy while adding memory safety, strong static typing, and modern abstractions. The compiler translates `.hoo` source files into executable code via LLVM.

## Build Commands

### Standard Build

**macOS/Linux:**
```bash
# Configure and build
cmake -B build && cmake --build build

# Clean and rebuild
cmake --build build --target clean_generated
cmake -B build && cmake --build build
```

**Windows (CRITICAL):**
```bash
# Must specify build type to avoid runtime library mismatch with LLVM
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo

# Clean and rebuild
cmake --build build --target clean_generated
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

**Why RelWithDebInfo on Windows?** LLVM libraries are built with Release runtime (`/MD`). Without explicit configuration, CMake defaults to Debug mode which uses Debug runtime (`/MDd`), causing linker errors with 100+ runtime library mismatches. `RelWithDebInfo` provides debug symbols while using the correct Release runtime.

### Running Tests
```bash
# Run all unit tests (67 tests)
./build/hoo_tests        # macOS/Linux
./build/hoo_tests.exe    # Windows

# Run specific test suite with verbose output
./build/hoo_tests --gtest_filter="CodeGeneratorTest.*"

# Run tests through CMake
cmake --build build --target run_tests
```

**Windows Note:** If you get exit code 127, ensure LLVM and ANTLR4 DLLs are in your PATH or located in the `build/` directory.

### Compiling hoo Programs
```bash
# Compile and execute a .hoo file
./build/hooc example.hoo          # macOS/Linux
./build/hooc.exe example.hoo      # Windows

# Try example programs
./build/hooc tests/examples/arithmetic.hoo
./build/hooc tests/examples/for_loops.hoo
./build/hooc tests/examples/array_example.hoo
```

### Grammar Regeneration
```bash
# After modifying src/Hooc.g4
cmake --build build --target generate_parser
cmake --build build
```

## Architecture Overview

### Compilation Pipeline

The compiler follows a multi-stage pipeline:

```
Source Code (.hoo)
    ↓
ProcessIsolatedParser (ANTLR4)
    ↓
Parse Tree Context
    ↓
SimpleASTBuilder
    ↓
AST (CompilationUnit)
    ↓
CodeGenerator
    ↓
LLVM IR Module
    ↓
LLVM ORC JIT
    ↓
Native Execution
```

### Key Components

**HooCompiler** (`src/HooCompiler.{h,cpp}`)
- High-level compilation interface that orchestrates the entire pipeline
- Owns and coordinates Parser → AST Builder → Code Generator
- Entry point: `compile(moduleName, sourceCode)` returns LLVM Module

**ProcessIsolatedParser** (`src/ProcessIsolatedParser.{h,cpp}`)
- Wraps ANTLR4-generated parser (from `src/Hooc.g4`)
- Provides two modes:
  - `parse()`: Process-isolated validation (avoids ANTLR4 global state corruption)
  - `parseForAST()`: Direct parsing that returns parse tree context for AST building
- Generated parser files live in `antlr4/generated/`

**SimpleASTBuilder** (`src/SimpleASTBuilder.{h,cpp}`)
- Converts ANTLR4 parse tree into type-safe AST
- Entry point: `buildAST(CompilationUnitContext*)` returns `CompilationUnit`
- Recursively builds AST nodes from parse tree contexts
- Handles all language constructs: declarations, types, statements, expressions

**CodeGenerator** (`src/CodeGenerator.{h,cpp}`)
- Translates AST to LLVM IR
- Entry point: `generateModule(CompilationUnit&)` returns LLVM Module
- Maintains symbol tables for variables and functions (`namedValues_`, `functions_`)
- Key methods:
  - `generateFunction()`: Creates LLVM functions with parameters and body
  - `generateExpression()`: Dispatches to specific expression generators
  - `generateStatement()`: Handles control flow and variable declarations
  - `generateType()`: Converts hooc types to LLVM types

**AST Hierarchy** (`src/ast/`)
- Complete type-safe representation of hoo language constructs
- Key headers:
  - `AST.h`: Main include that pulls in all AST types
  - `CompilationUnit.h`: Root AST node (imports + declarations)
  - `Declaration.h`: Functions, variables, classes, interfaces
  - `Type.h`: Primitive types, arrays, unions, user-defined types
  - `Statement.h`: Control flow (if/while/for), blocks, returns
  - `Expression.h`: Binary ops, function calls, assignments, member access
  - `Primary.h`: Literals (int, float, bool, char, string), identifiers

### Type System

**Namespace Collision Handling**
- Critical: `llvm::Type` vs `hooc::ast::Type` naming collision
- Use namespace aliases in implementation files:
  ```cpp
  namespace LLVMType = llvm;
  using LLVMType::Type;
  ```
- Be explicit with `llvm::Type*` vs `ast::Type` in headers

**Primitive Types**
All primitive types are fully implemented with LLVM IR generation:
- `byte` → i8
- `int64` → i64
- `double` → double
- `bool` → i1
- `char` → i8
- Array types with multi-dimensional support

**Not Yet Implemented**
- `string` type (pending LLVM string support)
- Function calls between hooc functions (only external C functions work)
- Classes, interfaces, and member access
- Import/module system

## Testing Strategy

### Test Organization
- `tests/CodeGeneratorTest.cpp`: LLVM IR generation tests
- `tests/SimpleASTBuilderTest.cpp`: Parse tree → AST conversion tests
- `tests/HooCompilerTest.cpp`: End-to-end compilation tests
- `tests/ProcessIsolatedParserTest.cpp`: Parser validation tests
- `tests/examples/*.hoo`: Real hoo programs for integration testing

### Writing Tests
```cpp
// Standard test pattern
TEST_F(CodeGeneratorTest, TestName) {
    std::string code = R"(
        func test() -> int64 {
            return 42;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    auto module = codeGen->generateModule(*ast);
    ASSERT_NE(module, nullptr);

    // Verify IR
    auto* func = module->getFunction("test");
    ASSERT_NE(func, nullptr);
}
```

## Development Workflow

### Adding Language Features

1. **Update Grammar** (`src/Hooc.g4`)
   - Add lexer tokens if needed
   - Add parser rules with proper precedence
   - Regenerate: `cmake --build build --target generate_parser`

2. **Extend AST** (`src/ast/`)
   - Add new AST node classes to appropriate header
   - Implement in `src/ast/ASTImpl.cpp`
   - Follow existing patterns (unique_ptr ownership, visitor support)

3. **Update ASTBuilder** (`src/SimpleASTBuilder.{h,cpp}`)
   - Add `build*` methods for new parse tree contexts
   - Handle all context alternatives
   - Extract values from terminal nodes correctly

4. **Implement CodeGenerator** (`src/CodeGenerator.cpp`)
   - Add `generate*` methods for new AST nodes
   - Update dispatch methods (generateExpression/generateStatement)
   - Emit proper LLVM IR using `builder_`
   - Update symbol tables as needed

5. **Write Tests**
   - Add unit tests in appropriate test file
   - Add example `.hoo` programs in `tests/examples/`
   - Verify with `./build/hoo_tests`

## Array Literals (v0.2)

Arrays are created using literal syntax with type inference:

```hoo
// Type inference from elements
var numbers = [1, 2, 3, 4, 5];         // int64[]
var floats = [1.0, 2.5, 3.14];         // double[]
var flags = [true, false, true];       // bool[]

// Explicit type annotation
var data: int64[] = [10, 20, 30];

// Multi-dimensional arrays
var matrix = [[1, 2], [3, 4]];         // int64[][]

// Empty arrays require explicit type
var empty: int64[] = [];
```

**Function Parameters:**
Arrays are passed as slices (references) to functions:

```hoo
func process(arr: int64[]) -> void {
    // arr is a slice reference to int64 array
}

func main() -> void {
    var data = [1, 2, 3];
    process(data);
}
```

**Implementation Details:**
- Array literals are stored as global constants in LLVM IR (.rodata section)
- Type inference ensures all elements have the same type
- Memory-efficient: literal arrays live in read-only data section
- Element access via array indexing: `arr[index]`

**Important:** Fixed-size array type syntax (e.g., `var arr: int64[10]`) is no longer supported. Use array literals instead.

### Current Implementation Status

**✅ Fully Working:**
- All primitive types and literals
- Variable declarations with type inference
- Array literals with type inference (`[1, 2, 3]`)
- Multi-dimensional array literals (`[[1, 2], [3, 4]]`)
- Array slice types for function parameters (`int64[]`)
- Array element access (`arr[index]`)
- All arithmetic, comparison, logical operators
- Assignments and compound assignments
- If/else statements
- While loops
- For-range loops (`for i in 0..10`)
- For-in loops (`for item in collection`)
- Function declarations with parameters and return types
- Return statements
- Expression statements
- Blocks with scoping

**✅ Function Calls:**
- Hooc-to-hooc function calls fully working
- Argument passing and return values
- External C function calls (e.g., `printf`)

**❌ Not Implemented:**
- String type and operations
- Classes and interfaces
- Advanced function features (pointers, callbacks, method calls)
- Import/module system
- Standard library

**🚫 Removed (v0.2):**
- Fixed-size array type declarations (e.g., `var arr: int64[10]`) - Use array literals instead

### Common Issues

**ANTLR4 State Corruption**
- Use `parseForAST()` for AST building (required for accessing parse tree)
- The process-isolated `parse()` is only for validation
- Avoid calling parser multiple times in same process when possible

**LLVM Type Conversions**
- Always use `builder_->getInt64Ty()` style accessors, not `Type::getInt64Ty(context)`
- Check for null pointers before using LLVM values
- Use `createEntryBlockAlloca()` for variable storage

**Symbol Table Management**
- `namedValues_` maps variable names to LLVM AllocaInst pointers
- `functions_` maps function names to LLVM Function pointers
- Clear/manage scope properly for nested blocks

## Key Files Reference

| Path | Purpose |
|------|---------|
| `src/main.cpp` | CLI entry point for hooc compiler |
| `src/HooCompiler.{h,cpp}` | Main compilation orchestrator |
| `src/ProcessIsolatedParser.{h,cpp}` | ANTLR4 parser wrapper |
| `src/SimpleASTBuilder.{h,cpp}` | Parse tree → AST converter |
| `src/CodeGenerator.{h,cpp}` | AST → LLVM IR translator |
| `src/HoocJIT.{h,cpp}` | LLVM ORC JIT execution engine |
| `src/Hooc.g4` | ANTLR4 grammar definition |
| `src/ast/*.h` | AST node type definitions |
| `src/ast/ASTImpl.cpp` | AST utility implementations |
| `CMakeLists.txt` | Build configuration |
| `docs/hooc_language_specification_v_0.md` | Complete language spec |
| `docs/implementation-status.md` | Detailed progress tracking |

## Dependencies

- **ANTLR4 C++ Runtime 4.13.2**: Parser generation (JAR in `tools/`, runtime as system library)
- **LLVM 21.1.8** (macOS) / 15.0+ (Windows): Code generation and JIT
- **CMake 3.16+**: Build system
- **C++17**: Implementation language
- **GoogleTest**: Unit testing framework
- **vcpkg** (Windows only): Package management

## Platform-Specific Notes

### macOS
```bash
brew install cmake llvm antlr4-cpp-runtime googletest
cmake -B build && cmake --build build
```

### Windows
```bash
# CRITICAL: Always specify build type to avoid runtime library mismatch
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

**Important Windows Build Notes:**
- Uses vcpkg for dependencies (see `vcpkg.json`)
- LLVM 21.1.8 installed (requires Release runtime `/MD`)
- **Must** use `RelWithDebInfo` or `Release` build type - never build without specifying
- Without explicit build type, CMake defaults to Debug (`/MDd` runtime) which causes linker errors
- The CMakeLists.txt attempts to force Release runtime, but explicit configuration is more reliable
- See `docs/building-on-windows.md` for detailed setup
