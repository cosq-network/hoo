# Args API Reference

## Module Name

`Args` — part of the core `hoo` module.

## Import Statement

```hoo
import hoo;
```

## Module Description

The `Args` class provides access to command-line arguments passed to the program. The Hoo runtime internally calls `args_init` and `args_shutdown` to initialize and clean up the argument state; these functions are not user-facing.

## Class: `Args`

### Declaration

```hoo
class Args
```

### Public Fields

None — Args is an opaque class with only static methods.

### Public Class (Static) Functions

---

#### `Args.get`

**Description:** Returns the command-line argument at the given zero-based index. Index `0` returns the program name.

**Syntax:**

```hoo
Args.get(index: int64) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | Zero-based argument index. `0` returns the program name. |

**Returns:** `string` — The argument value at the specified index.

**Errors:** Returns an empty string if the index is out of range.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var name = Args.get(0);
    println(name);
    return name.length();
}
```

---

#### `Args.count`

**Description:** Returns the total number of command-line arguments, including the program name.

**Syntax:**

```hoo
Args.count() :int64
```

**Parameters:** None.

**Returns:** `int64` — The total number of arguments.

**Errors:** Returns `0` when no argument data is available.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var argc = Args.count();
    println("arguments: " + argc);
    return argc;
}
```

## Usage Example

```hoo
import hoo;

func :int64 main() {
    var argc = Args.count();
    var i: int64 = 0;
    while i < argc {
        var arg = Args.get(i);
        println(arg);
        i = i + 1;
    }
    return argc;
}
```
