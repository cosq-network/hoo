# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**hooc** is a compiler for the **hoo** programming language - a modern, safe, and expressive language designed as an evolution of C's philosophy. It removes C's unsafe constructs while preserving clarity, predictability, and performance. The compiler is written in C++17 and targets LLVM for code generation.

**Current Status (v0.6):** Full ANTLR4 grammar, complete AST → LLVM IR pipeline, primitive types, strings, classes with methods, automatic reference counting (ARC), and C#-style generics with monomorphization. 577 comprehensive unit tests with 100% pass rate.

## Build & Development Commands

### Build the project

**Windows:**
```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

**macOS/Linux:**
```bash
# Initial configuration (replace <path/to/vcpkg> with your vcpkg installation)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build

# Build with Release configuration (recommended for performance)
cmake --build build --config Release
```

See `docs/11-building-on-windows.md` for detailed Windows setup instructions.

### Run tests
```bash
# Run all tests using ctest (recommended)
ctest --test-dir=build --progress --verbose

# Or using cmake
cmake --build build --target run_tests

# Or directly
./build/hoo-tests
```

### Run single test
```bash
# List available tests
./build/hoo-tests --gtest_list_tests

# Run a specific test
./build/hoo-tests --gtest_filter="TestClassName.TestMethodName"

# Run tests matching a pattern
./build/hoo-tests --gtest_filter="*Generic*"
```

### Compile a .hoo program
```bash
# Compile and print LLVM IR
./build/hooc tests/examples/arithmetic.hoo

# Help and options
./build/hooc --help
```

### Regenerate ANTLR4 parser files
```bash
# After modifying src/Hooc.g4
cmake --build build --target generate_parser
```

### Clean generated ANTLR4 files
```bash
cmake --build build --target clean_generated
```

## Architecture Overview

The compilation pipeline follows this flow:

```
Source Code (.hoo)
    ↓
ProcessIsolatedParser (ANTLR4 parsing)
    ↓
Parse Tree
    ↓
SimpleASTBuilder (tree → AST conversion)
    ↓
AST (CompilationUnit)
    ↓
LLVMCodeGenerator (AST → LLVM IR)
    ↓
LLVM IR Module
    ↓
(Output to stdout or JIT execution)
```

### Key Components

**ProcessIsolatedParser** (`src/ProcessIsolatedParser.{h,cpp}`)
- Parses .hoo source code using ANTLR4 grammar
- Provides `parseForAST()` for direct parse tree access (needed for AST building)
- Handles ANTLR4 state isolation to prevent parser corruption

**SimpleASTBuilder** (`src/SimpleASTBuilder.{h,cpp}`)
- Converts ANTLR4 parse tree to type-safe AST
- Builds declarations, statements, expressions, types
- Key method: `buildAST()` converts parse tree to `ast::CompilationUnit`

**LLVMCodeGenerator** (`src/LLVMCodeGenerator.{h,cpp}`)
- Concrete implementation of `CodeGenerator` interface
- Generates LLVM IR from AST nodes
- Manages type conversion, function/variable scoping, operator dispatch
- Main entry: `generateModule()` for compilation units

**HooCompiler** (`src/HooCompiler.{h,cpp}`)
- High-level compilation interface orchestrating the entire pipeline
- Single method: `compile(moduleName, sourceCode)` → LLVM Module
- Used by `src/main.cpp` (compiler executable)

**Runtime Class Injection Framework** (`src/runtime/`)
- X-Macro pattern for adding runtime types without boilerplate
- `RuntimeClassRegistry.h`: Central registry of all runtime classes (String, Array, etc.)
- `RuntimeClassCodeGen.h`: Documentation and patterns
- `MacroHelpers.h`: Variadic argument handling utilities
- Used by `HoocJIT.cpp` for JIT registration and `LLVMCodeGenerator.cpp` for LLVM declarations

### AST Structure

All AST nodes are in `src/ast/`. Key files:

- `AST.h`: Type declarations and hierarchy
- `ASTNode.h`: Base class for all AST nodes
- `CompilationUnit.h`: Root node containing functions and class declarations
- `Expression.h`, `Statement.h`, `Declaration.h`: Base classes for expression/statement/declaration nodes
- `Type.h`: Type system (PrimitiveType, ArrayType, OptionalType, BaseType, UnionType)
- `ClassDeclaration.h`: Class definitions with members and methods
- `QualifiedIdentifier.h`: Module-qualified identifiers for module system

### Generic Type Implementation

C#-style generics use monomorphization (code duplication) via type parameter name mangling:

- `mangleClassName()`: Converts `Array<int64>` → `Array_int64`
- `mangleFunctionNameWithTypes()`: Converts `swap<string, int64>` → `swap_string_int64`
- `typeToMangledString()`: Utility for type → mangled string conversion
- Type parameter scope stack: `pushTypeParameterScope()` / `popTypeParameterScope()`
- See `docs/09-generics-implementation-guide.md` for design details

## Code Organization

```
src/
  ├── Hooc.g4                    # ANTLR4 grammar (language definition)
  ├── HooCompiler.{h,cpp}        # Main compilation interface
  ├── ProcessIsolatedParser.{h,cpp} # ANTLR4 parsing with state isolation
  ├── SimpleASTBuilder.{h,cpp}   # Parse tree → AST conversion
  ├── LLVMCodeGenerator.{h,cpp}  # AST → LLVM IR generation
  ├── HoocJIT.{h,cpp}           # LLVM ORC JIT infrastructure
  ├── CodeGenerator.h            # Abstract code generator interface
  ├── ModuleSystem.{h,cpp}       # Module resolution (partial implementation)
  ├── main.cpp                   # Compiler executable entry point
  ├── ast/                       # AST node definitions
  │   ├── AST.h, ASTNode.h
  │   ├── Expression.h, Statement.h, Declaration.h
  │   ├── Type.h, ClassDeclaration.h
  │   ├── QualifiedIdentifier.h, ImportStatement.h
  │   └── ASTImpl.cpp
  ├── rt/                        # Runtime support (reference counting, strings, arrays)
  │   ├── hoo_runtime.{h,c}
  │   ├── hoo_string.{h,cpp}
  │   └── hoo_generic_array.{h,cpp}
  └── runtime/                   # Runtime class injection framework
      ├── RuntimeClassRegistry.h # X-Macro central registry
      ├── RuntimeClassCodeGen.h  # Code generation patterns
      └── MacroHelpers.h         # Macro utilities

tests/
  ├── test_main.cpp              # Test entry point
  ├── *ParsingTest.cpp           # Grammar parsing tests
  ├── *CodeGenTest.cpp           # LLVM code generation tests
  ├── *IntegrationTest.cpp       # End-to-end tests
  └── examples/                  # .hoo example programs (many deleted, used for testing)

docs/
  ├── 01-language-specification.md    # Language design (v0.6)
  ├── 03-implementation-status.md     # Detailed progress tracking
  ├── 06-object-creation-guide.md     # Classes and object instantiation
  ├── 07-memory-management-design.md  # ARC implementation details
  ├── 08-string-integration-guide.md  # String type architecture
  ├── 09-generics-implementation-guide.md # Generics with monomorphization
  └── 11-module-system.md             # Module system design
```

## Testing Strategy

Tests use Google Test framework:

- **Parsing Tests**: Verify ANTLR4 grammar and AST building (e.g., `ClassDeclarationParsingTest`)
- **Code Gen Tests**: Verify LLVM IR generation (e.g., `FunctionCallCodeGenTest`)
- **Integration Tests**: End-to-end tests (e.g., `GenericIntegrationTest`)
- **Generic Tests**: Specific to C#-style generics (49 tests covering monomorphization)

Run tests frequently during development. The test suite is comprehensive (577 tests) and fast (~few seconds).

## Important Architectural Patterns

### X-Macro Pattern (Runtime Class Injection)

The runtime class injection framework uses X-Macros to avoid boilerplate when adding new runtime types:

1. Define runtime class in `src/runtime/RuntimeClassRegistry.h` using `DEFINE_RUNTIME_CLASS` macro
2. Code generation is automatic:
   - JIT registration in `HoocJIT.cpp`
   - LLVM type declarations in `LLVMCodeGenerator.cpp`
   - Operator dispatch tables in `LLVMCodeGenerator.cpp`

Example: String type is fully defined in the registry and generates 30+ functions automatically.

### Generic Type Monomorphization

Generics are handled via **compile-time code duplication**:
- `Box<int64>` and `Box<string>` generate separate LLVM functions
- Type parameters are mangled into function/class names
- No runtime overhead, full specialization per type

### Reference Counting (ARC)

Objects use automatic reference counting:
- Implemented in `src/rt/hoo_runtime.c` and `.cpp`
- Classes have implicit reference counting on creation/destruction
- No manual memory management required

### Type System

- **Primitive types**: byte, uint8, int64, float, double, f64, bool, char, void
- **Nullable types**: `T?` syntax creates tagged union (value + null bit)
- **Array types**: `T[]` with multi-dimensional support
- **Union types**: `T | U` for multiple possible types
- **Generics**: `T`, `K`, `V` as type parameters

## Common Development Patterns

### Adding a parsing feature

1. Modify `src/Hooc.g4` (ANTLR4 grammar)
2. Run `cmake --build build --target generate_parser`
3. Add AST building method to `SimpleASTBuilder`
4. Add code generation to `LLVMCodeGenerator`
5. Write parsing test, then code gen test

### Adding a new runtime type (e.g., Array, Dict)

1. Implement C API in `src/rt/hoo_xxx.{h,c}`
2. Add entry to `RuntimeClassRegistry.h` using `DEFINE_RUNTIME_CLASS`
3. Rebuild - all registration and LLVM declarations auto-generated
4. Write tests and examples

### Debugging code generation

Check generated LLVM IR directly:
```bash
./build/hooc tests/examples/your_program.hoo | less
```

Verify IR structure with LLVM tools:
```bash
./build/hooc tests/examples/your_program.hoo | opt -S -o /tmp/opt.ll
```

## Known Limitations & TODOs

- **JIT Execution**: LLVM ORC JIT infrastructure present but execution not integrated into `main.cpp`
- **Module System**: Import/export parsing complete, resolution logic incomplete
- **Class Inheritance**: `extends` keyword parsed but code generation pending
- **Interfaces**: Grammar complete, code generation pending
- **Member Assignment**: `obj.field = value` parsed but not generated
- **Design Patterns**: Keywords parsed (singleton, factory, etc.) but code generation pending
- **Type Casting**: `as` keyword reserved but not implemented

See `docs/04-roadmap.md` for v0.7+ plans.

## Dependencies

- **LLVM 21.1.8+**: Code generation and JIT
- **ANTLR4 C++ Runtime**: Parsing
- **CMake 3.16+**: Build system
- **C++17**: Language standard
- **Google Test (gtest)**: Test framework (optional, tests disabled if not found)
- **vcpkg**: Dependency management (recommended)

## Documentation Files

Key docs for understanding the language and implementation:

- `docs/01-language-specification.md`: Complete language design (v0.6 with generics)
- `docs/03-implementation-status.md`: Feature-by-feature status
- `docs/06-object-creation-guide.md`: How classes and objects work
- `docs/07-memory-management-design.md`: ARC implementation details
- `docs/08-string-integration-guide.md`: String type architecture and runtime class injection
- `docs/09-generics-implementation-guide.md`: How C#-style generics with monomorphization work
- `docs/11-module-system.md`: Module system design
