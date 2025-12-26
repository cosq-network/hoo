# hoo Programming Language

> A modern, safe, and expressive programming language designed as an evolution of the C programming philosophy. Compiled by **hooc**.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](.)
[![Version](https://img.shields.io/badge/version-0.2--alpha-blue)](.)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)]()
[![Parser](https://img.shields.io/badge/parser-ANTLR4-orange)]()
[![Backend](https://img.shields.io/badge/backend-LLVM-red)]()
[![Tests](https://img.shields.io/badge/tests-88%20passing-success)](.)
[![License](https://img.shields.io/badge/license-MIT-green)]()

## 🎯 Project Overview

**hoo** is a modern, safe, and expressive programming language that removes C's unsafe constructs while preserving its clarity, predictability, and performance mindset. Compiled by **hooc**, it integrates modern abstractions directly into the language, including:

- ✅ **Strong static typing** with type inference
- ✅ **Type safety** with union types and optional types
- ✅ **Array literals** with automatic type inference
- ✅ **No null pointers** - optional types with explicit unwrapping
- ✅ **Modern syntax** - Python-style imports, clean control flow
- ✅ **Language-level design patterns** (Singleton, Factory, Observer, etc. - grammar defined)

## 🚀 Quick Start

### Prerequisites
```bash
# macOS (Homebrew)
brew install cmake llvm antlr4-cpp-runtime

# Verify installation
cmake --version && llvm-config --version
```

### Build & Run
```bash
# Clone and build
git clone <repository>
cd hoo0.1
cmake -B build && cmake --build build

# Compile and execute a hoo program
./build/hooc example.hoo

# Get help
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

    // For-in loop
    for item in numbers {
        // Process item
    }
}
```

### Classes & Objects (Grammar defined, code generation in progress)
```hoo
// Class declaration (parsed, code generation pending v0.3)
class User(string name, int64 age) {
    func greet() {
        print("Hello, I'm ${name}");
    }
}

// Object instantiation with 'new' (planned for v0.3)
func main() -> void {
    // var user = new User("Alice", 30);
    // user.greet();
}
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

**Status**: All design pattern modifiers (`singleton`, `immutable`, `factory`, `observable`, `service`, `strategy`, `actor`) are recognized by the grammar and can be parsed. Full code generation support is planned for v0.3+.

## 🏗️ Implementation Status

### ✅ **Completed (v0.2)**
- **Complete Grammar** - Full ANTLR4 grammar including design patterns and modern type system
- **LLVM Backend** - Full AST to LLVM IR translation with optimized code generation
- **LLVM ORC JIT** - Working JIT compilation and native code execution
- **Type System** - Union types, optional types, array types with proper inference
- **Primitive Types** - byte, uint8, int64, float, double, f64, bool, char, void
- **Array Literals** - Complete syntax with type inference, multi-dimensional support, global constant storage
- **Array Access** - Full support for indexed access with bounds checking
- **Variables** - Type inference and explicit type annotations
- **Expressions** - All arithmetic, comparison, logical, assignment operators with correct precedence
- **Control Flow** - If/else, while loops, for-range, for-in loops with proper scoping
- **Functions** - Full declarations, parameters, return types, recursive calls
- **Function Calls** - Hooc-to-hooc calls with argument passing and return values
- **External Functions** - Basic FFI support (e.g., printf)
- **Scope Statements** - Deterministic resource management blocks
- **Process-Isolated Parsing** - Robust parser with ANTLR4 state isolation
- **AST Infrastructure** - Complete type-safe AST hierarchy
- **Test Suite** - 88 comprehensive unit tests with 100% pass rate

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
LLVM ORC JIT ✅
     ↓
Native Execution ✅
```

### 🎯 **Next Development Steps (v0.3)**
1. **String Type** - Complete LLVM code generation for string literals and operations
2. **Classes & Objects** - Implement object instantiation with `new` keyword
3. **Design Patterns** - Add code generation for singleton, factory, observable patterns
4. **Type Casting** - Implement `as` keyword for explicit type conversions
5. **Class Inheritance** - Support `extends` for single inheritance
6. **Interface Implementation** - Full `implements` support with method resolution
7. **Standard Library** - Build core functions (print, I/O, collections)
8. **Module System** - Implement module resolution for import/export

## 🔧 Development Tools

### **hooc** - Unified Compiler
```bash
./build/hooc example.hoo
# Output:
# - Complete source-to-execution pipeline
# - Parse validation and AST building
# - LLVM IR generation
# - JIT execution ready
```

**Features:**
- Command-line interface with .hoo file support
- Integrated parsing, AST building, and LLVM IR generation
- Error handling and progress reporting
- End-to-end compilation pipeline

## 📚 Documentation

- **[Language Specification](docs/hooc_language_specification_v_0.md)** - Complete hoo language design
- **[Sample Programs](docs/hooc-sample-programs.md)** - Reference examples  
- **[Implementation Status](docs/implementation-status.md)** - Detailed progress tracking
- **[Quick Reference](docs/quick-reference.md)** - Developer guide

## 🎯 Language Philosophy

### **Core Principles**
- **Safety by construction** – No undefined behavior, no pointer arithmetic, no null
- **Simplicity over cleverness** – Minimal syntax, explicit behavior  
- **Predictable performance** – Ahead-of-time compilation, no hidden costs
- **Language-level power** – Patterns and algorithms built-in
- **C-like mental model** – Straightforward control flow

### **Non-Goals**
- Macro systems or template metaprogramming
- Multiple inheritance complexity
- Reflection-heavy runtimes  
- Implicit magical behavior

## 🏛️ Technical Foundation

### **Dependencies**
- **ANTLR4 C++ Runtime 4.13.2** - Robust parsing with process isolation
- **LLVM 21.1.8** - Industrial-strength code generation
- **CMake 3.16+** - Cross-platform build system
- **C++17** - Modern C++ implementation

### **Key Features**
- **Process-Isolated Parsing** - Eliminates ANTLR4 state corruption
- **LLVM ORC JIT** - Runtime compilation and execution
- **Complete AST Hierarchy** - Ready for semantic analysis  
- **Type-Safe Design** - Strong static typing throughout

### **Architecture**
**hooc** is the compiler for the **hoo** programming language, translating `.hoo` source files into executable code via LLVM.

## 🐛 Current Status & Known Limitations

### **Fully Implemented (v0.2)**
- ✅ **Primitive types** - byte, uint8, int64, float, double, f64, bool, char, void
- ✅ **Function declarations** - Complete with parameters, return types, and recursion
- ✅ **Function calls** - Hooc-to-hooc calls and external C function calls
- ✅ **Variable declarations** - Type inference and explicit annotations
- ✅ **All expressions** - Arithmetic, comparison, logical, assignment operators
- ✅ **Control flow** - If/else, while loops, for-range, for-in loops
- ✅ **Array literals** - Complete with type inference and multi-dimensional support
- ✅ **Array access** - Full element indexing with proper bounds handling
- ✅ **Type system** - Union types, optional types, array slice types
- ✅ **Scope statements** - Deterministic resource management

### **Parsed But Code Generation Incomplete**
- ⚠️ **String type** - Lexer/parser complete, LLVM generation pending
- ⚠️ **Classes & interfaces** - Grammar complete, AST building partial, code generation not started
- ⚠️ **Design patterns** - Keywords parsed, code generation planned for v0.3
- ⚠️ **Object instantiation** - `new` keyword parsed, not implemented
- ⚠️ **Type casting** - `as` keyword reserved, not implemented

### **Not Yet Implemented**
- ❌ **Advanced function features** - Function pointers, callbacks, method calls
- ❌ **Module system** - Import/export parsing works, resolution not implemented
- ❌ **Standard library** - No built-in functions beyond LLVM intrinsics

### **Technical Foundation**
- **LLVM Compatibility** - Tested with LLVM 21.1.8 (macOS), 15.0+ (Windows)
- **Parser Robustness** - Process-isolated ANTLR4 parsing prevents state corruption
- **Type Safety** - Strong, static typing with inference throughout
- **Architecture** - Clean abstract CodeGenerator interface supports multiple backends

## 🤝 Contributing

### **Development Workflow**
1. **Modify grammar** in `src/Hooc.g4`
2. **Build & test**: `cmake --build build && ./build/hooc --help`
3. **Update AST** in `src/ast/` if needed
4. **Test integration** with `./build/hooc example.hoo`

### **Priority Areas**
- **🔥 High**: Add string type support with LLVM integration
- **🔥 High**: Build standard library (print, I/O, collections)
- **📈 Medium**: Implement import/module system
- **📈 Medium**: Add advanced function features (pointers, callbacks, methods)
- **🧹 Low**: Enhance error diagnostics and add optimization passes

### **Getting Started with Development**
```bash
# 1. Run existing tests to verify setup
cmake --build build
./build/hoo_tests              # Run full test suite (88 tests)

# 2. Try compiling example programs
./build/hooc tests/examples/arithmetic.hoo
./build/hooc tests/examples/for_loops.hoo

# 3. Key files for development
src/LLVMCodeGenerator.cpp      # LLVM IR generation (concrete implementation)
src/CodeGenerator.h            # Abstract code generator interface
src/SimpleASTBuilder.cpp       # Parse tree to AST conversion
src/HooCompiler.cpp            # Main compilation pipeline
src/ast/                       # AST node definitions

# 4. Add new features and test
cmake --build build && ./build/hoo_tests
```

## 📈 Project Roadmap

### **v0.2 (Current) - ✅ Complete**
- ✅ Full ANTLR4 grammar with design patterns
- ✅ Complete AST → LLVM IR pipeline
- ✅ All primitive types (8 types) with proper semantics
- ✅ Comprehensive expression system with correct precedence
- ✅ Control flow: if/else, while, for-range, for-in, scope blocks
- ✅ Array literals with type inference and multi-dimensional support
- ✅ Function declarations and calls (hooc-to-hooc and external)
- ✅ Type system: unions, optionals, array slices
- ✅ 88 unit tests with 100% pass rate

### **v0.3 (Planned)**
- 🔧 **String type** - LLVM code generation for string literals and operations
- 🔧 **Classes & Objects** - Object instantiation with `new`, method calls
- 🔧 **Design Patterns** - Code generation for singleton, factory, observer
- 🔧 **Type Casting** - Implement `as` keyword for conversions
- 🔧 **Inheritance** - Single inheritance with `extends` keyword
- 🔧 **Interfaces** - Full interface implementation with `implements`
- 🔧 **Standard Library** - Core I/O and utility functions

### **v0.4+ (Future)**
- 📋 Module system with proper import resolution
- 📋 Advanced type features: generics (restricted), pattern matching
- 📋 Runtime features: reflection, serialization
- 📋 Performance: optimization passes, inline hints
- 📋 Tooling: LSP, debugger integration, formatter
- 📋 Self-hosting: hooc compiler written in hooc

## 📄 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

Built with modern compiler construction tools:
- **ANTLR4** for robust parsing infrastructure
- **LLVM** for world-class code generation  
- **CMake** for reliable build management

**Current Status (v0.2)**: All core features complete and fully tested. The compiler handles a substantial subset of the hoo language with 88 passing unit tests. Complete type system with inference, full expression evaluation, comprehensive control flow, array literals with multi-dimensional support, and robust function handling. Grammar includes design patterns and modern features. Ready for v0.3 with string types, classes, and object instantiation.

---

> *"hoo v0.2 demonstrates a mature compiler foundation: complete grammar, full type system, robust parsing, and comprehensive LLVM IR generation. With all primitive types, control flow, functions, and arrays fully implemented, the foundation is solid for adding classes, strings, and advanced features. The hooc compiler is production-ready for its supported feature set and positioned for rapid expansion into v0.3."*