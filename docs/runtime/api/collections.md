# Collections API Reference

The hoo collections module provides managed data structures for storing and organizing data: **Arrays** and **Maps**.

## 1. Arrays (`Array`)

Hoo arrays are dynamic, contiguous blocks capable of storing 64-bit values (integers, floats, or managed objects).

### `Array.new() :array`
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

## 2. Maps (`Map`)

Hoo maps are type-safe dictionaries that map keys to values. Keys are restricted to specific types (int64, string, etc.) for efficient hashing.

### `Map.new(keyType: int64) :map`
Creates a new map bound to a specific key type.
- **Key Types**:
  - `2`: `int64`
  - `4`: `string`

### `m.length() :int64`
Returns the number of entries in the map.

### `m.set(key: string, val: int64)`
Associates an integer value with a string key.

### `m.getInt64(key: string) :int64`
Retrieves the integer value associated with a string key. Returns 0 if not found.

### `m.set(key: string, val: string)`
Associates a string value with a string key.

### `m.contains(key: string) :int64`
Returns 1 if the map contains the specified string key, 0 otherwise.

### `m.remove(key: string)`
Removes the entry associated with the string key.

---

## Usage Example

```hoo
func :int64 main() {
    // Array Example
    var numbers = Array.new();
    numbers.push(10);
    numbers.push(20);
    numbers.push(30);
    
    var len = numbers.length();
    var first = numbers.getInt64(0); // 10
    
    // Map Example
    var config = Map.new(4); // 4 = string key type
    config.set("port", 8080);
    config.set("timeout", 30);
    
    if (config.contains("port")) {
        var p = config.getInt64("port");
        println("Port: ".concat(p.toString()));
    }
    
    return 0;
}
```
