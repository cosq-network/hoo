# Hoo Project Understanding

**Last Updated**: 2026-08-12  
**Version**: 1.4.0  
**Status**: Production-ready with 2,117 passing tests

---

## 1. What is Hoo?

Hoo is a **high-performance, statically-typed systems programming language** designed for modern hardware. It compiles to a pure, physical-silicon-ready 64-bit RISC architecture called **HVM (Hoo Virtual Machine) v1.5**.

### Core Philosophy: "Hardware Purity"

Unlike traditional VMs (JVM, Python) that use high-level semantic bytecode, Hoo:
- Generates low-level RISC instructions with **zero abstraction overhead**
- Uses **aggressive compile-time lowering** to handle complex concepts (objects, arrays, exceptions) at compile time
- Maps cleanly to physical CPU designs—no "magic opcodes" for OOP or exceptions
- Compiles through LLVM ORC v2 JIT for high-fidelity dynamic binary translation

**Key Analogy**: If Java compiles to JVM bytecode, Hoo compiles to an ISA that could run on actual silicon with minimal changes.

---

## 2. Architecture Overview

### 2.1 Compilation Pipeline

```
Hoo Source Code (.hoo)
  ↓
[Parsing: ANTLR4 Grammar (Hooc.g4)]
  ↓
[AST Building: SimpleASTBuilder]
  ↓
[Type Analysis & Lowering: HVMCodeGenerator]
  ↓
[HVM Bytecode (.ho module)]
  ↓
[Archive Packing (.ha files with Zstd compression)]
  ↓
[JIT Compilation: HVMJIT (LLVM ORC v2)]
  ↓
[Native Code Execution]
```

### 2.2 Project Structure

```
src/
├── parsing/          # ANTLR4 grammar (Hooc.g4) + parser wrapper
├── ast/              # AST node definitions, type system, SimpleASTBuilder
├── codegen/          # HVMCodeGenerator (AST → bytecode)
├── hvm/              # ISA definitions, module serialization, HVMJIT engine
├── runtime/lib/      # hoort runtime library (24 modules: strings, arrays, maps, etc.)
├── core/             # CLI, compiler driver, symbol mangling, IO providers
├── repl/             # Interactive shell (--repl flag)
└── archive/          # Archive loading (.ha format), local imports

tests/
├── parsing/          # Grammar + AST construction tests
├── ast/              # Type system, expression, statement tests
├── codegen/          # HVM code generation tests
├── hvm/              # ISA instruction semantics, loader tests
├── runtime/          # ARC, string, array, map, threading tests
├── integration/      # End-to-end CLI + JIT tests
├── core/             # Symbol mangling, CLI flag parsing
└── repl/             # REPL session tests

docs/
├── hvm/              # ISA spec, instruction reference, file formats
├── dev/              # Developer guides (AST builder, codegen, etc.)
├── runtime/          # API documentation for hoort library
├── language/         # Language features (async/await)
└── issues/           # Feature tracking (ISSUE-001, etc.)
```

---

## 3. Key Components

### 3.1 Parser: ANTLR4-Based

**File**: `src/parsing/Hooc.g4`

Defines Hoo's complete grammar:
- **Declarations**: classes, functions, modules
- **Statements**: if/else, loops, try/catch, async/await
- **Expressions**: operators, method calls, array/map literals
- **Types**: primitives, arrays, maps, tensors, nullable types, generics

**Entry Point**: `compilationUnit` rule → `CompilationUnit` AST

**Lexer Rules**: Whitespace, keywords (let, var, class, fn, async, etc.), literals, operators

### 3.2 AST: Type-Safe Representation

**Files**: `src/ast/*.h` (Expression.h, Statement.h, Type.h, Declaration.h, etc.)

**Root**: `ast::CompilationUnit` (contains imports and declarations)

**Key Node Types**:
- **Declarations**: FunctionDeclaration, ClassDeclaration, VariableDeclarationStatement
- **Statements**: IfStatement, ForLoop, WhileLoop, TryCatchStatement, ThrowStatement
- **Expressions**: BinaryExpression, FunctionCall, ArrayAccess, MemberAccess
- **Types**: PrimitiveType, ArrayType, MapType, TensorType, OptionalType, FutureType

**Special Features**:
- **Type Inference** (Phase 8.2): `getTypeId()` automatically infers return types from function calls, class methods, and array subscripts
- **Nullable Types** (ISSUE-047): `T?` syntax with null-safety checks
- **Tensor Types** (Phase 10): `tensor<T>[N]` with rank-1/2/3 support
- **Serializable Classes** (Phase 11.3): `serializable` modifier for JSON round-tripping
- **Overloaded Functions** (Phase 13): Multiple function signatures with same name

### 3.3 Code Generator: AST → HVM Bytecode

**File**: `src/codegen/HVMCodeGenerator.cpp` (~6,600 lines)

**Responsibilities**:
1. **Register Allocation**: Maps variables to HVM registers (r0-r31), skipping r4 (thread pointer)
2. **Stack Frame Management**: Allocates local variable offsets, manages function prologue/epilogue
3. **Type ID Tracking**: Assigns numeric IDs to classes, primitives, arrays
4. **Instruction Emission**: Generates HVM bytecode with correct operand formats (R, I, J, B types)
5. **Symbol Resolution**: Maps global identifiers to relocations for linking
6. **Method Dispatch**: Converts high-level method calls to JIT function calls
7. **Object Layout**: Calculates field offsets, vtable indices for classes
8. **Exception Handling**: Generates shadow stack pushes/pops for try/catch

**Key Functions**:
- `visitFunctionDeclaration()` — emits function prologue, walks statement body
- `visitExpression()` — handles expression lowering (binary ops, calls, member access)
- `emitInstruction()` — appends encoded bytecode
- `bindLabel()`, `fixupLabel()` — manages forward references for control flow

### 3.4 HVM ISA: 64-Bit RISC Instruction Set

**Files**: `src/hvm/HVMInstruction.h`, `hvm-spec.md`

**Instruction Formats**:
- **R-Type**: `opcode rd, rs1, rs2` (3 operands, register-register operations)
- **I-Type**: `opcode rd, rs1, imm12` (2 operands + 12-bit immediate)
- **B-Type**: `opcode rs1, rs2, imm12` (2 operands + offset for branches)
- **J-Type**: `opcode rd, imm20` (register + 20-bit target)

**Key Opcodes**:
- Arithmetic: ADD, SUB, MUL, DIV, MOD
- Logic: AND, OR, XOR, NOT
- Comparison: CMP_LT, CMP_LE, CMP_EQ, CMP_NE, CMP_B (byte compare)
- Shifts: SHL, SHR, SAR
- Memory: LD, ST, LD.D (double-width load)
- Control: BEQ, BNE, JAL (jump-and-link), CALL, RET
- System: ECALL (exception call), SYSCALL (OS services)

**Physical Hardware Target**: The ISA is designed so that HVM bytecode could run on actual silicon with minimal changes. No VM-specific magic needed.

### 3.5 HVMJIT: LLVM ORC-Based JIT Execution Engine

**File**: `src/hvm/HVMJIT.cpp` (~10,000 lines)

**Responsibilities**:
1. **Bytecode Loading**: Deserializes `.ho` modules, sets up memory regions
2. **Symbol Resolution**: Registers runtime functions (alloc, retain, release, string ops, etc.)
3. **LLVM IR Generation**: Translates HVM instructions to LLVM IR for each function
4. **Compilation**: Uses LLVM ORC v2 to compile IR to native x86-64 or ARM64 code
5. **Dynamic Linking**: Manages inter-module calls, virtual table dispatch
6. **Exception Handling**: Implements shadow stack for non-local jumps (try/catch)
7. **Async/Await Support**: Integrates with libuv event loop for Future<T> semantics

**Runtime Symbols** (~100+):
- **Memory**: `jit_hoo_alloc`, `jit_hoo_retain`, `jit_hoo_release`
- **String**: `jit_hoo_string_concat`, `jit_hoo_string_length`, `jit_hoo_string_from_cstr`
- **Array**: `jit_hoo_array_new`, `jit_hoo_array_push`, `jit_hoo_array_sort`
- **Map**: `jit_hoo_map_new`, `jit_hoo_map_set`, `jit_hoo_map_get`
- **Tensor**: `jit_hoo_tensor_new`, `jit_hoo_tensor_matmul`, `jit_hoo_tensor_softmax`
- **Net**: `jit_hoo_net_socket_new`, `jit_hoo_net_socket_connect`, `jit_hoo_net_http_client_get`
- **Async**: `jit_hoo_future_new`, `jit_hoo_future_wait`, `jit_hoo_future_set_value`

### 3.6 Runtime Library: hoort

**Location**: `src/runtime/lib/` (24 specialized modules)

**Core Principle**: All managed objects are **64-bit handles** (pointers) to C++ instances with a **16-byte ARC header**:

```
[-16 bytes: atomic<int64> refcount] [ARC-managed instance] ← handle points here
[-8 bytes: int64 type_id        ]
[0 bytes: user data starts      ]
```

**Key Modules**:

| Module | Purpose | Type ID |
|--------|---------|---------|
| **string** | Immutable UTF-8 strings with ARC | 101 |
| **buffer** | Mutable byte array with dynamic resize | 113 |
| **array** | Generic typed array | 100 (objects) / type-specific |
| **map** | Type-safe key-value store | 102 |
| **tensor** | N-dimensional numeric arrays | 115 |
| **character** | Unicode character representation | 118 |
| **datetime** | Instantiable timestamp class | 119 |
| **future** | Async result container | 123 |
| **exception** | Error objects with shadow stack | 103 |
| **net** | TCP/UDP sockets, HTTP client, TLS | — |
| **fs** | File/directory operations (class-based API) | — |
| **json** | JSON serialization/deserialization | — |
| **regex** | Regular expression matching | — |
| **crypto** | Hashing (MD5, SHA256), UUID generation | — |
| **threading** | Mutexes, condition vars, semaphores | — |
| **math** | Abs, min, max, gcd, sqrt, trig, random | — |
| **encoding** | Base64, hex encoding/decoding | — |
| **compression** | Zstd, Gzip, Deflate | — |
| **csv** | CSV parsing/generation | — |
| **system** | Environment, process, CPU info | — |
| **datetime** | Timestamps, formatting, arithmetic | — |
| **decimal** | High-precision fixed-point numbers | — |

**Memory Model**: Automatic Reference Counting (ARC) with atomic operations. Every retain/release is thread-safe and concurrent GC-free.

---

## 4. Current Capabilities

### 4.1 Language Features

✅ **Type System**:
- Primitives: `int64`, `f64`, `bool`, `char`, `byte` (int8), `bit`
- Collections: `Array<T>`, `Map<K, V>`, `Tensor<T>[N]`
- Nullable: `T?` with null-safety checks
- Futures: `Future<T>` for async operations
- Custom classes with inheritance

✅ **OOP**:
- Classes with fields, methods, constructors
- Access qualifiers: `public`, `private`
- Modifiers: `serializable`, `final`, `sealed`
- Virtual methods with dynamic dispatch
- Function overloading (Phase 13)

✅ **Control Flow**:
- If/else, while, do-while loops
- For loops (C-style and for-in)
- Try/catch/finally exception handling
- Switch statements with pattern matching
- Async/await with Future<T> (Phase 15)

✅ **Advanced Features**:
- String interpolation: `"Hello, \(name)"`
- Array/map/tensor literals
- Object construction: `new ClassName(...)`
- Method chaining and operator overloading
- REPL interactive mode (--repl flag)
- Cross-file local imports + `.ha` archive format (Phase 14)
- Nullable type safety with compile-time checks (ISSUE-047)
- Tensor operations: element-wise ops, matrix multiplication, reshape, transpose, softmax (Phase 10)
- Async/await with libuv event loop integration (Phase 15)

### 4.2 Built-in Classes

Accessible via `new ClassName(...)` or class-qualified factory methods:

- `new String(...)` / `String.fromUtf8(buf)` / `String.join([...])`
- `new Buffer(capacity)` / `Buffer.fromBytes(data)`
- `new Array<T>(capacity)` / `Array<T>.empty()`
- `new Map<K, V>()` / `Map<K, V>.fromPairs([...])`
- `new Character(codepoint)` / `Character.fromUtf8(bytes)`
- `new DateTime(timestamp)` / `DateTime.now()` / `DateTime.parse(iso8601)`
- `new Tensor<T>[N]()` / `Tensor<T>[N].zeros()` / `Tensor<T>[N].ones()`
- `new Future<T>()` / `Future<T>.resolve(value)` / `Future<T>.reject(error)`

### 4.3 Modules & Imports

- **Local imports**: `import ./local_module;` (resolved at compile time)
- **Qualified imports**: `from package.submodule import Function, Class;`
- **Archive support**: Multi-module `.ha` files with ZIP+Zstd compression
- **Dependency resolution**: Automatic transitive ordering with cycle detection (ISSUE-036)

### 4.4 Async/Await (Phase 15)

Native `async`/`await` syntax with libuv-backed event loop:

```hoo
async fn fetchData(): Future<String> {
    let response = await httpClient.get("https://example.com/api/data");
    return response.body();
}

fn main(): void {
    let future = fetchData();
    // Awaits pump the event loop
}
```

### 4.5 Exception Handling

Try/catch with shadow stack mechanism:

```hoo
try {
    let result = riskyOperation();
} catch (e: Exception) {
    print("Error: " + e.message());
} finally {
    cleanup();
}
```

---

## 5. Recent Developments (2026-08-12)

### Phase 15: Async/Await via libuv
- Native `async`/`await` syntax with `Future<T>` values (type ID 123)
- Mutex-protected libuv event loop with cooperative waiting
- Full test coverage in `NewLanguageFeaturesTest.cpp` and `HooFutureJitTest.cpp`

### Phase 14: Archive Loading & Cross-File Imports
- `.ha` (Hoo Archive) format: ZIP-compatible with Zstd compression
- `HooArchiveLoader` for seamless multi-module loading into JIT
- `LocalImportResolver` for automatic dependency resolution
- Transitive topological ordering with cycle detection (ISSUE-036)

### Phase 11.3: Serializable Class Modifier
- `serializable` keyword for JSON round-tripping
- Tagged buffer/tensor JSON conversion
- Nested field lowering and cycle validation
- Full preset verification (2,117 tests pass)

### Phase 11.2: DateTime as Instantiable Class
- Converted from singleton API to ARC-managed class (type ID 119)
- Instance methods: `format()`, `addDays()`, `compare()`
- Factory dispatch: `DateTime.now()`, `DateTime.parse()`

### ISSUE-047: Nullable Type Safety
- End-to-end `T?` tracking with compile-time validation
- Catchable null dereference checks
- Distinct nullable overload mangling
- ARC cleanup for nullable objects in generic slots

### ISSUE-040: HVM 1.5 Spec Compatibility
- Native `CMP_B` (byte comparison) instruction
- CPU profile instructions (ICACHE.RNG, LD.P/ST.P, LR.D/SC.D)
- ECALL/TRAPRET/CSRRW for exception handling
- Module feature flags with loader validation
- HVM-V vector ISA expansion

---

## 6. Performance & Testing

### Test Coverage
- **2,520 tests** passing (0 failures)
- **Unit tests**: Parsing, AST, codegen, HVM instruction semantics, runtime (ARC, strings, arrays, etc.)
- **Integration tests**: CLI, JIT compilation, exception handling, async operations
- **End-to-end tests**: Collections, operators, literals, statements, arguments

### Build Variants
- **ninja-relwithdebinfo**: Default with debug symbols + optimizations
- **windows-vs18-env**: Windows MSVC toolchain with LLVM 22.1.4
- Multi-platform: macOS (Intel/Apple Silicon), Linux, Windows

### Performance Characteristics
- **Zero abstraction overhead**: Aggressive compile-time lowering
- **Lock-free ARC**: Atomic operations for thread safety
- **JIT optimization**: LLVM ORC v2 native code generation
- **Memory efficiency**: 16-byte overhead per managed object
- **Thread safety**: Fine-grained mutex striping (ISSUE-064/046)

---

## 7. File Organization Reference

### Source Files by Purpose

**Compiler Frontend**:
- `src/parsing/Hooc.g4` — Grammar definition
- `src/ast/SimpleASTBuilder.cpp` — AST construction from parse tree
- `src/core/HooCLI.cpp` — Command-line interface and file I/O

**Compilation Backend**:
- `src/codegen/HVMCodeGenerator.cpp` — AST to bytecode translation
- `src/hvm/HVMInstruction.cpp` — Instruction encoding/decoding
- `src/core/SymbolMangler.cpp` — Name mangling for linking

**Runtime & Execution**:
- `src/hvm/HVMJIT.cpp` — JIT compilation and execution engine
- `src/hvm/HOModule.cpp` — Binary module format serialization
- `src/runtime/lib/*.cpp` — 24 runtime modules (strings, arrays, etc.)

**Archive & Loading**:
- `src/archive/HAArchive.cpp` — ZIP+Zstd container handling
- `src/archive/HooArchiveLoader.cpp` — Multi-module loading
- `src/archive/LocalImportResolver.cpp` — Dependency resolution

**Interactive Features**:
- `src/repl/REPLSession.cpp` — REPL shell implementation

### Test Files by Category

**Language & Parsing**:
- `tests/parsing/*ParsingTest.cpp` — Grammar and AST tests
- `tests/ast/*Test.cpp` — Type system, expression, statement tests

**Code Generation & Execution**:
- `tests/codegen/HVMCodeGenerator*Test.cpp` — Bytecode generation
- `tests/hvm/HVMInstruction*Test.cpp` — Instruction semantics
- `tests/hvm/HVMJITLoaderTest.cpp` — Module loading and JIT

**Runtime Library**:
- `tests/runtime/Hoo*Test.cpp` — ARC, strings, arrays, maps, threading
- `tests/integration/jit/*JitTest.cpp` — JIT integration tests

**End-to-End**:
- `tests/integration/cli/HooCLIIntegrationTest.cpp` — CLI flag tests
- `tests/integration/primitive-types/*Test.cpp` — Type system integration
- `tests/integration/collections/CollectionIntegrationTest.cpp` — Collections

---

## 8. Development Workflow

### Building from Source

```bash
# Configure (CMake presets)
cmake --preset ninja-relwithdebinfo

# Build
cmake --build --preset ninja-relwithdebinfo

# Build tests
cmake --build --preset ninja-relwithdebinfo-tests

# Run tests
ctest --preset ninja-relwithdebinfo
```

### Typical Edit Cycle

1. **Grammar Change** → Update `src/parsing/Hooc.g4` → ANTLR regenerates parser
2. **AST Change** → Modify `src/ast/*.h` → Update `SimpleASTBuilder.cpp` → Rebuild
3. **Codegen Change** → Edit `HVMCodeGenerator.cpp` → Recompile → Test with `.hoo` files
4. **Runtime Change** → Update `src/runtime/lib/hoo_*.cpp` → Rebuild runtime library → Test

### Key Entry Points for Debugging

- **Parse failure**: Add breakpoint in `SimpleASTBuilder::buildAST()` or ANTLR `visitCompilationUnit()`
- **Codegen failure**: Set breakpoint in `HVMCodeGenerator::visitExpression()` or `visitStatement()`
- **Runtime failure**: Debug in HVMJIT.cpp's instruction execution or specific runtime function
- **Test failure**: Run single test with `ctest -R <test_name>` for detailed output

---

## 9. What's Next?

### Roadmap (Next Phases)

- **Phase 16**: Extended operator overloading and custom iterators
- **Phase 17**: Compile-time metaprogramming (generic specialization)
- **Physical Hardware**: FPGA Soft-Core implementation based on HVM spec

### Known Limitations & TODOs

- `LD.D.NZ` (load with zero-flag) optimization deferred (trap path not catchable)
- Standalone executable compilation (currently JIT only)
- GPU/SIMD vector operations (HVM-V expansion in progress)
- Standard library expansion (more algorithms, data structures)

---

## 10. Key Insights for Developers

### Design Principles

1. **Hardware Purity**: Every language feature must lower to HVM instructions that could run on real silicon
2. **Aggressive Lowering**: Complex semantics (OOP, exceptions) are resolved at compile time, not runtime
3. **ARC-First**: All managed objects use Automatic Reference Counting with atomic operations
4. **ISA Minimalism**: The instruction set contains only fundamental operations—no "magic" for objects or exceptions
5. **Specs-First**: Changes start with grammar updates, then lowering rules, then codegen and runtime

### Critical Files to Know

| File | Lines | Purpose |
|------|-------|---------|
| `HVMCodeGenerator.cpp` | 6,600 | The core compiler—99% of language features are codegen'd here |
| `HVMJIT.cpp` | 10,000 | JIT execution engine—all runtime symbol dispatch |
| `hoo_runtime.c` | 550 | Memory management (ARC, TLAB allocation) |
| `SimpleASTBuilder.cpp` | 1,500 | AST construction from parse tree |
| `HooParserWrapper.cpp` | 90 | Parser integration (tests use this) |
| `SymbolMangler.cpp` | 805 | Name mangling for linking (critical for debugging) |

### Common Debugging Techniques

**Symbol Mangling Issues**:
```cpp
// Print demangled symbol name
auto demangled = SymbolMangler::demangleSymbol(mangledName);
std::cout << demangled.module_path << "::" << demangled.class_name << "::" << demangled.function_name << std::endl;
```

**JIT Runtime Dispatch**:
```cpp
// Find which runtime symbol is being called
// Search HVMJIT.cpp for the mangled name in buildRuntimeSymbols()
// Look for jit_hoo_* function wrappers
```

**Bytecode Inspection**:
```cpp
// In tests, use jit->setLoaderState(...) to inspect generated LLVM IR
// HVMJIT.cpp emits debug output when tracing is enabled
```

---

## 11. References

- **ISA Spec**: `docs/hvm/hvm-spec.md`
- **Instruction Reference**: `docs/hvm/hvm_instruction_set.csv`
- **Module Format**: `docs/hvm/ho-file-format.md`
- **Dev Guides**: `docs/dev/` (codegen, AST builder, symbol mangler, etc.)
- **Runtime API**: `docs/runtime/api/` (24 module API documentation)
- **Language Features**: `docs/language/` (async/await, etc.)
- **Issues**: `docs/issues/` (ISSUE-001 through ISSUE-055)

---

## Summary

Hoo is a **systems programming language that compiles directly to a hardware-ready RISC ISA**, using aggressive compile-time lowering to eliminate abstraction overhead. It features:

- **Type Safety**: Static typing with inference, nullable types, function overloading
- **Modern OOP**: Classes, inheritance, access qualifiers, virtual methods
- **Async/Await**: Native coroutines with libuv integration
- **Zero Runtime Overhead**: All complex semantics resolved at compile time
- **Production Ready**: 2,117 tests passing, cross-platform (macOS, Linux, Windows)
- **Extensible**: 24 runtime modules covering strings, collections, networking, async, threading, JSON, regex, compression, and more

The codebase is well-structured, well-tested, and designed for clear separation of concerns: parser → AST → codegen → bytecode → JIT → native code. Every component has a clear responsibility, making the project maintainable and easy to extend.
