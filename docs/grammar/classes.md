# Classes & Inheritance

Hoo is an object-oriented language with support for single inheritance and advanced architectural modifiers.

## 1. Class Declaration
A class is declared using the `class` keyword.
`[modifiers] class Name [extends BaseClass] { body }`

```hoo
class Animal {
    var species: string;
    constructor(s: string) {
        this.species = s;
    }
}
```

## 2. Inheritance
A class can extend exactly one other class using the `extends` keyword.
```hoo
class Dog extends Animal {
    constructor() {
        this.species = "Canine";
    }
}
```
**Implementation Note**: Inheritance is implemented via **Layout Prefixing** in the HVM backend, where the child class appends its fields to the end of the parent class's layout.

## 3. Class Modifiers
Hoo supports several advanced modifiers to enforce architectural patterns:

| Modifier | Description |
| :--- | :--- |
| `singleton` | Ensures only one instance of the class can exist. |
| `immutable` | All fields are constant after initialization. |
| `factory` | Class is designed to produce other objects. |
| `observable` | Built-in support for observer patterns. |
| `service` | Represents a stateless singleton or utility group. |
| `strategy` | Part of a strategy pattern implementation. |
| `actor` | Isolated state for concurrent execution (future phase). |
| `final` | Class cannot be extended. |

## 4. Class Members
A class body can contain:
- **Variables**: `var name: type;` or `var name = value;`
- **Constructors**: `constructor(...) { ... }`
- **Methods**: `[modifiers] func ...`

## 5. Constructors & `this`
The `constructor` keyword is used for initialization. Within a class, `this` refers to the current instance.
- Constructors do not have a return type.
- Fields are accessed using `this.fieldName`.
