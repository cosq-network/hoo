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
