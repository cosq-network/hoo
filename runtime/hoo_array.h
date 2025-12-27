#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooArray - Dynamic Array Type with Reference Counting
// ============================================================================
//
// Opaque handles representing typed array values:
// - HooInt64Array: Dynamic array of int64 values
// - HooDoubleArray: Dynamic array of double values
//
// Internally managed with automatic reference counting.
//

typedef void* HooInt64Array;
typedef void* HooDoubleArray;

// ============================================================================
// Int64 Array - Creation and Destruction
// ============================================================================

/**
 * Create a new empty int64 array with initial capacity
 *
 * The returned HooInt64Array has refcount=1. Caller is responsible for
 * releasing when no longer needed.
 *
 * @param capacity Initial capacity (number of elements)
 * @return New HooInt64Array with refcount=1, or NULL on allocation failure
 */
HooInt64Array hoo_int64_array_new(int64_t capacity);

/**
 * Create an int64 array from a buffer
 *
 * @param data Pointer to int64 data (may be NULL)
 * @param length Number of elements to copy
 * @return New HooInt64Array with refcount=1
 */
HooInt64Array hoo_int64_array_from_buffer(const int64_t* data, int64_t length);

/**
 * Create an int64 array with a repeated value
 *
 * @param value Value to repeat
 * @param count Number of times to repeat
 * @return New HooInt64Array containing the repeated value
 */
HooInt64Array hoo_int64_array_repeat(int64_t value, int64_t count);

// ============================================================================
// Int64 Array - Basic Operations
// ============================================================================

/**
 * Get the number of elements in an int64 array
 *
 * @param arr Array (may be NULL)
 * @return Number of elements, or 0 if arr is NULL
 */
int64_t hoo_int64_array_length(HooInt64Array arr);

/**
 * Get an element from an int64 array by index
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @return Element value, or 0 if index out of bounds
 */
int64_t hoo_int64_array_get(HooInt64Array arr, int64_t index);

/**
 * Set an element in an int64 array by index
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @param value Value to set
 * @return 1 if successful, 0 if index out of bounds
 */
int64_t hoo_int64_array_set(HooInt64Array arr, int64_t index, int64_t value);

/**
 * Append an element to an int64 array
 *
 * Automatically grows the array if needed.
 *
 * @param arr Array
 * @param value Value to append
 * @return 1 on success, 0 on allocation failure
 */
int64_t hoo_int64_array_push(HooInt64Array arr, int64_t value);

/**
 * Remove and return the last element from an int64 array
 *
 * @param arr Array
 * @return Last element value, or 0 if array is empty
 */
int64_t hoo_int64_array_pop(HooInt64Array arr);

/**
 * Clear all elements from an int64 array
 *
 * @param arr Array
 */
void hoo_int64_array_clear(HooInt64Array arr);

/**
 * Check if an int64 array contains a value
 *
 * @param arr Array
 * @param value Value to search for
 * @return 1 if found, 0 if not found
 */
int64_t hoo_int64_array_contains(HooInt64Array arr, int64_t value);

/**
 * Find the index of a value in an int64 array
 *
 * @param arr Array
 * @param value Value to search for
 * @return Index of first occurrence, or -1 if not found
 */
int64_t hoo_int64_array_index_of(HooInt64Array arr, int64_t value);

/**
 * Concatenate two int64 arrays
 *
 * Returns a new array containing all elements from arr1 followed by all
 * elements from arr2. Neither input array is modified.
 *
 * @param arr1 First array (may be NULL, treated as empty)
 * @param arr2 Second array (may be NULL, treated as empty)
 * @return New concatenated HooInt64Array with refcount=1
 */
HooInt64Array hoo_int64_array_concat(HooInt64Array arr1, HooInt64Array arr2);

/**
 * Get a slice of an int64 array
 *
 * @param arr Source array
 * @param start Starting index (0-based)
 * @param length Number of elements to extract
 * @return New HooInt64Array containing the slice
 */
HooInt64Array hoo_int64_array_slice(HooInt64Array arr, int64_t start, int64_t length);

/**
 * Create a clone of an int64 array
 *
 * @param arr Source array
 * @return New HooInt64Array with refcount=1, or NULL if arr is NULL
 */
HooInt64Array hoo_int64_array_clone(HooInt64Array arr);

// ============================================================================
// Int64 Array - Reference Counting
// ============================================================================

/**
 * Increment reference count of an int64 array
 *
 * @param arr Array (may be NULL)
 * @return The array (for chaining)
 */
HooInt64Array hoo_int64_array_retain(HooInt64Array arr);

/**
 * Decrement reference count of an int64 array
 *
 * If refcount reaches 0, the array is freed.
 *
 * @param arr Array (may be NULL)
 */
void hoo_int64_array_release(HooInt64Array arr);

/**
 * Get the reference count of an int64 array
 *
 * @param arr Array (may be NULL)
 * @return Current refcount, or 0 if arr is NULL
 */
int64_t hoo_int64_array_refcount(HooInt64Array arr);

// ============================================================================
// Double Array - Creation and Destruction
// ============================================================================

/**
 * Create a new empty double array with initial capacity
 *
 * The returned HooDoubleArray has refcount=1. Caller is responsible for
 * releasing when no longer needed.
 *
 * @param capacity Initial capacity (number of elements)
 * @return New HooDoubleArray with refcount=1, or NULL on allocation failure
 */
HooDoubleArray hoo_double_array_new(int64_t capacity);

/**
 * Create a double array from a buffer
 *
 * @param data Pointer to double data (may be NULL)
 * @param length Number of elements to copy
 * @return New HooDoubleArray with refcount=1
 */
HooDoubleArray hoo_double_array_from_buffer(const double* data, int64_t length);

/**
 * Create a double array with a repeated value
 *
 * @param value Value to repeat
 * @param count Number of times to repeat
 * @return New HooDoubleArray containing the repeated value
 */
HooDoubleArray hoo_double_array_repeat(double value, int64_t count);

// ============================================================================
// Double Array - Basic Operations
// ============================================================================

/**
 * Get the number of elements in a double array
 *
 * @param arr Array (may be NULL)
 * @return Number of elements, or 0 if arr is NULL
 */
int64_t hoo_double_array_length(HooDoubleArray arr);

/**
 * Get an element from a double array by index
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @return Element value, or 0.0 if index out of bounds
 */
double hoo_double_array_get(HooDoubleArray arr, int64_t index);

/**
 * Set an element in a double array by index
 *
 * @param arr Array
 * @param index Element index (0-based)
 * @param value Value to set
 * @return 1 if successful, 0 if index out of bounds
 */
int64_t hoo_double_array_set(HooDoubleArray arr, int64_t index, double value);

/**
 * Append an element to a double array
 *
 * Automatically grows the array if needed.
 *
 * @param arr Array
 * @param value Value to append
 * @return 1 on success, 0 on allocation failure
 */
int64_t hoo_double_array_push(HooDoubleArray arr, double value);

/**
 * Remove and return the last element from a double array
 *
 * @param arr Array
 * @return Last element value, or 0.0 if array is empty
 */
double hoo_double_array_pop(HooDoubleArray arr);

/**
 * Clear all elements from a double array
 *
 * @param arr Array
 */
void hoo_double_array_clear(HooDoubleArray arr);

/**
 * Concatenate two double arrays
 *
 * Returns a new array containing all elements from arr1 followed by all
 * elements from arr2. Neither input array is modified.
 *
 * @param arr1 First array (may be NULL, treated as empty)
 * @param arr2 Second array (may be NULL, treated as empty)
 * @return New concatenated HooDoubleArray with refcount=1
 */
HooDoubleArray hoo_double_array_concat(HooDoubleArray arr1, HooDoubleArray arr2);

/**
 * Get a slice of a double array
 *
 * @param arr Source array
 * @param start Starting index (0-based)
 * @param length Number of elements to extract
 * @return New HooDoubleArray containing the slice
 */
HooDoubleArray hoo_double_array_slice(HooDoubleArray arr, int64_t start, int64_t length);

/**
 * Create a clone of a double array
 *
 * @param arr Source array
 * @return New HooDoubleArray with refcount=1, or NULL if arr is NULL
 */
HooDoubleArray hoo_double_array_clone(HooDoubleArray arr);

// ============================================================================
// Double Array - Reference Counting
// ============================================================================

/**
 * Increment reference count of a double array
 *
 * @param arr Array (may be NULL)
 * @return The array (for chaining)
 */
HooDoubleArray hoo_double_array_retain(HooDoubleArray arr);

/**
 * Decrement reference count of a double array
 *
 * If refcount reaches 0, the array is freed.
 *
 * @param arr Array (may be NULL)
 */
void hoo_double_array_release(HooDoubleArray arr);

/**
 * Get the reference count of a double array
 *
 * @param arr Array (may be NULL)
 * @return Current refcount, or 0 if arr is NULL
 */
int64_t hoo_double_array_refcount(HooDoubleArray arr);

#ifdef __cplusplus
}
#endif
