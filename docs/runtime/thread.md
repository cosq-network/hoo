# Threading (`hoo.thread`)

The `hoo.thread` module provides cross-platform thread management, including spawning, joining, and synchronization primitives. It uses **Win32 Threads** on Windows and **pthreads** on macOS and Linux.

## 1. Thread Operations

- `Thread.spawn(func, arg)` — Spawn a new thread running `func(arg)`. Returns a cross-platform thread ID (int64_t), or -1 on failure.
- `thread.join()` — Wait for thread to exit. Returns the thread's return value (cast to `int64_t`), or -1 on failure.
- `Thread.self()` — Returns the calling thread's ID.

## 2. Mutex Operations

- `Mutex.new()` — Create a new mutex. Returns opaque `HooMutex` handle (using Critical Sections on Windows, pthread_mutex on POSIX).
- `mutex.lock()` — Lock mutex. Returns 0 on success, -1 on error.
- `mutex.unlock()` — Unlock mutex. Returns 0 on success, -1 on error.
- `mutex.destroy()` — Destroy mutex and free its memory. Returns 0 on success.
