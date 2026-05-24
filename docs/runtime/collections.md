# Collections

The runtime provides high-level `Array` and `Map` primitives that are natively implemented in C++ and exposed to the HVM via the opaque handle ABI.

## 1. Arrays (`HooArray`)

HVM arrays use a **hardware-ready, low-level representation** designed for direct ISA accessibility and zero-abstraction indexing.

### Internal Layout
An array is a contiguous memory block allocated via `hoo_alloc` with the `HOO_TYPE_ARRAY` identifier.
```cpp
// Format in memory:
// [Header: 16 bytes (ARC refcount + Type ID)]
// [Length: 8 bytes (int64_t)]
// [Element 0: 8 bytes (int64_t / pointer)]
// [Element 1: 8 bytes (int64_t / pointer)]
// ...
```
- **Length-Prefixed**: The first 64-bit slot in the data area stores the current element count.
- **Fixed-Width Slots**: All elements occupy exactly 64 bits. Primitives (`int64`, `double`) are stored as bit patterns, and managed objects (`String`, `Character`, nested `Array`) are stored as pointers.
- **ARC Integration**: The array itself is ARC-managed. Currently, elements inside a low-level array are not automatically scanned for ARC; they must be managed via explicit `retain`/`release` if necessary (e.g., when popping an object).

### Key Operations
- **Creation**: `hoo_array_new()` allocates with initial capacity.
- **Indexing**: Arrays support O(1) direct indexing. `val = arr[i]` maps to `LD.D dest, arr_base, (8 + i*8)`.
- **Type-Specific Push**:
  - `hoo_array_push_int64(arr, val)`
  - `hoo_array_push_double(arr, val)`
  - `hoo_array_push_string(arr, str_handle)`
  - `hoo_array_push_object(arr, obj_handle)`
- **Type-Specific Get**:
  - `hoo_array_get_int64(arr, index, *dest)` (Returns 1 on success, 0 on out-of-bounds).
- **Inspection**: `hoo_array_length(arr)`.

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
