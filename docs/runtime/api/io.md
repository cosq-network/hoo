# I/O API Reference

## Module

`hoo.io`

## Import Statement

```hoo
import hoo.io;
```

Alternatively, import all top-level modules:

```hoo
import hoo;
```

## Module Description

The `hoo.io` module provides basic console input/output functions. These are global free functions that print to stdout/stderr or read from stdin. Strings are passed as native Hoo strings.

---

## Free Functions

### `println`

Prints the string `s` followed by a newline to stdout.

**Syntax:**
```hoo
println(s: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `s` | `string` | String to print (can be `null`, prints `"null"`) |

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    println("Hello, world!");
}
```

---

### `print`

Prints the string `s` to stdout without a trailing newline.

**Syntax:**
```hoo
print(s: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `s` | `string` | String to print (can be `null`, prints `"null"`) |

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    print("Enter your name: ");
}
```

---

### `print_error`

Prints the string `s` followed by a newline to stderr.

**Syntax:**
```hoo
print_error(s: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `s` | `string` | String to print to stderr |

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    print_error("An error occurred.");
}
```

---

### `readln`

Reads a line of text from stdin. Reads until a newline or EOF is encountered.

**Syntax:**
```hoo
readln() :string
```

**Parameters:** None.

**Returns:** `string` — the line read (empty string if EOF is reached immediately).

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var name = readln();
    println("Hello, ".concat(name));
}
```

---

### `print_format`

Prints a formatted string to stdout, supporting printf-style format specifiers.

**Syntax:**
```hoo
print_format(fmt: string, ...) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `fmt` | `string` | Format string with printf-style placeholders |
| `...` | variadic | Values to substitute into the format string |

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var name = "Alice";
    var age = 30;
    print_format("Name: %s, Age: %d\n", name, age);
}
```

---
### `readchar`

Reads a single character from stdin. Blocks until a character is available.

**Syntax:**
```hoo
readchar() :int64
```
**Parameters:** None.
**Returns:** `int64` — The character code of the read character, or `-1` if EOF is reached.
**Errors:** Returns `-1` on EOF or error.
**Complete Example:**
```hoo
import hoo;

func :void example() {
    print("Press any key: ");
    var ch = readchar();
    println("You pressed: " + ch);
}
```

---

## Usage Example

```hoo
import hoo;

func :int64 main() {
    print("Enter your name: ");
    var name = readln();

    if (name.length() == 0) {
        print_error("No input received.");
        return 1;
    }

    print_format("Hello, %s!\n", name);
    return 0;
}
```
