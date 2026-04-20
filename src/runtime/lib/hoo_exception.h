#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooException - Exception Type with Reference Counting
// ============================================================================
//
// Opaque handle representing a hoo exception value.
// Used for try...catch...finally exception handling.
// All exceptions carry a message and optional cause.
//
// Supported exception types:
//   - RuntimeException: General runtime errors
//   - NullPointerException: Null reference accessed
//   - IndexOutOfBoundsException: Array/string index out of range
//   - DivisionByZeroException: Division or modulo by zero
//   - InvalidCastException: Type casting failed
//   - CustomException: User-defined exceptions

typedef void* HooException;

// ============================================================================
// Exception Type IDs
// ============================================================================

#define HOO_EXCEPTION_RUNTIME         0
#define HOO_EXCEPTION_NULL_POINTER   1
#define HOO_EXCEPTION_INDEX_OUT_OF_BOUNDS 2
#define HOO_EXCEPTION_DIVISION_BY_ZERO 3
#define HOO_EXCEPTION_INVALID_CAST   4
#define HOO_EXCEPTION_CUSTOM        99

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create an exception with a message
 *
 * The returned HooException has refcount=1. Caller is responsible for
 * releasing when no longer needed.
 *
 * @param typeId Exception type ID (HOO_EXCEPTION_*)
 * @param message Error message (may be NULL, treated as empty)
 * @return New HooException with refcount=1
 */
HooException hoo_exception_create(int64_t typeId, const char* message);

/**
 * Create an exception with a message and cause
 *
 * The cause is retained by the exception.
 *
 * @param typeId Exception type ID
 * @param message Error message (may be NULL)
 * @param cause Nested exception (may be NULL)
 * @return New HooException with refcount=1
 */
HooException hoo_exception_create_with_cause(int64_t typeId, const char* message, HooException cause);

/**
 * Create a RuntimeException
 *
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_runtime(const char* message);

/**
 * Create a NullPointerException
 *
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_null_pointer(const char* message);

/**
 * Create an IndexOutOfBoundsException
 *
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_index_out_of_bounds(const char* message);

/**
 * Create a DivisionByZeroException
 *
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_division_by_zero(const char* message);

/**
 * Create an InvalidCastException
 *
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_invalid_cast(const char* message);

/**
 * Create a custom exception
 *
 * @param exceptionType Custom exception type name
 * @param message Error message (may be NULL)
 * @return New HooException
 */
HooException hoo_exception_custom(const char* exceptionType, const char* message);

// ============================================================================
// Exception Properties
// ============================================================================

/**
 * Get exception type ID
 *
 * @param exc Exception (may be NULL, returns 0)
 * @return Exception type ID
 */
int64_t hoo_exception_get_type_id(HooException exc);

/**
 * Get exception type name
 *
 * @param exc Exception (may be NULL, returns empty string)
 * @return Exception type name string
 */
const char* hoo_exception_get_type_name(HooException exc);

/**
 * Get exception message
 *
 * @param exc Exception (may be NULL, returns empty string)
 * @return Exception message
 */
const char* hoo_exception_get_message(HooException exc);

/**
 * Check if exception has a cause
 *
 * @param exc Exception (may be NULL, returns 0)
 * @return 1 if has cause, 0 otherwise
 */
int64_t hoo_exception_has_cause(HooException exc);

/**
 * Get the cause exception
 *
 * @param exc Exception (may be NULL)
 * @return Cause exception, or NULL if none
 */
HooException hoo_exception_get_cause(HooException exc);

/**
 * Get print stack trace
 *
 * Returns a string representation of the exception including
 * all chained causes.
 *
 * @param exc Exception (may be NULL)
 * @return Print stack string (must be released by caller)
 */
const char* hoo_exception_get_stack_trace(HooException exc);

/**
 * Get number of stack frames in trace
 *
 * @param exc Exception (may be NULL, returns 0)
 * @return Number of stack frames
 */
int64_t hoo_exception_get_frame_count(HooException exc);

/**
 * Get stack frame at index
 *
 * @param exc Exception (may be NULL)
 * @param index Frame index (0-based)
 * @return Stack frame string (must be released by caller), or NULL if invalid
 */
const char* hoo_exception_get_frame(HooException exc, int64_t index);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count
 *
 * Used when assigning an exception to a new variable.
 * Every hoo_exception_retain() should be paired with hoo_exception_release().
 *
 * @param exc Exception to retain (may be NULL)
 * @return The same exception (for chaining)
 */
HooException hoo_exception_retain(HooException exc);

/**
 * Decrement reference count and free if reaches zero
 *
 * Should be called when an exception variable goes out of scope.
 *
 * @param exc Exception to release (may be NULL)
 */
void hoo_exception_release(HooException exc);

/**
 * Get current reference count (for debugging)
 *
 * @param exc Exception to check (may be NULL)
 * @return Current refcount, or 0 if exc is NULL
 */
int64_t hoo_exception_refcount(HooException exc);

// ============================================================================
// Runtime Throwing
// ============================================================================

/**
 * Throw an exception (initiates stack unwinding)
 *
 * This function does not return - it triggers C++ exception
 * handling which will propagate to the nearest catch block.
 *
 * @param exc Exception to throw (must not be NULL)
 */
void hoo_exception_throw(HooException exc);

/**
 * Catch and retrieve the current exception
 *
 * Used in catch blocks to retrieve the exception being handled.
 * Returns NULL if no exception is currently being processed.
 *
 * @return Current exception, or NULL
 */
HooException hoo_exception_current(void);

/**
 * Clear the current exception
 *
 * Called after handling an exception to clear the current exception.
 * Decrements refcount of the caught exception.
 */
void hoo_exception_clear(void);

// ============================================================================
// Comparison
// ============================================================================

/**
 * Check if two exceptions are equal
 *
 * Two exceptions are equal if they have the same type ID and message.
 *
 * @param a First exception
 * @param b Second exception
 * @return 1 if equal, 0 otherwise
 */
int64_t hoo_exception_equals(HooException a, HooException b);

// ============================================================================
// Debugging
// ============================================================================

/**
 * Print exception to stderr
 *
 * Outputs exception details including type, message, and stack trace.
 *
 * @param exc Exception to print (may be NULL, prints <null>)
 */
void hoo_exception_print(HooException exc);

/**
 * Print exception to stderr with newline
 *
 * @param exc Exception to print (may be NULL)
 */
void hoo_exception_println(HooException exc);

/**
 * Get debug representation of exception
 *
 * Returns a new string showing the exception details.
 *
 * @param exc Exception to inspect
 * @return New debug string (must be released by caller)
 */
const char* hoo_exception_debug(HooException exc);

#ifdef __cplusplus
}
#endif