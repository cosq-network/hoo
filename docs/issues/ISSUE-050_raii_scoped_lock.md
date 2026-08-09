# ISSUE-050: No RAII Scoped Lock for Mutex

## 1. Overview
The `Mutex` API provides raw `lock()` and `unlock()` operations with no RAII scoped-lock mechanism. Every lock must be manually paired with unlock, making early-return and exception paths error-prone.

## 2. Technical Analysis
- **Location**: `src/runtime/lib/hoo_thread.h:14-17`, `src/runtime/lib/hoo_thread.cpp:96-142`
- **Current API**: `hoo_thread_mutex_lock/unlock/destroy`
- **Missing**:
  - `ScopedLock` / `LockGuard` RAII wrapper
  - `withLock` / `synchronized` language-level pattern
  - `try_lock_for`, `try_lock_until` timed lock attempts

## 3. Impact
- Any early return, exception, or error path before `unlock()` causes a deadlock.
- Manual lock management is error-prone and verbose.
- No way to express scoped locking in Hoo source code safely.

## 4. Suggested Fix
1. Add a C++ RAII helper (`ScopedLock`) in `hoo_thread.h` that calls `lock()` in the constructor and `unlock()` in the destructor.
2. Optionally expose a `withLock(fn)` pattern at the Hoo language level.
3. Add `try_lock` variants for non-blocking acquisition.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED**
- **Priority**: **MEDIUM**

## 6. Resolution
The runtime provides `hoo::thread::ScopedLock`, a non-copyable and move-aware
C++ RAII wrapper over `HooMutex`, plus `hoo_thread_mutex_try_lock`. The C ABI
remains available for generated Hoo code.
