#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooArray - Generic Dynamic Array (Hardware-Ready)
// ============================================================================
//
// A type-agnostic dynamic array backed by contiguous 64-bit slots managed
// through hoo_alloc/hoo_realloc with ARC (Automatic Reference Counting).
//
// Each element occupies exactly one int64_t slot regardless of actual type.
// Scalars are stored by value; pointers are stored as raw bit patterns.
//
// The array's element type is set on the first typed push (push_int64,
// push_double) and can also be written directly into the header for
// managed-element arrays (element_type >= 100) to enable automatic
// release on destruction.
//
// Thread safety: a HooArray is NOT internally synchronised.  Concurrent
// access from multiple threads must be externally serialised by the caller.
//

typedef void* HooArray;  // Opaque handle to raw memory layout

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a new empty generic array
 * @return New HooArray with refcount=1, or NULL on allocation failure
 */
HooArray hoo_array_new(void);

/**
 * Create a generic array from a buffer
 * @param data Pointer to array data
 * @param length Number of elements
 * @return New HooArray with refcount=1
 */
HooArray hoo_array_from_buffer(const void* data, int64_t length);

/**
 * Create a generic array with repeated value
 * @param value Pointer to value to repeat
 * @param count Number of repetitions
 * @return New HooArray
 */
HooArray hoo_array_repeat(const void* value, int64_t count);

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Get number of elements in array
 * @param arr Array (may be NULL)
 * @return Number of elements, or 0 if NULL
 */
int64_t hoo_array_length(HooArray arr);

/**
 * Get element by index via pointer
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination buffer
 * @return 1 if success, 0 if out of bounds
 */
int64_t hoo_array_get(HooArray arr, int64_t index, void* dest);

/**
 * Set element by index via pointer.
 * For managed-element arrays (element_type >= 100), this transfers ownership:
 * the array releases its old reference and takes ownership of the new value
 * without retaining it.  The caller must ensure the new value outlives the
 * array, or explicitly retain it before calling set.
 * @param arr Array
 * @param index Index (0-based)
 * @param value Pointer to value
 * @return 1 if success, 0 if out of bounds
 */
int64_t hoo_array_set(HooArray arr, int64_t index, const void* value);

/**
 * Add element to end of array
 * @param arr Array handle
 * @param value Pointer to value
 * @return The array handle (possibly a new handle if reallocation occurred),
 *         or NULL on failure. The returned handle must be used for all
 *         subsequent operations; the original handle may be invalid after growth.
 */
HooArray hoo_array_push(HooArray arr, const void* value);

/**
 * Push a value and update the handle in-place.
 * Safe for variables that may be invalidated by reallocation.
 * @param arr_ptr Pointer to array handle (will be updated if reallocation occurs)
 * @param value Pointer to value
 * @return 1 on success, 0 on failure
 */
int64_t hoo_array_push_h(HooArray* arr_ptr, const void* value);

/**
 * Remove and return last element.
 * For managed-element arrays (element_type >= 100), the array releases its
 * reference and transfers ownership of the popped value to the caller.
 * @param arr Array
 * @param dest Destination buffer
 * @return 1 if success, 0 if empty
 */
int64_t hoo_array_pop(HooArray arr, void* dest);

/**
 * Remove all elements
 * @param arr Array
 */
void hoo_array_clear(HooArray arr);

/**
 * Check if array is empty
 * @param arr Array (may be NULL)
 * @return 1 if empty or NULL, 0 otherwise
 */
int64_t hoo_array_empty(HooArray arr);

// ============================================================================
// Type-Specific Push Operations
// ============================================================================

/**
 * Push int64 value
 * @param arr Array handle
 * @param value int64 value
 * @return The array handle (possibly new if reallocation occurred), or NULL on failure
 */
HooArray hoo_array_push_int64(HooArray arr, int64_t value);

/**
 * Push double value
 * @param arr Array handle
 * @param value double value
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_double(HooArray arr, double value);

/**
 * Push float value
 * @param arr Array handle
 * @param value float value
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_float(HooArray arr, float value);

/**
 * Push bool value
 * @param arr Array handle
 * @param value bool value
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_bool(HooArray arr, int64_t value);

/**
 * Push char value
 * @param arr Array handle
 * @param value char value
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_char(HooArray arr, char value);

/**
 * Push string pointer.
 * Unowning: the array stores the raw pointer without retaining it.
 * The caller retains ownership and must ensure the string outlives the array.
 * @param arr Array handle
 * @param value Pointer to string
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_string(HooArray arr, const char* value);

/**
 * Push object pointer (class instance).
 * Unowning: the array stores the raw pointer without retaining it.
 * The caller retains ownership and must ensure the object outlives the array,
 * or retain the object before pushing to transfer ownership to the array.
 * @param arr Array handle
 * @param value Pointer to object
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_object(HooArray arr, void* value);

/**
 * Push array (for multi-dimensional arrays).
 * Retaining: the array calls hoo_retain on the child array, sharing ownership.
 * The child array is automatically released when the parent is cleared or
 * destroyed.
 * @param arr Array handle
 * @param value HooArray handle
 * @return The array handle (possibly new), or NULL on failure
 */
HooArray hoo_array_push_array(HooArray arr, HooArray value);

/// Push a batch of int64 values using SIMD-friendly vector operation
/// @param arr Array handle (must be homogeneous int64)
/// @param src Pointer to contiguous int64 values
/// @param count Number of elements (multiple of active vector length)
/// @return The array handle (possibly new after reallocation) or nullptr on failure
HooArray hoo_array_push_vector_int64(HooArray arr, const int64_t* src, int64_t count);

// ============================================================================
// Type-Specific Get Operations
// ============================================================================

/**
 * Get int64 value from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_int64(HooArray arr, int64_t index, int64_t* dest);

/**
 * Get double value from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_double(HooArray arr, int64_t index, double* dest);

/**
 * Get float value from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_float(HooArray arr, int64_t index, float* dest);

/**
 * Get bool value from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_bool(HooArray arr, int64_t index, int64_t* dest);

/**
 * Get char value from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_char(HooArray arr, int64_t index, char* dest);

/**
 * Get string pointer from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer to string pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_string(HooArray arr, int64_t index, const char** dest);

/**
 * Get object pointer from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer to object pointer
 * @return 1 if success, 0 if index out of bounds or type mismatch
 */
int64_t hoo_array_get_object(HooArray arr, int64_t index, void** dest);

/**
 * Get nested array from array
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination pointer to HooArray handle
 * @return 1 if success, 0 if index out of bounds or not an array
 */
int64_t hoo_array_get_array(HooArray arr, int64_t index, HooArray* dest);

// ============================================================================
// Ordering Operations
// ============================================================================

/**
 * Sort array elements in-place.
 * For int64 element arrays: uses numeric int64 comparison.
 * For double element arrays: uses IEEE 754 double comparison.
 * For all other types (bool, char, string, object): uses bitwise integer
 * comparison of the 64-bit storage slots (pointer ordering for strings/objects).
 * @param arr Array handle
 * @return The array handle (same as input)
 */
HooArray hoo_array_sort(HooArray arr);

/**
 * Reverse array elements in-place.
 * @param arr Array handle
 * @return The array handle (same as input)
 */
HooArray hoo_array_reverse(HooArray arr);

/**
 * Shuffle array elements in-place randomly.
 * @param arr Array handle
 * @return The array handle (same as input)
 */
HooArray hoo_array_shuffle(HooArray arr);

/**
 * Sort a sub-range of array elements in-place.
 * @param arr Array handle
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @return The array handle (same as input)
 */
HooArray hoo_array_sort_range(HooArray arr, int64_t start, int64_t end);

/**
 * Binary search for int64 value. Array must be sorted.
 * @param arr Array handle
 * @param value Value to search for
 * @return Index if found, -1 if not found
 */
int64_t hoo_array_binary_search_int64(HooArray arr, int64_t value);

/**
 * Binary search for double value. Array must be sorted.
 * @param arr Array handle
 * @param value Value to search for
 * @return Index if found, -1 if not found
 */
int64_t hoo_array_binary_search_double(HooArray arr, double value);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count
 * @param arr Array
 * @return Array handle (same as input)
 */
HooArray hoo_array_retain(HooArray arr);

/**
 * Decrement reference count, free if zero
 * @param arr Array
 */
void hoo_array_release(HooArray arr);

/**
 * Get reference count
 * @param arr Array
 * @return Current refcount, or 0 if NULL
 */
int64_t hoo_array_refcount(HooArray arr);

// ============================================================================
// Type Information
// ============================================================================

/**
 * Get element type name
 * @param arr Array
 * @return Type name string
 */
const char* hoo_array_element_type(HooArray arr);

/**
 * Check if array contains specific type
 * @param arr Array
 * @param type_name Type name to check
 * @return 1 if matches, 0 otherwise
 */
int64_t hoo_array_is_type(HooArray arr, const char* type_name);

#ifdef __cplusplus
}  // extern "C"
#endif
