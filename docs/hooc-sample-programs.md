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

## 4. Conditional Logic

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

## 5. Iteration

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

## 6. Multi-Dimensional Arrays

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

## 7. Functions

``` hooc
func add(int64 a, int64 b) -> int64 {
    return a + b;
}

func main() {
    print(add(10, 20));
}
```

------------------------------------------------------------------------

## 8. Classes & Objects

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

## 9. Interfaces

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

## 10. Searching & Sorting

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

## 11. List Comprehension

``` hooc
func main() {
    int64[] squares = [x * x for x in 1..10 if x % 2 == 0];
    print(squares);
}
```

------------------------------------------------------------------------

## 12. Error Handling

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

## 13. Singleton Pattern

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

## 14. Immutable Object

``` hooc
immutable class Money(double amount, string currency);

func main() {
    Money m = Money(100.0, "INR");
    print("${m.amount} ${m.currency}");
}
```

------------------------------------------------------------------------

## 15. Actor (Concurrency)

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

## 16. Data Processing Example

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
