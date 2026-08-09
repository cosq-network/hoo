#ifndef HOO_RUNTIME_H
#define HOO_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hooc Runtime Library
 *
 * Provides automatic reference counting (ARC) for heap-allocated objects.
 * All objects are allocated with a hidden header containing:
 * - Reference count (int64_t)
 * - Type ID (int64_t) for runtime type information
 */

/**
 * Core Type IDs for managed objects.
 * These are stored in the object header at offset -8.
 */
#define HOO_TYPE_OBJECT       100  // Generic object
#define HOO_TYPE_STRING       101  // HooString
#define HOO_TYPE_ARRAY        102  // HooArray
#define HOO_TYPE_MAP          103  // HooMap
#define HOO_TYPE_EXCEPTION    104  // HooException (base)
#define HOO_TYPE_RANDOM       105  // HooRandom state
#define HOO_TYPE_NET_URL      106  // HooURL
#define HOO_TYPE_NET_HTTP_RES 107  // HooHttpResponse
#define HOO_TYPE_NET_HTTP_CLI 108  // HooHttpClient
#define HOO_TYPE_CHARACTER    109  // HooCharacter
#define HOO_TYPE_UUID         110  // HooUUID
#define HOO_TYPE_REGEX        111
#define HOO_TYPE_JSON         112  // HooRegex
#define HOO_TYPE_BUFFER       113  // HooBuffer
#define HOO_TYPE_CSV          114  // HooCsv
#define HOO_TYPE_HASHMAP      117  // HooHashMap intrinsic
#define HOO_TYPE_ANYARRAY     118  // HooAnyArray intrinsic
#define HOO_TYPE_DATETIME     119  // HooDateTime
#define HOO_TYPE_FUTURE       123  // HooFuture<T>
#define HOO_TYPE_UV_HANDLE    124  // HooUVHandle (reserved for libuv integration)
#define HOO_TYPE_NET_SOCKET   127  // HooSocket TCP handle
#define HOO_TYPE_CONDITION   128  // HooCondition synchronization handle
#define HOO_TYPE_SEMAPHORE   129  // HooSemaphore synchronization handle
#define HOO_TYPE_BYTE_SLICE  130  // Borrowed byte-slice view handle
#define HOO_TYPE_TENSOR_SERIALIZED 126 // Tensor value stored inside HooAnyValue

// Primitive Type IDs (for runtime conversion/reflection)
#define HOO_TYPE_INT64        1
#define HOO_TYPE_FLOAT64      2
#define HOO_TYPE_BOOL         3
#define HOO_TYPE_VOID         4
#define HOO_TYPE_INT8         5
#define HOO_TYPE_BYTE         6
#define HOO_TYPE_CHAR         7  // Raw char (not the Character object)

/**
 * Allocate a new object on the heap with reference count initialized to 1.
 *
 * @param size Size of the object data (excluding header)
 * @param type_id Unique identifier for the object's class type
 * @return Pointer to object data (not the header)
 */
void* hoo_alloc(size_t size, int64_t type_id);

/**
 * Reallocate a managed object with a new size.
 * If the existing capacity is sufficient, returns the same pointer.
 * Otherwise, allocates a new block, copies data, and releases the old one.
 * @return New pointer to object data.
 */
void* hoo_realloc(void* obj, size_t new_size);

/**
 * Increment the reference count of an object.
 * Called automatically on assignment and parameter passing.
 *
 * @param obj Pointer to object (can be NULL)
 * @return The same pointer (for convenience)
 */
void* hoo_retain(void* obj);

/**
 * Decrement the reference count of an object.
 * If the count reaches zero, the object is freed.
 * Called automatically when variables go out of scope.
 *
 * @param obj Pointer to object (can be NULL)
 */
void hoo_release(void* obj);

/**
 * Release an object and return whether the refcount reached zero.
 * Performs the same decrement and optional destruction as hoo_release,
 * but returns 1 if the object was freed (refcount hit zero), 0 otherwise.
 *
 * @param obj Pointer to object (can be NULL)
 * @return 1 if object was freed (zero flag set), 0 otherwise
 */
int64_t hoo_release_zero_flag(void* obj);

/**
 * Get the current reference count of an object (for debugging/testing).
 *
 * @param obj Pointer to object (can be NULL)
 * @return Current reference count, or 0 if obj is NULL
 */
int64_t hoo_get_refcount(void* obj);

/**
 * Get the type ID of an object (for RTTI).
 *
 * @param obj Pointer to object (must not be NULL)
 * @return Type ID assigned during allocation
 */
int64_t hoo_get_type_id(void* obj);

/**
 * Destructor callback type for managed objects.
 * Registered per type_id and called automatically by hoo_release
 * when an object's reference count reaches zero (before freeing memory).
 * @param obj Pointer to the object data (after ARC header).
 */
typedef void (*HooDestructor)(void* obj);

/**
 * Register a destructor callback for a given type_id.
 * The callback is invoked by hoo_release when refcount reaches zero.
 * Only one destructor per type_id; subsequent calls overwrite.
 * @param type_id The type ID to register for (e.g. HOO_TYPE_MAP).
 * @param dtor The destructor function, or NULL to unregister.
 */
void hoo_register_destructor(int64_t type_id, HooDestructor dtor);

/**
 * Returns non-zero when obj is a live object allocated by hoo_alloc.
 */
int64_t hoo_is_managed_object(const void* obj);

/**
 * Write an int64/pointer value to an object field at the given byte offset.
 */
void hoo_object_set_field(void* obj, int64_t offset, int64_t value);

/**
 * Read an int64/pointer value from an object field at the given byte offset.
 */
int64_t hoo_object_get_field(void* obj, int64_t offset);

/**
 * Get the current capacity of a managed object in bytes.
 */
int64_t hoo_get_capacity(void* obj);

/**
 * Update the capacity field of a managed object (internal use).
 */
void hoo_set_capacity(void* obj, int64_t capacity);

/**
 * Print memory statistics (for debugging).
 * Shows total allocations, deallocations, and current live objects.
 */
void hoo_print_memory_stats(void);

/**
 * Reset memory statistics counters.
 */
void hoo_reset_memory_stats(void);

/**
 * Thread-local allocation buffer (TLAB) runtime stats.
 * Counts represent process lifetime unless reset APIs are called.
 */
typedef struct {
    int64_t tlab_hits;
    int64_t tlab_misses;
    int64_t tlab_blocks_allocated;
} HooTLABStats;

/**
 * Returns 1 when TLAB fast-path allocation is enabled.
 */
int32_t hoo_tlab_enabled(void);

/**
 * Returns current process TLAB stats counters.
 */
HooTLABStats hoo_get_tlab_stats(void);

/**
 * Reset TLAB stats counters to zero.
 */
void hoo_reset_tlab_stats(void);

/**
 * Release thread-local TLAB cached blocks for the calling thread.
 * Safe to call multiple times.
 */
void hoo_tlab_reset_thread_cache(void);

#ifdef __cplusplus
}
#endif

#endif // HOO_RUNTIME_H
