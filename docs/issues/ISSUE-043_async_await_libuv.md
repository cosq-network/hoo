# ISSUE-043 Async/Await Integration via libuv

## Overview
The Hoo language currently lacks native asynchronous primitives. This issue proposes integrating **libuv** – a cross‑platform asynchronous I/O library – to provide **`async` / `await`** support throughout the language. The integration will affect the runtime APIs (`fs`, `io`, `net`, etc.), introduce new coroutine‑style APIs, and require extensive changes across the compiler pipeline: grammar, mangling, demangling, parsing, code generation, execution, and documentation.

---

## Motivation
* **Modern concurrency model:** Developers expect `async`/`await` syntax similar to JavaScript, Rust, or C#. It simplifies non‑blocking I/O and improves scalability.
* **Cross‑platform reliability:** libuv abstracts OS‑specific event‑loop mechanisms (epoll, kqueue, IOCP) ensuring consistent behaviour on macOS, Linux, and Windows.
* **Unified runtime:** Existing `fs`, `io`, and `net` APIs can be extended to return *future* objects, enabling composable asynchronous workflows without breaking current synchronous semantics.

---

## Design Goals
1. **Zero‑cost sync fallback** – Existing synchronous APIs continue to work unchanged; a new async variant is added (e.g., `fs_read_async`).
2. **Transparent event loop** – The Hoo VM hosts a libuv event loop; it is started automatically when the first async operation is scheduled and runs until all pending tasks complete.
3. **Type‑safe futures** – `Future<T>` is a generic class representing the eventual result of an async computation. `await` extracts the value, suspending the current coroutine until completion.
4. **Mangle‑compatible names** – Async functions receive a distinct mangling suffix (`_a`) and the future return type is encoded in the mangled name.
5. **Minimal VM instruction set impact** – Introduce only a handful of new opcodes (`OP_ASYNC_START`, `OP_ASYNC_AWAIT`, `OP_ASYNC_YIELD`, `OP_ASYNC_RESUME`). All other behavior is implemented via libuv callbacks.

---

## Integration with libuv
* **Event Loop Management** – Add a singleton `uv_loop_t* hooc_uv_loop()` accessible from the runtime. The VM starts the loop on the first async call and exits when the loop becomes idle.
* **Thread‑safe callbacks** – All libuv callbacks will schedule a *resume* of the suspended coroutine via a thread‑safe queue (`uv_async_t`).
* **Resource Ownership** – libuv handles are wrapped in ARC‑managed objects (`HooUVHandle`) with retain/release semantics to avoid leaks.

---

## Runtime API Changes
### New Types
```c
// Generic future object (reference‑counted)
typedef struct HooFuture HooFuture;        // opaque
HooFuture* hoo_future_new(int64_t elem_type_id);
int64_t   hoo_future_get_type_id(HooFuture* f);
int64_t   hoo_future_is_ready(HooFuture* f);   // 1 = ready, 0 = pending
void*     hoo_future_get(HooFuture* f);       // blocks if not ready (used by await)
void      hoo_future_set(HooFuture* f, void* value); // internal use by libuv callbacks

// Async handle wrappers (file, socket, etc.)
typedef struct HooUVFile   HooUVFile;   // wraps uv_fs_t
typedef struct HooUVTcp    HooUVTcp;    // wraps uv_tcp_t
```
### Modified Modules
* **fs** – Add `fs_read_async(path: String) -> Future<ByteArray>` and `fs_write_async(path: String, data: ByteArray) -> Future<Void>`.
* **io** – `stdin_read_async() -> Future<String>` and `stdout_write_async(data: String) -> Future<Void>`.
* **net** – `tcp_connect_async(host: String, port: Int) -> Future<TcpSocket>`; `tcp_accept_async(listener: TcpListener) -> Future<TcpSocket>`.
* **timers** – New API `sleep_async(duration_ms: Int) -> Future<Void>` implemented via `uv_timer_t`.
* **process** – `process_spawn_async(command: String, args: List<String>) -> Future<ProcessHandle>`.

All existing synchronous variants (`fs_read`, `tcp_connect`, …) remain unchanged.

---

## Grammar Changes
```bnf
function_decl   ::= "async"? "fn" identifier "(" param_list? ")" "->" type_annotation? block
await_expr      ::= "await" "(" expression ")"
```
* `async fn` marks a function that returns a `Future<T>` where `T` is the declared return type.
* `await(expr)` may only appear inside an `async` function or another coroutine; it suspends execution until the future resolves.

---

## Mangling & Demangling
* **Mangled name schema** – Append `_a` to the function name and encode the future return type:
  `foo(Int) -> Future<String>` → `foo_aS` (where `S` is the type ID for `String`).
* Update `hoo_mangle` and `hoo_demangle` to recognise the `_a` suffix and reconstruct the generic future type.

---

## Parsing Adjustments
1. Extend the parser to recognise the optional `async` keyword before `fn`.
2. Produce a new AST node `AsyncFunctionDecl` storing a flag `is_async` and the underlying `FunctionDecl`.
3. Parse `await` as a `AwaitExpression` node; type‑check ensures the operand evaluates to a `Future<T>`.
4. Update symbol tables to store both sync and async overloads (see ISSUE‑041 for overloading). 

---

## Code Generation
### New Bytecode Opcodes
| Opcode | Description |
|--------|-------------|
| `OP_ASYNC_START` | Create a `Future` object, push onto stack. |
| `OP_ASYNC_AWAIT` | Suspend current coroutine; register continuation with libuv. |
| `OP_ASYNC_YIELD` | Explicit yield point (used by `await`). |
| `OP_ASYNC_RESUME`| Resume a suspended coroutine when the associated future becomes ready. |

* The compiler will emit `OP_ASYNC_START` at the start of an `async` function, and `OP_ASYNC_AWAIT` for each `await` expression.
* JIT: Generate calls to `hoo_future_*` and libuv APIs; for primitive futures the JIT can inline the result retrieval.

---

## Execution Model
* **Coroutines** – Each async function runs as a lightweight coroutine (stackful). The VM scheduler tracks runnable coroutines and those waiting on futures.
* **Event Loop** – The libuv loop runs in the same thread as the VM; the VM periodically yields control to the loop (via `uv_run(loop, UV_RUN_NOWAIT)`).
* **Synchronization** – When a future is resolved, libuv callback enqueues a resume request; the VM processes the queue at the next tick.
* **Memory Management** – Futures and UV handles are ARC‑managed; releasing a future before it resolves cancels the pending operation (via `uv_cancel`).

---

## Error Handling & Exceptions
| Condition | Exception Type |
|-----------|----------------|
| libuv error code (e.g., `UV_ECONNREFUSED`) | `IOError` |
| Attempt to `await` a non‑future value | `TypeMismatchException` |
| Timeout on `await` (future not ready within deadline) | `TimeoutException` |
| Cancellation of pending async operation | `CancellationException` |

All runtime functions set `errno` appropriately; language bindings translate to the above exception classes.

---
## Future<T> – Detailed Design

`Future<T>` is the cornerstone of the async/await integration. It is a **generic, reference‑counted placeholder** for a value of type `T` that will become available later. The design spans every layer of the language implementation:

### 1. Language level
- Syntax: `Future<T>` appears in type annotations.
- `async fn` functions implicitly return a `Future<ReturnType>`.
- `await(expr)` suspends the current coroutine until `expr` (which must be a `Future<U>`) resolves, then yields the concrete `U` value.

### 2. Parser & AST
- Grammar extension: `future_type ::= "Future" "<" type ">"`.
- New AST nodes:
  - `FutureTypeNode { TypeNode* elem; }`
  - `AwaitExpression { ExprNode* future; }`
  - `AsyncFunctionDecl { bool is_async; FunctionDecl* decl; }`

### 3. Semantic analysis
- `Future<T>` is represented by a **type descriptor** with `type_id = HOO_TYPE_FUTURE` and a **parameter** `elem_type_id` (the ID of `T`).
- `await` is type‑checked to ensure its operand is a `Future<U>`.
- Async functions are required to return a `Future<R>` matching the declared return type `R`.

### 4. Mangling & Demangling
- Async symbols get the suffix `_a` and encode the future element type, e.g. `foo(Int) -> Future<String>` → `foo_aS` (`S` = String type code).
- `hoo_mangle.cpp` and `hoo_demangle.cpp` are updated to handle this schema.

### 5. Bytecode (code generation)
- New opcodes (already defined):
  - `OP_ASYNC_START` – allocate a `Future` and push it on the stack.
  - `OP_ASYNC_AWAIT` – pop a `Future`, register the continuation, and suspend.
  - `OP_ASYNC_YIELD` – explicit yield point used by `await`.
  - `OP_ASYNC_RESUME` – push the resolved value and resume the coroutine.
- No new HVM instructions beyond this async family; existing vector opcodes are reused where possible.

### 6. JIT integration
- JIT emits calls to the C‑level API (`hoo_future_new`, `hoo_future_is_ready`, `hoo_future_get`, `hoo_future_set`).
- For primitive element types the JIT may forward to existing vector opcodes (`VEC_PUSH`, `VEC_GET`) for performance.

### 7. Runtime (C/C++)
- **Structure (`hoo_future.h`):**
  ```c
  typedef struct HooFuture {
      int64_t elem_type_id;   // ID of T
      void *value;            // resolved value (ARC managed)
      int ready;              // 0 = pending, 1 = resolved
      int32_t refcount;       // ARC reference count
      uv_async_t *notify;     // libuv async handle for VM wake‑up
  } HooFuture;
  ```
- Core functions:
  - `hoo_future_new(int64_t elem_type_id)` – allocate and initialise.
  - `hoo_future_is_ready(HooFuture *f)` – query readiness.
  - `hoo_future_get(HooFuture *f)` – used by `await` to obtain the concrete value (blocks only logically; the VM yields).
  - `hoo_future_set(HooFuture *f, void *value)` – called from libuv callbacks; stores the result, marks ready, and triggers `uv_async_send`.
  - ARC helpers `hoo_future_retain` / `hoo_future_release`.
- **Notification:** each future owns a `uv_async_t` that, when the future is resolved, wakes the VM so it can schedule `OP_ASYNC_RESUME`.

### 8. Libuv wrappers
- Every async API creates a `Future` and starts a libuv request, passing the future as `req->data`.
- Example wrapper:
  ```c
  HooFuture *fs_read_async(const char *path) {
      HooFuture *f = hoo_future_new(HOO_TYPE_BYTE_ARRAY);
      uv_fs_t *req = malloc(sizeof(uv_fs_t));
      req->data = f;
      uv_fs_open(hooc_uv_loop(), req, path, O_RDONLY, 0, fs_open_cb);
      return f;
  }
  ```
- The callback `fs_open_cb` eventually calls `hoo_future_set(f, bytearray_ptr)`.

### 9. Memory management (ARC)
- Futures are reference‑counted. When a future is released before it resolves, the associated libuv request is cancelled via `uv_cancel`.
- The resolved value is retained by the future and released when the future’s refcount drops to zero.

### 10. Testing strategy
- Unit tests for `hoo_future_*` (creation, retain/release, set/get, cancellation).
- Integration tests for each async API (`fs_read_async`, `tcp_connect_async`, `sleep_async`).
- Coroutine tests verifying suspension, resumption, and proper exception propagation.
- Stress tests spawning many concurrent futures to validate the event‑loop scaling.

### 11. Documentation updates
- All runtime API docs (`fs.md`, `io.md`, `net.md`) now contain async variants returning `Future<T>`.
- New language guide `docs/language/async_await.md` with usage examples.
- Top‑level `README.md` mentions async/await support.

This comprehensive description ensures that every component—from source syntax to the underlying libuv event loop—understands and correctly handles `Future<T>`.

## Implementation Checklist
- **Runtime**
  - Add `hoo_future.h/.cpp` with reference‑counted future implementation.
  - Wrap libuv handles (`uv_fs_t`, `uv_tcp_t`, `uv_timer_t`, `uv_process_t`) in ARC objects (`HooUVHandle`).
  - Implement global event‑loop singleton and async scheduling utilities.
- **Header**
  - Define new type IDs: `HOO_TYPE_FUTURE`, `HOO_TYPE_UV_HANDLE` in `hoo_runtime.h`.
- **Core Language**
  - Extend grammar (`parser.y`) for `async fn` and `await`.
  - Add AST nodes (`AsyncFunctionDecl`, `AwaitExpression`).
  - Update semantic analysis to type‑check futures and enforce coroutine rules.
- **Mangling/Demangling**
  - Modify `hoo_mangle.cpp` and `hoo_demangle.cpp` to handle `_a` suffix and future type encoding.
- **Codegen**
  - Add new opcodes to `bytecode.h` and implementation in `codegen.cpp`.
  - JIT stubs for libuv callbacks and future resolution.
- **Tests**
  - Unit tests for each async API (`fs_read_async`, `tcp_connect_async`, `sleep_async`).
  - Tests for coroutine suspension/resumption, cancellation, and error propagation.
  - Integration tests exercising multiple concurrent futures.
- **Docs**
  - Update `docs/runtime/api/fs.md`, `io.md`, `net.md` with async variants and `Future<T>` description.
  - Add a new section in `docs/language/async_await.md` with usage examples.
  - Revise `docs/runtime/api/index.md` and top‑level `README.md` to mention async/await support.
- **Build System**
  - Link against libuv (`-luv`) for all targets.
  - Add detection of libuv in CMake/Makefiles, with fallback error if missing.

---

## Risks & Mitigations
* **Event‑loop starvation** – Ensure the VM yields regularly; use cooperative multitasking.
* **Cross‑platform differences** – libuv abstracts most OS specifics, but test on macOS, Linux, and Windows CI.
* **Memory leaks** – ARC integration must correctly retain/release UV handles; add stress tests.
* **ABI stability** – Futures are opaque; existing binary interfaces remain unchanged.

---

## Acceptance Criteria
1. `async fn` and `await` compile without errors.
2. All async APIs return a `Future<T>`.
3. `await` blocks the coroutine until the future resolves, then returns the concrete value.
4. Event loop runs automatically; program exits only after all futures are settled.
5. Documentation updated and examples compile and run.
6. No regressions in existing synchronous code paths.

---

## Status
- **Date**: 2026-06-21
- **Status**: **PROPOSED**

*Prepared by Antigravity AI – 2026‑06‑21*
