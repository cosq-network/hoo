# Type System

Hooc is a statically-typed language. Every expression and variable has a type known at compile-time.

## 1. Primitive Types

| Type | Description |
| :--- | :--- |
| `int8` | 8-bit signed integer. |
| `byte` | 8-bit unsigned integer (alias for `int8` in current HVM). |
| `int64` | 64-bit signed integer (Standard integer). |
| `float` | 32-bit floating point. |
| `double` | 64-bit floating point. |
| `f64` | Alias for `double`. |
| `bool` | Boolean (`true` or `false`). |
| `char` | Single character. |
| `string` | Managed UTF-8 string. (*See: [Runtime Strings](../runtime/strings.md)*) |
| `void` | Represents the absence of a value (used for return types). |

## 2. Complex Types

**Implementation Note:** All complex types (Strings, Arrays, Maps, and Objects) are implemented as Automatic Reference Counting (ARC) managed opaque handles. For details on how the runtime manages these objects, see the [Runtime Memory Model](../runtime/memory-model.md).

### Arrays
Arrays are dynamic, managed buffers of a specific type. They are declared by appending `[]` to a base type.
- `int64[]` - An array of integers.
- `string[][]` - A two-dimensional array of strings.

*See also: [Runtime Collections: Arrays](../runtime/collections.md#1-arrays-hooarray).*

### Maps
Maps are built-in dictionary types mapping keys to values.
- `map[string, int64]` - A map from strings to integers.
- **Valid Key Types**: `byte`, `int8`, `int64`, `char`, `string`.

*See also: [Runtime Collections: Maps](../runtime/collections.md#2-maps-hoomap).*

### Nullable (Optional) Types
Types can be marked as nullable by appending a `?`.
- `string?` - A string that could be `null`.
- `User?` - An object reference that could be `null`.

## 3. Qualified Types
Types can be qualified by their module path using the dot notation.
- `math.Matrix`
- `hoo.io.File`

## 4. FFI-Specific Types
In `native` and `extern` declarations, additional low-level types are available:
- `pointer[T]` - A raw memory pointer to type `T`.
- `array[N] T` - A fixed-size C-style array of `N` elements of type `T`.
- `function(T1, T2) -> T3` - A native function pointer.
