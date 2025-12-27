# Object Creation and Memory Management Design

## Overview

This document describes the design for implementing object creation with automatic memory management in the Hooc programming language. The implementation uses **automatic reference counting (ARC)** similar to Swift and Objective-C.

## Memory Management Strategy

### Why Reference Counting?

We chose reference counting over garbage collection for several reasons:

1. **Deterministic cleanup** - Objects are freed immediately when reference count reaches zero
2. **Simpler implementation** - No need for GC pause times or complex mark-sweep algorithms
3. **Predictable performance** - No unpredictable GC pauses
4. **Better for systems programming** - Deterministic resource cleanup (RAII)
5. **Easier to integrate with C** - No runtime threads or GC coordination needed

### Trade-offs

**Advantages:**
- Immediate deallocation when no longer needed
- No GC pause times
- Predictable memory usage
- Works well with RAII pattern

**Disadvantages:**
- Cannot handle circular references automatically (need weak references)
- Small overhead on every assignment/copy
- Potential for retain cycles if not careful

## Object Memory Layout

Each object allocated on the heap has the following structure:

```
+-------------------+
| Reference Count   | 8 bytes (int64)
+-------------------+
| Class Type ID     | 8 bytes (int64) - for RTTI
+-------------------+
| Field 1           | variable size
+-------------------+
| Field 2           | variable size
+-------------------+
| ...               |
+-------------------+
```

The pointer returned to user code points to the **first field**, not the reference count. This allows normal field access while keeping metadata hidden.

## Runtime Functions

We need to implement these runtime functions (in C, linked with LLVM):

### Core Allocation

```c
// Allocate object with refcount initialized to 1
void* hoo_alloc(size_t size, int64_t type_id);

// Increment reference count (called on assignment/copy)
void* hoo_retain(void* obj);

// Decrement reference count, free if reaches 0
void hoo_release(void* obj);

// Get current reference count (for debugging)
int64_t hoo_get_refcount(void* obj);
```

### Constructor Support

```c
// Call constructor on allocated object
void hoo_call_constructor(void* obj, ...constructor_args);
```

## Language Syntax

### Object Creation

```hoo
class Point {
    constructor(x: int64, y: int64) {
        this.x = x;
        this.y = y;
    }
}

// Create object on heap with automatic memory management
var p = new Point(10, 20);  // refcount = 1

var q = p;  // refcount = 2 (automatic retain)

// When p goes out of scope, automatic release (refcount = 1)
// When q goes out of scope, automatic release (refcount = 0, freed)
```

### Automatic Memory Management Rules

1. **Creation**: `new` initializes refcount to 1
2. **Assignment**: Increment refcount of source, decrement refcount of old destination
3. **Scope exit**: Decrement refcount of local variables
4. **Function parameters**: Increment refcount when passing, decrement on return
5. **Return values**: Transfer ownership (no refcount change)

## LLVM IR Generation

### Object Allocation

```llvm
; For: var p = new Point(10, 20);

; 1. Allocate memory (size = 16 bytes for refcount + type_id + fields)
%obj_raw = call i8* @hoo_alloc(i64 32, i64 %point_type_id)

; 2. Cast to Point* type
%obj = bitcast i8* %obj_raw to %Point*

; 3. Call constructor
call void @Point_constructor(%Point* %obj, i64 10, i64 20)

; 4. Store in local variable (no retain needed, already refcount=1)
store %Point* %obj, %Point** %p
```

### Assignment

```llvm
; For: var q = p;

; 1. Load source pointer
%p_val = load %Point*, %Point** %p

; 2. Retain (increment refcount)
%p_retained = call i8* @hoo_retain(i8* %p_val)
%p_retained_typed = bitcast i8* %p_retained to %Point*

; 3. Load old destination value (if any)
%q_old = load %Point*, %Point** %q

; 4. Release old value (if not null)
%q_old_ptr = bitcast %Point* %q_old to i8*
call void @hoo_release(i8* %q_old_ptr)

; 5. Store new value
store %Point* %p_retained_typed, %Point** %q
```

### Scope Exit

```llvm
; At end of scope, release all object variables

; For each object variable in scope:
%obj_val = load %Point*, %Point** %obj_var
%obj_ptr = bitcast %Point* %obj_val to i8*
call void @hoo_release(i8* %obj_ptr)
```

## Implementation Phases

### Phase 1: Basic Infrastructure (Foundation)
- [ ] Create runtime C library with hoo_alloc/hoo_retain/hoo_release
- [ ] Link runtime with LLVM generated code
- [ ] Add external function declarations in LLVM code generator

### Phase 2: AST Support
- [ ] Ensure NewObjectExpression AST node exists (already in AST.h)
- [ ] Update SimpleASTBuilder to handle new expressions
- [ ] Add support for constructor parameter passing

### Phase 3: Code Generation - Allocation
- [ ] Generate LLVM IR for `new` expression
- [ ] Call hoo_alloc with correct size
- [ ] Call constructor with parameters
- [ ] Return typed pointer

### Phase 4: Code Generation - Reference Counting
- [ ] Generate retain calls on assignment
- [ ] Generate release calls on scope exit
- [ ] Track object variables in symbol table
- [ ] Generate cleanup code for function returns

### Phase 5: Constructor Implementation
- [ ] Generate LLVM functions for class constructors
- [ ] Support field initialization in constructors
- [ ] Handle `this` pointer in constructor body

### Phase 6: Field Access
- [ ] Generate code for field access (obj.field)
- [ ] Generate code for field assignment (obj.field = value)
- [ ] Handle nested object fields

### Phase 7: Testing and Validation
- [ ] Unit tests for allocation/deallocation
- [ ] Memory leak detection tests
- [ ] Circular reference tests (should leak for now)
- [ ] Performance benchmarks

### Phase 8: Advanced Features (Future)
- [ ] Weak references (to break cycles)
- [ ] Thread-safe reference counting (atomic operations)
- [ ] Destructor support
- [ ] Move semantics (transfer ownership without retain/release)

## Example Runtime Implementation (C)

```c
// hoo_runtime.c

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Object header (not visible to Hooc code)
typedef struct {
    int64_t refcount;
    int64_t type_id;
} HooObjectHeader;

#define HEADER_SIZE sizeof(HooObjectHeader)

void* hoo_alloc(size_t size, int64_t type_id) {
    // Allocate header + object data
    HooObjectHeader* header = (HooObjectHeader*)malloc(HEADER_SIZE + size);

    if (!header) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    // Initialize header
    header->refcount = 1;  // Start with refcount = 1
    header->type_id = type_id;

    // Zero-initialize object data
    memset((char*)header + HEADER_SIZE, 0, size);

    // Return pointer to object data (skip header)
    return (char*)header + HEADER_SIZE;
}

void* hoo_retain(void* obj) {
    if (!obj) return NULL;

    // Get header
    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - HEADER_SIZE);

    // Increment refcount
    header->refcount++;

    return obj;
}

void hoo_release(void* obj) {
    if (!obj) return;

    // Get header
    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - HEADER_SIZE);

    // Decrement refcount
    header->refcount--;

    // Free if refcount reaches 0
    if (header->refcount == 0) {
        // TODO: Call destructor if exists
        free(header);
    } else if (header->refcount < 0) {
        fprintf(stderr, "ERROR: Negative refcount!\n");
        exit(1);
    }
}

int64_t hoo_get_refcount(void* obj) {
    if (!obj) return 0;

    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - HEADER_SIZE);
    return header->refcount;
}
```

## Null Safety

Hooc supports nullable types (`Type?`). For objects:

```hoo
var p: Point? = new Point(10, 20);  // Can be null
var q: Point = new Point(5, 5);     // Cannot be null

p = null;  // OK - releases object, sets to null
// q = null;  // Compile error - q is non-nullable
```

The LLVM IR generation must handle null checks for nullable object types:

```llvm
; Release only if not null
%obj_ptr = load %Point*, %Point** %p
%is_null = icmp eq %Point* %obj_ptr, null
br i1 %is_null, label %skip_release, label %do_release

do_release:
    %obj_i8 = bitcast %Point* %obj_ptr to i8*
    call void @hoo_release(i8* %obj_i8)
    br label %skip_release

skip_release:
    ; continue
```

## Performance Considerations

### Optimization Opportunities

1. **Eliminate redundant retain/release pairs**
   - Compiler can track ownership and skip unnecessary ops
   - Example: `var x = obj; use(x);` doesn't need retain if x not used after

2. **Use LLVM optimization passes**
   - Dead store elimination
   - Common subexpression elimination
   - Inline small retain/release calls

3. **Atomic vs non-atomic**
   - Single-threaded: use non-atomic increments
   - Multi-threaded: use atomic operations (future)

### Memory Overhead

- 16 bytes per object (refcount + type_id)
- For small objects, this can be significant
- Consider: object pooling for frequently allocated types

## Integration with Existing Code

### Current Status

The grammar already supports:
- `new` keyword for object creation
- Class declarations with constructors
- Member access syntax

What needs implementation:
- LLVM code generation for `new`
- Runtime library integration
- Automatic retain/release insertion
- Constructor code generation
- Field access code generation

## Testing Strategy

### Unit Tests

1. **Basic allocation/deallocation**
   ```hoo
   func test_basic() {
       var p = new Point(1, 2);
       // Should free automatically
   }
   ```

2. **Assignment and copying**
   ```hoo
   func test_assignment() {
       var p = new Point(1, 2);
       var q = p;  // refcount = 2
       var r = q;  // refcount = 3
       // All should release correctly
   }
   ```

3. **Null handling**
   ```hoo
   func test_null() {
       var p: Point? = null;
       p = new Point(1, 2);
       p = null;  // Should release
   }
   ```

4. **Memory leak detection**
   - Run with Valgrind or AddressSanitizer
   - Verify no leaks in test suite
   - Track allocations vs deallocations

## Future Enhancements

### Weak References

To break circular references:

```hoo
class Node {
    var next: Node?;
    var parent: weak Node?;  // Weak reference, doesn't increase refcount
}
```

### Thread Safety

Use atomic operations for refcounting:

```c
#include <stdatomic.h>

typedef struct {
    atomic_int_fast64_t refcount;
    int64_t type_id;
} HooObjectHeader;

void* hoo_retain(void* obj) {
    if (!obj) return NULL;
    HooObjectHeader* header = (HooObjectHeader*)((char*)obj - HEADER_SIZE);
    atomic_fetch_add(&header->refcount, 1);
    return obj;
}
```

### Move Semantics

Transfer ownership without retain/release overhead:

```hoo
var p = new Point(1, 2);
var q = move(p);  // Transfer ownership, no retain/release
// p is now null/invalid
```

## Conclusion

This design provides a foundation for automatic memory management in Hooc using reference counting. The implementation is straightforward, predictable, and integrates well with LLVM's code generation capabilities.

The phased approach allows incremental development and testing, starting with basic allocation and gradually adding more sophisticated features like automatic cleanup and weak references.
