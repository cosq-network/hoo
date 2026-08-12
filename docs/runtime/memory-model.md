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
- `112`: Reserved legacy JSON type ID. The current JSON API does not allocate or return a managed JSON document handle; it operates on `Dict<int64, any>`, `List`, and `HooString`.
- `113`: HooBuffer
- `117`: HooDict
- `118`: HooList

Type IDs are not limited to the reserved built-in range. The runtime
destructor registry accepts any non-negative `int64_t` type ID and grows as
needed. Registration, replacement, removal, and lookup are synchronized;
destructor callbacks execute after the registry lock is released.

Type ID `0` is reserved for the virtual `any` tagged value. It is not a managed object header type; it identifies the two-slot `{ type_id, data }` value shape used by heterogeneous runtime containers.

## 2. Borrowed Byte Slices

Hoo source may declare a borrowed byte view as `slice<byte>`. Its ABI is a
pointer to a `HooByteSliceHandle`; the handle contains a pointer and length but
does not retain or own the backing `Buffer`. The producer must keep the backing
Buffer alive for the entire slice lifetime. Slice-aware encoding, hashing,
compression, and socket functions consume the view and return independent
strings or owned Buffers.

### Buffer storage

A `HooBuffer` stores its bytes out-of-line: the managed block holds only
`length`, `capacity`, and a heap pointer to the byte storage. Growing a buffer
reallocates only the byte storage, so the buffer **handle never moves** and
refcount state is never disturbed. A pointer from `hoo_buffer_data` may become
stale after an append; re-query it afterwards. Appending a buffer to itself is
safe.

## 3. Thread-Local Allocation Buffer (TLAB)
To minimize lock contention during allocation, the runtime utilizes a TLAB.

- **Block Size**: 64 KB per thread block (`HOO_TLAB_BLOCK_SIZE`).
- **Threshold**: Objects $\le$ 2048 bytes are allocated from the TLAB via simple pointer bumping.
- **Fallback**: Objects > 2048 bytes (or when a block is exhausted) fall back to the system `malloc`.
- **Tracking**: TLAB allocations are tracked in a thread-local linked list (`g_tlab_objects`) so they can be properly identified and freed independently of block lifetimes upon release.

## 4. Core API (C ABI)

- `void* hoo_alloc(size_t size, int64_t type_id)`: Allocates zeroed memory, initializing the header with `refcount = 1` and the specified `type_id`.
- `void* hoo_realloc(void* obj, size_t new_size)`: Resizes a managed object, copying data if necessary. **Caveat:** releasing the old block runs the type's registered destructor. Do not use `hoo_realloc` on types whose destructor frees values the new block still references (e.g. arrays of managed elements). Managed arrays grow by allocating a new block, copying element handles with a `hoo_retain` per element, then releasing the old block — the old block's destructor drops the old references, leaving each element owned exactly once.
- `void* hoo_retain(void* obj)`: Atomically increments the `refcount`. Returns the original pointer.
- `void hoo_release(void* obj)`: Atomically decrements the `refcount`. If it reaches `0`, the memory (and linked structures) are freed.
- `int64_t hoo_get_refcount(void* obj)`: Returns the current reference count.
- `int64_t hoo_get_type_id(void* obj)`: Returns the object's runtime `type_id`.
- `void hoo_register_destructor(int64_t type_id, HooDestructor dtor)`: Registers
  or removes a destructor for a managed type. Negative IDs and allocation
  failures are reported as fatal runtime errors rather than ignored.

## 5. Diagnostics & Statistics
The runtime tracks allocation telemetry which can be dumped using:
- `hoo_print_memory_stats()`: Prints total allocations, deallocations, and current live object count (leak detection).
- `hoo_get_tlab_stats()`: Returns TLAB hit/miss ratios and blocks allocated.
