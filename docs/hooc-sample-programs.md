# hoo Programming Language -- Sample Programs

This document contains reference examples for the **hoo** programming
language, compiled by **hooc**, demonstrating core syntax and language features.

------------------------------------------------------------------------

## 1. Hello World (Minimal)

``` hoo
func main() {
    print("Hello, World!");
}
```

------------------------------------------------------------------------

## 2. Hello World (Multiline + Interpolation)

``` hoo
func main() {
    string lang = "hoo";

    print("""
    Hello from ${lang}!
    Welcome to a safer C-style language.
    """);
}
```
*Note: Full string interpolation (expressions inside `${}`) is a planned feature. The current grammar parses the placeholder.*```

------------------------------------------------------------------------

## 3. Variables & Basic Types

``` hoo
func main() {
    byte b = 255;
    int64 count = 42;
    double pi = 3.14159;
    bool active = true;
    char letter = 'H';
    string name = "Benoy";

    print("Name: ${name}, Count: ${count}, Active: ${active}");
}
```

------------------------------------------------------------------------

## 4. Character Operations & Classification

``` hoo
func classify_char(char ch) -> int {
    if (ch >= 'A' && ch <= 'Z') {
        return 1; // uppercase
    } else if (ch >= 'a' && ch <= 'z') {
        return 2; // lowercase  
    } else if (ch >= '0' && ch <= '9') {
        return 3; // digit
    } else {
        return 0; // other
    }
}

func char_examples() -> void {
    var letter = 'A';
    var digit = '9';
    var symbol = '@';
    
    // Character comparisons
    var is_upper = letter >= 'A' && letter <= 'Z';
    var is_equal = letter == 'A';
    var is_less = digit < letter; // '9' < 'A' (ASCII comparison)
    
    return;
}
```

------------------------------------------------------------------------

## 5. Conditional Logic

``` hoo
func main() {
    int64 age = 20;

    if age >= 18 {
        print("Adult");
    } else {
        print("Minor");
    }
}
```

------------------------------------------------------------------------

## 6. Arrays (v0.2 - Fully Implemented)

### 6.1 Array Literals and Declarations

``` hoo
func array_declarations() -> void {
    // Type inferred from elements
    var numbers = [1, 2, 3, 4, 5];          // int64[]
    var floats = [1.0, 2.5, 3.14];          // double[]
    var names = ["Alice", "Bob"];           // string[]
    var flags = [true, false, true];        // bool[]

    // Explicit type annotation for empty arrays
    var empty_ints: int64[] = [];
    var empty_strings: string[] = [];

    // Array access
    var value = numbers[2];  // Returns 3
    print("Value at index 2: ${value}");
}
```

### 6.2 Array Types in Functions

``` hoo
// Arrays are passed as slices (unsized array types)
func process_array(arr: int64[]) -> void {
    // Array access syntax (arr[index]) is now implemented
    if arr.len > 0 { // .len property is available on slices
        print("First element: ${arr[0]}");
    }
    // Iteration over arrays is also fully supported
    for item in arr {
        // print(item); // Example of iteration
    }
}

func array_function_demo() -> void {
    var my_numbers = [10, 20, 30, 40];
    process_array(my_numbers);

    // This shows passing an array literal directly
    process_array([1, 2, 3]);
}
```

### 6.3 Multi-Dimensional Array Literals

``` hoo
func multi_dimensional_array_demo() -> void {
    // Multi-dimensional arrays use nested array literals
    var matrix = [
        [1, 2, 3],
        [4, 5, 6]
    ]; // int64[][]

    // Accessing elements
    print("Element at [0][1]: ${matrix[0][1]}"); // Prints 2

    // Iterating over multi-dimensional arrays
    for row in matrix {
        for value in row {
            print(value);
        }
    }
}
```

------------------------------------------------------------------------

## 7. For Loops (v0.2 - Fully Implemented)

### 7.1 Range-Based Loops

``` hoo
func range_example() -> void {
    // For-range loop is fully implemented (AST and LLVM IR generation)
    // This generates proper LLVM IR with:
    // - Entry basic block
    // - Condition check block  
    // - Loop body block
    // - Increment block
    // - Exit block
    for i in 0..5 { // Example syntax (full parsing and code generation works)
        print("i = ${i}");
    }
    return;
}
```

### 7.2 For-In Loops

``` hoo
func for_in_example() -> void {
    var arr = [10, 20, 30];
    
    // For-in loop (for-each) AST and code generation complete
    // Works with any iterable, including arrays
    for item in arr {
        print(item);
    }
    return;
}
```

------------------------------------------------------------------------

## 8. Legacy Examples (Syntax In Development)

### Range-Based Loop (Planned Syntax)

``` hoo
// Note: Full parsing implementation in progress
func main() {
    for i in 1..5 {
        print("i = ${i}");
    }
}
```

### Collection Iteration (Planned Syntax)

``` hoo
// Note: Requires array access syntax completion
func main() {
    int64[] numbers = [10, 20, 30];

    for n in numbers {
        print(n);
    }
}
```
```

------------------------------------------------------------------------

## 9. Functions

``` hoo
func add(int64 a, int64 b) -> int64 {
    return a + b;
}

func main() {
    print(add(10, 20));
}
```

------------------------------------------------------------------------

## 10. Classes & Objects

``` hoo
class User(string name, int64 age) {
    func greet() {
        print("Hello, my name is ${name} and I am ${age}");
    }
}

func main() {
    var u = new User("Benoy", 30); // Use 'new' keyword for instantiation
    u.greet();
}
```

------------------------------------------------------------------------

## 11. Interfaces

``` hoo
interface Greeter {
    func greet();
}

class Person(string name) implements Greeter {
    func greet() {
        print("Hello from ${name}");
    }
}

func main() {
    Greeter g = Person("Alice");
    g.greet();
}
```

------------------------------------------------------------------------

## 12. Searching & Sorting

``` hoo
func main() {
    int64[] numbers = [5, 2, 9, 1];

    numbers.sort();
    print(numbers);

    int64? found = numbers.find(n => n == 9);

    if found {
        print("Found ${found}");
    }
}
```

------------------------------------------------------------------------

## 13. List Comprehension

``` hoo
func main() {
    int64[] squares = [x * x for x in 1..10 if x % 2 == 0];
    print(squares);
}
```

------------------------------------------------------------------------

## 14. Error Handling

``` hoo
func divide(int64 a, int64 b) -> int64 | Error {
    if b == 0 {
        return Error("Division by zero");
    }
    return a / b;
}

func main() {
    result = divide(10, 0) else {
        print("Error occurred");
        return;
    };

    print(result);
}
```

------------------------------------------------------------------------

## 15. Singleton Pattern

``` hoo
singleton class Logger {
    func log(string msg) {
        print("[LOG] ${msg}");
    }
}

func main() {
    Logger.log("Application started");
}
```

------------------------------------------------------------------------

## 16. Immutable Object

``` hoo
immutable class Money(double amount, string currency);

func main() {
    Money m = Money(100.0, "INR");
    print("${m.amount} ${m.currency}");
}
```

------------------------------------------------------------------------

## 17. Actor (Concurrency)

``` hoo
actor class Counter {
    int64 value = 0;

    func increment() {
        value = value + 1;
    }

    func get() -> int64 {
        return value;
    }
}

func main() {
    Counter c = Counter();
    c.increment();
    print(c.get());
}
```

------------------------------------------------------------------------

## 18. Data Processing Example

``` hoo
class User(string name, int64 age, bool active);

func main() {
    User[] users = [
        User("Alice", 25, true),
        User("Bob", 17, false),
        User("Charlie", 30, true)
    ];

    names = [
        u.name
        for u in users
        if u.active && u.age >= 18
    ].sorted();

    print(names.join(", "));
}
```

------------------------------------------------------------------------

*End of hooc sample programs.*

