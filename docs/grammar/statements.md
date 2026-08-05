# Statements & Control Flow

Statements are the building blocks of Hoo functions and blocks.

## 1. Variables & Constants

- **Variables**: Mutable storage.
  - `var x = 10;`
  - `var y: int64 = 20;`
- **Constants**: Immutable storage.
  - `const PI = 3.14;`

## 2. Conditionals

### If Statement
Standard if-else construct.
```hoo
if (x > 0) {
    print("Positive");
} else {
    print("Zero or Negative");
}
```

## 3. Loops

### While Loop
Executes as long as the condition is true.
```hoo
while (i < 10) {
    i++;
}
```

### For-In Loop
Supports iterating over ranges, arrays, strings, and map keys.
- **Range**: `for i in 0..10 by 2 { ... }`
- **Array**: `for item in array { ... }`
- **String**: `for ch in text { ... }` (character objects)
- **Map keys**: `for key in map { ... }`

### Loop Control
- `break;`: Exits the innermost loop.
- `continue;`: Jumps to the next iteration.

### Do-While Loop
Executes its body at least once, then evaluates the condition.
```hoo
do {
    i++;
} while (i < 10);
```

### Switch Statement
Integer-like discriminants support ordered cases, fall-through, `default`,
and `break`.
```hoo
switch value {
case 1:
    result = 10;
    break;
default:
    result = 0;
}
```

## 5. Exception Handling

Hoo provides a `try-catch-finally` model backed by ARC-managed exceptions and
the HVM/JIT shadow stack. Normal-path `finally` execution is covered by the
current runtime tests; exceptional-path integration remains under focused JIT
verification.

**Implementation Note:** The exception model is implemented using an ARC-managed `HooException` object and a Shadow Stack for routing control flow across the native JIT boundary. For details, see [Runtime Exceptions & Shadow Stack](../runtime/exceptions.md).

### Try-Catch-Finally
```hoo
try {
    performAction();
} catch (e: Exception) {
    logError(e);
} finally {
    cleanup();
}
```

### Throw & Rethrow
- `throw new Exception("Error");`
- `rethrow;` (Valid only within a catch block).

## 6. Other Statements

- **Return**: `return expr;` or `return;`.
- **Block**: `{ statement* }`
- **Expression Statement**: `expr;`
- **Scope**: `scope { ... }` (Creates an isolated lexical lifetime boundary).
