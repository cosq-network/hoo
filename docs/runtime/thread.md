# Threading (`hoo.thread`)

The `hoo.thread` module provides thread spawn/join/self via pthreads, and mutex create/lock/unlock/destroy for concurrent synchronization. **Not available on Windows.**

## 1. Thread Operations

- `hoo_thread_spawn(func, arg)` — Spawn a new thread running `func(arg)`. Returns thread ID, or -1 on failure.
- `hoo_thread_join(thread_id)` — Wait for thread to exit. Returns the thread's return value (cast to `int64_t`), or -1 on failure.
- `hoo_thread_self()` — Returns the calling thread's ID.

## 2. Mutex Operations

- `hoo_thread_mutex_create()` — Create a new mutex. Returns opaque `HooMutex` handle.
- `hoo_thread_mutex_lock(mutex)` — Lock mutex. Returns 0 on success, -1 on error.
- `hoo_thread_mutex_unlock(mutex)` — Unlock mutex. Returns 0 on success, -1 on error.
- `hoo_thread_mutex_destroy(mutex)` — Destroy mutex and free its memory. Returns 0 on success.
