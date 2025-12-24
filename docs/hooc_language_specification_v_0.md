# hoo Programming Language Specification (v0.2)

> **hoo** is a compiled, statically typed programming language designed as a modern, safe, and expressive evolution of the C programming philosophy. Compiled by **hooc**.
>
> hoo removes C's unsafe constructs while preserving its clarity, predictability, and performance mindset, and integrates modern abstractions directly into the language.

---

## 1. Language Philosophy

hoo is built on the following principles:

- **Safety by construction** – no undefined behavior, no pointer arithmetic, no null
- **Simplicity over cleverness** – minimal syntax, few keywords, explicit behavior
- **Predictable performance** – ahead-of-time compilation, no hidden costs
- **Language-level power** – patterns, collections, and algorithms are built-in
- **C-like mental model** – straightforward control flow and data layout

Non-goals:
- Macro systems
- Template metaprogramming
- Multiple inheritance
- Reflection-heavy runtimes
- Implicit behavior

---

## 2. Compilation Model & Modules

### 2.1 Source Files and Modules

- One source file defines one module
- Module name is derived from file path
- No header files, no forward declarations

### 2.2 Import System (TypeScript Style)

``` hoo
import { User, Role } from "auth/user";
import * as math from "core/math";
import "net/http"; // side-effect import
```

Rules:
- Imports are resolved statically
- Circular dependencies are compile-time errors
- No textual inclusion

---

## 3. Type System Overview

hooc uses a **strong, static, non-nullable** type system.

### 3.1 Primitive Types

| Type | Alias | Description |
|-----|------|------------|
| `byte` | `uint8` | Unsigned 8-bit integer |
| `int64` | — | Signed 64-bit integer (default integer) |
| `double` | `f64` | 64-bit IEEE-754 floating point |
| `bool` | — | Boolean value |
| `char` | — | Unicode scalar (UTF-32) |
| `string` | — | UTF-8 immutable string |

Rules:
- No implicit narrowing
- No numeric-to-boolean coercion
- Arithmetic overflow is checked by default

**Implementation Status**: All primitive types (`byte`, `int64`, `double`, `bool`, `char`) are fully implemented with comprehensive test coverage (67 unit tests) in the current hooc compiler v0.1.

---

## 4. Strings

### 4.1 Single-Line Strings

``` hoo
string name = "Benoy";
```

### 4.2 Multiline Strings

``` hoo
string text = """
Hello,
This is a multiline string.
""";
```

- Preserves newlines
- Indentation normalized

### 4.3 String Interpolation

``` hoo
string msg = "Hello ${user.name}, age ${user.age}";
```

- Expressions inside `${}` are type-checked
- Interpolation invokes `to_string()`

---

## 5. Optional Types

hooc does not support `null`.

``` hoo
int64? value;
```

Rules:
- Optional values must be explicitly unwrapped
- Compiler enforces checks

---

## 6. Variables and Type Inference

``` hoo
var x = 10;      // int64
var y = 3.14;    // double
var z = "hi";   // string
```

- Inference is local
- Type is fixed at declaration

---

## 7. Arrays and Collections

### 7.1 Arrays

Arrays are created using literal syntax with automatic type inference:

``` hoo
// Type inference from elements
var numbers = [1, 2, 3, 4, 5];          // int64[]
var floats = [1.0, 2.5, 3.14];          // double[]
var flags = [true, false, true];        // bool[]

// Explicit type annotation
var data: int64[] = [10, 20, 30, 40, 50];

// Multi-dimensional arrays (nested syntax)
var matrix = [[1, 2, 3], [4, 5, 6]];    // int64[][]
var cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];  // int64[][][]

// Empty arrays require explicit type
var empty: int64[] = [];

// Array access
var value = numbers[2];  // Returns 3
```

Properties:
- **Type Inference**: Element type automatically inferred from first element
- **Multi-dimensional**: Supports nested array literals with arbitrary depth
- **Bounds-checked access**: Array indexing validated at runtime
- **Memory Model**: Array literals stored as LLVM global constants in .rodata section
- **Uniform Types**: All elements must have the same type (enforced at compile time)
- **Compile-time Constants**: Array literal elements must be constant expressions

**Function Parameters**: Arrays are passed as slices (unsized array types):

``` hoo
func process(arr: int64[]) -> void {
    for item in arr {
        print(item);
    }
}

func main() {
    var numbers = [1, 2, 3, 4, 5];
    process(numbers);
}
```

**Removed Syntax (v0.2)**: Fixed-size array type declarations are no longer supported:

``` hoo
// ❌ NO LONGER SUPPORTED:
var numbers: int64[5];
var matrix: int64[3][4];
```

**Implementation Status (v0.2)**: Array literals are fully implemented with complete type inference, multi-dimensional support, LLVM global constant generation, and array element access.

### 7.2 Iterable Model

All collections implicitly implement `Iterable<T>` (planned feature).

---

## 8. Control Flow

### 8.1 Conditionals

``` hoo
if condition {
    ...
} else {
    ...
}
```

Condition must be `bool`.

### 8.2 Iteration

#### For-each Loop

``` hoo
for item in items {
    print(item);
}
```

**Implementation Status (v0.1.1)**: AST building and LLVM IR generation fully implemented. Requires array access syntax for practical usage.

#### Range-based Loop

``` hoo
for i in 0..10 {
    print(i);
}
```

**Implementation Status (v0.1.1)**: Complete infrastructure implemented with proper basic block generation in LLVM IR.

#### While Loop

``` hoo
while condition {
    work();
}
```

**Implementation Status**: Not yet implemented.

---

## 9. Functions

``` hoo
func add(int64 a, int64 b) -> int64 {
    return a + b;
}
```

Rules:
- Explicit return types
- No overloading
- No variadic functions (v0.x)

---

## 10. Classes and Objects

### 10.1 Class Declaration

``` hoo
class User(string name, int64 age) {
    func greet() {
        print("Hello " + name);
    }
}
```

- Dart-style primary constructors
- Fields auto-declared
- GC-managed instances

---

### 10.2 Inheritance

``` hoo
class Admin extends User {
    int64 level;
}
```

Rules:
- Single inheritance only
- Methods virtual by default
- `final` prevents overriding

---

### 10.3 Interfaces

``` hoo
interface Serializable {
    func serialize() -> string;
}
```

``` hoo
class User implements Serializable {
    func serialize() -> string { ... }
}
```

- Interfaces have no fields
- Multiple interfaces allowed

---

## 11. Memory Management

- No pointers
- No manual allocation
- Garbage collection
- Deterministic lifetime via `scope`

``` hoo
scope {
    Buffer b = Buffer(1024);
}
```

---

## 12. Error Handling

### 12.1 Union-Based Errors

``` hoo
func read_file(string path) -> File | Error;
```

``` hoo
file = read_file("data.txt") else {
    print("Failed to read file");
};
```

No unchecked exceptions.

---

## 13. Collections & Algorithms (Language-Level)

### 13.1 Searching

``` hoo
users.find(u => u.id == 10);
```

Returns optional.

### 13.2 Sorting

``` hoo
users.sort();
users.sort((a, b) => a.age < b.age);
```

- Stable
- In-place by default

### 13.3 Functional Operations

``` hoo
users.filter(u => u.active);
users.map(u => u.name);
numbers.reduce(0, (a, b) => a + b);
```

---

### 13.4 List Comprehensions

``` hoo
squares = [x * x for x in 1..10];
evans = [x for x in numbers if x % 2 == 0];
```

Nested comprehensions allowed.

---

## 14. Language-Integrated Design Patterns

### 14.1 Singleton

``` hoo
singleton class Logger {
    func log(string msg);
}
```

### 14.2 Immutable Objects

``` hoo
immutable class Money(double amount, string currency);
```

### 14.3 Factory

``` hoo
factory class Shape {
    Circle(int r);
    Rectangle(int w, int h);
}
```

### 14.4 Observer

``` hoo
observable class Button {
    event clicked;
}
```

### 14.5 Dependency Injection

``` hoo
service class UserService { }
```

- Constructor injection only

### 14.6 Strategy

``` hoo
strategy interface Payment {
    func pay(double amount);
}
```

### 14.7 Actor Model

``` hoo
actor class Queue {
    int[] data;
}
```

- Single-threaded execution
- Message-driven

---

## 15. Concurrency Model

- No shared mutable state by default
- Actors for concurrency
- Compiler-enforced safety

---

## Implementation Status (v0.2)

### ✅ Fully Implemented
- **All Primitive Types**: `byte`, `int64`, `double`, `bool`, `char` with comprehensive testing
- **Array Literals**: Complete array literal syntax with type inference, multi-dimensional support, and global constant storage
- **Array Access**: Full support for array element access with `arr[index]` syntax
- **Variable Declarations**: Complete support with type inference and explicit type annotations
- **Expression System**: All arithmetic, comparison, logical, and assignment expressions
- **Control Flow**: if/else statements, while loops, for-range loops, for-in loops
- **For Loop Infrastructure**: Both for-in and for-range with proper LLVM IR generation
- **Function Declarations**: Complete function support with parameter handling and return types
- **Function Calls**: Full hooc-to-hooc function calls with argument passing and return values
- **LLVM Integration**: Modern API compatibility and robust code generation

### ⚠️ Partially Implemented
- **Advanced Function Features**: Function pointers, callbacks, and method calls not yet implemented

### ❌ Not Yet Implemented
- **String Type**: Not yet implemented in code generator
- **Classes & Objects**: Grammar exists but AST building and code generation incomplete
- **Module System**: Import/export functionality planned
- **Advanced Types**: Union types, optionals, generics
- **Design Patterns**: Language-level pattern support planned

### Test Coverage
**12 array literal parsing tests passing** validating SimpleASTBuilder's parsing capabilities for array literals, type inference, multi-dimensional arrays, and function parameters with slice syntax.

---

## 16. Equality and Comparison

- Value-based equality by default
- No reference equality
- Explicit numeric conversions

---

## 17. Compilation Targets

- Native AOT binaries
- Optional C ABI interoperability
- Optional WebAssembly backend

---

## 18. Roadmap Notes

Planned (post v0.1):
- Generics (restricted)
- Parallel iteration
- Optional region-based GC
- Package manager

---

## 19. Summary

**hooc** is a pragmatic, safe, and expressive language that modernizes C without inheriting the complexity of C++, the verbosity of Java, or the cognitive overhead of Rust.

This document defines **hooc v0.2**, suitable as a foundation for compiler implementation and ecosystem design.


