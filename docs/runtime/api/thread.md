# Thread API Reference

## Module Name

Part of the `hoo` module.

## Import Statement

```hoo
import hoo.thread;
```

## Module Description

The `thread` module provides free functions for thread spawning and sleeping, and a `Mutex` class for synchronization. Threading enables concurrent execution of Hoo functions. Mutexes protect shared state from concurrent access.

## Class: Mutex

### Declaration

```hoo
class Mutex
```

### Public Fields

None.

### Public Instance Functions

#### Constructor: `Mutex`

Creates a new `Mutex` instance backed by a runtime mutex handle.

**Syntax:**

```hoo
Mutex() :Mutex
```

**Parameters:**

None.

**Returns:**

`Mutex` — A new `Mutex` instance.

**Errors:**

Returns a null handle if the underlying mutex cannot be created.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    var mtx = Mutex();
    return 0;
}
```

---

#### `lock`

Acquires the mutex, blocking until it becomes available.

**Syntax:**

```hoo
lock() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. If called on a null mutex handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.thread;

func worker(mtx: Mutex) :int64 {
    mtx.lock();
    // critical section
    mtx.unlock();
    return 0;
}

func :int64 main() {
    var mtx = Mutex();
    mtx.lock();
    mtx.unlock();
    return 0;
}
```

---

#### `unlock`

Releases the mutex. The mutex must have been previously locked by the calling thread.

**Syntax:**

```hoo
unlock() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. If called on a null mutex handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    var mtx = Mutex();
    mtx.lock();
    mtx.unlock();
    return 0;
}
```

---

#### `retain`

Increments the mutex's reference count by one.

**Syntax:**

```hoo
retain() :Mutex
```

**Parameters:**

None.

**Returns:**

`Mutex` — The mutex with incremented reference count.

**Errors:**

No errors. If called on a null mutex handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    var mtx = Mutex();
    mtx.retain();
    mtx.release();
    mtx.release();
    return 0;
}
```

---

#### `release`

Decrements the mutex's reference count by one. When the reference count reaches zero the mutex is destroyed.

**Syntax:**

```hoo
release() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. Calling `release` on an already-freed or null mutex handle is a no-op.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    var mtx = Mutex();
    mtx.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of the mutex.

**Syntax:**

```hoo
refcount() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — The current reference count.

**Errors:**

Returns `0` for a null mutex handle.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    var mtx = Mutex();
    mtx.retain();
    var rc = mtx.refcount(); // 2
    mtx.release();
    mtx.release();
    return 0;
}
```

## Free Functions

---

#### `thread_spawn`

Spawns a new thread that executes the given function.

**Syntax:**

```hoo
thread_spawn(func: any) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `func` | `any` | A function that takes an `int64` argument and returns `int64`. |

**Returns:**

`int64` — `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero error code if thread creation fails.

**Complete Example:**

```hoo
import hoo.thread;

func worker(arg: int64) :int64 {
    println("hello from thread");
    return 0;
}

func :int64 main() {
    var rc = thread_spawn(worker);
    println("spawned: " + rc);
    return 0;
}
```

---

#### `thread_sleep`

Sleeps the current thread for the specified number of milliseconds.

**Syntax:**

```hoo
thread_sleep(millis: int64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `millis` | `int64` | Sleep duration in milliseconds. |

**Returns:**

`void`

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.thread;

func :int64 main() {
    thread_sleep(100); // sleep 100ms
    return 0;
}
```

## Usage Example

```hoo
import hoo.thread;

func worker(counter: Mutex) :int64 {
    counter.lock();
    println("in thread");
    counter.unlock();
    return 0;
}

func :int64 main() {
    var mtx = Mutex();

    thread_spawn(worker);
    thread_spawn(worker);

    thread_sleep(100);
    mtx.release();
    return 0;
}
```
