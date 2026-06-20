# Collections API Reference

## Collections

The Collections module belongs to the Hoo standard library.

## Import Statement

```hoo
import hoo.collections;
```

Some types (Array, Map, Any) are available via `import hoo;` — see each class for details.

## Module Description

The Collections module provides managed data structures for storing and organizing data: dynamic arrays (Array), heterogeneous arrays (AnyArray), associative maps (Map), native hash maps (HashMap), and multi-dimensional tensors (Tensor). All collection types use automatic reference counting (ARC) for memory management.

## Class: Array

A type-agnostic dynamic array that stores elements in contiguous memory. Elements are accessed via type-specific operations.

### Declaration

```hoo
class Array
```

### Import

```hoo
import hoo;
```

### Public Fields

None.

### Public Class (Static) Functions

#### new

Creates a new empty Array with an optional initial capacity hint.

```hoo
Array.new(capacity: int64 = 0): array
```

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity hint (default 0). |

**Returns:** `array` — A new Array handle.

---

#### new_int64

Creates a new Array pre-filled with int64 values.

```hoo
Array.new_int64(capacity: int64 = 0, default_value: int64 = 0): array
```

**Parameters:**

| Parameter      | Type    | Description                         |
|----------------|---------|--------------------------------------|
| `capacity`     | `int64` | Initial capacity (default 0).        |
| `default_value`| `int64` | Default element value (default 0).   |

**Returns:** `array`

---

#### new_double

Creates a new Array pre-filled with double values.

```hoo
Array.new_double(capacity: int64 = 0, default_value: double = 0.0): array
```

**Parameters:**

| Parameter      | Type    | Description                           |
|----------------|---------|----------------------------------------|
| `capacity`     | `int64` | Initial capacity (default 0).          |
| `default_value`| `double`| Default element value (default 0.0).   |

**Returns:** `array`

---

#### new_string

Creates a new Array for storing strings.

```hoo
Array.new_string(capacity: int64 = 0): array
```

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity (default 0).    |

**Returns:** `array`

---

#### new_any

Creates a new Array for storing heterogeneous (any) values.

```hoo
Array.new_any(capacity: int64 = 0): array
```

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity (default 0).    |

**Returns:** `array`

### Public Instance Functions

#### length

Returns the number of elements in the array.

```hoo
arr.length(): int64
```

**Returns:** `int64` — The number of elements.

---

#### push

Appends an element to the end of the array and returns the array itself for chaining.

```hoo
arr.push(val: int64): array
arr.push_int64(val: int64): array
arr.push_double(val: double): array
arr.push_string(val: string): array
arr.push_any(val): array
```

**Parameters:**

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `val`     | *varied* | The value to append. |

**Returns:** `array` — The array handle (may differ after reallocation).

**Errors:** Out-of-memory may cause a runtime error.

---

#### get

Retrieves the element at the specified index.

```hoo
arr.get(index: int64): value
arr.get_int64(index: int64): int64
arr.get_double(index: int64): double
arr.get_string(index: int64): string
arr.get_any(index: int64): any
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |

**Returns:** The element at the given index.

**Errors:** Index out of bounds causes a runtime error.

---

#### set

Sets the element at the specified index.

```hoo
arr.set(index: int64, val: int64)
arr.set_int64(index: int64, val: int64)
arr.set_double(index: int64, val: double)
arr.set_string(index: int64, val: string)
arr.set_any(index: int64, val)
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `index`   | `int64`  | Zero-based element index. |
| `val`     | *varied* | The value to store.       |

**Returns:** `void`

**Errors:** Index out of bounds causes a runtime error.

---

#### clear

Removes all elements from the array.

```hoo
arr.clear(): void
```

**Returns:** `void`

---

#### retain

Increments the reference count.

```hoo
arr.retain(): array
```

**Returns:** `array` — The same array handle.

---

#### release

Decrements the reference count. The array is freed when the count reaches zero.

```hoo
arr.release(): void
```

**Returns:** `void`

---

#### refcount

Returns the current reference count.

```hoo
arr.refcount(): int64
```

**Returns:** `int64` — The reference count.

---

## Class: Map

A type-safe associative dictionary that maps typed keys to typed values. Key and value types are specified at creation.

### Declaration

```hoo
class Map
```

### Import

```hoo
import hoo;
```

### Public Fields

None.

### Public Class (Static) Functions

#### new

Creates a new Map.

```hoo
Map.new(): map
```

**Returns:** `map` — A new Map handle.

**Key Types:**

| Value | Type     |
|-------|----------|
| 0     | `byte`   |
| 1     | `int8`   |
| 2     | `int64`  |
| 3     | `char`   |
| 4     | `string` |

**Value Types:**

| Value | Type      |
|-------|-----------|
| 0     | `any`     |
| 1     | `int64`   |
| 2     | `double`  |
| 3     | `bool`    |
| 4     | `string`  |
| 5     | `object`  |

### Public Instance Functions

#### put

Inserts or updates a key-value pair.

```hoo
m.put(key, value)
```

**Parameters:**

| Parameter | Type  | Description         |
|-----------|-------|----------------------|
| `key`     | *key* | The key to insert.   |
| `value`   | *val* | The value to store.  |

**Returns:** `void`

---

#### get

Retrieves the value associated with a key.

```hoo
m.get(key): value
```

**Parameters:**

| Parameter | Type  | Description          |
|-----------|-------|----------------------|
| `key`     | *key* | The key to look up.  |

**Returns:** The value if the key exists.

**Errors:** Key not found causes a runtime error.

---

#### has

Checks whether a key exists in the map.

```hoo
m.has(key): bool
```

**Parameters:**

| Parameter | Type  | Description          |
|-----------|-------|----------------------|
| `key`     | *key* | The key to check.    |

**Returns:** `bool` — `true` if the key exists, `false` otherwise.

---

#### remove

Removes the entry for the specified key.

```hoo
m.remove(key)
```

**Parameters:**

| Parameter | Type  | Description         |
|-----------|-------|----------------------|
| `key`     | *key* | The key to remove.   |

**Returns:** `void`

---

#### clear

Removes all entries from the map.

```hoo
m.clear()
```

**Returns:** `void`

---

#### keys

Returns an array containing all keys in the map.

```hoo
m.keys(): array
```

**Returns:** `array` — An Array containing all keys.

---

#### values

Returns an array containing all values in the map.

```hoo
m.values(): array
```

**Returns:** `array` — An Array containing all values.

---

#### length

Returns the number of entries in the map.

```hoo
m.length(): int64
```

**Returns:** `int64` — The entry count.

---

#### retain

Increments the reference count.

```hoo
m.retain(): map
```

**Returns:** `map` — The same map handle.

---

#### release

Decrements the reference count. The map is freed when the count reaches zero.

```hoo
m.release(): void
```

**Returns:** `void`

---

#### refcount

Returns the current reference count.

```hoo
m.refcount(): int64
```

**Returns:** `int64` — The reference count.

---

## Class: HashMap

A native hash map with fixed key and value types. Keys are restricted to `byte`, `int8`, or `int64`.

### Declaration

```hoo
class HashMap
```

### Import

```hoo
import hoo.collections;
```

### Public Fields

None.

### Public Class (Static) Functions

#### new

Creates a new HashMap with the specified initial bucket count.

```hoo
HashMap.new(size: int64 = 16): HashMap
```

**Parameters:**

| Parameter | Type    | Description                        |
|-----------|---------|-------------------------------------|
| `size`    | `int64` | Initial bucket count (default 16). |

**Returns:** `HashMap`

### Public Instance Functions

#### put

Inserts or updates a key-value pair.

```hoo
m.put(key, value)
```

**Parameters:**

| Parameter | Type  | Description         |
|-----------|-------|----------------------|
| `key`     | *key* | The key to insert.   |
| `value`   | *val* | The value to store.  |

**Returns:** `void`

---

#### get

Retrieves the value associated with a key.

```hoo
m.get(key): value
```

**Parameters:**

| Parameter | Type  | Description          |
|-----------|-------|----------------------|
| `key`     | *key* | The key to look up.  |

**Returns:** The value if the key exists.

**Errors:** Key not found causes a runtime error.

---

#### has

Checks whether a key exists in the map.

```hoo
m.has(key): bool
```

**Parameters:**

| Parameter | Type  | Description          |
|-----------|-------|----------------------|
| `key`     | *key* | The key to check.    |

**Returns:** `bool` — `true` if the key exists, `false` otherwise.

---

#### remove

Removes the entry for the specified key.

```hoo
m.remove(key)
```

**Parameters:**

| Parameter | Type  | Description         |
|-----------|-------|----------------------|
| `key`     | *key* | The key to remove.   |

**Returns:** `void`

---

#### clear

Removes all entries from the map.

```hoo
m.clear()
```

**Returns:** `void`

---

#### length

Returns the number of entries in the map.

```hoo
m.length(): int64
```

**Returns:** `int64` — The entry count.

---

#### keys

Returns an array containing all keys in the map.

```hoo
m.keys(): array
```

**Returns:** `array`

---

#### values

Returns an array containing all values in the map.

```hoo
m.values(): array
```

**Returns:** `array`

---

#### key_type

Returns the key type identifier of the map.

```hoo
m.key_type(): int64
```

**Returns:** `int64` — The key type identifier.

---

#### value_type

Returns the value type identifier of the map.

```hoo
m.value_type(): int64
```

**Returns:** `int64` — The value type identifier.

---

#### retain

Increments the reference count.

```hoo
m.retain(): HashMap
```

**Returns:** `HashMap` — The same map handle.

---

#### release

Decrements the reference count. Freed when count reaches zero.

```hoo
m.release(): void
```

**Returns:** `void`

---

#### refcount

Returns the current reference count.

```hoo
m.refcount(): int64
```

**Returns:** `int64` — The reference count.

---

## Class: Any

A tagged union type that can hold values of any type. The runtime representation pairs a type ID with a 64-bit data payload.

### Declaration

```hoo
class Any
```

### Import

```hoo
import hoo;
```

### Public Fields

None.

### Public Class (Static) Functions

None.

### Public Instance Functions

#### is_null

Checks whether the value is null.

```hoo
val.is_null(): bool
```

**Returns:** `bool`

---

#### is_array

Checks whether the value is an array.

```hoo
val.is_array(): bool
```

**Returns:** `bool`

---

#### is_string

Checks whether the value is a string.

```hoo
val.is_string(): bool
```

**Returns:** `bool`

---

#### is_int64

Checks whether the value is an int64.

```hoo
val.is_int64(): bool
```

**Returns:** `bool`

---

#### is_double

Checks whether the value is a double.

```hoo
val.is_double(): bool
```

**Returns:** `bool`

---

#### is_bool

Checks whether the value is a bool.

```hoo
val.is_bool(): bool
```

**Returns:** `bool`

---

#### as_int64

Extracts the value as an int64.

```hoo
val.as_int64(): int64
```

**Returns:** `int64`

**Errors:** Type mismatch causes a runtime error.

---

#### as_double

Extracts the value as a double.

```hoo
val.as_double(): double
```

**Returns:** `double`

**Errors:** Type mismatch causes a runtime error.

---

#### as_string

Extracts the value as a string.

```hoo
val.as_string(): string
```

**Returns:** `string`

**Errors:** Type mismatch causes a runtime error.

---

#### as_bool

Extracts the value as a bool.

```hoo
val.as_bool(): bool
```

**Returns:** `bool`

**Errors:** Type mismatch causes a runtime error.

---

#### as_array

Extracts the value as an array.

```hoo
val.as_array(): array
```

**Returns:** `array`

**Errors:** Type mismatch causes a runtime error.

---

## Class: AnyArray

A heterogeneous dynamic array whose elements are typed as `any`. Elements are stored as a tagged pair of type ID and data payload.

### Declaration

```hoo
class AnyArray
```

### Import

```hoo
import hoo.collections;
```

### Public Fields

None.

### Public Class (Static) Functions

None.

### Public Instance Functions

#### length

Returns the number of elements in the array.

```hoo
arr.length(): int64
```

**Returns:** `int64`

---

#### push_back

Appends a value to the end of the array.

```hoo
arr.push_back(val)
```

**Parameters:**

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `val`     | *varied* | The value to append. |

**Returns:** `void`

---

#### get

Retrieves the element at the specified index.

```hoo
arr.get(index: int64): any
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |

**Returns:** `any`

**Errors:** Index out of bounds causes a runtime error.

---

#### set

Sets the element at the specified index.

```hoo
arr.set(index: int64, val)
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `index`   | `int64`  | Zero-based element index. |
| `val`     | *varied* | The value to store.       |

**Returns:** `void`

**Errors:** Index out of bounds causes a runtime error.

---

#### retain

Increments the reference count.

```hoo
arr.retain(): AnyArray
```

**Returns:** `AnyArray`

---

#### release

Decrements the reference count. Freed when count reaches zero.

```hoo
arr.release(): void
```

**Returns:** `void`

---

#### refcount

Returns the current reference count.

```hoo
arr.refcount(): int64
```

**Returns:** `int64`

---

## Class: Tensor

A multi-dimensional array (tensor) supporting up to 3 dimensions with typed elements. Supports element-wise arithmetic, comparison, and logical operations.

### Declaration

```hoo
class Tensor
```

### Import

```hoo
import hoo.collections;
```

### Public Fields

None.

### Public Class (Static) Functions

#### new

Creates a new Tensor with the given dimension sizes.

```hoo
Tensor.new(dimensions: int64, ...): Tensor
```

**Parameters:**

| Parameter    | Type    | Description                           |
|--------------|---------|---------------------------------------|
| `dimensions` | `int64` | One or more dimension sizes (1–3).    |

**Returns:** `Tensor`

**Example:**

```hoo
var t = Tensor.new(4, 4)       // 4x4 matrix
var t3 = Tensor.new(2, 3, 4)   // 2x3x4 tensor
```

### Public Instance Functions

#### rank

Returns the number of dimensions.

```hoo
t.rank(): int64
```

**Returns:** `int64` — The rank (1, 2, or 3).

---

#### shape

Returns the size of a specific dimension.

```hoo
t.shape(dim: int64): int64
```

**Parameters:**

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| `dim`     | `int64` | The dimension axis (0-based).    |

**Returns:** `int64` — The size of the specified dimension.

---

#### num_elements

Returns the total number of elements in the tensor.

```hoo
t.num_elements(): int64
```

**Returns:** `int64`

---

#### get

Retrieves an element at the specified linear index.

```hoo
t.get(index: int64): value
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Linear (flattened) index. |

**Returns:** The element value.

**Errors:** Index out of bounds causes a runtime error.

---

#### set

Sets an element at the specified linear index.

```hoo
t.set(index: int64, val)
```

**Parameters:**

| Parameter | Type     | Description                |
|-----------|----------|----------------------------|
| `index`   | `int64`  | Linear (flattened) index.  |
| `val`     | *varied* | The value to store.        |

**Returns:** `void`

**Errors:** Index out of bounds causes a runtime error.

---

#### retain

Increments the reference count.

```hoo
t.retain(): Tensor
```

**Returns:** `Tensor`

---

#### release

Decrements the reference count. Freed when count reaches zero.

```hoo
t.release(): void
```

**Returns:** `void`

---

#### refcount

Returns the current reference count.

```hoo
t.refcount(): int64
```

**Returns:** `int64`

---

## Usage Example

```hoo
import hoo;
import hoo.collections;

func :int64 main() {
    // Array example
    var numbers = Array.new();
    numbers.push_int64(10);
    numbers.push_int64(20);
    numbers.push_int64(30);
    var first = numbers.get_int64(0);

    // Map example
    var config = Map.new();
    config.put("port", 8080);
    if config.has("port") {
        var port = config.get("port");
    }

    // HashMap example
    var hmap = HashMap.new();
    hmap.put(1, "hello");
    hmap.put(2, "world");

    // AnyArray example
    var anyarr = AnyArray.new();
    anyarr.push_back(42);
    anyarr.push_back("text");

    // Tensor example
    var tensor = Tensor.new(3, 3);
    tensor.set(0, 1);
    tensor.set(4, 2);

    return 0;
}
```
