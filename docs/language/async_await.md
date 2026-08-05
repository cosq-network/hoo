# Async/Await in Hoo

Hoo supports asynchronous function syntax through `async`, `await`, and
`Future<T>`. The current HVM implementation is Future-based and cooperative:
it can progress libuv work while waiting, but it does not yet suspend and
resume an HVM stack frame.

## `async`

Place `async` before `func`. An async function returns a pointer to a
`Future<T>` at the runtime ABI. If the function has no result expression, the
result is `Future<void>`.

```hoo
async func:Future<int64> getValue() {
    return 42;
}
```

The declared return type should be `Future<T>` when a result type is written.
The compiler validates async return types and creates the Future at function
entry. Returning a primitive stores its register bits directly; returning a
managed value preserves it with ARC ownership rules.

## `await`

`await(future)` is valid only inside an async function and only accepts a
`Future`. It returns the Future's element value, or raises the Future's error
when one is present.

```hoo
async func:Future<int64> process() {
    var value = await(getValue());
    return value + 1;
}
```

For an already-resolved Future, await unwraps immediately. For a pending
Future, the runtime waits on a condition variable, periodically runs the Hoo
libuv loop in `UV_RUN_NOWAIT` mode, and yields between checks. This avoids the
old busy-spin behavior and allows pending event-loop work to make progress.

## Futures and continuations

Future resolution is one-shot. A Future can hold either a primitive register
value or an ARC-managed object, and its error state is retained until cleanup.
Multiple continuation callbacks are supported; callbacks are detached safely
when resolution occurs, and queued callbacks retain the Future until they run.

## Current boundary

This implementation provides safe cooperative async execution and stable JIT
symbols for the Future operations. It intentionally does not emit
`llvm.coro.*` pseudo-calls: the HVM instruction set does not currently provide
the stack-frame capture and resume operations those intrinsics require. True
stackless coroutine suspension is reserved for a future HVM VM/codegen change.
