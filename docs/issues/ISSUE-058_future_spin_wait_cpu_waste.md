# ISSUE-058: Future Spin-Wait Consumes 100% CPU Without Yielding

## Status

- **Updated**: 2026-08-05
- **Status**: **RESOLVED**
- **Priority**: P0 in the original audit

## Problem

`hoo_future_get_value()` and the await unwrap helper previously used a tight
polling loop. Pending Futures consumed a CPU core and could prevent work queued
on the same libuv loop from progressing.

## Resolution

- Future state is protected by a mutex and condition variable.
- Resolution notifies waiting threads instead of requiring polling.
- Pending waits call `hoo_event_loop_run_nowait()`, which runs libuv in
  `UV_RUN_NOWAIT` mode, followed by a bounded timed wait.
- Event-loop access is synchronized and `uv_loop_close()` failures are checked.
- Primitive and managed Future payloads use distinct, ownership-safe handling.

This is cooperative waiting, not stack-frame suspension. A pending await may
still wait on the executing thread while allowing event-loop work to run.

## Verification

`HooFutureJitTest.cpp` covers immediate and delayed resolution, error handling,
primitive round trips, managed values, retention, event-loop waiting, and
multiple continuations. The complete suite passes with 2030 tests passed, 2
disabled, and 0 failures.
