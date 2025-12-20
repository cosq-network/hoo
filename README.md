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
- **ANTLR4 Grammar** - Complete language grammar with full operator precedence
- **LLVM JIT Integration** - Working ORC JIT with demo compilation (add function)
- **AST Infrastructure** - Complete type hierarchy for all language constructs
- **Build System** - CMake with ANTLR4/LLVM integration, all targets building
- **Parse Tree Generation** - Working parser with process isolation
- **SimpleASTBuilder** - Direct parse tree to AST conversion (basic features)
- **CodeGenerator Foundation** - AST to LLVM IR framework with namespace resolution

### 🔧 **Current Architecture**
```
Source Code (.hoo)
     ↓
ProcessIsolatedParser (ANTLR4)  
     ↓
Parse Tree (validated ✅)
     ↓  
SimpleASTBuilder (partial ⚠️)
     ↓
AST (CompilationUnit)
     ↓
CodeGenerator (foundation ✅)
     ↓
LLVM IR Module
     ↓
LLVM ORC JIT (working ✅)
     ↓
Native Execution ✅
```

### 🚧 **Integration Gaps**
- **Parse Tree Access** - ProcessIsolatedParser doesn't expose trees for AST building
- **Expression Support** - SimpleASTBuilder handles only primary expressions  
- **Binary Operations** - Grammar supports full expressions, builder doesn't implement them
- **Pipeline Connection** - Manual integration needed in HoocJIT::compileHoocCode()

### 🎯 **Next Critical Steps**
1. **Connect parsing → AST pipeline** (expose parse trees)
2. **Implement binary expressions** in SimpleASTBuilder
3. **Complete end-to-end compilation** for basic arithmetic functions
4. **Add missing AST node types** (BinaryExpression, FunctionCall, etc.)

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

## 🐛 Current Limitations & Known Issues

### **Integration Gaps**
- **Parse Tree Isolation** - ProcessIsolatedParser validates but doesn't expose parse trees
- **AST Building Incomplete** - SimpleASTBuilder covers ~20% of grammar features  
- **Manual Pipeline** - No automated connection from parsing to AST to IR
- **Missing Node Types** - AST lacks BinaryExpression, FunctionCall, UnaryMinus classes

### **Implementation Coverage**  
- ✅ **Function declarations** (basic)
- ✅ **Primary expressions** (identifiers, literals)
- ✅ **Return statements** and simple blocks
- ⚠️ **Binary operations** (grammar only, no AST support)
- ❌ **Variable declarations** not implemented
- ❌ **Control flow** (if/for/while) not implemented  
- ❌ **Type system** (unions, optionals, arrays) incomplete

### **Technical Debt**
- **Deprecated LLVM APIs** - Using PointerType::get instead of context-based APIs
- **Namespace Conflicts** - Workarounds needed for llvm::Type vs hooc::ast::Type
- **Error Handling** - Limited diagnostic information for compilation failures

## 🤝 Contributing

### **Development Workflow**
1. **Modify grammar** in `src/Hooc.g4`
2. **Build & test**: `cmake --build build && ./build/hooc --help`
3. **Update AST** in `src/ast/` if needed
4. **Test integration** with `./build/hooc example.hoo`

### **Priority Areas**
- **🚨 Critical**: Connect ProcessIsolatedParser to SimpleASTBuilder (parse tree access)
- **🔥 High**: Implement binary expressions and basic arithmetic in AST builder
- **📈 Medium**: Add missing AST node types (BinaryExpression, FunctionCall, UnaryMinus)
- **🧹 Low**: Update deprecated LLVM APIs and improve error messages

### **Getting Started with Development**
```bash
# 1. Understand current pipeline by running demos
./build/hooc example.hoo     # Complete compilation pipeline
./build/test_codegen      # See working AST → LLVM IR generation

# 2. Key files for immediate work
src/ProcessIsolatedParser.cpp  # Add parse tree access method
src/SimpleASTBuilder.cpp       # Expand expression support  
src/HoocJIT.cpp               # Connect the complete pipeline

# 3. Test changes
cmake --build build && ./build/hooc example.hoo
```

## 📈 Project Goals

### **Short Term (Q1 2025)**
- ✅ Complete AST → LLVM IR foundation (CodeGenerator class)
- 🔧 **Bridge parsing and AST building** (expose parse trees from ProcessIsolatedParser)
- 🔧 **Implement binary expressions** in SimpleASTBuilder 
- 🔧 **Connect end-to-end pipeline** in HoocJIT for basic functions
- 📋 Add comprehensive error diagnostics and validation

### **Medium Term (2025)**
- 📋 Complete expression support (function calls, unary ops, member access)
- 📋 Variable declarations with type inference and validation
- 📋 Control flow statements (if, for, while) with proper code generation
- 📋 Module system and import resolution
- 📋 Standard library foundation (print, basic data structures)

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

**Current Status**: **Solid foundation with working components, critical integration gap blocking end-to-end compilation. Architecture is sound, implementation ~70% complete.**

---

> *"hooc represents a thoughtful evolution of C's philosophy - keeping the clarity and predictability while adding modern safety and expressiveness. The compiler foundation is strong; the missing piece is connecting parsing to AST building."*