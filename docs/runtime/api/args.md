# Args API Reference

## Module Name

`args` — part of the core `hoo` module.

## Import Statement

```hoo
import hoo.args;
```

## Module Description

The `args` module provides free functions for accessing command-line arguments passed to the program. The Hoo runtime internally initializes and cleans up the argument state; these internals are not user-facing.

## Free Functions

---

#### `args_get`

**Description:** Returns the command-line argument at the given zero-based index. Index `0` returns the program name.

**Syntax:**

```hoo
args_get(index: int64) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | Zero-based argument index. `0` returns the program name. |

**Returns:** `string` — The argument value at the specified index.

**Errors:** Returns an empty string if the index is out of range.

**Complete Example:**

```hoo
import hoo.args;

func :int64 main() {
    var name = args_get(0);
    println(name);
    return name.length();
}
```

---

#### `args_count`

**Description:** Returns the total number of command-line arguments, including the program name.

**Syntax:**

```hoo
args_count() :int64
```

**Parameters:** None.

**Returns:** `int64` — The total number of arguments.

**Errors:** Returns `0` when no argument data is available.

**Complete Example:**

```hoo
import hoo.args;

func :int64 main() {
    var argc = args_count();
    println("arguments: " + argc);
    return argc;
}
```

## Usage Example

```hoo
import hoo.args;

func :int64 main() {
    var argc = args_count();
    var i: int64 = 0;
    while i < argc {
        var arg = args_get(i);
        println(arg);
        i = i + 1;
    }
    return argc;
}
```
