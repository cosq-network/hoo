# Hooc Language Features Guide

This guide provides comprehensive documentation for all features of the Hooc programming language, including syntax, semantics, and usage examples.

## Table of Contents

1. [Type System](#type-system)
2. [Variables](#variables)
3. [Functions](#functions)
4. [Classes and Objects](#classes-and-objects)
5. [Generics](#generics)
6. [Arrays](#arrays)
7. [Control Flow](#control-flow)
8. [Module System](#module-system)
9. [Memory Management](#memory-management)
10. [String Operations](#string-operations)
11. [Advanced Features](#advanced-features)

## Type System

### Primitive Types

Hooc provides a rich set of primitive types for different use cases:

```hoo
// Integer types
var b: byte = 127;          // 8-bit signed integer (-128 to 127)
var u: uint8 = 255;         // 8-bit unsigned integer (0 to 255)
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
func doSomething() -> void {
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

### Union Types

Combine multiple types with the pipe operator:

```hoo
var value: int64 | string;

value = 42;         // OK: int64
value = "hello";    // OK: string

// Union with null
var result: bool | null;
result = true;
result = null;

// Multiple types
var data: int64 | double | string;
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
```

## Variables

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

### Assignment

```hoo
var x: int64;
x = 10;                  // Simple assignment

var y = x + 5;           // Assignment with expression

x = x + 1;               // Update variable
```

### Scope

Variables are scoped to their containing block:

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

### Module-Level Variables

Variables can be declared at module level:

```hoo
var globalCounter: int64 = 0;
var appName: string = "MyApp";

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
func add(a: int64, b: int64) -> int64 {
    return a + b;
}

// Multiple parameters and return
func calculate(x: int64, y: int64, op: string) -> int64 {
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
func square(x: int64) -> int64 {
    return x * x;
}

// Early return
func divide(a: int64, b: int64) -> int64? {
    if b == 0 {
        return null;  // Early return
    }
    return a / b;
}

// Multiple returns
func abs(x: int64) -> int64 {
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

### Generic Functions

See [Generics](#generics) section for details.

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

    func getResult() -> int64 {
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
        var self.x = x;
        var self.y = y;
    }
}

class Rectangle {
    var width: int64;
    var height: int64;

    constructor(w: int64, h: int64) {
        var width = w;
        var height = h;
    }

    func area() -> int64 {
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
        var count = 0;
    }

    func increment() {
        count = count + 1;
    }

    func decrement() {
        count = count - 1;
    }

    func getValue() -> int64 {
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

### Inheritance

```hoo
class Animal {
    var name: string;

    constructor(name: string) {
        var self.name = name;
    }

    func makeSound() -> void {
        print("Some sound");
    }
}

class Dog extends Animal {
    var breed: string;

    constructor(name: string, breed: string) {
        // Call parent constructor
        var self.name = name;
        var self.breed = breed;
    }

    func makeSound() -> void {
        print("Woof!");
    }
}

func main() {
    var dog = new Dog("Rex", "Labrador");
    dog.makeSound();  // "Woof!"
}
```

### Interfaces

```hoo
interface Drawable {
    func draw() -> void;
    func getArea() -> double;
}

interface Printable {
    func print() -> void;
}

class Circle implements Drawable, Printable {
    var radius: double;

    constructor(r: double) {
        var radius = r;
    }

    func draw() -> void {
        print("Drawing circle");
    }

    func getArea() -> double {
        return 3.14159 * radius * radius;
    }

    func print() -> void {
        print("Circle with radius: " + radius);
    }
}
```

## Generics

### Generic Functions

```hoo
// Simple generic function
func identity<T>(value: T) -> T {
    return value;
}

// Usage with explicit type
var intValue = identity<int64>(42);
var strValue = identity<string>("hello");

// Multiple type parameters
func pair<T, U>(first: T, second: U) -> void {
    print(first);
    print(second);
}

pair<int64, string>(42, "answer");
```

### Generic Classes

```hoo
class Box<T> {
    var value: T;

    constructor(val: T) {
        var value = val;
    }

    func get() -> T {
        return value;
    }

    func set(val: T) {
        value = val;
    }
}

// Usage
var intBox = new Box<int64>(42);
var strBox = new Box<string>("hello");

print(intBox.get());  // 42
print(strBox.get());  // "hello"
```

### Generic Arrays

```hoo
class Stack<T> {
    var items: T[];

    constructor() {
        var items = [];
    }

    func push(item: T) {
        // Add to array
    }

    func pop() -> T? {
        // Remove from array
        return null;
    }

    func isEmpty() -> bool {
        return items.length() == 0;
    }
}

var intStack = new Stack<int64>();
intStack.push(1);
intStack.push(2);

var strStack = new Stack<string>();
strStack.push("hello");
```

### Multiple Type Parameters

```hoo
class Pair<K, V> {
    var key: K;
    var value: V;

    constructor(k: K, v: V) {
        var key = k;
        var value = v;
    }

    func getKey() -> K {
        return key;
    }

    func getValue() -> V {
        return value;
    }
}

var stringIntPair = new Pair<string, int64>("age", 30);
var boolDoublePair = new Pair<bool, double>(true, 3.14);
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

### Generic Arrays

Arrays are fully generic and type-safe:

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

// Nested generic arrays
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

// With variable range
var start = 5;
var end = 15;
for i in start..end {
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
import std.collections.generic; // Nested module

// Using qualified names
var str = new std.String("hello");
var arr = new std.collections.List<int64>();
```

### Standard Library Modules

```hoo
// String module
from std import String;
var s = new String("hello");

// Array module
from std import Array;
var arr = new Array<int64>();

// IO module (planned)
from std.io import File, Console;
var file = new File("data.txt");

// Collections (planned)
from std.collections import List, Map, Set;
var list = new List<string>();
var map = new Map<string, int64>();
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

// Multi-line strings
var multiline = """
    This is a
    multi-line
    string
""";

// Empty string
var empty = "";
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

### Nullable Chaining

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

### Type Guards

```hoo
var value: int64 | string;

// Type checking (future feature)
if value is int64 {
    var number = value;  // Treated as int64
} else {
    var text = value;    // Treated as string
}
```

### Generic Constraints (Planned)

```hoo
// Future feature: constrain type parameters
func sortable<T: Comparable>(items: T[]) -> void {
    // T must implement Comparable interface
}
```

### Pattern Matching (Planned)

```hoo
// Future feature: pattern matching
var result = match value {
    0 => "zero",
    1 => "one",
    _ => "other"
};
```

### Properties (Planned)

```hoo
// Future feature: computed properties
class Circle {
    var radius: double;

    property area: double {
        get { return 3.14 * radius * radius; }
    }
}
```

### Async/Await (Planned)

```hoo
// Future feature: asynchronous operations
async func fetchData() -> string {
    var data = await http.get("https://api.example.com");
    return data;
}
```

## Best Practices

### Naming Conventions

```hoo
// Classes: PascalCase
class UserAccount { }

// Functions and variables: camelCase
func calculateTotal() { }
var currentValue = 0;

// Constants: UPPER_SNAKE_CASE
var MAX_SIZE: int64 = 100;
```

### Error Handling

Use nullable types for operations that may fail:

```hoo
func divide(a: int64, b: int64) -> int64? {
    if b == 0 {
        return null;
    }
    return a / b;
}

var result = divide(10, 0);
if result == null {
    print("Error: division by zero");
} else {
    print(result);
}
```

### Resource Management

Use scope blocks for resources:

```hoo
func processFile(filename: string) {
    scope {
        var file = openFile(filename);
        // Process file
        // File automatically closed at end of scope
    }
}
```

### Generic Code

Prefer generic functions and classes for reusable code:

```hoo
// Instead of separate functions for each type
func swapInt(a: int64, b: int64) { }
func swapString(a: string, b: string) { }

// Use generic function
func swap<T>(a: T, b: T) {
    var temp = a;
    a = b;
    b = temp;
}
```

## See Also

- [Grammar Specification](grammar.md) - Complete language grammar
- [Implementation Status](implementation-status.md) - Current implementation status
- [Roadmap](roadmap.md) - Future development plans
