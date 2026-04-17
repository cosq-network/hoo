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

## What Belongs Here

**DO add files here if they:**
- Implement runtime functionality (String, Array, future types like Dict, Set, etc.)
- Are written in C or C++ with no LLVM dependencies
- Need to be callable from JIT-compiled code
- Provide fundamental types used across the runtime

**DO NOT add files here if they:**
- Depend on LLVM headers or IR generation
- Are compiler infrastructure (AST, code generation, etc.)
- Register types with the runtime system (use `src/runtime/llvm` instead)

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
