# Hooc Language Features Guide

This guide provides comprehensive documentation for all features of the Hooc programming language, including syntax, semantics, and usage examples.

## Table of Contents

1. [Type System](#type-system)
2. [Variables](#variables)
3. [Functions](#functions)
4. [Classes and Objects](#classes-and-objects)
5. [Arrays](#arrays)
6. [Control Flow](#control-flow)
7. [Module System](#module-system)
8. [Memory Management](#memory-management)
9. [String Operations](#string-operations)
10. [Advanced Features](#advanced-features)
11. [Command-Line Interface](#command-line-interface)

## Type System

### Primitive Types

Hooc provides a rich set of primitive types for different use cases:

```hoo
// Integer types
var b: int8 = 127;          // 8-bit signed integer (-128 to 127)
var u: byte = 255;          // 8-bit unsigned integer (0 to 255)
var i: int64 = 1000000;     // 64-bit signed integer (default for integers)

// Floating-point types
var f: float = 3.14;        // 32-bit floating point
var d: double = 3.14159265; // 64-bit floating point (default for decimals)
var f64: f64 = 2.71828;     // Alias for double

// Boolean type
var flag: bool = true;      // true or false

// Character and string
var ch: char = 'A';         // Single character
var str: string = "Hello";  // UTF-8 string

// Void type (no return value)
func doSomething() {
    // No return value
}
```

### Nullable Types

Any type can be made nullable by adding `?`:

```hoo
var maybeNumber: int64? = null;
var maybeName: string? = "Alice";

// Checking for null
if maybeName != null {
    print(maybeName);
}

// Nullable with default
var value: int64? = null;
if value == null {
    value = 42;
}
```

### Array Types

Arrays are denoted with square brackets:

```hoo
var numbers: int64[];           // Array of integers
var names: string[];            // Array of strings
var matrix: double[][];         // 2D array (array of arrays)
var cube: int64[][][];          // 3D array

// Nullable arrays
var maybeArray: int64[]?;       // Nullable array
var arrayOfNullables: int64?[]; // Array of nullable integers
```

### Custom Types (Classes)

User-defined types via classes:

```hoo
class Point {
    var x: int64;
    var y: int64;
}

var p: Point;                   // Variable of type Point
var maybePoint: Point?;         // Nullable Point
var points: Point[];            // Array of Points
```

### Type Inference

Hooc can infer types from initialization values:

```hoo
var x = 42;              // Inferred as int64
var pi = 3.14;           // Inferred as double
var flag = true;         // Inferred as bool
var name = "Alice";      // Inferred as string
var items = [1, 2, 3];   // Inferred as int64[]
const MAX = 100;         // Inferred as int64
```

## Variables and Constants

### Variable Declaration

Variables are declared with the `var` keyword:

```hoo
// With type annotation
var count: int64;
var name: string;

// With initialization
var x: int64 = 42;
var message: string = "Hello";

// Type inference
var value = 100;         // int64
var ratio = 0.5;         // double
var active = true;       // bool
```

### Constant Declaration

Constants are declared with the `const` keyword and must be initialized. They are read-only and supported for standard data types and arrays of standard data types.

```hoo
const PI = 3.14159;
const APP_NAME: string = "Hooc App";
const PRIMES = [2, 3, 5, 7];
```

### Assignment

```hoo
var x: int64;
x = 10;                  // Simple assignment

var y = x + 5;           // Assignment with expression

x = x + 1;               // Update variable

// Compound assignment operators
x += 5;                  // x = x + 5
x -= 3;                  // x = x - 3
x *= 2;                  // x = x * 2
x /= 4;                  // x = x / 4
x %= 7;                  // x = x % 7
x <<= 2;                 // x = x << 2 (left shift)
x >>= 1;                 // x = x >> 1 (right shift)

// Increment/Decrement
x++;                     // x = x + 1 (postfix)
x--;                     // x = x - 1 (postfix)

// PI = 3.14;            // ERROR: Cannot assign to a constant
```

### Scope

Variables are scoped to their containing block. Constants and module-level variables are accessible throughout the module.

```hoo
func example() {
    var outer = 10;

    {
        var inner = 20;
        print(outer);    // OK: outer is visible
        print(inner);    // OK: inner is visible
    }

    print(outer);        // OK: outer still visible
    // print(inner);     // ERROR: inner out of scope
}
```

### Module-Level Declarations

Variables and constants can be declared at module level. They support dynamic initialization (like array or string literals) which runs at module load time.

```hoo
var globalCounter: int64 = 0;
const VERSION = "1.1.0";
const DEFAULT_SCORES = [10, 20, 30];

func incrementCounter() {
    globalCounter = globalCounter + 1;
}
```

## Functions

### Function Declaration

```hoo
// No parameters, no return value
func sayHello() {
    print("Hello!");
}

// With parameters
func greet(name: string) {
    print("Hello, " + name);
}

// With return type
func:int64 add(a: int64, b: int64) {
    return a + b;
}

// Multiple parameters and return
func:int64 calculate(x: int64, y: int64, op: string) {
    if op == "add" {
        return x + y;
    } else {
        return x - y;
    }
}
```

### Return Statements

```hoo
// Return void (implicit)
func doWork() {
    print("Working...");
    return;  // Optional for void functions
}

// Return value
func:int64 square(x: int64) {
    return x * x;
}

// Early return
func:int64? divide(a: int64, b: int64) {
    if b == 0 {
        return null;  // Early return
    }
    return a / b;
}

// Multiple returns
func:int64 abs(x: int64) {
    if x < 0 {
        return -x;
    } else {
        return x;
    }
}
```

### Function Calls

```hoo
// Simple call
sayHello();

// With arguments
greet("Alice");

// Using return value
var sum = add(10, 20);
var result = calculate(15, 5, "add");

// Nested calls
var value = square(add(2, 3));  // square(5) = 25
```

## Classes and Objects

### Class Declaration

```hoo
class Person {
    var name: string;
    var age: int64;
}

// With methods
class Calculator {
    var result: int64;

    func add(x: int64) {
        result = result + x;
    }

    func:int64 getResult() {
        return result;
    }
}
```

### Constructors

```hoo
class Point {
    var x: int64;
    var y: int64;

    constructor(x: int64, y: int64) {
        this.x = x;
        this.y = y;
    }
}

class Rectangle {
    var width: int64;
    var height: int64;

    constructor(w: int64, h: int64) {
        width = w;
        height = h;
    }

    func:int64 area() {
        return width * height;
    }
}
```

### Object Creation

Use the `new` keyword to create instances:

```hoo
// Simple creation
var p = new Point(10, 20);

// With type annotation
var rect: Rectangle = new Rectangle(5, 10);

// Using the object
var a = rect.area();

// Accessing members
print(p.x);  // 10
print(p.y);  // 20
```

### Methods

```hoo
class Counter {
    var count: int64;

    constructor() {
        count = 0;
    }

    func increment() {
        count = count + 1;
    }

    func decrement() {
        count = count - 1;
    }

    func:int64 getValue() {
        return count;
    }

    func reset() {
        count = 0;
    }
}

func main() {
    var c = new Counter();
    c.increment();
    c.increment();
    print(c.getValue());  // 2
}
```

### Class Modifiers

Hooc supports several design pattern modifiers:

```hoo
// Singleton: Only one instance allowed
singleton class Database {
    var connection: string;
}

// Immutable: Fields cannot be modified after construction
immutable class Point {
    var x: int64;
    var y: int64;
}

// Final: Cannot be inherited
final class FinalClass {
    // ...
}

// Factory pattern
factory class ObjectFactory {
    // ...
}

// Observable pattern
observable class DataModel {
    // ...
}

// Service pattern
service class AuthService {
    // ...
}

// Strategy pattern
strategy class SortStrategy {
    // ...
}

// Actor model
actor class MessageProcessor {
    // ...
}
```

### Function Modifiers

Member functions can have access and behavior modifiers:

```hoo
class User {
    var name: string;
    var age: int64;

    // Public: accessible from outside (default)
    public func greet() {
        print("Hello!");
    }

    // Private: only accessible within class
    private func calculate() -> int64 {
        return age * 2;
    }

    // Async: for future async support
    async func fetch() {
        print("Fetching data...");
    }

    // Multiple modifiers
    public async func save() {
        print("Saving...");
    }
}
```

### Inheritance

```hoo
class Animal {
    var name: string;

    constructor(name: string) {
        this.name = name;
    }

    func makeSound() {
        print("Some sound");
    }
}

class Dog extends Animal {
    var breed: string;

    constructor(name: string, breed: string) {
        // Call parent constructor
        this.name = name;
        this.breed = breed;
    }

    func makeSound() {
        print("Woof!");
    }
}

func main() {
    var dog = new Dog("Rex", "Labrador");
    dog.makeSound();  // "Woof!"
}
```

## Arrays

### Array Literals

```hoo
// Empty array
var empty: int64[] = [];

// Array with elements
var numbers = [1, 2, 3, 4, 5];
var names = ["Alice", "Bob", "Charlie"];

// Mixed expressions
var values = [10, 20 + 5, getValue()];

// Multi-dimensional
var matrix = [[1, 2], [3, 4]];
var cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
```

### Array Indexing

```hoo
var numbers = [10, 20, 30, 40];

var first = numbers[0];    // 10
var second = numbers[1];   // 20

// Nested indexing
var matrix = [[1, 2], [3, 4]];
var value = matrix[0][1];  // 2
```

### Array Methods

The runtime provides built-in array operations:

```hoo
var arr: int64[] = [1, 2, 3];

// Get length
var len = arr.length();    // 3

// Push element
arr.push(4);              // [1, 2, 3, 4]

// Pop element
var last = arr.pop();     // 4, arr is now [1, 2, 3]

// Clear array
arr.clear();              // []

// Check if empty
var isEmpty = arr.empty(); // true
```

### Arrays with Custom Types

Arrays can hold custom types:

```hoo
// Array of custom types
class Point {
    var x: int64;
    var y: int64;
}

var points: Point[] = [];
points.push(new Point(0, 0));
points.push(new Point(10, 20));

// Array of nullable types
var maybeValues: int64?[] = [null, 42, null, 100];

// Nested arrays
var listOfLists: int64[][] = [[1, 2], [3, 4], [5, 6]];
```

## Control Flow

### If-Else Statements

```hoo
// Simple if
if x > 0 {
    print("positive");
}

// If-else
if x > 0 {
    print("positive");
} else {
    print("non-positive");
}

// Nested if-else
if x > 0 {
    print("positive");
} else {
    if x < 0 {
        print("negative");
    } else {
        print("zero");
    }
}

// With complex conditions
if (x > 0 && y > 0) || z == 100 {
    print("condition met");
}
```

### While Loops

```hoo
// Basic while loop
var count = 0;
while count < 10 {
    print(count);
    count = count + 1;
}

// With break condition
var running = true;
var iterations = 0;
while running {
    iterations = iterations + 1;
    if iterations >= 100 {
        running = false;
    }
}

// Infinite loop
while true {
    // Loop forever (use with caution)
    if someCondition() {
        // break out somehow
    }
}
```

### For Loops

```hoo
// For-in loop (iterate over collection)
var items = [1, 2, 3, 4, 5];
for item in items {
    print(item);
}

// For-range loop
for i in 0..10 {
    print(i);  // Prints 0 through 9
}

// For-range with step
for i in 0 .. 10 by 2 {
    print(i);  // Prints 0, 2, 4, 6, 8
}

// Reverse range
for i in 10 .. 0 by -1 {
    print(i);  // Prints 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
}

// With variable range
var start = 5;
var end = 15;
var step = 3;
for i in start..end by step {
    print(i);
}
```

### Conditional Expressions

```hoo
// Comparison operators
if x == y { }
if x != y { }
if x < y { }
if x <= y { }
if x > y { }
if x >= y { }

// Logical operators
if x > 0 && y > 0 { }       // AND
if x > 0 || y > 0 { }       // OR
if !flag { }                 // NOT

// Combined conditions
if (x > 0 && y > 0) || (x < 0 && y < 0) {
    print("same sign");
}
```

## Module System

### Import Statements

```hoo
// Import entire module
import std.io;

// Import with alias
import std.collections as coll;

// Import specific items
from std.io import File, Directory;

// Import with renaming
from std.collections import List as ArrayList;

// Multiple items
from std.math import sin, cos, tan, sqrt;
```

### Module Paths

Modules use dot notation for hierarchical organization:

```hoo
import std;                    // Top-level std module
import std.io;                 // std.io submodule
import std.collections; // Nested module

// Using qualified names
var str = new std.String("hello");
```

### Standard Library Modules

```hoo
// String module
from std import String;
var s = new String("hello");

// Array module
from std import Array;
var arr = new Array();

// IO module (planned)
from std.io import File, Console;

// Collections (planned)
from std.collections import List, Map, Set;
```

### Creating Modules

Modules are organized by file and directory structure:

```
myproject/
├── main.hoo              # Main program
├── utils/
│   ├── math.hoo         # utils.math module
│   └── string.hoo       # utils.string module
└── models/
    └── user.hoo         # models.user module
```

```hoo
// In main.hoo
from utils.math import calculate;
from models.user import User;

func main() {
    var result = calculate(10, 20);
    var user = new User("Alice");
}
```

## Memory Management

### Automatic Reference Counting (ARC)

Hooc uses automatic reference counting for memory management. Objects are automatically freed when no longer referenced.

```hoo
class Node {
    var value: int64;
    var next: Node?;
}

func example() {
    var node = new Node();  // refcount = 1
    node.value = 42;

    var another = node;     // refcount = 2 (retain)

    // When 'node' goes out of scope: refcount = 1
    // When 'another' goes out of scope: refcount = 0, freed
}
```

### Scope Management

Use `scope` blocks for explicit lifetime control:

```hoo
func processData() {
    scope {
        var temp = new LargeObject();
        // Use temp
        // temp is automatically released at end of scope
    }

    // temp is no longer accessible and has been freed
}
```

### Object Lifecycle

```hoo
class Resource {
    var data: int64;

    constructor() {
        print("Resource created");
        var data = 0;
    }

    // Destructor (called when refcount reaches 0)
    // Note: Destructors are automatic, not explicitly defined
}

func demo() {
    var r = new Resource();  // "Resource created"
    // Use r
    // When r goes out of scope, object is freed
}
```

### Memory Statistics

For debugging, the runtime provides memory statistics:

```hoo
// In C/C++ code or via FFI
hoo_print_memory_stats();  // Shows allocations, deallocations, live objects
hoo_reset_memory_stats();  // Reset counters
```

## String Operations

Hooc provides a comprehensive string library with UTF-8 support.

### String Creation

```hoo
// String literals
var s1 = "hello";
var s2 = "world";

// Empty string
var empty = "";

// Multiline strings (triple-quoted)
var multi = """This is a
multiline string
that preserves
newlines""";
```

### String Concatenation

```hoo
var greeting = "Hello, " + "World!";
var name = "Alice";
var message = "Welcome, " + name;

// Concatenating with numbers (requires conversion)
var age = 30;
var info = "Age: " + String.fromInt(age);
```

### String Methods

```hoo
var str = "  Hello, World!  ";

// Length
var len = str.length();

// Trim whitespace
var trimmed = str.trim();  // "Hello, World!"

// Case conversion
var upper = str.toUpper();  // "  HELLO, WORLD!  "
var lower = str.toLower();  // "  hello, world!  "

// Substring
var sub = str.substring(2, 5);  // "Hel"

// Search
var index = str.indexOf("World");     // 9 (after trim)
var lastIdx = str.lastIndexOf("l");   // 13
var contains = str.contains("Hello"); // true
var starts = str.startsWith("  ");    // true
var ends = str.endsWith("!  ");       // true

// Replace
var replaced = str.replace("World", "Universe");

// Comparison
var equal = str.equals("  Hello, World!  ");  // true
var compare = str.compare("abc");              // 1 (str > "abc")
```

### String Formatting

```hoo
// Format strings (via runtime)
var formatted = String.format("Name: %s, Age: %d", name, age);
var number = String.format("Value: %.2f", 3.14159);
```

## Advanced Features

### Null Handling

```hoo
class Person {
    var name: string;
    var address: Address?;
}

class Address {
    var street: string;
    var city: string;
}

var person: Person? = getPerson();

// Check each level for null
if person != null {
    if person.address != null {
        print(person.address.city);
    }
}
```

## Command-Line Interface

The Hooc compiler provides a unified command-line tool `hooc` for compiling and running Hooc source code.

### Usage

```bash
hooc [options] <input_file>
```

- `<input_file>`: A `.hoo` source file or a `.ho` bytecode file (reserved).

### Options

| Option | Alias | Description |
|--------|--------|-------------|
| `--help` | `-h` | Display help message and exit |
| `--version` | `-v` | Display version information and exit |
| `--compile` | `-c` | Compile and validate source without execution (only for `.hoo`) |
| `--output` | `-o` | Specify output `.ho` path (implies `-c`, reserved for future) |
| `--verbose` | | Enable verbose logging for debugging |
| `--print-ir` | | Print generated LLVM IR to stdout |

### Examples

```bash
# Run a script directly via JIT
hooc main.hoo

# Compile and validate source
hooc main.hoo -c

# Reserved: Build AOT bytecode
hooc main.hoo -o main.ho

# Reserved: Run pre-compiled bytecode
hooc main.ho
```

## See Also


- [Grammar Specification](grammar.md) - Complete language grammar
- [Implementation Status](implementation-status.md) - Current implementation status
- [Roadmap](roadmap.md) - Future development plans
