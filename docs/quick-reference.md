# hooc Compiler Quick Reference

**Language**: hoo  
**Compiler**: hooc  
**Current Build**: v0.1-alpha (December 2025)

---

## 🚀 Getting Started

### Prerequisites

#### macOS (Homebrew)
```bash
# Install dependencies via Homebrew
brew install cmake llvm antlr4-cpp-runtime

# Verify installations
cmake --version    # Should be 3.16+
llvm-config --version  # Should be 21.x
```

#### Windows
```cmd
# Install dependencies via vcpkg
cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
.\vcpkg install antlr4-cpp-runtime:x64-windows
.\vcpkg install gtest:x64-windows

# Install LLVM and Java (see full guide)
```

**📘 For detailed Windows setup**: See [building-on-windows.md](building-on-windows.md)

### Build the Compiler

#### macOS
```bash
# Clone and build
cd /Users/benoybose/Projects/hoo0.1
cmake --build build

# Expected output:
# [  7%] Built target generate_parser
# [ 53%] Built target hoo-parser
# [ 69%] Built target hooc
# [100%] Built target hoo-tests
```

#### Windows
```cmd
# Configure with CMake
cd D:\Projects\hooc\build
cmake -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="D:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  ..

# Build with Ninja
ninja
```

**📘 Full Windows build instructions**: [building-on-windows.md](building-on-windows.md)

---

## 🛠️ Available Tools

### 1. **hooc_jit** - JIT Compiler Demo
**Purpose**: Demonstrates LLVM ORC JIT compilation and execution

```bash
# macOS/Linux
./build/hooc_jit

# Windows
.\build\hooc_jit.exe
```

**Output Example**:
```
=== Hooc JIT Compiler Demo ===
HoocJIT initialized successfully!
Generated LLVM IR:
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b  
  ret i32 %sum
}
Module added to JIT successfully!
Result: add(15, 27) = 42

=== Testing ANTLR4 Parsing ===
Parsing Hooc code: "func test() { }"
Parse successful!
Parse tree children: 2
Parse tree: (compilationUnit ...)
```

### 2. **hooc_parse** - Standalone Parser
**Purpose**: Parse hooc source code and display syntax tree

```bash
# macOS/Linux
./build/hooc_parse "SOURCE_CODE"

# Windows
.\build\hooc_parse.exe "SOURCE_CODE"
```

**Examples**:
```bash
# Empty function (macOS/Linux)
./build/hooc_parse "func test() { }"

# Windows
.\build\hooc_parse.exe "func test() { }"

# Function with return
./build/hooc_parse "func calc() { return; }"

# Function with expression
./build/hooc_parse "func math() { 42; }"

# Function with parameters (limited support)
./build/hooc_parse "func add(n) { return; }"
```

**Output Format**:
```
SUCCESS
2
(compilationUnit (declaration (functionDeclaration func test ( ) (block { }))) <EOF>)
```

- Line 1: `SUCCESS` or `ERROR` or `FAILED`
- Line 2: Number of parse tree children
- Line 3: Parse tree in LISP-style notation

---

## 📝 Supported hooc Syntax

### ✅ **Currently Working**

#### Function Declarations
```hooc
func functionName() {
    // body
}
```

#### Return Statements  
```hooc
func example() {
    return;
}
```

#### Simple Expressions
```hooc
func numbers() {
    42;
    123;
}
```

#### Block Statements
```hooc
func nested() {
    {
        return;
    }
}
```

### ⚠️ **Limited Support**

#### Parameters (Basic)
```hooc
func withParam(n) {  // Type parsing is simplified
    return;
}
```

### ❌ **Not Yet Supported**

#### Arithmetic Operations
```hooc
// NOT YET WORKING
func math() {
    1 + 2;      // ❌ Complex expressions disabled  
    a * b;      // ❌ Binary operators not implemented
    func();     // ❌ Function calls disabled
}
```

#### Variable Declarations
```hooc
// NOT YET WORKING  
int64 x = 10;       // ❌ Temporarily disabled
var y = "hello";    // ❌ Type inference disabled
```

#### Import Statements
```hooc
// NOT YET WORKING
import { User } from "module";  // ❌ Disabled during debugging
```

---

## 🔍 Debugging & Troubleshooting

### Common Issues

#### Build Failures
```bash
# Missing ANTLR4
brew install antlr4-cpp-runtime

# Missing LLVM  
brew install llvm

# Clean build
rm -rf build && mkdir build && cmake -B build && cmake --build build
```

#### Parse Failures
```bash
# Check grammar compatibility
./build/hooc_parse "func test() { }"

# If getting "ERROR" - check syntax against current grammar limitations
```

#### JIT Execution Issues  
```bash
# Verify LLVM installation
llvm-config --version

# Check for proper linking in build output
cmake --build build --verbose
```

### Advanced Debugging

#### View Generated ANTLR4 Files
```bash
ls antlr4/generated/
# Should contain: HoocLexer.{h,cpp}, HoocParser.{h,cpp}, etc.
```

#### Manual Grammar Testing
```bash
# Generate fresh parser
cmake --build build --target generate_parser

# Test specific syntax
./build/hooc_parse "func example() { 42; }"
```

---

## 🏗️ Development Workflow

### Testing Changes

1. **Modify Grammar** (`src/Hooc.g4`)
2. **Rebuild Parser**:
   ```bash
   cmake --build build
   ```
3. **Test Parsing**:
   ```bash
   ./build/hooc_parse "func test() { }"
   ```
4. **Test JIT**:
   ```bash  
   ./build/hooc_jit
   ```

### Adding New Language Features

1. **Update Grammar** - Add new rules to `Hooc.g4`
2. **Extend AST** - Add corresponding AST node classes
3. **Update ASTBuilder** - Add visitor methods for new constructs
4. **Implement CodeGen** - Add LLVM IR generation (when ready)

---

## 📁 Key Files Reference

```
src/
├── Hooc.g4                    # ANTLR4 grammar definition
├── HoocJIT.{h,cpp}           # LLVM JIT integration  
├── ProcessIsolatedParser.*   # Parsing with crash protection
├── ASTBuilder.*              # Parse tree → AST conversion
├── main.cpp                  # JIT demo program
├── hooc_parse.cpp           # Standalone parser utility
└── ast/                     # AST class hierarchy
    ├── AST.h                # Main AST header
    ├── CompilationUnit.h    # Root AST node
    ├── Declaration.h        # Function/class/variable declarations
    ├── Expression.h         # Expression nodes
    ├── Statement.h          # Statement nodes  
    ├── Type.h              # Type system
    └── ...                 # Other AST components

docs/
├── hooc_language_specification_v_0.md  # Language design
├── hooc-sample-programs.md            # Example programs
├── implementation-status.md           # Current project status  
└── quick-reference.md                 # This file

CMakeLists.txt                         # Build configuration
antlr4/generated/                      # Generated parser files
build/                                 # Build output directory
```

---

## 🎯 Next Steps for Contributors

### **Immediate Priorities**
1. **Implement CodeGenerator** - Bridge AST → LLVM IR
2. **Restore Expression Grammar** - Add arithmetic operators carefully
3. **Integration Testing** - End-to-end compilation pipeline

### **Development Areas**
- **Language Features**: Variables, imports, classes  
- **Error Handling**: Better diagnostics and error recovery
- **Standard Library**: Built-in functions and types
- **Optimization**: LLVM optimization passes
- **Testing**: Comprehensive test suite

---

## 📞 Support & Documentation

- **Language Spec**: `docs/hooc_language_specification_v_0.md`
- **Implementation Status**: `docs/implementation-status.md`  
- **Sample Code**: `docs/hooc-sample-programs.md`
- **Source Code**: Well-commented C++ with clear architecture

**Current Status**: Solid foundation complete, ready for core compilation pipeline implementation.