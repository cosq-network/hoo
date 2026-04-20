# Runtime Library (`src/runtime/lib`)

This directory contains the core runtime library implementations for the Hooc programming language. These are pure C/C++ implementations with no dependencies on LLVM or the compiler infrastructure.

## Purpose

The runtime library provides fundamental data types and memory management for Hooc programs:

- **Memory Management**: Automatic Reference Counting (ARC) for heap-allocated objects
- **Core Types**: String, Array, and other built-in types
- **Standard Functions**: Operations on runtime types (string manipulation, array operations, etc.)

## Contents

| File | Description |
|------|-------------|
| `hoo_runtime.h` / `hoo_runtime.c` | Core memory management (ARC) - allocation, retain/release, type IDs |
| `hoo_string.h` / `hoo_string.cpp` | UTF-8 string implementation with ARC |
| `hoo_generic_array.h` / `hoo_generic_array.cpp` | Generic dynamic array using `std::any` |
| `hoo_io.h` / `hoo_io.cpp` | I/O functions (print, println, readline, readchar) |
| `hoo_exception.h` / `hoo_exception.cpp` | Exception handling with stack traces |
| `hoo_map.h` / `hoo_map.cpp` | Generic key-value map with typed keys |

## Module and Class Reference

The runtime library implements the builtized standard library under the `hoo` namespace.

### Core Module: `hoo`

| Class / Type | Implementation | Description |
|--------------|----------------|-------------|
| `hoo.String` | `hoo_string.cpp` | Immutable UTF-8 string with ARC management. |
| `hoo.Array`  | `hoo_generic_array.cpp` | Dynamic array using `std::any` for heterogeneous element support. |

### IO Module: `hoo.io`

The IO module provides standard input and output functions.

| Function | Signature | Description |
|----------|-----------|-------------|
| `hoo.print` | `(string) -> void` | Prints a string to stdout. |
| `hoo.println` | `(string) -> void` | Prints a string followed by a newline to stdout. |
| `hoo.readline` | `() -> string` | Reads a line from stdin. |
| `hoo.readchar` | `() -> int64` | Reads a single character/byte from stdin. |

### Exception Module: `hoo`

The Exception module provides exception types for error handling.

| Exception Type | Description |
|----------------|-------------|
| `RuntimeException` | General runtime errors |
| `NullPointerException` | Null reference accessed |
| `IndexOutOfBoundsException` | Array/string index out of range |
| `DivisionByZeroException` | Division or modulo by zero |
| `InvalidCastException` | Type casting failed |
| `CustomException` | User-defined exceptions |

| Function | Signature | Description |
|----------|-----------|-------------|
| `hoo.Exception.create` | `(typeId: int64, message: string?) -> hoo.Exception` | Create exception with type ID |
| `hoo.Exception.createWithCause` | `(typeId: int64, message: string?, cause: hoo.Exception?) -> hoo.Exception` | Create with cause chain |
| `hoo.Exception.runtime` | `(message: string?) -> hoo.RuntimeException` | Create runtime exception |
| `hoo.Exception.nullPointer` | `(message: string?) -> hoo.NullPointerException` | Create null pointer exception |
| `hoo.Exception.indexOutOfBounds` | `(message: string?) -> hoo.IndexOutOfBoundsException` | Create index out of bounds |
| `hoo.Exception.divisionByZero` | `(message: string?) -> hoo.DivisionByZeroException` | Create division by zero |
| `hoo.Exception.invalidCast` | `(message: string?) -> hoo.InvalidCastException` | Create invalid cast |
| `hoo.Exception.custom` | `(type: string, message: string?) -> hoo.CustomException` | Create custom exception |
| `hoo.Exception.getTypeId` | `(exc: hoo.Exception) -> int64` | Get exception type ID |
| `hoo.Exception.getTypeName` | `(exc: hoo.Exception) -> string` | Get exception type name |
| `hoo.Exception.getMessage` | `(exc: hoo.Exception) -> string` | Get exception message |
| `hoo.Exception.hasCause` | `(exc: hoo.Exception) -> bool` | Check if has cause |
| `hoo.Exception.getCause` | `(exc: hoo.Exception) -> hoo.Exception?` | Get cause exception |
| `hoo.Exception.getStackTrace` | `(exc: hoo.Exception) -> string` | Get stack trace |
| `hoo.Exception.getFrameCount` | `(exc: hoo.Exception) -> int64` | Get stack frame count |
| `hoo.Exception.getFrame` | `(exc: hoo.Exception, index: int64) -> string` | Get stack frame |
| `hoo.Exception.equals` | `(a: hoo.Exception, b: hoo.Exception) -> bool` | Check equality |
| `hoo.Exception.debug` | `(exc: hoo.Exception) -> string` | Get debug string |

### Map Module: `hoo`

The Map module provides generic key-value maps with type-safe keys.

| Key Type | Description |
|----------|-------------|
| `byte` / `int8` | 8-bit signed integer keys |
| `int64` | 64-bit signed integer keys |
| `char` | Character keys |
| `string` | String keys |

| Function | Signature | Description |
|----------|-----------|-------------|
| `hoo.Map.new` | `(keyType: int64) -> hoo.Map` | Create new map with key type |
| `hoo.Map.fromPairs` | `(keyType: int64, keys: [], values: []?) -> hoo.Map` | Create from key-value pairs |
| `hoo.Map.length` | `(map: hoo.Map) -> int64` | Get number of entries |
| `hoo.Map.contains` | `(map: hoo.Map, key) -> bool` | Check if key exists |
| `hoo.Map.get` | `(map: hoo.Map, key) -> value?` | Get value by key |
| `hoo.Map.set` | `(map: hoo.Map, key, value)` | Set key-value pair |
| `hoo.Map.remove` | `(map: hoo.Map, key) -> bool` | Remove entry by key |
| `hoo.Map.clear` | `(map: hoo.Map)` | Remove all entries |
| `hoo.Map.empty` | `(map: hoo.Map) -> bool` | Check if map is empty |
| `hoo.Map.keyType` | `(map: hoo.Map) -> int64` | Get key type |
| `hoo.Map.refcount` | `(map: hoo.Map) -> int64` | Get reference count |

### Usage Examples

```hoo
// Strings and Printing
var message: string = "Hello, Hooc!";
hoo.println(message);

// String Concatenation
var name = "Alice";
hoo.println("Welcome, " + name + "!");

// Arrays
var scores: int64[] = [95, 88, 72];
var first = scores[0];
hoo.println("Count: " + scores.length());

// Input
hoo.print("What is your name? ");
var inputName = hoo.readline();
hoo.println("Greetings, " + inputName);

// Object Creation (Standard Library Classes)
var strObj = new hoo.String("Custom String");
var arrObj = new hoo.Array(); // Empty dynamic array

// Exception Handling
try {
    var result = divide(10, 0);
} catch e: RuntimeException {
    hoo.println("Error: " + e.getMessage());
} finally {
    hoo.println("Operation completed");
}

// Maps (type-safe key-value storage)
var scores = new hoo.Map(HOO_MAP_KEY_STRING);
scores.set("Alice", 95);
scores.set("Bob", 87);
var aliceScore = scores.get("Alice");
var containsBob = scores.contains("Bob");
scores.remove("Bob");
scores.clear();
```

## Implementation Guidelines

### C API Conventions

All public functions exposed to Hooc code must follow these conventions:

```c
// Function naming: hoo_<type>_<operation>
// Example: hoo_string_length, hoo_array_push

// Return types:
// - HooString, HooArray for object returns
// - int64_t for integers, doubles for floats
// - void for procedures

// Parameter handling:
// - NULL is allowed for nullable types
// - Return zero/false/null for invalid inputs
```

### Reference Counting

All heap-allocated types must implement ARC:

```c
// Retain: increment refcount, return same pointer
HooString hoo_string_retain(HooString str);

// Release: decrement refcount, free if zero
void hoo_string_release(HooString str);

// New objects start with refcount = 1
HooString hoo_string_new(void) {
    HooString str = allocate_string(0);
    // refcount is initialized to 1
    return str;
}
```

### Header Structure

Use `extern "C"` guards for C++ compatibility:

```c
#ifdef __cplusplus
extern "C" {
#endif

// Declarations...

#ifdef __cplusplus
}
#endif
```

### Error Handling

- Return `NULL` (C) or `nullptr` for allocation failures
- Log errors to stderr with descriptive messages
- Use `std::exit(1)` only for unrecoverable errors (out of memory)

### Internal C++ Classes

If using C++ for implementation, place implementation classes in the `hooc` namespace:

```cpp
namespace hooc {

class HooArrayImpl {
public:
    void retain();
    void release();
    // ...
};

} // namespace hooc
```

## Usage

### Including Headers

```cpp
#include "runtime/lib/hoo_string.h"    // For String functions
#include "runtime/lib/hoo_generic_array.h"  // For Array functions
```

### Linkage

These files are compiled into the `hoort` library. Link against `hoort` to access runtime functions.

## Adding a New Runtime Type

1. Create `hoo_<type>.h` and `hoo_<type>.cpp` in this directory
2. Implement the C API following conventions above
3. Add registration callbacks in `src/runtime/llvm/<type>_registration.cpp`
4. Update CMakeLists.txt to include new source files
5. Add tests in `tests/` using the header include path

Example structure:

```c
// hoo_mytype.h
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooMyType;

HooMyType hoo_mytype_new(void);
HooMyType hoo_mytype_from_int64(int64_t value);
void hoo_mytype_release(HooMyType obj);

#ifdef __cplusplus
}
#endif
```
