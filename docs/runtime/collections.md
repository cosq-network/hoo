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
- **Creation**: `new Array()` allocates with initial capacity.
- **Indexing**: Arrays support O(1) direct indexing. `val = arr[i]` maps to `LD.D dest, arr_base, (8 + i*8)`.
- **Push**:
  - `arr.push(val)` — Generic push of a 64-bit slot.
  - `arr.pushInt64(val)`, `arr.pushDouble(val)`, `arr.pushString(str)`, `arr.pushObject(obj)`, etc.
- **Type-Specific Get**:
  - `arr.getInt64(index)` (Returns a result or `none` on out-of-bounds).
- **Inspection**: `arr.length()`.

---

## 2. Maps (`Map`)

The `Map` is a type-safe dictionary backed by a **polymorphic C-ABI** (~15 functions) that replaces the legacy 70+ type-specific functions. The internal implementation (`MapImpl`) utilizes disjoint `std::unordered_map` instances per key type, with key/value type dispatch handled internally by template helpers.

### Internal Maps:
```cpp
std::unordered_map<int8_t, std::any> data_int8_;
std::unordered_map<int64_t, std::any> data_int64_;
std::unordered_map<int8_t, std::any> data_char_;   // int8_t (not char) for cross-platform signedness
std::unordered_map<std::string, std::any> data_string_;
```

Note: `data_char_` uses `int8_t` instead of `char` to guarantee consistent signedness across x86 (signed char) and ARM64 Linux (unsigned char per AAPCS64).

### Polymorphic C-ABI (15 functions)

| Function | Signature | Returns | Description |
| :--- | :--- | :--- | :--- |
| `hoo_map_new` | `(int64_t keyType, int64_t valueType)` | `HooMap*` | Creates a map (2 args: key type, value type) |
| `hoo_map_retain` | `(HooMap*)` | `HooMap*` | Retains the map |
| `hoo_map_release` | `(HooMap*)` | `void` | Releases the map |
| `hoo_map_refcount` | `(HooMap*)` | `int64_t` | Current reference count |
| `hoo_map_count` | `(HooMap*)` | `int64_t` | Entry count (replaces `hoo_map_length`) |
| `hoo_map_is_empty` | `(HooMap*)` | `int64_t` | 1 if empty (replaces `hoo_map_empty`) |
| `hoo_map_key_type` | `(HooMap*)` | `int64_t` | Key type constant |
| `hoo_map_value_type` | `(HooMap*)` | `int64_t` | Value type constant |
| `hoo_map_contains_key` | `(HooMap*, const void* key)` | `int64_t` | 1 if key exists, 0 otherwise |
| `hoo_map_set` | `(HooMap*, const void* key, const void* value)` | `int64_t` | 1 on success, -1 on error |
| `hoo_map_try_get` | `(HooMap*, const void* key, void* out)` | `int64_t` | 1 if found (+value in `out`), 0 if not found |
| `hoo_map_remove` | `(HooMap*, const void* key)` | `int64_t` | 1 if removed, 0 if not found |
| `hoo_map_clear` | `(HooMap*)` | `void` | Removes all entries |
| `hoo_map_get_keys` | `(HooMap*)` | `HooArray*` | Returns array of all keys |
| `hoo_map_get_values` | `(HooMap*)` | `HooArray*` | Returns array of all values |

### Key/Value Type Dispatch

Keys and values are type-erased at the C-ABI layer via `const void*`. The convention for passing values varies by type:

| Type Constant | Key Convention | Value Convention |
| :--- | :--- | :--- |
| `HOO_MAP_KEY_BYTE` / `HOO_MAP_KEY_INT8` | `*(const int8_t*)key` | `*(const int8_t*)value` |
| `HOO_MAP_KEY_INT64` | `*(const int64_t*)key` | `*(const int64_t*)value` |
| `HOO_MAP_KEY_CHAR` | `*(const int8_t*)key` (deref as int8_t) | `*(const int64_t*)value` (read as int64_t, cast to int8_t) |
| `HOO_MAP_KEY_STRING` | `(const char*)key` (direct, `strdup`'d) | `(const char*)value` (direct, `strdup`'d) |
| `HOO_MAP_VAL_DOUBLE` | — | `*(const double*)value` |
| `HOO_MAP_VAL_BOOL` | — | `*(const int64_t*)value` |
| `HOO_MAP_VAL_OBJECT` | — | `(void*)value` (direct, no ARC) |

Numeric keys are always passed as pointers to the value (e.g., pass `&int64_key` for int64). String keys/values are passed directly as `const char*`.

### Key Operations

- **Creation**: `new Map(keyType, valueType)` — two arguments (key type constant, value type constant). The old single-arg form is removed.
- **Thread Safety**: Map release uses the runtime's per-type destructor callback mechanism, invoked atomically by `hoo_release` when refcount reaches zero. No separate mutex is needed.
- **hooc method calls** resolve through JIT wrappers that extract typed registers and delegate to the polymorphic C-ABI:
  - `map.setInt64Int64(k, v)`, `map.setStringString(k, v)`, `map.setInt64String(k, v)`, etc.
  - `map.getInt64("key")`, `map.getObject(100)` — return found flag in bits [63:1], value in bit 0
  - `map.containsString("key")`, `map.containsInt64(100)` — return 1 if found
  - `map.removeString("key")` — return 1 if removed
  - `map.length()` — returns `int64` entry count
  - `map.isEmpty()` — returns `int64` (1 if empty)
  - `map.clear()` — removes all entries
- **Legacy functions removed**: `hoo_map_set_int64_int64`, `hoo_map_set_string_string`, `hoo_map_length`, `hoo_map_empty`, `hoo_map_new_with_keytype`, etc. — all replaced by the 15 polymorphic functions above.

### Ownership & Lifetime

- **String values**: The map deep-copies string values via `strdup` on insertion and `free`s them on removal, overwrite, or map destruction. Returned `const char*` pointers are valid until the entry is modified or the map is destroyed.
- **Object values**: Object handles are stored as raw `void*` **without** `hoo_retain`/`hoo_release` (consistent with `HooArray`). The caller is responsible for retaining objects that must survive beyond the map's lifetime. Null object values (`nullptr`) are permitted.
- **Primitive values** (`int64`, `double`, `bool`, `int8`): Stored by value; no lifetime management needed.
- **Generic value API** (`hoo_map_set`/`hoo_map_try_get` via `void*`): The caller is responsible for lifetime management when using the raw `void*` path.
