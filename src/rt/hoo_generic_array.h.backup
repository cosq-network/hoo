#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooArray - Generic Dynamic Array Type with Reference Counting
// ============================================================================
//
// A type-agnostic dynamic array implementation that can store elements of
// any size. Elements are stored as untyped bytes, with the element size
// tracked in the array header.
//
// Internally managed with automatic reference counting.
//

typedef void* HooArray;

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a new empty generic array with initial capacity
 *
 * The returned HooArray has refcount=1. Caller is responsible for
 * releasing when no longer needed.
 *
 * @param element_size Size of each element in bytes (must be > 0)
 * @param capacity Initial capacity (number of elements)
 * @return New HooArray with refcount=1, or NULL on allocation failure
 */
HooArray hoo_array_new(size_t element_size, int64_t capacity);

/**
 * Create a generic array from a buffer
 *
 * @param element_size Size of each element in bytes (must be > 0)
 * @param data Pointer to array data (may be NULL)
 * @param length Number of elements to copy
 * @return New HooArray with refcount=1
 */
HooArray hoo_array_from_buffer(size_t element_size, const void* data, int64_t length);

/**
 * Create a generic array with a repeated value
 *
 * @param element_size Size of each element in bytes (must be > 0)
 * @param value Pointer to value to repeat (size must be element_size bytes)
 * @param count Number of times to repeat
 * @return New HooArray containing the repeated value
 */
HooArray hoo_array_repeat(size_t element_size, const void* value, int64_t count);

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Get the number of elements in a generic array
 *
 * @param arr Array (may be NULL)
 * @return Number of elements, or 0 if arr is NULL
 */
int64_t hoo_array_length(HooArray arr);

/**
 * Get an element from a generic array by index
 *
 * The element is copied into the destination buffer. Caller must ensure
 * the destination buffer is at least element_size bytes.
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @param dest Destination buffer for element (may not be NULL)
 * @return 1 if successful, 0 if index out of bounds
 */
int64_t hoo_array_get(HooArray arr, int64_t index, void* dest);

/**
 * Set an element in a generic array by index
 *
 * The element is copied from the source buffer. Caller must ensure
 * the source buffer contains at least element_size bytes.
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @param src Source buffer containing element (may not be NULL)
 * @return 1 if successful, 0 if index out of bounds
 */
int64_t hoo_array_set(HooArray arr, int64_t index, const void* src);

/**
 * Append an element to a generic array
 *
 * Automatically grows the array if needed.
 *
 * @param arr Array
 * @param value Pointer to value to append (may not be NULL)
 * @return 1 on success, 0 on allocation failure
 */
int64_t hoo_array_push(HooArray arr, const void* value);

/**
 * Remove and return the last element from a generic array
 *
 * The element is copied into the destination buffer. Caller must ensure
 * the destination buffer is at least element_size bytes.
 *
 * @param arr Array
 * @param dest Destination buffer for popped element (may not be NULL)
 * @return 1 on success (element copied to dest), 0 if array is empty
 */
int64_t hoo_array_pop(HooArray arr, void* dest);

/**
 * Clear all elements from a generic array
 *
 * @param arr Array
 */
void hoo_array_clear(HooArray arr);

/**
 * Concatenate two generic arrays
 *
 * Returns a new array containing all elements from arr1 followed by all
 * elements from arr2. Neither input array is modified. Both arrays must
 * have the same element size.
 *
 * @param arr1 First array (may be NULL, treated as empty)
 * @param arr2 Second array (may be NULL, treated as empty)
 * @return New concatenated HooArray with refcount=1
 */
HooArray hoo_array_concat(HooArray arr1, HooArray arr2);

/**
 * Get a slice of a generic array
 *
 * @param arr Source array
 * @param start Starting index (0-based)
 * @param length Number of elements to extract
 * @return New HooArray containing the slice
 */
HooArray hoo_array_slice(HooArray arr, int64_t start, int64_t length);

/**
 * Create a clone of a generic array
 *
 * @param arr Source array
 * @return New HooArray with refcount=1, or NULL if arr is NULL
 */
HooArray hoo_array_clone(HooArray arr);

// ============================================================================
// Element Size Query
// ============================================================================

/**
 * Get the element size of a generic array
 *
 * @param arr Array (may be NULL)
 * @return Element size in bytes, or 0 if arr is NULL
 */
size_t hoo_array_element_size(HooArray arr);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count of a generic array
 *
 * @param arr Array (may be NULL)
 * @return The array (for chaining)
 */
HooArray hoo_array_retain(HooArray arr);

/**
 * Decrement reference count of a generic array
 *
 * If refcount reaches 0, the array is freed.
 *
 * @param arr Array (may be NULL)
 */
void hoo_array_release(HooArray arr);

/**
 * Get the reference count of a generic array
 *
 * @param arr Array (may be NULL)
 * @return Current refcount, or 0 if arr is NULL
 */
int64_t hoo_array_refcount(HooArray arr);

// ============================================================================
// Type-Specific Wrappers - Convenience Functions
// ============================================================================
//
// The following are type-specific wrappers around the generic array that
// provide convenience APIs for common types (int64, double).
// Internally, they all use HooArray (generic array) but provide type-safe
// interfaces for specific element types.
//

// int64 Array Wrappers
typedef HooArray HooInt64Array;

HooInt64Array hoo_int64_array_new(int64_t capacity);
HooInt64Array hoo_int64_array_from_buffer(const int64_t* data, int64_t length);
HooInt64Array hoo_int64_array_repeat(int64_t value, int64_t count);
int64_t hoo_int64_array_length(HooInt64Array arr);
int64_t hoo_int64_array_get(HooInt64Array arr, int64_t index);
int64_t hoo_int64_array_set(HooInt64Array arr, int64_t index, int64_t value);
int64_t hoo_int64_array_push(HooInt64Array arr, int64_t value);
int64_t hoo_int64_array_pop(HooInt64Array arr);
void hoo_int64_array_clear(HooInt64Array arr);
int64_t hoo_int64_array_contains(HooInt64Array arr, int64_t value);
int64_t hoo_int64_array_index_of(HooInt64Array arr, int64_t value);
HooInt64Array hoo_int64_array_concat(HooInt64Array arr1, HooInt64Array arr2);
HooInt64Array hoo_int64_array_slice(HooInt64Array arr, int64_t start, int64_t length);
HooInt64Array hoo_int64_array_clone(HooInt64Array arr);
HooInt64Array hoo_int64_array_retain(HooInt64Array arr);
void hoo_int64_array_release(HooInt64Array arr);
int64_t hoo_int64_array_refcount(HooInt64Array arr);

// double Array Wrappers
typedef HooArray HooDoubleArray;

HooDoubleArray hoo_double_array_new(int64_t capacity);
HooDoubleArray hoo_double_array_from_buffer(const double* data, int64_t length);
HooDoubleArray hoo_double_array_repeat(double value, int64_t count);
int64_t hoo_double_array_length(HooDoubleArray arr);
double hoo_double_array_get(HooDoubleArray arr, int64_t index);
int64_t hoo_double_array_set(HooDoubleArray arr, int64_t index, double value);
int64_t hoo_double_array_push(HooDoubleArray arr, double value);
double hoo_double_array_pop(HooDoubleArray arr);
void hoo_double_array_clear(HooDoubleArray arr);
HooDoubleArray hoo_double_array_concat(HooDoubleArray arr1, HooDoubleArray arr2);
HooDoubleArray hoo_double_array_slice(HooDoubleArray arr, int64_t start, int64_t length);
HooDoubleArray hoo_double_array_clone(HooDoubleArray arr);
HooDoubleArray hoo_double_array_retain(HooDoubleArray arr);
void hoo_double_array_release(HooDoubleArray arr);
int64_t hoo_double_array_refcount(HooDoubleArray arr);

#ifdef __cplusplus
}
#endif
