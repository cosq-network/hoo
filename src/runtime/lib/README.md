# Runtime Library (`src/runtime/lib`)

The **Hooc Runtime Library (`hoort`)** is a statically compiled C++ library (`libhoort.a`) that provides high-level services and intrinsic functions for JIT and AOT execution. It is designed for seamless interoperability with the **HVM v1.4 (Hardware Ready)** RISC core.

## 1. Core Philosophy: The Opaque Handle Model

Since HVM v1.4 is a pure physical architecture, it has no native concept of "managed objects". The runtime library bridges this gap by treating all high-level types (Strings, Arrays, Maps, Objects) as **Opaque Handles**.
- **JIT/HVM View**: A simple 64-bit integer (`i64`) or pointer.
- **Runtime View**: A pointer to a C++ instance with a normative ARC header.

## 2. Memory Model: The 16-Byte ARC Header

Every object managed by the runtime MUST be prefixed with a 16-byte hidden header. This allows the JIT to perform atomic reference counting and RTTI checks with zero-abstraction overhead.

| Offset | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| **-16** | `refcount` | `atomic<i64>` | Thread-safe reference count (ARC). |
| **-8** | `type_id` | `i64` | Global identifier for RTTI and dynamic dispatch. |
| **0** | **User Data** | (variable) | The address stored in HVM registers (`r1..r31`). |

## 3. Module & Intrinsic Reference

The library implements the standard `hoo` and `hoo.io` namespaces.

### **A. Core Logic (`hoo_runtime.h`)**
Provides the fundamental memory primitives required for JIT execution.
- `_F_module_init_v`: The mandatory entry point for every module to initialize its vtables and global state.
- `hoo_alloc(size, type_id)`: Allocates zeroed memory with a 16-byte ARC header.
- `hoo_retain(obj)`: Atomically increments the refcount.
- `hoo_release(obj)`: Atomically decrements and frees the instance if zero.

### **B. String Management (`hoo_string.h`)**
HVM strings are immutable UTF-8 buffers.
- `hoo_string_from_cstr(char*)`: Essential for loading `.rodata` literals.
- `hoo_string_concat(s1, s2)`: Dynamic concatenation with ARC management.

### **C. Buffer (`hoo_buffer.h`)**
Mutable byte array with ARC management and dynamic resizing. The handle points to `BufferImpl` (right after the 16-byte ARC header), using the same layout as `HooString`.
- `hoo_buffer_new(capacity)`: Allocates a zero-length buffer with pre-allocated capacity.
- `hoo_buffer_from_bytes(data, len)`: Creates a buffer initialized from raw bytes.
- `hoo_buffer_copy(buf)`: ARC-managed deep copy.
- `hoo_buffer_append(buf, data, len)`: Appends bytes, may reallocate via `hoo_realloc`.
- `hoo_buffer_slice(buf, start, end)`: Returns a new buffer with a sub-range.
- Type ID: `HOO_TYPE_BUFFER 113`.

### **D. Collections (`hoo_generic_array.h`, `hoo_map.h`)**
- **Arrays**: Managed dynamic buffers with a 64-bit length header at offset 0.
- **Maps**: Type-safe key-value stores with a polymorphic ~15-function C-ABI (`hoo_map_new`, `hoo_map_set`, `hoo_map_try_get`, `hoo_map_contains_key`, `hoo_map_remove`, `hoo_map_clear`, `hoo_map_count`, `hoo_map_is_empty`, etc.). Keys/values are type-erased via `void*` with documented per-type conventions (numeric via pointer dereference, strings as `const char*`). Internal type dispatch uses template helpers over disjoint `std::unordered_map` instances per key type. Object values stored as raw `void*` (no ARC).

### **D. Exception Unwinding (`hoo_exception.h`)**
- `hoo_push_handler(pc)`: Registers a recovery point on the shadow stack.
- `hoo_exception_throw(exc)`: Performs a **Non-Local Jump**, restoring the virtual register state and jumping to the handler PC.

## 4. JIT Integration (Static Linkage)

The library is integrated into the JIT via the **`StaticHOModule`** bridge. This allows the JIT to emit direct `call` instructions to absolute physical host addresses for runtime services.

```cpp
// Mapping HVM symbol to Native Address
hooModule->registerFunction("alloc", (void*)&hoo_alloc, "_F_hoo_alloc_p_i8_i8");
```

### 4.1 Class-Based Dispatch

Runtime modules are accessible via class-based method-call syntax for singleton utility APIs (e.g., `Math.abs(x)`) and instance syntax for object APIs (e.g., `new Character(65)`, `map.length()`, `arr.push(val)`). The code generator in `HVMCodeGenerator.cpp` resolves class names to module prefixes via `classToPrefix()` and redirects the call to the `hoo` module path, where JIT wrapper functions registered in `buildRuntimeSymbols()` perform the actual C-ABI dispatch. Instance methods on `var` variables (e.g., `s.length()`, `arr.push(val)`) are resolved through type-ID inference and follow the same path. JSON is intentionally free-function only (`json_serialize_hashmap`, `json_serialize_anyarray`, `json_deserialize_hashmap`, `json_deserialize_anyarray`, `json_minify`, `json_beautify`) and has no `Json` class or opaque `ptr` document handles. All built-in classes listed in `isBuiltinClassName()` are covered; see `classToPrefix()` for the full mapping (e.g., `Character` -> `character`, `Array` -> `array`, `Thread` -> `thread`).

Serializable class support builds on the JSON free functions. Generated class
methods use numeric positional HashMap keys, tagged Base64 objects for buffers,
and tagged element-type/dimension/raw-bit objects for tensors. Nested
serializable objects are recursively converted through their generated methods.

### 4.2 SYSCALL Bridge

Runtime services are also accessible via the `SYSCALL` instruction (opcode `0xC0`). SYSCALLs 1-11 map directly to `hoort` library functions (alloc, retain, release, exception handling, string data). SYSCALLs 12-23 extend this interface with OS-level services — file I/O, threading, clock, and random — implemented in `HVMJIT.cpp` following the same `extern "C"` ABI convention for direct LLVM IR invocation.

## 5. Contribution Guidelines

When adding new features to `hoort`:
1. **Public API**: Prefer C++ classes in the `hoo::` namespace (e.g., `hoo::fs::File`) as the primary interface unless the module is explicitly designed as free-function-only. Provide `extern "C"` bridge functions alongside for JIT/FFI compatibility, delegating to the C++ class methods or free-function implementation as appropriate.
2. **Mangled Tags**: Document the HVM mangled name for every function (e.g., `_F_...`) in the header comments.
3. **ARC Compliance**: Ensure all methods taking or returning objects properly handle the refcount header.
4. **Header Isolation**: Keep module-specific logic in isolated files (e.g., `hoo_crypto.cpp`).

## 6. Testing

The runtime library is verified through the `tests/runtime/` suite, which performs:
- ARC integrity checks (leak detection).
- Stack-unwinding verification.
- String UTF-8 encoding/decoding accuracy.
- JSON `HashMap`/`AnyArray` serialization, deserialization, minify/beautify, and exception paths.
- Multi-threaded map contention tests.
