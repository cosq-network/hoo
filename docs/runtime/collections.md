# Collections

The runtime provides high-level `Array` and `Map` primitives that are natively implemented in C++ and exposed to the HVM via the opaque handle ABI.

## 1. Arrays (`HooArray`)

The generic dynamic array is backed by a custom `HooArrayImpl` class that wraps a `std::vector<std::any>`. 
- **Type Safety via `std::any`**: Elements are securely wrapped in `std::any` (e.g., `std::any(int64_t)`).
- **ARC Integration**: The `HooArrayImpl` instance itself is allocated via `hoo_alloc` so it possesses the standard 16-byte ARC header.
- **Nested Arrays**: Supports multidimensional arrays. When an array is pushed into another, it is explicitly retained (`hoo_array_retain`).

### Key Operations
- **Creation**: `hoo_array_new()`
- **Type-Specific Push**: Because HVM registers hold raw 64-bit bits, type-specific push methods are used to correctly package the values into `std::any`:
  - `hoo_array_push_int64(arr, val)`
  - `hoo_array_push_double(arr, val)`
  - `hoo_array_push_string(arr, str_handle)`
  - `hoo_array_push_object(arr, obj_handle)`
- **Type-Specific Get**: Similarly, retrievals use type-specific functions to unpack the `std::any`.
  - `hoo_array_get_int64(arr, index, *dest)` (Returns 1 on success, 0 on out-of-bounds/mismatch).
- **Inspection**: `hoo_array_length(arr)`, `hoo_array_element_type(arr)`.

---

## 2. Maps (`HooMap`)

The `HooMap` is a type-safe dictionary. Because keys must be hashed efficiently, the internal implementation (`HooMapImpl`) utilizes disjoint `std::unordered_map` instances based on the key type.

### Internal Maps:
```cpp
std::unordered_map<int8_t, std::any> data_int8_;
std::unordered_map<int64_t, std::any> data_int64_;
std::unordered_map<char, std::any> data_char_;
std::unordered_map<std::string, std::any> data_string_;
```

### Key Operations
- **Creation**: `hoo_map_new(int keyType)` initializes the map bound to a specific `HOO_MAP_KEY_*` identifier.
- **Insertion**: Utilizes the cross-product of key and value types.
  - `hoo_map_set_string_int64(map, "key", 42)`
  - `hoo_map_set_int64_object(map, 100, obj_handle)`
- **Retrieval**: 
  - `hoo_map_get_string_int64(map, "key", *dest)`
- **Utility**: `hoo_map_length(map)`, `hoo_map_contains_string(map, "key")`, `hoo_map_remove_string(map, "key")`.
