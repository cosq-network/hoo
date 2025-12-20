# hooc Programming Language

> A modern, safe, and expressive programming language designed as an evolution of the C programming philosophy.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](.)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)]() 
[![Parser](https://img.shields.io/badge/parser-ANTLR4-orange)]()
[![Backend](https://img.shields.io/badge/backend-LLVM-red)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

## 🎯 Project Overview

**hooc** removes C's unsafe constructs while preserving its clarity, predictability, and performance mindset. It integrates modern abstractions directly into the language, including:

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

# Try the JIT compiler
./build/hooc_jit

# Test the parser
./build/hooc_parse "func hello() { return; }"
```

## 📖 Language Examples

### Hello World
```hooc
func main() {
    print("Hello, World!");
}
```

### Variables & Types
```hooc
func example() {
    int64 count = 42;
    string name = "hooc";
    bool active = true;
    
    print("Name: ${name}, Count: ${count}");
}
```

### Classes & Objects  
```hooc
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
```hooc
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
- **ANTLR4 Grammar** - Functional parser with process isolation
- **LLVM JIT Integration** - Runtime compilation and execution  
- **Complete AST Infrastructure** - Full language construct representation
- **Build System** - CMake with ANTLR4/LLVM integration
- **Parse Tree Generation** - Robust syntax analysis

### 🔧 **Current Architecture**
```
Source Code (.hooc)
     ↓
ProcessIsolatedParser (ANTLR4)  
     ↓
Parse Tree
     ↓  
ASTBuilder
     ↓
AST (CompilationUnit)
     ↓
[CodeGenerator] ← 🚧 IN PROGRESS
     ↓
LLVM IR Module
     ↓
LLVM ORC JIT
     ↓
Native Execution
```

### 🚧 **Next Milestones**
- **AST → LLVM IR Code Generation** (Critical)
- **Expression Grammar Expansion** 
- **Variable Declarations & Type System**
- **Function Calls & Module System**

## 🔧 Development Tools

### **hooc_jit** - JIT Compiler Demo
```bash
./build/hooc_jit
# Demonstrates LLVM compilation: add(15, 27) = 42
```

### **hooc_parse** - Standalone Parser  
```bash
./build/hooc_parse "func test() { 42; }"
# Output: SUCCESS + parse tree structure
```

## 📚 Documentation

- **[Language Specification](docs/hooc_language_specification_v_0.md)** - Complete language design
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

## 🐛 Current Limitations

### **Grammar Constraints**
- **Simplified expressions** - Basic arithmetic temporarily disabled to avoid parser overflow
- **Limited type syntax** - Full type system grammar being incrementally restored
- **No imports yet** - Module system disabled during core development

### **Missing Components** 
- **Code generation** - AST → LLVM IR bridge not yet implemented
- **Semantic analysis** - Type checking beyond syntax validation
- **Standard library** - Built-in functions and data structures

## 🤝 Contributing

### **Development Workflow**
1. **Modify grammar** in `src/Hooc.g4`
2. **Build & test**: `cmake --build build && ./build/hooc_parse "test"`
3. **Update AST** in `src/ast/` if needed
4. **Test integration** with `./build/hooc_jit`

### **Priority Areas**
- **CodeGenerator implementation** - Critical missing piece
- **Expression grammar expansion** - Restore arithmetic safely  
- **Error diagnostics** - Better compiler messages
- **Test suite development** - Comprehensive validation

## 📈 Project Goals

### **Short Term (Q1 2025)**
- Complete AST → LLVM IR code generation pipeline
- Basic arithmetic expressions working end-to-end
- Simple function definitions and calls
- Variable declarations with type inference

### **Medium Term (2025)**  
- Full hooc language feature support
- Standard library implementation
- Module system and import resolution
- Optimization passes and error recovery

### **Long Term**
- Self-hosting compiler (hooc written in hooc)
- Package manager and ecosystem
- IDE integration and tooling
- Production-ready compiler

## 📄 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

Built with modern compiler construction tools:
- **ANTLR4** for robust parsing infrastructure
- **LLVM** for world-class code generation  
- **CMake** for reliable build management

**Current Status**: Solid foundation complete, core compilation pipeline in active development.

---

> *"hooc represents a thoughtful evolution of C's philosophy - keeping the clarity and predictability while adding modern safety and expressiveness."*