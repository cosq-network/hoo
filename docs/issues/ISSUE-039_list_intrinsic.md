# ISSUE-039 List Intrinsic Data Type

## Overview
The **List** intrinsic provides a generic, type‑aware container that can hold any value from the Hoo ecosystem (intrinsic data types, user‑defined classes, etc.).  Each `List` instance is created for **one element type**, guaranteeing type safety while offering the flexibility of a dynamic array.

## Runtime API (C)
```c
/* Allocation */
HooList hoo_list_new(int64_t element_type_id);      // creates a List for a given element type
HooList hoo_list_retain(HooList list);             // ARC retain
void    hoo_list_release(HooList list);            // ARC release

/* Introspection */
int64_t hoo_list_element_type_id(HooList list);    // returns the stored element type ID
int64_t hoo_list_length(HooList list);
int64_t hoo_list_capacity(HooList list);

/* Mutating operations */
int64_t hoo_list_push(HooList list, void *elem);   // append, retains element
void*   hoo_list_get(HooList list, int64_t index); // returns raw pointer (managed or primitive)
int64_t hoo_list_set(HooList list, int64_t index, void *elem); // replace, retains new, releases old
int64_t hoo_list_pop(HooList list);               // removes last element, returns pointer (caller owns ARC)
int64_t hoo_list_clear(HooList list);             // empties list, releases all elements
```
All functions validate that `list` is not `NULL` and that the element’s runtime type matches the list’s element type (`hoo_get_type_id(elem) == hoo_list_element_type_id(list)`).  Primitive values are automatically wrapped in their respective intrinsic objects (e.g., `hoo_int64_new`).

## Core Module – Object‑Oriented API
The language binding layer should expose a class **List** with methods mirroring the C API:
```
class List<T> {
    static List<T> new();
    int length();
    int capacity();
    void push(T elem);
    T get(int index);
    void set(int index, T elem);
    T pop();
    void clear();
}
```
The generic parameter `T` is bound to a single Hoo type at construction time.

## Grammar, Mangling & Demangling (No Conflict with Array)

**Syntax** – Use the `list` keyword to construct a list literal, distinct from the existing array literal `[...]`.

```
list_literal ::= "list" "[" [list_elements] "]"
list_elements ::= expression ("," expression)*
```

An empty list can be written as `list[]` or with an explicit generic annotation:

```
List<Int64> empty = list[];
```

### Parsing Adjustments
1. Extend the AST with a `ListLiteral` node storing a vector of sub‑expressions and a resolved `element_type_id`.
2. During semantic analysis, ensure homogeneous element types and emit **TypeMismatchException** when violated.

### Mangling
* Use short code `l` for the List type. Overloaded functions involving a List are mangled as `<name>__l<elem_type_id>` (e.g., `foo__li64` for `foo(List<Int64>)`).

### Demangling
* Extend `hoo_demangle` to recognize the `l` token and reconstruct the List type with its element type ID.

## Code Generation

* **Bytecode**: No new HVM instructions are introduced for List. The compiler reuses existing vector opcodes (`VEC_NEW`, `VEC_PUSH`, `VEC_GET`, `VEC_SET`, `VEC_LENGTH`, `VEC_CAPACITY`, `VEC_POP`, `VEC_CLEAR`) when the element type is a primitive vector; for non‑primitive types it falls back to generic list handling using the same opcodes with runtime type checks.

* **JIT**: Emit calls to the runtime C API for List operations. When the element type maps to a primitive vector, JIT may forward to the existing vector implementation for efficiency.

* **Type Metadata**: Allocate a hidden header field `element_type_id` in the List object (similar to `HOO_TYPE_ARRAY`).

## Execution Model
Lists obey the same ARC semantics as other heap‑allocated objects. On `push`, the runtime retains the element; on `pop`/`clear`/`release`, the runtime releases stored references, guaranteeing no memory leaks even with mixed‑type contents.

## Error Handling & Exceptions
| Condition | Returned Code | Exception Type |
|-----------|---------------|----------------|
| `list == NULL` | -1 | `NullReferenceException` |
| Element type mismatch | -1 | `TypeMismatchException` |
| Index out of bounds | -1 | `IndexOutOfRangeException` |
| Allocation failure | -1 | `OutOfMemoryException` |

All functions set `errno` accordingly (`EINVAL`, `ERANGE`, `ENOSPC`).  Language bindings should translate these into the above exception classes.

## Implementation Checklist
- **Runtime**: Add `hoo_list.h/.cpp` with the API and integrate in `src/runtime/lib`.
- **Header**: Define `HOO_TYPE_LIST 120` in `hoo_runtime.h`.
- **Core Language**: Update grammar (`parser.y`), AST (`ast.h`), and semantic analysis (`typecheck.cpp`).
- **Codegen**: Add bytecode opcodes and JIT stubs.
- **Tests**: Add unit tests covering creation, push/pop, type safety, and error paths.
- **Docs**: Update `docs/runtime/api/index.md` and `README.md` with a description and usage examples.

---
*Prepared by Antigravity AI – 2026‑06‑21*

## Status
- **Date**: 2026-06-21
- **Status**: **PROPOSED**
- **Priority**: Medium
- **Audit 2026-06-21**: No `hoo_list` runtime, `HOO_TYPE_LIST`, `ListLiteral` AST node, list grammar, list bytecodes, or list JIT bridge was found. This remains a design proposal.
