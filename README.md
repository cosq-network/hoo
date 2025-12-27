# hoo Programming Language

> A modern, safe, and expressive programming language designed as an evolution of the C programming philosophy. Compiled by **hooc**.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](.)
[![Version](https://img.shields.io/badge/version-0.5--alpha-blue)](.)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)]()
[![Parser](https://img.shields.io/badge/parser-ANTLR4-orange)]()
[![Backend](https://img.shields.io/badge/backend-LLVM-red)]()
[![Tests](https://img.shields.io/badge/tests-111%20key%20tests-success)](.)
[![Framework](https://img.shields.io/badge/framework-Runtime%20Class%20Injection-blue)](.)
[![License](https://img.shields.io/badge/license-MIT-green)]()

## 🎯 Project Overview

**hoo** is a modern, safe, and expressive programming language that removes C's unsafe constructs while preserving its clarity, predictability, and performance mindset. Compiled by **hooc**, it integrates modern abstractions directly into the language, including:

- ✅ **Strong static typing** with type inference
- ✅ **Type safety** with union types and optional types (nullable system)
- ✅ **Array literals** with automatic type inference and global constant storage
- ✅ **No null pointers** - optional types with explicit unwrapping
- ✅ **Module-level variable declarations** - Top-level variables for global scope
- ✅ **Modern syntax** - Python-style imports, clean control flow
- ✅ **Object-oriented programming** - Classes with constructors and member variables
- ✅ **Member access** - Read class fields with the `.` operator
- ✅ **Automatic memory management** - Reference counting for safe object lifetime management
- ✅ **Language-level design patterns** (Singleton, Factory, Observer, etc. - grammar defined, code generation planned)

## 🚀 Quick Start

### Prerequisites
*   **C++17 Compiler:** A modern C++ compiler supporting C++17 (e.g., Clang, GCC, MSVC).
*   **CMake:** Version 3.16 or higher.
*   **LLVM:** Version 21.1.8 (or compatible version as specified in `CMakeLists.txt`).
    *   On macOS (using Homebrew): `brew install llvm`
*   **ANTLR4 C++ Runtime:**
    *   On macOS (using Homebrew): `brew install antlr4-cpp-runtime`
    *   The build process will attempt to download the ANTLR4 Java JAR (`antlr-4.13.1-complete.jar`) if not found.
*   **vcpkg:** (Recommended for managing dependencies like Google Test). Follow [vcpkg installation instructions](https://github.com/microsoft/vcpkg#getting-started). Ensure `vcpkg integrate install` has been run.

# Verify installation
```bash
cmake --version
llvm-config --version
```
*Note: Depending on your LLVM installation method, you might need to adjust your PATH or use specific CMake toolchain files.*

### Build & Run

1.  **Clone the repository** (if you haven't already):
    ```bash
    git clone <repository_url>
    cd <project_directory> # e.g., cd hooc
    ```
    *Note: If you are already in the project's root directory (`/Users/benoybose/Projects/hooc/`), skip the clone and cd steps.*

2.  **Configure and Build:**
    *   Ensure `vcpkg` is integrated or use its toolchain file.

    ```bash
    # Create a build directory and configure the project using vcpkg toolchain
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake

    # Build the project (default configuration, typically Debug or RelWithDebInfo)
    cmake --build build

    # For a Release build (recommended for performance):
    # cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
    # cmake --build build --config Release
    ```
    *Note: Replace `<path/to/vcpkg>` with the actual path to your vcpkg installation.*

3.  **Compile a `hoo` program and view LLVM IR:**
    The `hooc` compiler executable is located in the `build/` directory. It translates `.hoo` source files into LLVM Intermediate Representation (IR) and prints it to standard output. JIT execution is currently pending integration.

    ```bash
    # Compile and print LLVM IR for an example program
    ./build/hooc tests/examples/example.hoo

    # For detailed help and compiler options:
    ./build/hooc --help
    ```

## 📖 Language Examples

### Hello World
```hoo
func main() {
    print("Hello, World!");
}
```

### Variables & Types
```hoo
func example() -> void {
    var count = 42;              // int64 (inferred)
    var pi = 3.14;               // double (inferred)
    var active = true;           // bool (inferred)
    var flag: bool = false;      // explicit type

    // Array literals with type inference (v0.2)
    var numbers = [1, 2, 3, 4, 5];           // int64[]
    var floats = [1.0, 2.5, 3.14];           // double[]
    var matrix = [[1, 2], [3, 4]];           // int64[][]

    // Type annotations
    var data: int64[] = [10, 20, 30];
}
```

### Arrays & Loops (v0.2)
```hoo
func array_example() -> void {
    // Array literals with type inference
    var numbers = [1, 2, 3, 4, 5];
    var data: int64[] = [10, 20, 30, 40, 50];

    // For-range loop
    for i in 0..5 {
        var item = numbers[i];
    }

    // Multi-dimensional arrays
    var matrix = [[1, 2], [3, 4]];

    // For-in loop (currently experimental or requires specific array type handling)
    // for item in numbers {
    //     // Process item
    // }
}
```

### Classes & Objects (Grammar defined, code generation in progress)
```hoo
// Class declaration (parsed, AST building in progress, code generation pending v0.3)
class User(string name, int64 age) {
    func greet() {
        print("Hello, I'm ${name}");
    }
}

// Object instantiation with 'new' (planned for v0.3)
// func main() -> void {
//     var user = new User("Alice", 30);
//     user.greet();
// }
```

**Status**: Class declarations are parsed and AST building is in progress. Object instantiation with the `new` keyword is reserved but not yet implemented. Planned for v0.3.

### Design Patterns (Grammar defined, code generation planned)
```hoo
// Design pattern keywords are parsed but code generation is planned for v0.3+

singleton class Logger {
    func log(string msg) {
        print("[LOG] ${msg}");
    }
}

immutable class Money(double amount, string currency);

factory class Shape {
    Circle(int64 r);
    Rectangle(int64 w, int64 h);
}

observable class Button {
    event clicked;
}

actor class Counter {
    int64 value;
}
```

**Status**: All design pattern modifiers (`singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`) are recognized by the grammar and can be parsed. Full code generation support is planned for v0.3.

## 🏗️ Implementation Status

### ✅ **Completed (v0.2)**
- **Complete Grammar** - Full ANTLR4 grammar including design patterns and modern type system
- **LLVM Backend** - Full AST to LLVM IR translation with optimized code generation
- **LLVM ORC JIT** - JIT compilation support is present, but execution integration is pending in `main.cpp`.
- **Type System** - Union types, optional types (nullable system), array types with proper inference
- **Module-level variable declarations** - Full parsing and AST building for global scope variables.
- **Primitive Types** - byte, uint8, int64, float, double, f64, bool, char, void, string (string code gen pending)
- **Array Literals** - Robust parsing, AST building, and LLVM IR generation with type inference, multi-dimensional support, and global constant storage.
- **Array Access** - Full support for indexed access with bounds checking (basic implementation)
- **Variables** - Type inference and explicit type annotations
- **Expressions** - All arithmetic, comparison, logical, assignment operators with correct precedence
- **Control Flow** - If/else, while loops, for-range, for-in loops (for-in basic implementation) with proper scoping
- **Functions** - Full declarations, parameters, return types, recursive calls
- **Function Calls** - Hooc-to-hooc calls with argument passing and return values
- **External Functions** - Basic FFI support (e.g., printf)
- **Scope Statements** - Deterministic resource management blocks
- **Process-Isolated Parsing** - Robust parser with ANTLR4 state isolation
- **AST Infrastructure** - Complete type-safe AST hierarchy
- **Test Suite** - 287 comprehensive unit tests with 100% pass rate

### 🔧 **Current Architecture**
```
Source Code (.hoo)
     ↓
ProcessIsolatedParser (ANTLR4) ✅
     ↓
Parse Tree ✅
     ↓
SimpleASTBuilder ✅
     ↓
AST (CompilationUnit) ✅
     ↓
LLVMCodeGenerator (implements CodeGenerator) ✅
     ↓
LLVM IR Module ✅
     ↓
(JIT Infrastructure Present) 🚧
     ↓
(Execution Pending Integration) 🚧
```

### 🎯 **Next Development Steps (v0.3)**
1.  **String Type** - Complete LLVM code generation for string literals and operations.
2.  **Classes & Objects** - Implement object instantiation with `new` keyword, method calls.
3.  **Design Patterns** - Add code generation for singleton, factory, observable patterns.
4.  **Type Casting** - Implement `as` keyword for explicit type conversions.
5.  **Class Inheritance** - Support `extends` for single inheritance.
6.  **Interface Implementation** - Full `implements` support with method resolution.
7.  **Standard Library** - Build core functions (print, I/O, collections).
8.  **Module System** - Implement module resolution for import/export.

## 🔧 Development Tools

### **hooc** - Unified Compiler
```bash
./build/hooc example.hoo
```
**Output:**
*   Compiles `.hoo` source code into LLVM IR.
*   Prints the generated LLVM IR to standard output.
*   Includes integrated parsing, AST building, and LLVM IR generation.
*   Supports command-line arguments for help and source file input.
*   Error handling and progress reporting during compilation.
*   JIT execution is not yet integrated into the main compiler executable.

## 📚 Documentation

-   **[Language Specification](docs/01-language-specification.md)** - Complete hoo language design
-   **[String Quick Reference](docs/02-hoo-string-quick-reference.md)** - HooString API reference
-   **[Implementation Status](docs/03-implementation-status.md)** - Detailed progress tracking
-   **[Roadmap](docs/04-roadmap.md)** - Feature roadmap and future plans
-   **[Sample Programs](docs/05-sample-programs.md)** - Reference examples
-   **[Object Creation Guide](docs/06-object-creation-guide.md)** - Classes and instantiation
-   **[Memory Management Design](docs/07-memory-management-design.md)** - ARC implementation details
-   **[String Integration Guide](docs/08-string-integration-guide.md)** - String type architecture
-   **[Quick Reference](docs/09-quick-reference.md)** - Developer guide
-   **[FFI Implementation Plan](docs/10-ffi-plan.md)** - Foreign function interface
-   **[Windows Build Guide](docs/11-building-on-windows.md)** - Windows-specific setup

## 🏛️ Technical Foundation

### **Dependencies**
-   **ANTLR4 C++ Runtime:** For robust parsing.
-   **LLVM 21.1.8+:** For industrial-strength code generation and JIT infrastructure.
-   **CMake 3.16+:** For cross-platform build system management.
-   **C++17:** Modern C++ implementation standard.
-   **vcpkg:** For dependency management (e.g., Google Test).

### **Key Features**
-   **Process-Isolated Parsing:** Prevents ANTLR4 state corruption.
-   **LLVM IR Generation:** Generates verifiable LLVM IR.
-   **AST Infrastructure:** Complete type-safe Abstract Syntax Tree.
-   **Type-Safe Design:** Strong, static typing with inference throughout.
-   **Nullable Types:** A safe approach to handling potential null values via tagged unions.

### **Architecture**
**hooc** is the compiler for the **hoo** programming language, translating `.hoo` source files into LLVM IR via an AST.

## 🐛 Current Status & Known Limitations

### **Fully Implemented (v0.5)**
-   ✅ **Primitive types** - byte, uint8, int64, float, double, f64, bool, char, void
-   ✅ **String type** - Full parsing and LLVM code generation with HooString runtime library
-   ✅ **String operations** - 30+ string functions via runtime class injection framework
-   ✅ **Function declarations** - Complete with parameters, return types, and recursion
-   ✅ **Function calls** - Hooc-to-hooc calls and external C function calls
-   ✅ **Variable declarations** - Type inference and explicit annotations
-   ✅ **All expressions** - Arithmetic, comparison, logical, assignment operators
-   ✅ **Control flow** - If/else, while loops, for-range, for-in loops, scope blocks
-   ✅ **Array literals** - Complete with type inference and multi-dimensional support
-   ✅ **Array access** - Full support for indexed access
-   ✅ **Type system** - Unions, optionals (nullable system), array slice types
-   ✅ **Classes & Objects** - Class declarations with member variables and methods (v0.4)
-   ✅ **Member access** - Read class fields with the `.` operator (v0.5)
-   ✅ **Method calls** - Invoke methods on object instances (v0.5)
-   ✅ **Automatic Reference Counting** - Memory management via runtime library
-   ✅ **111 key unit tests** - StringCodeGenTest, StringBasicsTest, and comprehensive feature tests

### **Parsed But Code Generation Incomplete**
-   ⚠️ **Design patterns** - Keywords parsed, code generation planned for future versions.
-   ⚠️ **Interface declarations** - Grammar complete, code generation pending.
-   ⚠️ **Type casting** - `as` keyword reserved, not implemented.
-   ⚠️ **Class inheritance** - `extends` keyword parsed, code generation pending.
-   ⚠️ **Member assignment** - Grammar parsed, code generation pending.

### **Not Yet Implemented**
-   ❌ **JIT Execution:** The `hooc` executable compiles to LLVM IR but does not execute it. JIT integration is pending.
-   ❌ **Module system:** Import/export parsing works, resolution not implemented.
-   ❌ **Standard library:** String library implemented, other core functions pending.
-   ❌ **Advanced function features:** Function pointers, callbacks.

## 🏗️ Runtime Class Injection Framework

Hooc features a novel **runtime class injection framework** that enables easy addition of new runtime types (like String) to the compiler without boilerplate code duplication. The framework uses the **X-Macro pattern** for compile-time code generation.

### **Key Features**
-   **Single Source of Truth** - Define runtime class metadata once in `RuntimeClassRegistry.h`
-   **Zero-Cost Abstraction** - All code generation happens at compile time
-   **JIT Registration** - Automatic registration of runtime functions with LLVM ORC JIT
-   **Type-Aware Operators** - Binary operators automatically dispatched to correct runtime implementations
-   **Extensible Design** - Add new runtime classes (Array, Dict, custom types) with minimal code

### **Framework Files**
-   `src/runtime/MacroHelpers.h` - Macro utilities for variadic argument handling
-   `src/runtime/RuntimeClassRegistry.h` - Central registry of all runtime classes (X-Macro)
-   `src/runtime/RuntimeClassCodeGen.h` - Code generation patterns and documentation

### **Example: Adding a New Runtime Class**
To add an `Array` runtime class, simply extend `RuntimeClassRegistry.h`:
```cpp
DEFINE_RUNTIME_CLASS(Array, HooArray, isPointerTy)
    BEGIN_RUNTIME_FUNCTIONS
        RUNTIME_FUNCTION(new, HooArray, LLVM_PTR, (int64_t, LLVM_I64))
        RUNTIME_FUNCTION(push, void, LLVM_VOID, (HooArray, LLVM_PTR), (int64_t, LLVM_I64))
        // ... more functions ...
    END_RUNTIME_FUNCTIONS
    BEGIN_RUNTIME_OPERATORS
        // Optional operator overloads
    END_RUNTIME_OPERATORS
```

Everything else (JIT registration, LLVM declarations, operator dispatch) is auto-generated! See `docs/08-string-integration-guide.md` for detailed architecture.

## 🤝 Contributing

### **Development Workflow**
1.  **Modify grammar** in `src/Hooc.g4`.
2.  **Regenerate ANTLR4 files** (if grammar changes): `cmake --build build --target generate_parser`.
3.  **Build and test**: `cmake --build build && ./build/hoo-tests`.
4.  **Update AST** in `src/ast/` if needed.
5.  **Test integration** with `./build/hooc tests/examples/example.hoo`.
6.  **Run the compiler executable** `./build/hooc <your_program.hoo>`.

### **Priority Areas**
-   **🔥 High**: Implement JIT execution for the `hooc` compiler.
-   **🔥 High**: Build standard library (I/O, collections, utilities beyond strings).
-   **🔥 High**: Implement class inheritance and interfaces.
-   **📈 Medium**: Add Array and Dict runtime types using injection framework.
-   **📈 Medium**: Implement import/module system.
-   **📈 Medium**: Implement member assignment (obj.field = value).
-   **🧹 Low**: Enhance error diagnostics and add optimization passes.

### **Getting Started with Development**
```bash
# 1. Ensure prerequisites are installed (CMake, LLVM, ANTLR4 C++ runtime, vcpkg)

# 2. Configure and build the project (using vcpkg toolchain)
#    Replace <path/to/vcpkg> with your vcpkg installation path.
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 3. Run existing tests to verify setup
./build/hoo-tests              # Run full test suite with String support tests

# 4. Try compiling example programs and view LLVM IR
./build/hooc tests/examples/arithmetic.hoo
./build/hooc tests/examples/for_loops.hoo

# 5. Key files for development
src/LLVMCodeGenerator.cpp      # LLVM IR generation (concrete implementation)
src/CodeGenerator.h            # Abstract code generator interface
src/SimpleASTBuilder.cpp       # Parse tree to AST conversion
src/HooCompiler.cpp            # Main compilation pipeline
src/main.cpp                   # Compiler executable entry point
src/ast/                       # AST node definitions
src/Hooc.g4                    # ANTLR grammar
```

## 📈 Project Roadmap

### **v0.5 (Current) - ✅ Mostly Complete**
-   ✅ Full ANTLR4 grammar with design patterns
-   ✅ Complete AST → LLVM IR pipeline
-   ✅ All primitive types with proper semantics
-   ✅ String type - Full LLVM code generation (30+ functions)
-   ✅ Runtime class injection framework (X-Macro pattern)
-   ✅ Classes with member variables and methods
-   ✅ Member access operator (`.`) for field read access
-   ✅ Method calls on object instances
-   ✅ Automatic Reference Counting (ARC) memory management
-   ✅ Nullable types with `T?` syntax
-   ✅ Array literals with type inference and multi-dimensional support
-   ✅ Control flow: if/else, while, for-range, for-in, scope blocks
-   ✅ 111 key unit tests with comprehensive feature coverage

### **v0.6 (Planned)**
-   🔧 **JIT Execution** - Integrate LLVM ORC JIT for direct execution of compiled code.
-   🔧 **Class Inheritance** - Implement `extends` for single inheritance.
-   🔧 **Interfaces** - Full `implements` support with method resolution.
-   🔧 **Member Assignment** - Implement `obj.field = value` for mutable fields.
-   🔧 **Design Patterns** - Code generation for singleton, factory, observer.
-   🔧 **Array Type** - Add Array runtime type using injection framework.
-   🔧 **Dict Type** - Add Dictionary runtime type using injection framework.

### **v0.7+ (Future)**
-   📋 Type casting with `as` keyword
-   📋 Module system with proper import resolution.
-   📋 Advanced type features: generics (restricted), pattern matching.
-   📋 Standard Library expansion - I/O, collections, utilities.
-   📋 Runtime features: reflection, serialization.
-   📋 Performance: optimization passes, inline hints.
-   📋 Tooling: LSP, debugger integration, formatter.
-   📋 Self-hosting: hooc compiler written in hooc.

## 📄 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

Built with modern compiler construction tools:
-   **ANTLR4** for robust parsing infrastructure.
-   **LLVM** for world-class code generation and JIT capabilities.
-   **CMake** for reliable build management.
-   **vcpkg** for dependency management.

**Current Status (v0.5)**: The hooc compiler now features a complete string type implementation with 30+ functions, automatic reference counting for objects, member variables and method calls on classes, and a powerful runtime class injection framework for easy addition of new types. The framework uses compile-time code generation (X-Macros) to eliminate boilerplate while maintaining type safety and performance. The project is production-ready for string and object-oriented programming features, with a clear roadmap for inheritance, interfaces, and additional runtime types.

---

> *"hoo v0.5 delivers a mature compiler with string support, object-oriented programming, and a novel runtime class injection framework. The hooc compiler successfully handles complex language features while maintaining code clarity and extensibility. With comprehensive testing (111 key tests passing) and a proven architecture pattern, the project is production-ready for its supported feature set and well-prepared for future expansions."*