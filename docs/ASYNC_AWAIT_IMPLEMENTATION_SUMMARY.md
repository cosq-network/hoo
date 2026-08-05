# Async/Await Implementation Summary

## Status

Async/await is implemented for the current HVM Future model. Async functions
create and resolve `Future<T>` values, and `await` unwraps them through the
runtime ABI. Waiting is cooperative and pumps the libuv event loop; true
stack-frame suspension is future work because HVM has no suspend/resume
instructions yet.

## Implementation

- `hoo_future` stores primitive results as register bits and retains/releases
  ARC-managed results correctly.
- Pending waits use a condition variable and bounded timed waits. The runtime
  calls `hoo_event_loop_run_nowait()` between waits instead of busy-spinning.
- The event loop is mutex-protected and checks `uv_loop_close()` results.
- Futures support multiple continuations. Continuation nodes are detached
  safely, and queued callbacks retain the Future until execution.
- Async codegen creates `Future<void>` when no result is declared and
  `Future<T>` for typed results. Async functions return the stable pointer ABI.
- `await` is restricted to async functions and Future operands; its element
  type is inferred for subsequent codegen.
- HVM/JIT registration uses isolated native aliases for Future creation,
  resolution, and unwrapping. No invalid `llvm.coro.*` calls are emitted into
  HVM bytecode.

## Verification

- `NewLanguageFeaturesTest.cpp` verifies an async function executes and
  resolves its primitive result.
- `HooFutureJitTest.cpp` covers Future creation, primitive round trips,
  managed values, errors, waiting, retention, and multiple continuations.
- Full CTest run: 2030 tests passed, 2 disabled, 0 failures.

## Limitation and next step

An `await` on a pending Future cooperatively waits on the executing thread
while allowing libuv work to run. It is not yet a resumable HVM stack frame.
Implementing true non-blocking suspension requires coordinated VM, bytecode,
and codegen support for capturing and resuming execution state.
