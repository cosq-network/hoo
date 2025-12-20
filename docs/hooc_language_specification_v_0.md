# hooc Programming Language Specification (v0.1)

> **hooc** is a compiled, statically typed programming language designed as a modern, safe, and expressive evolution of the C programming philosophy.
>
> hooc removes C’s unsafe constructs while preserving its clarity, predictability, and performance mindset, and integrates modern abstractions directly into the language.

---

## 1. Language Philosophy

hooc is built on the following principles:

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

```hooc
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

```hooc
string name = "Benoy";
```

### 4.2 Multiline Strings

```hooc
string text = """
Hello,
This is a multiline string.
""";
```

- Preserves newlines
- Indentation normalized

### 4.3 String Interpolation

```hooc
string msg = "Hello ${user.name}, age ${user.age}";
```

- Expressions inside `${}` are type-checked
- Interpolation invokes `to_string()`

---

## 5. Optional Types

hooc does not support `null`.

```hooc
int64? value;
```

Rules:
- Optional values must be explicitly unwrapped
- Compiler enforces checks

---

## 6. Variables and Type Inference

```hooc
var x = 10;      // int64
var y = 3.14;    // double
var z = "hi";   // string
```

- Inference is local
- Type is fixed at declaration

---

## 7. Arrays and Collections

### 7.1 Arrays

```hooc
int64[3][4] matrix;
int[][] grid = new int[rows][cols];
```

Properties:
- Multi-dimensional arrays supported
- Bounds-checked access
- Row-major contiguous layout
- GC-managed

### 7.2 Iterable Model

All collections implicitly implement `Iterable<T>`.

---

## 8. Control Flow

### 8.1 Conditionals

```hooc
if condition {
    ...
} else {
    ...
}
```

Condition must be `bool`.

### 8.2 Iteration

#### For-each Loop

```hooc
for item in items {
    print(item);
}
```

#### Range-based Loop

```hooc
for i in 0..10 {
    print(i);
}
```

#### While Loop

```hooc
while condition {
    work();
}
```

---

## 9. Functions

```hooc
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

```hooc
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

```hooc
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

```hooc
interface Serializable {
    func serialize() -> string;
}
```

```hooc
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

```hooc
scope {
    Buffer b = Buffer(1024);
}
```

---

## 12. Error Handling

### 12.1 Union-Based Errors

```hooc
func read_file(string path) -> File | Error;
```

```hooc
file = read_file("data.txt") else {
    print("Failed to read file");
};
```

No unchecked exceptions.

---

## 13. Collections & Algorithms (Language-Level)

### 13.1 Searching

```hooc
users.find(u => u.id == 10);
```

Returns optional.

### 13.2 Sorting

```hooc
users.sort();
users.sort((a, b) => a.age < b.age);
```

- Stable
- In-place by default

### 13.3 Functional Operations

```hooc
users.filter(u => u.active);
users.map(u => u.name);
numbers.reduce(0, (a, b) => a + b);
```

---

### 13.4 List Comprehensions

```hooc
squares = [x * x for x in 1..10];
evans = [x for x in numbers if x % 2 == 0];
```

Nested comprehensions allowed.

---

## 14. Language-Integrated Design Patterns

### 14.1 Singleton

```hooc
singleton class Logger {
    func log(string msg);
}
```

### 14.2 Immutable Objects

```hooc
immutable class Money(double amount, string currency);
```

### 14.3 Factory

```hooc
factory class Shape {
    Circle(int r);
    Rectangle(int w, int h);
}
```

### 14.4 Observer

```hooc
observable class Button {
    event clicked;
}
```

### 14.5 Dependency Injection

```hooc
service class UserService { }
```

- Constructor injection only

### 14.6 Strategy

```hooc
strategy interface Payment {
    func pay(double amount);
}
```

### 14.7 Actor Model

```hooc
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

This document defines **hooc v0.1**, suitable as a foundation for compiler implementation and ecosystem design.

