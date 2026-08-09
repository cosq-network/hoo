# ISSUE-052: No Condition Variables or Thread Notification Mechanism

## 1. Overview
The thread API provides only `spawn`, `join`, `self`, `mutex_create`, `mutex_lock`, `mutex_unlock`, and `mutex_destroy`. There is no condition variable, `wait`/`notify`/`notifyAll`, `Semaphore`, `Barrier`, `CountDownLatch`, or futex wrapper for thread synchronization beyond basic mutual exclusion.

## 2. Technical Analysis
- **Location**: `src/runtime/lib/hoo_thread.h` (21 lines)
- **Current API surface**:
  - `thread_spawn`, `thread_join`, `thread_self`
  - `mutex_create`, `mutex_lock`, `mutex_unlock`, `mutex_destroy`
- **Missing**:
  - Condition variables (`wait`, `notify`, `notify_all`)
  - `Semaphore` for producer-consumer patterns
  - `Barrier` for phased parallelism
  - `CountDownLatch` for one-time synchronization
  - Timed wait variants

## 3. Impact
- Multi-threaded programs requiring producer-consumer patterns or complex synchronization cannot be written correctly.
- Busy-wait loops are the only alternative, wasting CPU and battery.
- ISSUE-043 (async/await with libuv) requires a notification mechanism internally.

## 4. Suggested Fix
1. Add `hoo_thread_condition_create/wait/notify_one/notify_all/destroy` wrapping `std::condition_variable`.
2. Expose `hoo_thread_semaphore_create/acquire/release/destroy`.
3. Add a language-level `sync { ... }` or `condition` keyword for ergonomic use.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED**
- **Priority**: **MEDIUM**

## 6. Resolution
The libuv-backed runtime provides condition-variable create/wait/timed-wait,
notify-one, notify-all, and destroy operations. It also provides semaphore
create/wait/try-wait/post/destroy primitives.
