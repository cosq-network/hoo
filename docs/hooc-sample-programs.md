# hooc Programming Language -- Sample Programs

This document contains reference examples for the **hooc** programming
language, demonstrating core syntax and language features.

------------------------------------------------------------------------

## 1. Hello World (Minimal)

``` hooc
func main() {
    print("Hello, World!");
}
```

------------------------------------------------------------------------

## 2. Hello World (Multiline + Interpolation)

``` hooc
func main() {
    string lang = "hooc";

    print("""
    Hello from ${lang}!
    Welcome to a safer C-style language.
    """);
}
```

------------------------------------------------------------------------

## 3. Variables & Basic Types

``` hooc
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

``` hooc
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

``` hooc
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

## 6. Iteration

### Range-Based Loop

``` hooc
func main() {
    for i in 1..5 {
        print("i = ${i}");
    }
}
```

### Collection Iteration

``` hooc
func main() {
    int64[] numbers = [10, 20, 30];

    for n in numbers {
        print(n);
    }
}
```

------------------------------------------------------------------------

## 7. Multi-Dimensional Arrays

``` hooc
func main() {
    int64[2][3] matrix = [
        [1, 2, 3],
        [4, 5, 6]
    ];

    for row in matrix {
        for value in row {
            print(value);
        }
    }
}
```

------------------------------------------------------------------------

## 8. Functions

``` hooc
func add(int64 a, int64 b) -> int64 {
    return a + b;
}

func main() {
    print(add(10, 20));
}
```

------------------------------------------------------------------------

## 9. Classes & Objects

``` hooc
class User(string name, int64 age) {
    func greet() {
        print("Hello, my name is ${name} and I am ${age}");
    }
}

func main() {
    User u = User("Benoy", 30);
    u.greet();
}
```

------------------------------------------------------------------------

## 10. Interfaces

``` hooc
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

## 11. Searching & Sorting

``` hooc
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

## 12. List Comprehension

``` hooc
func main() {
    int64[] squares = [x * x for x in 1..10 if x % 2 == 0];
    print(squares);
}
```

------------------------------------------------------------------------

## 13. Error Handling

``` hooc
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

## 14. Singleton Pattern

``` hooc
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

## 15. Immutable Object

``` hooc
immutable class Money(double amount, string currency);

func main() {
    Money m = Money(100.0, "INR");
    print("${m.amount} ${m.currency}");
}
```

------------------------------------------------------------------------

## 16. Actor (Concurrency)

``` hooc
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

## 17. Data Processing Example

``` hooc
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
