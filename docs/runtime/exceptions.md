# Exceptions & Shadow Stack

The Hooc exception model (`HooException`) provides detailed stack traces and nested causes, implemented securely via a Shadow Stack to bridge HVM dynamic execution with the native C++ unwinder.

## 1. Exception Objects (`HooException`)
An exception is a standard ARC-managed object containing a message, a type identifier, a cause, and stack trace frames.

```cpp
struct HooExceptionImpl {
    int64_t typeId;
    const char* typeName;
    const char* message;
    int64_t frameCount;
    char** frames;
    HooException cause; // Retained nested exception
};
```

### Standard Types
- `HOO_EXCEPTION_RUNTIME` (0)
- `HOO_EXCEPTION_NULL_POINTER` (1)
- `HOO_EXCEPTION_INDEX_OUT_OF_BOUNDS` (2)
- `HOO_EXCEPTION_DIVISION_BY_ZERO` (3)
- `HOO_EXCEPTION_INVALID_CAST` (4)
- `HOO_EXCEPTION_CUSTOM` (99)

## 2. The Shadow Stack Mechanism
Because the JIT translates HVM bytecode to native execution, standard C++ `try-catch` blocks in the host environment cannot automatically route to Hooc `catch` blocks. The runtime provides a **Shadow Stack** to manage this routing.

### Workflow:
1. **Try Block Entry**: The Hooc compiler emits `CALL hoo_push_handler(catchPc)`. The JIT intercepts this (`kSysPushHandler`) and saves the current virtual registers (`lr`, `fp`, `sp`) and the handler's PC into a thread-local shadow frame array.
2. **Throwing**: The compiler emits `CALL hoo_throw(exc)`. The runtime calls `hoo_exception_throw(exc)`, which throws a native `HooStdException` (wrapping the handle). The JIT `SYSCALL` bridge catches this and triggers `hooc_hvm_sys_throw_to_handler_state`.
3. **Routing**: The JIT pops the latest shadow frame, restores the virtual `fp`/`sp`, places the exception handle into argument register `r1`, and modifies the virtual Program Counter (`PC`) to jump directly to the `catchPc` block.

## 3. Core API
- `hoo_exception_create(typeId, message)`
- `hoo_exception_create_with_cause(typeId, message, cause)`
- `hoo_exception_get_message(exc)`
- `hoo_exception_get_stack_trace(exc)`
- `hoo_push_handler(void* handler_pc)` / `hoo_pop_handler()`
- `hoo_exception_throw(exc)` / `hoo_exception_rethrow()`
