# Functions & Methods

Functions are the primary units of execution in Hooc. They can be declared at the top-level or as members of a class.

## 1. Function Declaration

The basic syntax for a function is:
`func [: returnType] name(parameters) { body }`

### Top-level Function
```hooc
func: int64 add(a: int64, b: int64) {
    return a + b;
}
```

### Void Function
If no return type is specified, it defaults to `void`.
```hooc
func sayHello() {
    print("Hello");
}
```

## 2. Parameters
Parameters are comma-separated and must have a name and a type.
`name: type`

## 3. Function Modifiers
Functions can be prefixed with modifiers to change their behavior or visibility:
- `public`: Visible to other modules.
- `private`: Visible only within the current module or class.
- `async`: Declares an asynchronous function (Scaffolding for future task models).

## 4. Methods
Methods are functions declared inside a `class`. They have access to the implicit `this` reference.

```hooc
class Calculator {
    func: int64 add(a: int64, b: int64) {
        return a + b;
    }
}
```

## 5. Constructors
Constructors are special functions in a class used for instance initialization. They use the `constructor` keyword and do not have a return type.

```hooc
class User {
    var name: string;
    constructor(n: string) {
        this.name = n;
    }
}
```
