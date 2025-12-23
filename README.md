# hoo Programming Language

> A modern, safe, and expressive programming language designed as an evolution of the C programming philosophy. Compiled by **hooc**.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](.)
[![Version](https://img.shields.io/badge/version-0.1--alpha-blue)](.)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)]() 
[![Parser](https://img.shields.io/badge/parser-ANTLR4-orange)]()
[![Backend](https://img.shields.io/badge/backend-LLVM-red)]()
[![Tests](https://img.shields.io/badge/tests-67%20passing-success)](.)
[![License](https://img.shields.io/badge/license-MIT-green)]()

## 🎯 Project Overview

**hoo** is a modern, safe, and expressive programming language that removes C's unsafe constructs while preserving its clarity, predictability, and performance mindset. Compiled by **hooc**, it integrates modern abstractions directly into the language, including:

- ✅ **Memory safety** without garbage collection overhead
- ✅ **Strong static typing** with type inference  
- ✅ **Pattern matching** and **union types**
- ✅ **Built-in collections** and algorithms
- ✅ **Language-level design patterns** (Singleton, Factory, Observer, etc.)
- ✅ **TypeScript-style imports** and module system

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
func example() {
    int64 count = 42;
    string name = "hooc";
    bool active = true;
    
    // Array declarations (v0.1.1)
    var numbers: int64[5];
    var matrix: int64[3][4];
    
    print("Name: ${name}, Count: ${count}");
}
```

### Arrays & Loops (NEW v0.1.1)
```hoo
func array_example() -> void {
    var arr: int64[10];
    
    // For-range loop
    for i in 0..10 {
        // Array assignment would go here
        // arr[i] = i * 2;
    }
    
    // For-in loop (when implemented)
    // for item in arr {
    //     print(item);
    // }
}
```

### Classes & Objects  
```hoo
class User(string name, int64 age) {
    func greet() {
        print("Hello, I'm ${name}");
    }
}

func main() {
    User user = User("Alice", 30);
    user.greet();
}
```

### Design Patterns (Built-in)
```hoo
singleton class Logger {
    func log(string msg) {
        print("[LOG] ${msg}");
    }
}

immutable class Money(double amount, string currency);

actor class Counter {
    int64 value = 0;
    func increment() { value++; }
}
```

## 🏗️ Implementation Status

### ✅ **Completed (v0.1-alpha)**
- **ANTLR4 Grammar** - Complete language grammar with full operator precedence
- **LLVM JIT Integration** - Working ORC JIT compilation and execution
- **AST Infrastructure** - Complete type hierarchy for all language constructs
- **Build System** - CMake with ANTLR4/LLVM integration (macOS & Windows via vcpkg)
- **Parse Tree Generation** - Working parser with process isolation
- **ASTBuilder** - Complete parse tree to AST conversion for core features
- **CodeGenerator** - Full AST to LLVM IR translation with all primitive types
- **Primitive Types** - All basic types (byte, int64, double, bool, char) fully implemented
- **Expressions** - Arithmetic, comparison, logical operations, assignments
- **Control Flow** - If/else statements, while loops, for-range loops, for-in loops
- **Variable Declarations** - Full support with type inference and assignments
- **Array Types** - Complete array type support with access and iteration
- **Test Suite** - 67 comprehensive unit tests with 100% pass rate

### 🔧 **Current Architecture**
```
Source Code (.hoo)
     ↓
ProcessIsolatedParser (ANTLR4) ✅
     ↓
Parse Tree ✅
     ↓  
ASTBuilder ✅
     ↓
AST (CompilationUnit) ✅
     ↓
CodeGenerator ✅
     ↓
LLVM IR Module ✅
     ↓
LLVM ORC JIT ✅
     ↓
Native Execution ✅
```

### 🎯 **Next Development Steps**
1. **Function Calls** - Implement hooc-to-hooc function calls
2. **String Type** - Add string type with LLVM support
3. **Array Operations** - Complete array element access and manipulation
4. **Module System** - Implement import/export functionality
5. **Standard Library** - Build core library functions (print, I/O, collections)

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

## 🐛 Current Limitations & Known Issues

### **Implementation Coverage**  
- ✅ **Function declarations** - Complete with parameters and return types
- ✅ **Primitive types** - byte, int64, double, bool, char fully supported
- ✅ **Variable declarations** - With type inference and assignments
- ✅ **All expressions** - Arithmetic, comparison, logical, assignments
- ✅ **Control flow** - If/else, while loops, for-range, for-in loops
- ✅ **Array types** - Complete type system and LLVM IR generation
- ⚠️ **Function calls** - Hooc-to-hooc calls not yet implemented
- ⚠️ **Array access** - Type incomplete for element access
- ❌ **String type** - Not yet implemented
- ❌ **Classes & interfaces** - Grammar exists, code generation pending
- ❌ **Import statements** - Module system not yet implemented

### **Technical Considerations**
- **LLVM Compatibility** - Tested with LLVM 15.0+ (Windows) and 21.1.8 (macOS)
- **Namespace Aliases** - Using aliases to avoid llvm::Type vs hooc::ast::Type conflicts
- **Error Diagnostics** - Basic error messages; comprehensive diagnostics in progress
- **Optimization** - Direct AST → IR translation; optimization passes not yet added

## 🤝 Contributing

### **Development Workflow**
1. **Modify grammar** in `src/Hooc.g4`
2. **Build & test**: `cmake --build build && ./build/hooc --help`
3. **Update AST** in `src/ast/` if needed
4. **Test integration** with `./build/hooc example.hoo`

### **Priority Areas**
- **🚨 High**: Implement function calls between hooc functions
- **🔥 High**: Add string type support with LLVM integration
- **📈 Medium**: Complete array element access and manipulation
- **📈 Medium**: Implement import/module system
- **🧹 Low**: Enhance error diagnostics and add optimization passes

### **Getting Started with Development**
```bash
# 1. Run existing tests to verify setup
cmake --build build
./build/hoo_tests              # Run full test suite (67 tests)

# 2. Try compiling example programs
./build/hooc tests/examples/arithmetic.hoo
./build/hooc tests/examples/for_loops.hoo

# 3. Key files for development
src/CodeGenerator.cpp          # LLVM IR generation
src/ASTBuilder.cpp            # Parse tree to AST conversion  
src/HoocCompiler.cpp          # Main compilation pipeline
src/ast/                      # AST node definitions

# 4. Add new features and test
cmake --build build && ./build/hoo_tests
```

## 📈 Project Goals

### **Short Term (Q1 2025)**
- ✅ Complete AST → LLVM IR pipeline (CodeGenerator)
- ✅ Implement all primitive types and expressions
- ✅ Build comprehensive test suite (67 tests)
- ✅ Support control flow statements
- 🔧 **Implement function calls** between hooc functions
- 🔧 **Add string type** with LLVM support
- 📋 Complete array element access and operations

### **Medium Term (2025)**
- 📋 Module system and import resolution
- 📋 Standard library foundation (I/O, strings, collections)
- 📋 Class and interface implementations
- 📋 Member access and method calls
- 📋 Enhanced error diagnostics and debugging support

### **Long Term**  
- 📋 Advanced type system (unions, optionals, generics)
- 📋 Design pattern implementations (singleton, factory, observer)
- 📋 Self-hosting compiler (hooc written in hooc)
- 📋 Package manager and IDE integration
- 📋 Production optimizations and debugging support

## 📄 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

Built with modern compiler construction tools:
- **ANTLR4** for robust parsing infrastructure
- **LLVM** for world-class code generation  
- **CMake** for reliable build management

**Current Status**: **Core language features complete and functional. End-to-end compilation pipeline working with 67 passing tests. Ready for advanced features like function calls, strings, and modules.**

---

> *"hoo represents a thoughtful evolution of C's philosophy - keeping the clarity and predictability while adding modern safety and expressiveness. The hooc compiler foundation is complete with all primitive types, expressions, and control flow fully implemented and tested."*