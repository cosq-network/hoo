# Memory Model & ARC

The Hoo Runtime uses Automatic Reference Counting (ARC) combined with a Thread-Local Allocation Buffer (TLAB) to provide high-performance, deterministic memory management without the pause times of a tracing garbage collector.

## 1. The 16-Byte Object Header
Every object managed by the runtime (including Arrays, Strings, Maps, and User Objects) is prefixed with a hidden 16-byte header. When `hoo_alloc` returns a pointer, it points *past* this header to the user data payload.

| Offset | Size | Type | Description |
| :--- | :--- | :--- | :--- |
| `-16` | 8 bytes | `_Atomic int64_t` | Thread-safe Reference Count. |
| `-8` | 8 bytes | `int64_t` | `type_id` (RTTI identifier). |
| `0` | variable| User Data | The pointer passed around in HVM registers. |

### Reserved Type IDs
- `1..9`: Primitive Type Placeholders (int64, float, bool, etc.)
- `100`: Generic Object
- `101`: HooString
- `102`: HooArray
- `103`: HooMap
- `104`: HooException
- `105-108`: Utility handles (Random, URL, Http)
- `109`: HooCharacter
- `110`: HooUUID
- `111`: HooRegex

## 2. Thread-Local Allocation Buffer (TLAB)
To minimize lock contention during allocation, the runtime utilizes a TLAB.

- **Block Size**: 64 KB per thread block (`HOO_TLAB_BLOCK_SIZE`).
- **Threshold**: Objects $\le$ 2048 bytes are allocated from the TLAB via simple pointer bumping.
- **Fallback**: Objects > 2048 bytes (or when a block is exhausted) fall back to the system `malloc`.
- **Tracking**: TLAB allocations are tracked in a thread-local linked list (`g_tlab_objects`) so they can be properly identified and freed independently of block lifetimes upon release.

## 3. Core API (C ABI)

- `void* hoo_alloc(size_t size, int64_t type_id)`: Allocates zeroed memory, initializing the header with `refcount = 1` and the specified `type_id`.
- `void* hoo_realloc(void* obj, size_t new_size)`: Resizes a managed object, copying data if necessary.
- `void* hoo_retain(void* obj)`: Atomically increments the `refcount`. Returns the original pointer.
- `void hoo_release(void* obj)`: Atomically decrements the `refcount`. If it reaches `0`, the memory (and linked structures) are freed.
- `int64_t hoo_get_refcount(void* obj)`: Returns the current reference count.
- `int64_t hoo_get_type_id(void* obj)`: Returns the object's runtime `type_id`.

## 4. Diagnostics & Statistics
The runtime tracks allocation telemetry which can be dumped using:
- `hoo_print_memory_stats()`: Prints total allocations, deallocations, and current live object count (leak detection).
- `hoo_get_tlab_stats()`: Returns TLAB hit/miss ratios and blocks allocated.
