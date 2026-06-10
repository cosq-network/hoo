# Thread — Concurrency

The `Thread` class provides static methods for thread management and mutex synchronization.

## Thread Methods

`Thread.self() :int64`
Returns the operating system thread ID of the current thread.

`Thread.mutexCreate() :ptr`
Creates a new mutex and returns an opaque handle.

`Thread.mutexLock(mutex: ptr) :int64`
Acquires the mutex, blocking until it becomes available. Returns 0 on success, -1 on error.

`Thread.mutexUnlock(mutex: ptr) :int64`
Releases the mutex. Returns 0 on success, -1 on error.

`Thread.mutexDestroy(mutex: ptr) :int64`
Destroys the mutex and frees its resources. Returns 0 on success, -1 on error.

## Example

```hoo
let mtx = Thread.mutexCreate()

func increment(counter: ptr) :int64 {
    for i in 0..1000 {
        Thread.mutexLock(mtx)
        counter = counter + 1
        Thread.mutexUnlock(mtx)
    }
    return 0
}

let pid = Thread.self()
println("Thread: " + pid)

Thread.mutexDestroy(mtx)
```
