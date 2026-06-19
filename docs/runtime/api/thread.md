# Thread — Concurrency

**Import Requirement:**
```hoo
import hoo.thread;
```

The `Thread` class provides static methods for thread management and mutex synchronization.

## Thread Methods

`Thread.self() :int64`
Returns the operating system thread ID of the current thread.

`Thread.mutex_create() :ptr`
Creates a new mutex and returns an opaque handle.

`Thread.mutex_lock(mutex: ptr) :int64`
Acquires the mutex, blocking until it becomes available. Returns 0 on success, -1 on error.

`Thread.mutex_unlock(mutex: ptr) :int64`
Releases the mutex. Returns 0 on success, -1 on error.

`Thread.mutex_destroy(mutex: ptr) :int64`
Destroys the mutex and frees its resources. Returns 0 on success, -1 on error.

## Example

```hoo
import hoo.thread;

let mtx = Thread.mutex_create()

func increment(counter: ptr) :int64 {
    for i in 0..1000 {
        Thread.mutex_lock(mtx)
        counter = counter + 1
        Thread.mutex_unlock(mtx)
    }
    return 0
}

let pid = Thread.self()
println("Thread: " + pid)

Thread.mutex_destroy(mtx)
```
