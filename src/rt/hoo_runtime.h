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

#ifdef __cplusplus
}
#endif

#endif // HOO_RUNTIME_H
