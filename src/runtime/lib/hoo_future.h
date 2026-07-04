#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HooFuture - Generic future/promise for async/await support.
 *
 * A Future<T> is a reference-counted placeholder for a value of type T
 * that becomes available at some point in the future.
 *
 * Type IDs:
 *   HOO_TYPE_FUTURE    = 123  (generic future object)
 *   HOO_TYPE_UV_HANDLE = 124  (libuv handle wrapper, reserved for future use)
 */

#define HOO_TYPE_FUTURE     123
#define HOO_TYPE_UV_HANDLE  124

/** Opaque future pointer */
typedef void* HooFuture;

/**
 * Allocate and initialize a new future.
 * @param elem_type_id  Type ID of the promised value (e.g. HOO_TYPE_STRING).
 * @return              New future with refcount=1, in pending state.
 */
HooFuture hoo_future_new(int64_t elem_type_id);

/**
 * Query the element type id stored in the future.
 */
int64_t hoo_future_get_elem_type_id(HooFuture f);

/**
 * Returns 1 when the future is resolved (either with a value or an error),
 * 0 while it is still pending.
 */
int64_t hoo_future_is_ready(HooFuture f);

/**
 * Returns 1 when the future resolved with an error, 0 otherwise.
 * Only meaningful when hoo_future_is_ready() == 1.
 */
int64_t hoo_future_has_error(HooFuture f);

/**
 * Resolve the future with a concrete value.
 * - value must be an ARC-managed pointer (or NULL for void futures).
 * - Calling this more than once on an already-resolved future is a no-op.
 */
void hoo_future_set_value(HooFuture f, void* value);

/**
 * Resolve the future with an error string.
 * - Calling this more than once on an already-resolved future is a no-op.
 */
void hoo_future_set_error(HooFuture f, const char* error_message);

/**
 * Get the resolved value.
 * Blocks (spin-waits) until the future is ready.
 * Returns NULL for void futures or if the future resolved with an error.
 */
void* hoo_future_get_value(HooFuture f);

/**
 * Get the error message if the future resolved with an error.
 * Returns NULL if there is no error.
 * The returned string is owned by the future; do not free it.
 */
const char* hoo_future_get_error(HooFuture f);

/**
 * Runtime helper for await keyword codegen.
 * Blocks (spin-waits) until the future is ready.
 * If the future resolved with an error, throws a Hoo runtime exception.
 * If successful, returns the resolved value (with its refcount incremented).
 */
void* _F_hoo_future_await_unwrap_p_p(HooFuture f);

/**
 * Coroutine continuation callback signature.
 * When the future resolves, this callback is invoked.
 */
typedef void (*HooFutureContinuation)(void* arg);

/**
 * Register a continuation callback to be executed when the future resolves.
 * If the future is already resolved, the callback is executed immediately
 * or scheduled on the event loop.
 */
void hoo_future_set_continuation(HooFuture f, HooFutureContinuation callback, void* arg);

/**
 * Retain (increment reference count).
 */
HooFuture hoo_future_retain(HooFuture f);

/**
 * Release (decrement reference count; frees when it reaches zero).
 */
void hoo_future_release(HooFuture f);

#ifdef __cplusplus
}
#endif
