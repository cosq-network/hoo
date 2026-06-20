# Thread — Concurrency and Mutexes

The `thread` module provides free functions for thread management and a `Mutex` class for synchronization.

## Mutex Class

Provides mutex synchronization.

### Constructor

`new Mutex() :Mutex`
Creates a new Mutex.

### Methods

`mutex.lock() :int64`
Acquires the mutex, blocking until it becomes available. Returns 0 on success, -1 on error.

`mutex.unlock() :int64`
Releases the mutex. Returns 0 on success, -1 on error.

`mutex.release() :int64`
Destroys the mutex and frees its resources. Returns 0 on success, -1 on error.

## Free Functions

`thread_self() :int64`
Returns the operating system thread ID of the current thread.

`thread_spawn(func: ptr, arg: ptr) :int64`
Spawns a new thread executing `func` with `arg`. Returns the thread ID.

`thread_join(thread_id: int64) :int64`
Waits for the thread to exit and returns its exit value.

## Example

```hoo
import hoo.thread;

let mtx = new Mutex()

func increment(counter: ptr) :int64 {
    for i in 0..1000 {
        mtx.lock()
        counter = counter + 1
        mtx.unlock()
    }
    return 0
}

let pid = thread_self()
println("Thread: " + pid)

mtx.release()
```
