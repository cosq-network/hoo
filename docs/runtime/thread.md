# Threading (`hoo.thread`)

The `hoo.thread` module provides thread spawn/join/self via pthreads, and mutex create/lock/unlock/destroy for concurrent synchronization. **Not available on Windows.**

## 1. Thread Operations

- `Thread.spawn(func, arg)` — Spawn a new thread running `func(arg)`. Returns thread ID, or -1 on failure.
- `thread.join()` — Wait for thread to exit. Returns the thread's return value (cast to `int64_t`), or -1 on failure.
- `Thread.self()` — Returns the calling thread's ID.

## 2. Mutex Operations

- `Mutex.new()` — Create a new mutex. Returns opaque `HooMutex` handle.
- `mutex.lock()` — Lock mutex. Returns 0 on success, -1 on error.
- `mutex.unlock()` — Unlock mutex. Returns 0 on success, -1 on error.
- `mutex.destroy()` — Destroy mutex and free its memory. Returns 0 on success.
