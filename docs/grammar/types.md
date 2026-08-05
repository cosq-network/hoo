# Type System

Hoo is a statically-typed language. Every expression and variable has a type known at compile-time.

## 1. Primitive Types

| Type | Description |
| :--- | :--- |
| `int8` | 8-bit signed integer. |
| `byte` | 8-bit unsigned integer. |
| `int64` | 64-bit signed integer (Standard integer). |
| `float` | 32-bit floating point. |
| `double` | 64-bit floating point. |
| `f64` | Alias for `double`. |
| `f8` | 8-bit floating point (used for AI/ML quantization). |
| `bit` | 1-bit value / boolean literal backing. |
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

### Intrinsic `any`, `HashMap`, and `AnyArray`
`any` is the tagged value type used by heterogeneous intrinsic collections. It carries a runtime type ID plus a 64-bit data payload.

- `any` - A virtual tagged value type. Type ID `0`.
- `HashMap<int64, int64>` - A native hash map from `int64` keys to fixed-width `int64` values.
- `HashMap<byte, any>` - A native hash map from `byte` keys to heterogeneous tagged values.
- `AnyArray` - A variable-length array whose element type is always `any`.

`HashMap` keys are intentionally restricted to hardware-friendly integer scalars: `byte`, `int8`, and `int64`. `HashMap` uses angle brackets (`HashMap<int64, any>`), while the older `map` type keeps square brackets (`map[string, int64]`).

`AnyArray` literals use an explicit `any` suffix:

```hoo
var values = [1, "two", 3.0]any;
var more = new AnyArray(16);
```

*See also: [ISSUE-033](../issues/ISSUE-033_hashmap_intrinsic.md) and [Runtime Collections](../runtime/collections.md#3-intrinsic-heterogeneous-collections-any-hashmap-anyarray).*

### Tensors
Tensors are multi-dimensional, fixed-size numerical arrays optimized for AI/ML algebra.
- Supported element types are `bit`, `int8`, `byte`, `int64`, `f8`, and `f64`.
- Shapes may have rank 1, 2, or 3 dimensions, for example `tensor<f8>[3, 3]`.
- `tensor<bit>` uses packed bits; `int8`, `byte`, and `f8` use one byte per element.
- Tensor literals use the `t` suffix, for example `[1, 2, 3]t`.

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
