# Thread API Reference

## Module Name

Part of the `hoo` module.

## Import Statement

```hoo
import hoo;
```

## Module Description

The `Thread` module provides static methods for thread spawning and sleeping, and a `Mutex` class for synchronization. Threading enables concurrent execution of Hoo functions. Mutexes protect shared state from concurrent access.

## Class: Thread

### Declaration

```hoo
class Thread
```

### Public Fields

None.

### Public Class (Static) Functions

#### `spawn`

Spawns a new thread that executes the given function. The function must accept a single `int64` argument and return `int64`. Returns `0` on success.

**Syntax:**

```hoo
Thread.spawn(func: any) :int64
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
import hoo;

func worker(arg: int64) :int64 {
    println("hello from thread");
    return 0;
}

func :int64 main() {
    var rc = Thread.spawn(worker);
    println("spawned: " + rc);
    return 0;
}
```

---

#### `sleep`

Sleeps the current thread for the specified number of milliseconds.

**Syntax:**

```hoo
Thread.sleep(millis: int64) :void
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
import hoo;

func :int64 main() {
    Thread.sleep(100); // sleep 100ms
    return 0;
}
```

---

#### `mutex_create`

Creates a new mutex object.

**Syntax:**

```hoo
Thread.mutex_create() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — An opaque mutex handle (positive integer), or `0` on failure.

**Errors:**

Returns `0` if memory allocation fails.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var mtx = Thread.mutex_create();
    println("mutex: " + mtx);
    return 0;
}
```

---

#### `mutex_lock`

Locks a mutex. Blocks the calling thread until the mutex becomes available.

**Syntax:**

```hoo
Thread.mutex_lock(mutex: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `mutex` | `int64` | The mutex handle returned by `mutex_create`. |

**Returns:**

`int64` — `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero error code if the mutex handle is invalid.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var mtx = Thread.mutex_create();
    var rc = Thread.mutex_lock(mtx);
    // critical section
    Thread.mutex_unlock(mtx);
    Thread.mutex_destroy(mtx);
    return 0;
}
```

---

#### `mutex_unlock`

Unlocks a mutex that was previously locked by the calling thread.

**Syntax:**

```hoo
Thread.mutex_unlock(mutex: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `mutex` | `int64` | The mutex handle to unlock. |

**Returns:**

`int64` — `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero error code if the mutex handle is invalid or was not locked by the calling thread.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var mtx = Thread.mutex_create();
    Thread.mutex_lock(mtx);
    Thread.mutex_unlock(mtx);
    Thread.mutex_destroy(mtx);
    return 0;
}
```

---

#### `mutex_destroy`

Destroys a mutex and frees its resources.

**Syntax:**

```hoo
Thread.mutex_destroy(mutex: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `mutex` | `int64` | The mutex handle to destroy. |

**Returns:**

`int64` — `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero error code if the mutex handle is invalid.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var mtx = Thread.mutex_create();
    Thread.mutex_lock(mtx);
    Thread.mutex_unlock(mtx);
    Thread.mutex_destroy(mtx);
    return 0;
}
```

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
import hoo;

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
import hoo;

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
import hoo;

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
retain() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. If called on a null mutex handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo;

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
import hoo;

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
import hoo;

func :int64 main() {
    var mtx = Mutex();
    mtx.retain();
    var rc = mtx.refcount(); // 2
    mtx.release();
    mtx.release();
    return 0;
}
```

## Usage Example

```hoo
import hoo;

func worker(counter: Mutex) :int64 {
    counter.lock();
    println("in thread");
    counter.unlock();
    return 0;
}

func :int64 main() {
    var mtx = Mutex();

    Thread.spawn(worker);
    Thread.spawn(worker);

    Thread.sleep(100);
    mtx.release();
    return 0;
}
```
