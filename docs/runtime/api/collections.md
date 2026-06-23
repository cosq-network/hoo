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

### Free Functions

#### array_new(capacity: int64) :array

Creates a new empty Array with the given initial capacity.

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity. |

**Returns:** `array` — A new Array handle.

---

#### array_new_int64(capacity: int64, default_value: int64) :array

Creates a new Array pre-filled with int64 values.

**Parameters:**

| Parameter      | Type    | Description                         |
|----------------|---------|--------------------------------------|
| `capacity`     | `int64` | Initial capacity. |
| `default_value`| `int64` | Default element value. |

**Returns:** `array`

---

#### array_new_double(capacity: int64, default_value: double) :array

Creates a new Array pre-filled with double values.

**Parameters:**

| Parameter      | Type    | Description                           |
|----------------|---------|----------------------------------------|
| `capacity`     | `int64` | Initial capacity. |
| `default_value`| `double`| Default element value. |

**Returns:** `array`

---

#### array_new_string(capacity: int64) :array

Creates a new Array for storing strings.

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity. |

**Returns:** `array`

### Public Instance Functions

#### length

Returns the number of elements in the array.

```hoo
arr.length(): int64
```

**Returns:** `int64` — The number of elements.

---
> **Supported Element Types**
> `Array` can store only one of the following intrinsic types per instance:
> - `int8`
> - `byte` (alias of `uint8`)
> - `int64`
> - `double`
> - `bool` (stored as `int64` 0/1)
> - `string` (UTF‑8)
> - `character` (Unicode code point)
>
> **Homogeneity rule** – The element type is fixed on the first `push_*` or by using a type‑specific constructor (`array_new_int64`, `array_new_string`, …). All subsequent operations must use the same type; mismatched pushes raise a runtime type‑mismatch error.
>
> **Syntax flavors**
> - **Typed constructor** – `array_new_int64(capacity)` creates an `Array` of `int64`.
> - **Generic constructor** – `array_new(capacity)` creates an empty array; the type is inferred from the first push.
> - **Literal** – `[1, 2, 3]` or `["a","b"]` creates a homogeneous array with the inferred type.
>
> **Validations**
> - Compile‑time: mixed‑type literals are rejected.
> - Runtime: `push_*` with a mismatched type aborts with a type‑mismatch error.
> - Index bounds are checked on `get_*`, `set_*`, `clear`, etc.
>
>---

#### push_int64

Appends an int64 element to the end of the array and returns the array itself for chaining.

```hoo
arr.push_int64(val: int64): array
```

**Parameters:**

| Parameter | Type    | Description          |
|-----------|---------|----------------------|
| `val`     | `int64` | The value to append. |

**Returns:** `array` — The array handle (may differ after reallocation).

**Errors:** Out-of-memory may cause a runtime error.

---

#### push_double

Appends a double element to the end of the array and returns the array itself for chaining.

```hoo
arr.push_double(val: double): array
```

**Parameters:**

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `val`     | `double` | The value to append. |

**Returns:** `array` — The array handle (may differ after reallocation).

**Errors:** Out-of-memory may cause a runtime error.

---

#### push_string

Appends a string element to the end of the array and returns the array itself for chaining.

```hoo
arr.push_string(val: string): array
```

**Parameters:**

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `val`     | `string` | The value to append. |

**Returns:** `array` — The array handle (may differ after reallocation).

**Errors:** Out-of-memory may cause a runtime error.

---

#### get_int64

Retrieves the int64 element at the specified index.

```hoo
arr.get_int64(index: int64): int64
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |

**Returns:** `int64` — The int64 element at the given index.

**Errors:** Index out of bounds causes a runtime error.

---

#### get_double

Retrieves the double element at the specified index.

```hoo
arr.get_double(index: int64): double
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |

**Returns:** `double` — The double element at the given index.

**Errors:** Index out of bounds causes a runtime error.

---

#### get_string

Retrieves the string element at the specified index.

```hoo
arr.get_string(index: int64): string
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |

**Returns:** `string` — The string element at the given index.

**Errors:** Index out of bounds causes a runtime error.

---

#### set_int64

Sets the int64 element at the specified index.

```hoo
arr.set_int64(index: int64, val: int64)
```

**Parameters:**

| Parameter | Type    | Description               |
|-----------|---------|---------------------------|
| `index`   | `int64` | Zero-based element index. |
| `val`     | `int64` | The value to store.       |

**Returns:** `void`

**Errors:** Index out of bounds causes a runtime error.

---

#### set_double

Sets the double element at the specified index.

```hoo
arr.set_double(index: int64, val: double)
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `index`   | `int64`  | Zero-based element index. |
| `val`     | `double` | The value to store.       |

**Returns:** `void`

**Errors:** Index out of bounds causes a runtime error.

---

#### set_string

Sets the string element at the specified index.

```hoo
arr.set_string(index: int64, val: string)
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `index`   | `int64`  | Zero-based element index. |
| `val`     | `string` | The value to store.       |

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

#### sort

Sorts the array elements in-place in ascending order.

```hoo
arr.sort(): array
```

**Type-specific behavior:**

| Element Type | Comparison |
|-------------|------------|
| `int64` | Numeric ascending |
| `double` | IEEE 754 ascending |
| `bool` | `false` (0) before `true` (1) |
| `char`, `string`, `object` | Bitwise/pointer ordering |

**Returns:** `array` — The same array handle (sorted in-place).

**Notes:** This is an in-place operation. The element type is detected at runtime:
- `int64` arrays use `qsort` with numeric comparison.
- `double` arrays use `qsort` with IEEE 754 double comparison (correct for negative values).
- All other types use bitwise comparison of the 64-bit storage slot.

---

#### reverse

Reverses the array elements in-place.

```hoo
arr.reverse(): array
```

**Returns:** `array` — The same array handle (reversed in-place).

**Example:**
```hoo
var a = new Array();
Array.pushInt64(a, 1);
Array.pushInt64(a, 2);
Array.pushInt64(a, 3);
a.reverse();  // a is now [3, 2, 1]
```

---

## Example Programs

```hoo
import hoo;

// int64 array example
arr = array_new_int64(0);
arr = arr.push_int64(10).push_int64(20).push_int64(30);
println("len=" + arr.length());
println("elem[1]=" + arr.get_int64(1));

// double array using generic constructor and first push type inference
arr2 = array_new(0);
arr2 = arr2.push_double(1.5);
arr2 = arr2.push_double(2.5);
for (i in arr2) {
    println(i);
}

// string array literal
strArr = ["foo", "bar", "baz"];
strArr = strArr.push_string("qux"); // type already inferred as string

// error handling: mismatched push (runtime abort) – shown as comment
// strArr.push_int64(123); // ❌ type‑mismatch error
```
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

### Free Functions

#### map_new() :map

Creates a new Map.

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
m.has(key): int64
```

**Parameters:**

| Parameter | Type  | Description          |
|-----------|-------|----------------------|
| `key`     | *key* | The key to check.    |

**Returns:** `int64` — `1` if the key exists, `0` otherwise.

---

#### remove

Removes the entry for the specified key.

```hoo
m.remove(key): int64
```

**Parameters:**

| Parameter | Type  | Description         |
|-----------|-------|----------------------|
| `key`     | *key* | The key to remove.   |

**Returns:** `int64` — `1` if the entry was removed, `0` if the key was not found.

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

### Constructor

#### HashMap(size: int64) :HashMap

Creates a new HashMap with the specified initial bucket count.

**Parameters:**

| Parameter | Type    | Description                        |
|-----------|---------|-------------------------------------|
| `size`    | `int64` | Initial bucket count. |

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

### Constructor

#### AnyArray(capacity: int64) :AnyArray

Creates a new AnyArray with the given initial capacity.

**Parameters:**

| Parameter  | Type    | Description                     |
|------------|---------|----------------------------------|
| `capacity` | `int64` | Initial capacity. |

**Returns:** `AnyArray`

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

### Constructor

#### Tensor(shape: array) :Tensor

Creates a new Tensor with the given dimension sizes.

**Parameters:**

| Parameter | Type    | Description                           |
|-----------|---------|---------------------------------------|
| `shape`   | `array` | Array of int64 dimension sizes (1–3). |

**Returns:** `Tensor`

**Example:**

```hoo
var t = Tensor([4, 4])       // 4x4 matrix
var t3 = Tensor([2, 3, 4])   // 2x3x4 tensor
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
    var numbers = array_new(0);
    numbers.push_int64(10);
    numbers.push_int64(20);
    numbers.push_int64(30);
    var first = numbers.get_int64(0);

    // Map example
    var config = map_new();
    config.put("port", 8080);
    if config.has("port") {
        var port = config.get("port");
    }

    // HashMap example
    var hmap = HashMap(16);
    hmap.put(1, "hello");
    hmap.put(2, "world");

    // AnyArray example
    var anyarr = AnyArray(0);
    anyarr.push_back(42);
    anyarr.push_back("text");

    // Tensor example
    var tensor = Tensor([3, 3]);
    tensor.set(0, 1);
    tensor.set(4, 2);

    return 0;
}
```
