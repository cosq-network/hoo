# Collections

The runtime provides high-level `Array` and `Map` primitives that are natively implemented in C++ and exposed to the HVM via the opaque handle ABI.

## 1. Arrays (`Array`)

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
- **ARC Integration**: The array itself is ARC-managed. Elements inside a low-level array are not automatically scanned for ARC; they must be managed via explicit `retain`/`release` if necessary (e.g., when popping an object).

### Key Operations
- **Creation**: `Array.new()` allocates with initial capacity.
- **Indexing**: Arrays support O(1) direct indexing. `val = arr[i]` maps to `LD.D dest, arr_base, (8 + i*8)`.
- **Push**:
  - `arr.push(val)` — Generic push of a 64-bit slot.
  - `arr.pushInt64(val)`, `arr.pushDouble(val)`, `arr.pushString(str)`, `arr.pushObject(obj)`, etc.
- **Type-Specific Get**:
  - `arr.getInt64(index)` (Returns a result or `none` on out-of-bounds).
- **Inspection**: `arr.length()`.

---

## 2. Maps (`Map`)

The `Map` is a type-safe dictionary. Because keys must be hashed efficiently, the internal implementation (`MapImpl`) utilizes disjoint `std::unordered_map` instances based on the key type.

### Internal Maps:
```cpp
std::unordered_map<int8_t, std::any> data_int8_;
std::unordered_map<int64_t, std::any> data_int64_;
std::unordered_map<char, std::any> data_char_;
std::unordered_map<std::string, std::any> data_string_;
```

### Key Operations
- **Creation**: `Map.new(key_type)` initializes the map bound to a specific key type.
- **Thread Safety**: Map operations use a mutex to guard the refcount check, ensuring safe concurrent access.
- **Insertion**:
  - `map.set("key", 42)`
  - `map.set(100, obj)`
- **Retrieval**: 
  - `map.get("key")`
- **Utility**: `map.length()`, `map.contains("key")`, `map.remove("key")`.
