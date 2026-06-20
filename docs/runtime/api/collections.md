# Collections API Reference

**Import Requirement:**
```hoo
import hoo.collections;
```

The hoo collections module provides managed data structures for storing and organizing data: **Arrays** and **Maps**.

## 1. Arrays (`Array`)

Hoo arrays are dynamic, contiguous blocks capable of storing 64-bit values (integers, floats, or managed objects).

### `new Array() :array`
Creates a new, empty array with initial capacity.

### `arr.length() :int64`
Returns the number of elements currently stored in the array.

### `arr.push(val: int64)`
Appends an integer to the end of the array.

### `arr.push(val: double)`
Appends a double-precision float to the end of the array.

### `arr.push(val: string)`
Appends a string to the end of the array.

### `arr.getInt64(index: int64) :int64`
Retrieves the integer at the specified index.
- **Note**: Ensure the index is within bounds (`0` to `length - 1`).

### `arr.getString(index: int64) :string`
Retrieves the string at the specified index.

### `arr.pop()`
Removes the last element from the array.

### `arr.clear()`
Removes all elements from the array.

---

## 1.5 Heterogeneous Arrays (`AnyArray`)

`AnyArray` is an intrinsic managed collection whose element type is `any`.

### `new AnyArray() :AnyArray`
Creates an empty heterogeneous array.

### `[expr, ...]any :AnyArray`
Creates an `AnyArray` literal and packs each element as `(type_id, data)`.

### `arr.length() :int64`
Returns the number of elements.

### `arr.push(value) :int64`
Appends any primitive or managed value. Returns `1` on success.

### `arr[index]`
Returns the stored value payload for current expression lowering. The raw runtime getter returns a full tagged `HooAnyValue`.

### `arr[index] = value`
Replaces an existing element and updates ARC ownership for managed payloads.

### `arr.clear()`
Releases managed payloads and removes all elements.

---

## 2. Maps (`Map`)

Hoo maps are type-safe dictionaries that map keys to values. Keys are restricted to specific types (int64, string, etc.) for efficient hashing, and value types can also be specified at creation.

### `new Map(keyType: int64, valueType: int64) :map`
Creates a new map bound to a specific key type and value type.
- **Key Types** (`HooMapKeyType`):
  - `0`: `byte`
  - `1`: `int8`
  - `2`: `int64`
  - `3`: `char` (runtime only — no Hoo-level wrappers; use C API)
  - `4`: `string`
- **Value Types** (`HooMapValueType`):
  - `0`: `any` (no enforcement)
  - `1`: `int64`
  - `2`: `double`
  - `3`: `bool`
  - `4`: `string`
  - `5`: `object`
### `m.length() :int64`
Returns the number of entries in the map.

### `m.empty() :int64`
Returns 1 if the map has no entries, 0 otherwise.

### `m.clear()`
Removes all entries from the map.

### `m.keyType() :int64`
Returns the key type of the map (HooMapKeyType value).

### `m.valueType() :int64`
Returns the value type of the map (HooMapValueType value).

### Int64 Key Operations

| Method | Signature |
|--------|-----------|
| `containsInt64` | `m.containsInt64(key: int64) :int64` |
| `removeInt64` | `m.removeInt64(key: int64)` |
| `setInt64Int64` | `m.setInt64Int64(key: int64, val: int64)` |
| `getInt64Int64` | `m.getInt64Int64(key: int64) :int64` |
| `setInt64Double` | `m.setInt64Double(key: int64, val: double)` |
| `getInt64Double` | `m.getInt64Double(key: int64) :double` |
| `setInt64String` | `m.setInt64String(key: int64, val: string)` |
| `getInt64String` | `m.getInt64String(key: int64) :string` |
| `setInt64Bool` | `m.setInt64Bool(key: int64, val: bool)` |
| `getInt64Bool` | `m.getInt64Bool(key: int64) :bool` |

### String Key Operations

| Method | Signature |
|--------|-----------|
| `containsString` | `m.containsString(key: string) :int64` |
| `removeString` | `m.removeString(key: string)` |
| `setStringInt64` | `m.setStringInt64(key: string, val: int64)` |
| `getStringInt64` | `m.getStringInt64(key: string) :int64` |
| `setStringDouble` | `m.setStringDouble(key: string, val: double)` |
| `getStringDouble` | `m.getStringDouble(key: string) :double` |
| `setStringString` | `m.setStringString(key: string, val: string)` |
| `getStringString` | `m.getStringString(key: string) :string` |
| `setStringBool` | `m.setStringBool(key: string, val: bool)` |
| `getStringBool` | `m.getStringBool(key: string) :bool` |

### Int8 Key Operations

| Method | Signature |
|--------|-----------|
| `containsInt8` | `m.containsInt8(key: int8) :int64` |
| `removeInt8` | `m.removeInt8(key: int8)` |
| `setInt8Int64` | `m.setInt8Int64(key: int8, val: int64)` |
| `getInt8Int64` | `m.getInt8Int64(key: int8) :int64` |
| `setInt8Double` | `m.setInt8Double(key: int8, val: double)` |
| `getInt8Double` | `m.getInt8Double(key: int8) :double` |
| `setInt8String` | `m.setInt8String(key: int8, val: string)` |
| `getInt8String` | `m.getInt8String(key: int8) :string` |
| `setInt8Bool` | `m.setInt8Bool(key: int8, val: bool)` |
| `getInt8Bool` | `m.getInt8Bool(key: int8) :bool` |

---

## 3. Native Hash Maps (`HashMap`)

`HashMap<K, V>` is an intrinsic hash map with `byte`, `int8`, or `int64` keys. `V` may be a fixed value type or `any`.

### `new HashMap<int64, int64>() :HashMap`
Creates a fixed-value native hash map.

### `new HashMap<byte, any>() :HashMap`
Creates a heterogeneous native hash map.

### `m[key] = value`
Stores `value`. Fixed maps store the 64-bit payload directly. `HashMap<K, any>` stores the runtime type ID plus payload and retains managed values.

### `m[key]`
Returns the stored value payload for current expression lowering. The raw runtime getter for `HashMap<K, any>` returns the full tagged `HooAnyValue`.

### `m.count() :int64`
Returns the number of entries.

### `m.remove(key) :int64`
Removes an entry and returns `1` if present.

### `m.clear()`
Clears all entries and releases managed `any` payloads.

---

## Usage Example

```hoo
import hoo.collections;

func :int64 main() {
    // Array Example
    var numbers = new Array();
    numbers.push(10);
    numbers.push(20);
    numbers.push(30);

    var len = numbers.length();
    var first = numbers.getInt64(0); // 10

    // Map Example (string key, int64 value)
    var config = new Map(4, 1); // 4 = string key, 1 = int64 value
    config.setStringInt64("port", 8080);
    config.setStringInt64("timeout", 30);

    if (config.containsString("port")) {
        var p = config.getStringInt64("port");
        println("Port: ".concat(p.toString()));
    }

    // Map Example (int64 key, string value)
    var users = new Map(2, 4); // 2 = int64 key, 4 = string value
    users.setInt64String(1001, "Alice");
    users.setInt64String(1002, "Bob");
    var name = users.getInt64String(1001);

    // Intrinsic heterogeneous collections
    var values = [1, "two", 3]any;
    var mixed: HashMap<byte, any> = new HashMap<byte, any>();
    mixed[1] = values[0];
    mixed[2] = "hello";

    return 0;
}
```
