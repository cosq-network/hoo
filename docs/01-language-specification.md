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

### 2.2 Import System (Python Style)

**Basic Import:**
``` hoo
import math;
import core.utils as utils;
```

**Selective Import (from-import):**
``` hoo
from auth.user import User, Role;
from core.math import add, multiply;
```

Rules:
- Module paths use dot notation (e.g., `core.utils`)
- Imports are resolved statically
- Circular dependencies are compile-time errors
- Optional `as` clause for aliasing imported items
- No textual inclusion

---

## 3. Type System Overview

hooc uses a **strong, static, non-nullable** type system.

### 3.1 Primitive Types

| Type | Alias | Description |
|-----|------|------------|
| `byte` | `uint8` | Unsigned 8-bit integer |
| `int64` | — | Signed 64-bit integer (default integer) |
| `float` | — | 32-bit IEEE-754 floating point |
| `double` | `f64` | 64-bit IEEE-754 floating point |
| `bool` | — | Boolean value |
| `char` | — | Unicode scalar (UTF-32) |
| `string` | — | UTF-8 immutable string |
| `void` | — | No value (function return type only) |

Rules:
- No implicit narrowing
- No numeric-to-boolean coercion
- Arithmetic overflow is checked by default

**Implementation Status**: Primitive types (`byte`, `uint8`, `int64`, `double`, `float`, `bool`, `char`) are implemented with comprehensive test coverage (88 unit tests) in the current hooc compiler v0.2. The `string` type is parsed but not yet fully implemented in code generation.

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

### 5.1 Type Casting (Planned)

The `as` keyword is reserved for explicit type casting in future versions:

``` hoo
var i: int64 = 42;
var f: double = i as double;  // Planned: explicit type conversion
```

**Implementation Status**: Reserved keyword. Not yet implemented in code generation.

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

**Implementation Status (v0.2)**: Grammar defined and AST building implemented. LLVM IR code generation complete with proper loop control flow.

---

## 9. Functions

``` hoo
// Function with explicit return type
func add(a: int64, b: int64) -> int64 {
    return a + b;
}

// Function without return type (defaults to void)
func printMessage(msg: string) {
    // No return statement needed
}

// Explicit void return type (equivalent to omitting it)
func doSomething() -> void {
    // Do something
}
```

Rules:
- Return types are optional - functions without a return type default to void
- Explicit return types can be specified with `-> type`
- No overloading
- No variadic functions (v0.x)

---

## 10. Classes and Objects

### 10.1 Class Declaration

``` hoo
class User {
    constructor(name: string, age: int64) {
        // Constructor body
    }

    func greet() {
        print("Hello " + name);
    }
}

// Object instantiation (planned)
var user = new User("Alice", 30);
```

- Kotlin-style constructors (single constructor allowed per class)
- Constructor parameters use modern syntax (name: type)
- GC-managed instances
- `new` keyword for explicit instantiation (currently in grammar, code generation planned)

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

## 11. Generics

hoo supports **C#-style generic parameters** for both classes and functions using **monomorphization** (compile-time specialization). This enables writing reusable, type-safe code without sacrificing performance.

### 11.1 Generic Classes

``` hoo
// Generic class with single type parameter
class Box<T> {
    constructor() {}

    func get() -> int64 {
        return 0;
    }
}

// Generic class with multiple type parameters
class Pair<K, V> {
    constructor() {}

    func setKeyValue(key: K, value: V) -> void {
        // Implementation
    }
}

// Usage with type arguments
func main() {
    var intBox = new Box<int64>();
    var doubleBox = new Box<double>();
    var stringDoubleBox = new Pair<string, double>();
}
```

**Key Features:**
- Type parameters denoted with angle brackets: `<T>`, `<K, V>`
- Each type instantiation generates specialized code
- Type safety enforced at compile time
- No runtime overhead (monomorphization)
- Supports nested generic types: `Box<Box<int64>>`

### 11.2 Generic Functions

``` hoo
// Generic function with single type parameter
func identity<T>(value: T) -> int64 {
    return 0;
}

// Generic function with multiple type parameters
func swap<T, U>(a: T, b: U) -> void {
    // Implementation
}

// Usage with explicit type arguments
func main() {
    var result1 = identity<int64>(42);
    var result2 = identity<string>("hello");

    var x: int64 = 5;
    var y: double = 2.5;
    swap<int64, double>(x, y);
}
```

**Key Features:**
- Type parameters in angle brackets before parameter list
- Explicit type arguments required at call site
- Full type substitution in function body
- Type parameter can appear in parameters and return type

### 11.3 Implementation Details

**Monomorphization Strategy:**
- Generic templates stored without immediate code generation
- Code generated on-demand when a type instantiation is encountered
- Each unique type combination gets its own specialized version
- Example: `Array<int64>` and `Array<string>` are compiled separately

**Name Mangling:**
Generic types are converted to unique names for code generation:
- `Box<int64>` → `Box_int64`
- `Pair<string, double>` → `Pair_string_double`
- `swap<int64, double>` → `swap_int64_double`

**Type Parameter Scope:**
Type parameters are resolved during code generation using a stack-based scope:
- Type parameters pushed when instantiation begins
- Type names resolved in method bodies and constructors
- Scope popped after instantiation completes
- Supports arbitrarily nested generic types

### 11.4 Restrictions and Future Work

**Current (v0.6):**
- No type constraints (all types accepted)
- Type parameters appear only in class fields and function bodies (inferred or substituted)
- Instantiation checked at compile time

**Planned (v1.0+):**
- Type constraints: `class Container<T: Serializable> { ... }`
- Variance annotations (covariance, contravariance)
- Higher-ranked types
- Associated types

**Implementation Status (v0.6)**: Fully implemented with comprehensive test coverage (49 unit tests covering generic classes, functions, nested types, and error conditions).

---

## 12. Memory Management

- No pointers
- No manual allocation
- Garbage collection
- Deterministic lifetime via `scope`

### 12.1 Scope Statement

The `scope` statement creates a deterministic scope block where resources can be explicitly managed:

``` hoo
scope {
    var buffer = Buffer(1024);
    // buffer is guaranteed to be cleaned up when scope exits
}
// buffer no longer valid here
```

Rules:
- Variables declared within a scope are bound to that scope
- Scope exit triggers resource cleanup (when needed)
- Scope can be nested
- Enables RAII-style patterns without ownership transfer

---

## 13. Error Handling

### 13.1 Union-Based Errors

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

## 14. Collections & Algorithms (Language-Level)

### 14.1 Searching

``` hoo
users.find(u => u.id == 10);
```

Returns optional.

### 14.2 Sorting

``` hoo
users.sort();
users.sort((a, b) => a.age < b.age);
```

- Stable
- In-place by default

### 14.3 Functional Operations

``` hoo
users.filter(u => u.active);
users.map(u => u.name);
numbers.reduce(0, (a, b) => a + b);
```

---

### 14.4 List Comprehensions

``` hoo
squares = [x * x for x in 1..10];
evans = [x for x in numbers if x % 2 == 0];
```

Nested comprehensions allowed.

---

## 15. Language-Integrated Design Patterns

The hoo grammar supports language-level design pattern keywords. These modifiers are parsed but code generation is not yet implemented (planned for future versions).

### 15.1 Singleton

``` hoo
singleton class Logger {
    func log(string msg);
}
```

Guarantees single instance throughout program lifetime.

### 15.2 Immutable Objects

``` hoo
immutable class Money(double amount, string currency);
```

All fields are immutable after construction.

### 15.3 Factory

``` hoo
factory class Shape {
    Circle(r: int64);
    Rectangle(w: int64, h: int64);
}
```

Multiple named constructors for different object creation patterns. (Future feature - currently only single constructor per class is supported)

### 15.4 Observer

``` hoo
observable class Button {
    event clicked;
}
```

Built-in event system with automatic subscriber management.

### 15.5 Dependency Injection

``` hoo
service class UserService { }
```

Constructor injection only. Compiler manages dependency resolution.

### 15.6 Strategy

``` hoo
strategy interface Payment {
    func pay(amount: double);
}
```

Explicitly marks interfaces as strategy patterns for compiler optimization.

### 15.7 Actor Model

``` hoo
actor class Queue {
    int[] data;
}
```

- Single-threaded execution
- Message-driven communication
- Automatic isolation of mutable state

**Implementation Status (v0.2)**: All pattern keywords are recognized by the grammar and can be parsed. AST building partially supports class declarations. Full code generation for design patterns planned.

---

## 16. Concurrency Model

- No shared mutable state by default
- Actors for concurrency
- Compiler-enforced safety

---

## Implementation Status (v0.6)

### ✅ Fully Implemented
- **Primitive Types**: `byte`, `uint8`, `int64`, `float`, `double` (f64), `bool`, `char`
- **Array Literals**: Complete syntax with type inference, multi-dimensional support, global constant storage
- **Array Access**: Full element access with `arr[index]` syntax
- **Variable Declarations**: Type inference and explicit annotations
- **Expression System**: All arithmetic, comparison, logical, and assignment operators with correct precedence
- **Control Flow**: if/else statements, while loops, for-range loops (`for i in 0..10`), for-in loops (`for item in arr`)
- **Function Declarations**: Parameters, return types, body scoping
- **Function Calls**: Hooc-to-hooc calls with argument passing and return values
- **External C Functions**: Basic FFI support (e.g., `printf`)
- **Type System**: Union types (`T | U`), optional types (`T?`), array slice types (`T[]`)
- **Scope Statements**: Deterministic resource management via `scope { ... }`
- **Classes and Objects**: Full support for class declarations with constructors, member variables, and method calls
- **Object Creation**: `new` keyword for instantiation with automatic reference counting (ARC)
- **Member Access**: Reading class fields via `.` operator
- **Method Calls**: Invoking methods on object instances with implicit `this` pointer
- **Generics**: Full C#-style generics with monomorphization for classes and functions (49 unit tests)
  - Generic class declarations: `class Box<T> { ... }`
  - Generic function declarations: `func identity<T>(value: T) -> T { ... }`
  - Nested generic types: `Box<Box<int64>>`
  - Multiple type parameters: `Pair<K, V>`
  - Type argument inference and validation
  - Name mangling: `Box<int64>` → `Box_int64`
- **Runtime Class Registration**: Callback-based registration system for runtime types
  - JIT registration callbacks for symbol registration
  - LLVM declaration callbacks for function declarations
  - Self-registering runtimes via static initialization and HOOC_REGISTER_RUNTIME macro
  - Full control over registration logic by runtime developers
  - Zero coupling between compiler and specific runtime types

### ⚠️ Partially Implemented
- **String Type**: Full parsing, LLVM generation, and 30+ runtime functions via callback-based runtime injection framework
- **Array Type**: Full parsing, LLVM generation, and 20+ runtime functions via callback-based runtime injection framework
- **Design Pattern Keywords**: All keywords parsed (singleton, immutable, factory, observable, service, strategy, actor), code generation not yet implemented

### ❌ Not Yet Implemented
- **Type Casting**: `as` keyword reserved but not implemented
- **Class Inheritance**: `extends` parsed but not implemented
- **Interface Implementation**: `implements` parsed but not implemented
- **Event System**: `event` keyword parsed but event handling not implemented
- **Module System**: Import/export parsing works, module resolution not implemented
- **String Interpolation**: Placeholder in grammar
- **Pattern Matching**: Not in current grammar
- **Type Constraints**: Generic constraints like `<T: Serializable>` planned for v1.0+

### Test Coverage
**577 comprehensive unit tests** across parsing, AST building, and code generation. Tests validate:
- Generic class instantiation and code generation (12 tests)
- Generic function instantiation and code generation (12 tests)
- Generic integration scenarios (10 tests)
- Generic error handling and validation (15 tests)
- Member access and method calls (25 tests)
- Class and object creation (32 tests)
- String operations (65 tests)
- Array operations, control flow, and expression evaluation
- LLVM IR generation for all supported features
- Zero regressions from previous implementations

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

### v0.3-v0.6 (Completed)
- ✅ Complete class and object implementation
- ✅ String type code generation with 30+ runtime functions
- ✅ Object instantiation with `new` keyword and ARC memory management
- ✅ Member variables and member access via `.` operator
- ✅ Method calls on object instances
- ✅ C#-style generics with monomorphization (v0.6)

### v0.7+ (Planned)
- Type casting with `as` keyword
- Design pattern code generation (singleton, factory, observable, etc.)
- Class inheritance and polymorphism
- Interface implementation
- Type constraints for generics: `<T: Serializable>`
- Variance annotations (covariance, contravariance)
- Parallel iteration
- Optional region-based GC
- Module system with proper import resolution
- Package manager
- Reflection capabilities
- String interpolation full implementation
- Pattern matching

---

## 19. Summary

**hooc** is a pragmatic, safe, and expressive language that modernizes C without inheriting the complexity of C++, the verbosity of Java, or the cognitive overhead of Rust.

**hooc v0.6** demonstrates a mature, feature-rich compiler:
- 577 comprehensive unit tests covering all implemented features
- Complete type system with inference, unions, optionals, and generics
- Full expression evaluation with proper operator precedence
- Comprehensive control flow (conditionals, loops, scoping)
- Robust function declarations and calls with generic support
- Array literals with multi-dimensional support and element access
- Class declarations with constructors, member variables, and methods
- Automatic Reference Counting (ARC) memory management
- C#-style generics with monomorphization for zero-overhead specialization
- String type with 30+ runtime functions via runtime injection framework
- Modern design pattern keywords integrated into grammar
- Clean LLVM-based compilation pipeline with ORC JIT execution

This version provides a solid foundation for practical system programming while maintaining safety and expressiveness.


