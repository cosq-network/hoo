# Collections API Reference

The hoo collections module provides powerful, managed data structures for storing and organizing data: **Arrays** and **Maps**. Both are managed via Automatic Reference Counting (ARC).

## 1. Arrays (`HooArray`)

Hoo arrays are dynamic, contiguous memory blocks capable of storing 64-bit values (integers, floats, or pointers to managed objects).

### `array_new() -> array`
Creates a new, empty array with initial capacity.

### `array_length(arr: array) -> int64`
Returns the number of elements currently stored in the array.

### `array_push_int64(arr: array, val: int64)`
Appends an integer to the end of the array.

### `array_push_double(arr: array, val: double)`
Appends a double-precision float to the end of the array.

### `array_push_string(arr: array, str: string)`
Appends a managed string to the end of the array.

### `array_push_object(arr: array, obj: ptr)`
Appends a managed object or pointer to the end of the array.

### `array_get_int64(arr: array, index: int64) -> int64`
Retrieves the integer at the specified index.
- **Note**: Ensure the index is within bounds (`0` to `length - 1`).

### `array_get_string(arr: array, index: int64) -> string`
Retrieves the string at the specified index.

### `array_pop(arr: array)`
Removes the last element from the array.

### `array_clear(arr: array)`
Removes all elements from the array.

---

## 2. Maps (`HooMap`)

Hoo maps are type-safe dictionaries that map keys to values. Keys are restricted to specific types (int64, string, etc.) for efficient hashing.

### `map_new(keyType: int) -> map`
Creates a new map bound to a specific key type.
- **Key Types**:
  - `2`: `int64`
  - `4`: `string`

### `map_length(map: map) -> int64`
Returns the number of entries in the map.

### `map_set_string_int64(map: map, key: string, val: int64)`
Associates an integer value with a string key.

### `map_get_string_int64(map: map, key: string) -> int64`
Retrieves the integer value associated with a string key. Returns 0 if not found.

### `map_set_string_string(map: map, key: string, val: string)`
Associates a string value with a string key.

### `map_contains_string(map: map, key: string) -> int64`
Returns 1 if the map contains the specified string key, 0 otherwise.

### `map_remove_string(map: map, key: string)`
Removes the entry associated with the string key.

---

## Usage Example

```hoo
func :int64 main() {
    // Array Example
    var numbers = array_new();
    array_push_int64(numbers, 10);
    array_push_int64(numbers, 20);
    array_push_int64(numbers, 30);
    
    var len = array_length(numbers);
    var first = array_get_int64(numbers, 0); // 10
    
    // Map Example
    var config = map_new(4); // 4 = string key type
    map_set_string_int64(config, "port", 8080);
    map_set_string_int64(config, "timeout", 30);
    
    if (map_contains_string(config, "port")) {
        var p = map_get_string_int64(config, "port");
        println(string_concat("Port: ", string_from_int64(p)));
    }
    
    return 0;
}
```
