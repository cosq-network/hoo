# ISSUE-061: Async/Await Implementation

## Status

- **Updated**: 2026-08-05
- **Status**: **RESOLVED for the cooperative HVM Future model**
- **Priority**: P0 in the original 2026-07-19 audit
- **Remaining scope**: true HVM stack-frame suspension is deferred

## Original defects

The initial implementation exposed async syntax but did not reliably wrap
results in Futures, did not integrate pending waits with the event loop, and
did not verify runtime behavior through the JIT. It also attempted to emit
LLVM coroutine pseudo-calls that HVM cannot execute as suspension operations.

## Resolution

- Async functions create a `Future<T>` or `Future<void>` and return its stable
  pointer ABI. Return values are resolved before the function returns.
- `await` is validated to occur inside async functions and to receive a
  Future. It calls the registered Future unwrap helper and propagates errors.
- Primitive results use raw register bits; managed results follow ARC retain
  and release rules.
- Pending waits use a condition variable plus bounded timed waits and pump the
  mutex-protected libuv loop with `UV_RUN_NOWAIT`.
- Multiple continuations are supported and cleaned up safely. Queued callbacks
  retain their Future until execution.
- Native aliases are registered in the JIT for Future creation, resolution,
  error handling, and await unwrapping.
- Invalid `llvm.coro.*` pseudo-calls are no longer emitted. The existing HVM
  coroutine passes remain future-facing infrastructure, not the current
  execution mechanism.

## Verification

- Async JIT execution is covered by `NewLanguageFeaturesTest.cpp`.
- Runtime/JIT Future behavior is covered by `HooFutureJitTest.cpp`, including
  primitive round trips and multiple continuations.
- The complete test suite passes: 2030 passed, 2 disabled, 0 failures.

## Deferred work

The current implementation may wait cooperatively on the executing thread
when a Future is pending. Fully non-blocking async execution requires HVM
bytecode and VM support for capturing, suspending, and resuming an execution
frame. That work should be tracked separately from the resolved Future ABI
and event-loop correctness fixes.
