# Exceptions & Shadow Stack

The Hoo exception model (`HooException`) provides detailed stack traces and nested causes, implemented securely via a Shadow Stack to bridge HVM dynamic execution with the native C++ unwinder.

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
Because the JIT translates HVM bytecode to native execution, standard C++ `try-catch` blocks in the host environment cannot automatically route to Hoo `catch` blocks. The runtime provides a **Shadow Stack** to manage this routing.

### Workflow:
1. **Try Block Entry**: The Hoo compiler emits `CALL Exception.pushHandler(catchPc)`. The JIT intercepts this (`kSysPushHandler`) and saves the current virtual registers (`lr`, `fp`, `sp`) and the handler's PC into a thread-local shadow frame array.
2. **Throwing**: The compiler emits `CALL throw(exc)`. On macOS/Linux, the runtime calls `Exception.throw(exc)`, which throws a native `HooStdException` (wrapping the handle). The JIT `SYSCALL` bridge catches this and triggers `hoo_hvm_sys_throw_to_handler_state`. On Windows, the JIT bridge directly sets the current exception via `Exception.setCurrent()` and transfers control through the shadow stack, bypassing C++ exception unwinding entirely.
3. **Routing**: The JIT pops the latest shadow frame, restores the virtual `fp`/`sp`, places the exception handle into argument register `r1`, and modifies the virtual Program Counter (`PC`) to jump directly to the `catchPc` block.

### Platform-Specific Behavior

| Platform | Throw Mechanism | C++ Exception Unwinding |
| :--- | :--- | :--- |
| macOS/Linux | `Exception.throw()` via `SYSCALL 9` | Yes — native `HooStdException` thrown and caught by JIT bridge |
| Windows | `Exception.setCurrent()` via JIT bridge | No — exception is set directly; control flow handled by shadow stack |

On Windows, the handler-related syscalls (`kSysPushHandler`, `kSysPopHandler`, `kSysThrowToHandler`, `kSysRethrowToHandler`) are not lowered to LLVM IR. Instead, they execute via the interpreter fallback path, which uses `Exception.setCurrent()` to manage exception state without relying on the host's C++ exception mechanism.

## 3. Core API
- `Exception.create(typeId, message)`
- `Exception.createWithCause(typeId, message, cause)`
- `exc.message()`
- `exc.stack_trace()`
- `Exception.pushHandler(handler_pc)` / `Exception.popHandler()`
- `Exception.throw(exc)` / `Exception.rethrow()`
- `Exception.setCurrent(exc)` — Set the current exception without native unwinding (used on Windows; also available on other platforms for direct state management)
