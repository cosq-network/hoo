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
# Run all unit tests (278 tests, 14 test suites)
./build/hoo_tests        # macOS/Linux
./build/hoo_tests.exe    # Windows

# Run specific test suite with verbose output
./build/hoo_tests --gtest_filter="BasicCodeGenTest.*"
./build/hoo_tests --gtest_filter="FunctionCallCodeGenTest.*"
./build/hoo_tests --gtest_filter="NullableCodeGenTest.*"

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
LLVMCodeGenerator (implements CodeGenerator)
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

**CodeGenerator** (`src/CodeGenerator.h`)
- Abstract base class interface for code generation
- Provides contract for translating AST to executable code
- Enables support for multiple backend targets (LLVM, bytecode, C, etc.)
- Key methods: `generateModule()`, `generateFunction()`, `generateExpression()`, `generateStatement()`, `generateType()`

**LLVMCodeGenerator** (`src/LLVMCodeGenerator.{h,cpp}`)
- Concrete implementation of CodeGenerator for LLVM backend
- Translates AST to LLVM IR
- Entry point: `generateLLVMModule(CompilationUnit&)` returns LLVM Module
- Maintains symbol tables for variables and functions (`namedValues_`, `functions_`)
- Key methods:
  - `generateLLVMFunction()`: Creates LLVM functions with parameters and body
  - `generateLLVMExpression()`: Dispatches to specific expression generators
  - `generateLLVMStatement()`: Handles control flow and variable declarations
  - `generateLLVMType()`: Converts hooc types to LLVM types

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
- `byte` / `uint8` → i8
- `int64` → i64
- `float` → f32
- `double` / `f64` → double
- `bool` → i1
- `char` → i8
- `void` → no return value
- Array types with multi-dimensional support

**Partially Implemented**
- `string` type (parsing complete, code generation pending)
- Classes and interfaces (grammar complete, AST building partial)
- Design pattern keywords (singleton, immutable, factory, observable, service, strategy, actor - parsed, code generation not implemented)

**Not Yet Implemented**
- Type casting with `as` keyword (reserved)
- Object instantiation with `new` keyword (parsed, not implemented)
- Class inheritance with `extends` (parsed, not implemented)
- Interface implementation (parsed, not implemented)
- Event system (parsed, not implemented)
- Module resolution (import/export parsing works, resolution not implemented)
- Alternative code generator backends (bytecode, C, JavaScript, etc.)

### Code Generator Architecture

**Design Pattern: Abstract Factory + Strategy**

The code generator has been refactored to use an abstract interface pattern, enabling support for multiple backend targets:

**CodeGenerator (Abstract Base Class)**
- Defines the contract for code generation
- Uses opaque wrapper types (`GeneratedModule`, `GeneratedFunction`, `GeneratedValue`, `GeneratedType`)
- Enables backend-agnostic AST processing
- Future backends can implement this interface (bytecode VM, C transpiler, JavaScript, etc.)

**LLVMCodeGenerator (Concrete Implementation)**
- Implements CodeGenerator interface for LLVM backend
- Provides LLVM-specific API for direct LLVM type access
- Methods prefixed with `generateLLVM*` return concrete LLVM types (`llvm::Module*`, `llvm::Function*`, etc.)
- Maintains LLVM-specific state (context, builder, symbol tables)

**Benefits:**
- Clean separation between AST and backend
- Easy to add new code generation targets
- Testing can use mock code generators
- Future support for WebAssembly, bytecode, or other targets without changing AST

**When to Use Which API:**
- Use `CodeGenerator` interface for backend-agnostic code
- Use `LLVMCodeGenerator::generateLLVM*()` methods when you specifically need LLVM types
- `HooCompiler` uses the abstract interface internally but returns concrete LLVM modules for now

## Testing Strategy

### Test Organization
- `tests/BasicCodeGenTest.cpp`: Basic LLVM IR generation tests
- `tests/FunctionCallCodeGenTest.cpp`: Function call code generation tests
- `tests/VariableDeclarationCodeGenTest.cpp`: Variable declaration code generation tests
- `tests/ArrayLiteralParsingTest.cpp`: Array literal parsing tests (12 tests)
- `tests/FunctionCallParsingTest.cpp`: Function call parsing tests (15 tests)
- `tests/VariableDeclarationParseTest.cpp`: Variable declaration parsing tests (15 tests)
- `tests/SimpleASTBuilderTest.cpp`: Parse tree → AST conversion tests
- `tests/HooCompilerTest.cpp`: End-to-end compilation tests
- `tests/ProcessIsolatedParserTest.cpp`: Parser validation tests
- `tests/examples/*.hoo`: Real hoo programs for integration testing

### Writing Tests
```cpp
// Standard test pattern for code generation
TEST_F(BasicCodeGenTest, TestName) {
    std::string code = R"(
        func test() -> int64 {
            return 42;
        }
    )";

    auto ast = parseAndBuildAST(code);
    ASSERT_NE(ast, nullptr);

    // LLVMCodeGenerator now returns GeneratedModule wrapper
    auto generatedModule = codeGen->generateModule(*ast);
    ASSERT_NE(generatedModule, nullptr);

    // For LLVM-specific tests, use generateLLVMModule directly
    auto llvmModule = llvmCodeGen->generateLLVMModule(*ast);
    ASSERT_NE(llvmModule, nullptr);

    // Verify IR
    auto* func = llvmModule->getFunction("test");
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

4. **Implement LLVMCodeGenerator** (`src/LLVMCodeGenerator.cpp`)
   - Add `generateLLVM*` methods for new AST nodes
   - Update dispatch methods (generateLLVMExpression/generateLLVMStatement)
   - Emit proper LLVM IR using `builder_`
   - Update symbol tables as needed
   - If targeting other backends, implement in separate CodeGenerator subclass

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

## Nullable Types (v0.3)

Hoo supports nullable types using the `T?` syntax, allowing variables to represent either a value or null. This provides memory safety and null-safety guarantees at compile time.

### Syntax and Usage

```hoo
// Nullable variable declarations
var x: int64? = 42;         // Non-null value
var y: int64? = null;       // Null value
var z: double? = 3.14;      // Works with all primitive types

// Nullable function parameters
func process(int64? value) -> int64 {
    return 10;
}

// Nullable return types
func maybeValue() -> int64? {
    return 42;
}

// Mixed nullable and non-nullable parameters
func compare(int64 required, int64? optional) -> bool {
    return true;
}

// Nullable arrays
var arr: int64[]? = [1, 2, 3];
var empty: int64[]? = null;
```

### Implementation Details

**LLVM Representation:**
- Nullable types use a **tagged union pattern**: `{ i1 isNull, T value }`
- First field: null flag (true if null, false if contains value)
- Second field: actual value (undefined if null)
- Example: `int64?` is represented as `{ i1, i64 }` in LLVM IR

**Type Support:**
- All primitive types support nullability: `byte?`, `uint8?`, `int64?`, `float?`, `double?`, `f64?`, `bool?`, `char?`
- Array types support nullability: `int64[]?`, `double[][]?`
- Union types can contain nullable types: `int64? | double?`

**Features:**
- Null literal support: `null` keyword for assigning null values
- Type inference: Nullable types are inferred from the `?` annotation
- Function parameters: Can be nullable to accept optional values
- Return values: Functions can return nullable types
- Variable scope: Nullable variables properly scoped in blocks

### Current Implementation Status

**✅ Fully Working (v0.3):**
- All primitive types: `byte`, `uint8`, `int64`, `float`, `double`, `f64`, `bool`, `char`, `void`
- Literals: integer, floating-point, boolean, character, string literal, array literals, null literals (`null`)
- Variable declarations with type inference and explicit type annotations
- Array literals with type inference (`[1, 2, 3]`)
- Multi-dimensional array literals (`[[1, 2], [3, 4]]`)
- Array slice types for function parameters (`int64[]`)
- Array element access (`arr[index]`)
- Type system: union types (`T | U`), optional types (`T?`), array types (`T[]`)
- **Nullable types** - Full support with `T?` syntax (v0.3 addition):
  - Nullable variable declarations (`var x: int64? = 42` or `var y: int64? = null`)
  - Nullable function parameters (`func process(int64? value) -> int64`)
  - Nullable return types (`func maybeValue() -> int64?`)
  - All primitive types support nullability
  - LLVM tagged union representation (`{ i1 isNull, T value }`)
  - Null literal support (`null` keyword for assigning null values)
- All arithmetic operators: `+`, `-`, `*`, `/`, `%`
- All comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
- All logical operators: `&&`, `||`, `!`
- Assignments and compound assignments
- If/else statements with proper control flow
- While loops with break/continue support
- For-range loops (`for i in 0..10`)
- For-in loops (`for item in collection`)
- Function declarations with parameters and return types
- Return statements
- Expression statements
- Blocks with proper scoping
- Scope statements (`scope { ... }`) for deterministic resource management
- Unary operators: negation (`-`), logical NOT (`!`)
- Postfix operators: array indexing, function calls, member access

**✅ Function Calls:**
- Hooc-to-hooc function calls fully working
- Argument passing and return values
- External C function calls (e.g., `printf`)

**⚠️ Parsed But Code Generation Incomplete:**
- String type (parsing works, LLVM generation pending)
- Class declarations (grammar and basic parsing work)
- Interface declarations (grammar parsed)
- Design pattern modifiers (singleton, immutable, factory, observable, service, strategy, actor)
- Event system (`event` keyword)
- Object instantiation (`new` keyword)
- Type casting (`as` keyword)

**❌ Not Implemented:**
- String type operations and LLVM generation
- Classes and object instantiation
- Inheritance and polymorphism
- Interface implementation
- Advanced function features (pointers, callbacks, method calls)
- Module system and import resolution
- Standard library

**🚫 Removed (v0.2-v0.3):**
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

**Nullable Type Operations (v0.3)**
- Nullable types use tagged union representation in LLVM (`{ i1, T }`)
- Helper methods available in `LLVMCodeGenerator`:
  - `createNullableType(valueType)` - Creates struct type for nullable value
  - `createNullValue(valueType)` - Creates a null value
  - `wrapValueInNullable(value, nullableType)` - Wraps value in nullable struct
  - `extractValueFromNullable(nullableValue)` - Extracts value from nullable
  - `extractNullFlagFromNullable(nullableValue)` - Extracts null flag
  - `isTypeNullable(type)` - Checks if type is nullable
- Nullable types are fully transparent to existing code generation logic
- No special handling needed in most contexts - LLVM IR handles struct operations

## Key Files Reference

| Path | Purpose |
|------|---------|
| `src/main.cpp` | CLI entry point for hooc compiler |
| `src/HooCompiler.{h,cpp}` | Main compilation orchestrator |
| `src/ProcessIsolatedParser.{h,cpp}` | ANTLR4 parser wrapper |
| `src/SimpleASTBuilder.{h,cpp}` | Parse tree → AST converter |
| `src/CodeGenerator.h` | Abstract code generator interface |
| `src/CodeGeneratorTypes.h` | Opaque wrapper types for generated code |
| `src/LLVMCodeGenerator.{h,cpp}` | LLVM backend implementation (AST → LLVM IR) |
| `src/LLVMCodeGeneratorTypes.h` | LLVM-specific wrapper implementations |
| `src/HoocJIT.{h,cpp}` | LLVM ORC JIT execution engine |
| `src/Hooc.g4` | ANTLR4 grammar definition |
| `src/ast/*.h` | AST node type definitions |
| `src/ast/ASTImpl.cpp` | AST utility implementations |
| `tests/NullableTypeParsingTest.cpp` | Nullable type parsing tests (15 tests) |
| `tests/NullableCodeGenTest.cpp` | Nullable type code generation tests (20 tests) - v0.3 |
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
