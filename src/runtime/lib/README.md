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

### **C. Collections (`hoo_generic_array.h`, `hoo_map.h`)**
- **Arrays**: Managed dynamic buffers with a 64-bit length header at offset 0.
- **Maps**: Type-safe key-value stores with optimized native hashing.

### **D. Exception Unwinding (`hoo_exception.h`)**
- `hoo_push_handler(pc)`: Registers a recovery point on the shadow stack.
- `hoo_exception_throw(exc)`: Performs a **Non-Local Jump**, restoring the virtual register state and jumping to the handler PC.

## 4. JIT Integration (Static Linkage)

The library is integrated into the JIT via the **`StaticHOModule`** bridge. This allows the JIT to emit direct `call` instructions to absolute physical host addresses for runtime services.

```cpp
// Mapping HVM symbol to Native Address
hooModule->registerFunction("alloc", (void*)&hoo_alloc, "_F_hoo_alloc_p_i8_i8");
```

## 5. Contribution Guidelines

When adding new features to `hoort`:
1. **C-ABI Linkage**: Use `extern "C"` for all public headers.
2. **Mangled Tags**: Document the HVM mangled name for every function (e.g., `_F_...`) in the header comments.
3. **ARC Compliance**: Ensure all methods taking or returning objects properly handle the refcount header.
4. **Header Isolation**: Keep module-specific logic in isolated files (e.g., `hoo_crypto.cpp`).

## 6. Testing

The runtime library is verified through the `tests/runtime/` suite, which performs:
- ARC integrity checks (leak detection).
- Stack-unwinding verification.
- String UTF-8 encoding/decoding accuracy.
- Multi-threaded map contention tests.
