#include "runtime/lib/concurrency/hoo_future.h"
#include "runtime/lib/core/hoo_runtime.h"
#include "runtime/lib/core/hoo_exception.h"
#include <string.h>
#include <stdlib.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "runtime/lib/concurrency/hoo_event_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Internal layout of a HooFuture object.
 * Allocated via hoo_alloc so ARC header is prepended automatically.
 * Synchronisation primitives are heap-allocated separately to keep the struct
 * POD-compatible with hoo_alloc's zero-initialisation.  Each future carries
 * its own mutex+CV pair so concurrent futures do not contend on a global lock.
 */
typedef struct {
    int64_t  elem_type_id;     /* type ID of the promised value          */
    void    *value;             /* resolved value (ARC-managed), or NULL  */
    int64_t  value_is_managed; /* whether value owns an ARC reference    */
    char    *error_message;    /* non-NULL when resolved with an error   */
    int32_t  ready;            /* 0 = pending, 1 = resolved              */
    struct HooFutureContinuationNode* continuations;
    std::mutex              *mutex; /* per-future mutex (heap-allocated) */
    std::condition_variable *cv;    /* per-future condvar                 */
} HooFutureImpl;

typedef struct HooFutureContinuationNode {
    HooFutureContinuation callback;
    void* arg;
    struct HooFutureContinuationNode* next;
} HooFutureContinuationNode;

static HooFutureImpl* get_impl(HooFuture f) {
    return (HooFutureImpl*)f;
}

/*
 * Returns 1 when elem_type_id corresponds to a primitive value type that is
 * passed through the void* ABI as raw register bits (never ARC-managed).
 * All other type IDs are treated as potentially managed objects that require
 * hoo_is_managed_object() verification before ARC operations.
 */
static int is_primitive_type_id(int64_t elem_type_id) {
    switch (elem_type_id) {
        case HOO_TYPE_INT64:
        case HOO_TYPE_FLOAT64:
        case HOO_TYPE_BOOL:
        case HOO_TYPE_VOID:
        case HOO_TYPE_INT8:
        case HOO_TYPE_BYTE:
        case HOO_TYPE_CHAR:
            return 1;
        default:
            return 0;
    }
}

static int future_value_is_managed(const HooFutureImpl* impl, void* value) {
    if (!value) return 0;
    if (is_primitive_type_id(impl->elem_type_id)) return 0;
    return hoo_is_managed_object(value) != 0;
}

/* Snapshot of a resolved future's payload, captured while holding the mutex
 * so readers never race with a concurrent resolution. */
typedef struct {
    void* value;             /* resolved value (ARC-managed), or NULL        */
    int64_t value_is_managed;/* whether value owns an ARC reference          */
    char* error_message;     /* non-NULL when resolved with an error         */
} FutureResolvedState;

/* ------------------------------------------------------------------ */
/* Destructor – registered with the ARC engine                         */
/* ------------------------------------------------------------------ */
static void future_destructor(void* obj) {
    HooFutureImpl* impl = (HooFutureImpl*)obj;

    void* value = NULL;
    int value_is_managed = 0;
    char* error_message = NULL;
    HooFutureContinuationNode* continuations = NULL;
    {
        std::lock_guard<std::mutex> lock(*impl->mutex);
        value = impl->value;
        value_is_managed = impl->value_is_managed;
        error_message = impl->error_message;
        continuations = impl->continuations;
        impl->value = NULL;
        impl->value_is_managed = 0;
        impl->error_message = NULL;
        impl->continuations = NULL;
    }

    while (continuations) {
        HooFutureContinuationNode* next = continuations->next;
        free(continuations);
        continuations = next;
    }

    /* Release the resolved ARC-managed value, if any */
    if (value && value_is_managed) {
        hoo_release(value);
    }
    /* Free the error string, if any */
    if (error_message) {
        free(error_message);
    }

    delete impl->cv;
    delete impl->mutex;
}

#ifdef __cplusplus
namespace {
    struct FutureDestructorRegistrar {
        FutureDestructorRegistrar() {
            hoo_register_destructor(HOO_TYPE_FUTURE, future_destructor);
        }
    } future_registrar;
}
#endif

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

HooFuture hoo_future_new(int64_t elem_type_id) {
    HooFutureImpl* impl = (HooFutureImpl*)hoo_alloc(sizeof(HooFutureImpl), HOO_TYPE_FUTURE);
    impl->elem_type_id = elem_type_id;
    impl->value        = NULL;
    impl->value_is_managed = 0;
    impl->error_message = NULL;
    impl->ready        = 0;
    impl->continuations = NULL;
    impl->mutex = new std::mutex();
    impl->cv    = new std::condition_variable();
    return (HooFuture)impl;
}

int64_t hoo_future_get_elem_type_id(HooFuture f) {
    if (!f) return 0;
    return get_impl(f)->elem_type_id;
}

int64_t hoo_future_is_ready(HooFuture f) {
    if (!f) return 0;
    HooFutureImpl* impl = get_impl(f);
    std::lock_guard<std::mutex> lock(*impl->mutex);
    return impl->ready ? 1 : 0;
}

int64_t hoo_future_has_error(HooFuture f) {
    if (!f) return 0;
    HooFutureImpl* impl = get_impl(f);
    std::lock_guard<std::mutex> lock(*impl->mutex);
    return (impl->ready && impl->error_message != NULL) ? 1 : 0;
}

static void trigger_continuation(HooFutureImpl* impl) {
    HooFutureContinuationNode* continuations = NULL;
    {
        std::lock_guard<std::mutex> lock(*impl->mutex);
        continuations = impl->continuations;
        impl->continuations = NULL;
    }
    if (!continuations) return;
    /*
     * Ownership protocol for continuation drain:
     *
     * Each enqueued node holds one retain on the future (added at enqueue
     * time in set_continuation).  We drain the list under our own temporary
     * retain so the future cannot be freed mid-pass:
     *
     *   1. Take a temporary retain on the future (our "drain" reference).
     *   2. For each node: capture next/callback/arg, free the node, release
     *      the node's enqueue-time retain, then invoke the callback.
     *   3. After the loop, release our drain reference.
     *
     * Between step 2's release and the next iteration, the future may
     * already be at refcount 0 — but we already captured `next` before the
     * release, so the subsequent iteration is safe.  Our drain reference
     * (taken in step 1 and released in step 3) guarantees the memory backing
     * `next` (which was captured while the node was alive) remains valid.
     *
     * NOTE: callbacks must NOT re-enter the future API on the same future
     * (e.g. calling set_value or set_continuation again), as the mutex is
     * not re-entrant and the continuation list has already been moved.
     */
    hoo_retain(impl);
    while (continuations) {
        HooFutureContinuationNode* next = continuations->next;
        HooFutureContinuation callback = continuations->callback;
        void* callback_arg = continuations->arg;
        free(continuations);
        continuations = next;
        hoo_release((HooFuture)impl);
        if (callback) callback(callback_arg);
    }
    /* Final release. This may free the future, so do not touch it after. */
    hoo_release((HooFuture)impl);
}

void hoo_future_set_value(HooFuture f, void* value) {
    if (!f) return;
    HooFutureImpl* impl = get_impl(f);
    {
        std::lock_guard<std::mutex> lock(*impl->mutex);
        if (impl->ready) return;
        impl->value = value;
        impl->value_is_managed = future_value_is_managed(impl, value);
        if (impl->value_is_managed) {
            hoo_retain(value);
        }
        impl->ready = 1;
    }
    impl->cv->notify_all();
    trigger_continuation(impl);
}

void hoo_future_set_error(HooFuture f, const char* error_message) {
    if (!f) return;
    HooFutureImpl* impl = get_impl(f);
    {
        std::lock_guard<std::mutex> lock(*impl->mutex);
        if (impl->ready) return;
        if (error_message) {
            impl->error_message = strdup(error_message);
            /* If strdup fails under OOM, store an empty string so callers
             * still observe a resolved-with-error state. */
            if (!impl->error_message) {
                impl->error_message = strdup("");
            }
        } else {
            impl->error_message = NULL;
        }
        impl->ready = 1;
    }
    impl->cv->notify_all();
    trigger_continuation(impl);
}

void hoo_future_set_continuation(HooFuture f, HooFutureContinuation callback, void* arg) {
    if (!f || !callback) return;
    HooFutureImpl* impl = get_impl(f);
    bool invoke_now = false;
    HooFutureContinuationNode* node = (HooFutureContinuationNode*)malloc(sizeof(HooFutureContinuationNode));
    if (!node) {
        /* OOM: fall back to synchronous invocation if already resolved. */
        std::lock_guard<std::mutex> lock(*impl->mutex);
        if (impl->ready) callback(arg);
        return;
    }
    node->callback = callback;
    node->arg = arg;
    node->next = NULL;
    {
        std::lock_guard<std::mutex> lock(*impl->mutex);
        if (impl->ready) {
            invoke_now = true;
        } else {
            /* Keep the Future alive until this callback has run. */
            hoo_retain(f);
            node->next = impl->continuations;
            impl->continuations = node;
        }
    }
    if (invoke_now) {
        free(node);
        callback(arg);
    }
}

/*
 * Maximum number of event-loop iterations to spin before aborting.
 * At 16 ms per wait_for iteration this gives ~160 seconds before the
 * runtime gives up, which is generous enough for real I/O but will
 * surface stuck futures during development.
 */
#define HOO_FUTURE_WAIT_MAX_ITERATIONS 10000

static void wait_for_future(HooFutureImpl* impl, FutureResolvedState* out) {
    std::unique_lock<std::mutex> lock(*impl->mutex);
    int64_t iterations = 0;
    while (!impl->ready) {
        if (iterations >= HOO_FUTURE_WAIT_MAX_ITERATIONS) {
            /* Future never resolved — abort the wait to avoid an infinite
             * hang.  The future is returned as-is (pending state); callers
             * will observe a NULL value / no error. */
            break;
        }
        lock.unlock();
        hoo_event_loop_run_nowait();
        lock.lock();
        if (!impl->ready) {
            impl->cv->wait_for(lock, std::chrono::milliseconds(16));
        }
        ++iterations;
    }
    /* Resolved fields are immutable once ready; capture them under the lock
     * so readers observe a consistent, race-free snapshot. */
    if (out) {
        out->value = impl->value;
        out->value_is_managed = impl->value_is_managed;
        out->error_message = impl->error_message;
    }
}

void* hoo_future_get_value(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    FutureResolvedState state;
    wait_for_future(impl, &state);
    return state.value;
}

void* hoo_future_await_wait(HooFuture f, int64_t* out_has_error,
                            HooException* out_exception) {
    if (out_has_error) *out_has_error = 0;
    if (out_exception) *out_exception = NULL;
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    FutureResolvedState state;
    wait_for_future(impl, &state);
    if (state.error_message) {
        if (out_has_error) *out_has_error = 1;
        if (out_exception) *out_exception = hoo_exception_runtime(state.error_message);
        return NULL;
    }
    void* result = state.value;
    if (result && state.value_is_managed) hoo_retain(result);
    return result;
}

void* _F_hoo_future_await_unwrap_p_p(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    FutureResolvedState state;
    wait_for_future(impl, &state);
    if (state.error_message) {
        HooException exc = hoo_exception_runtime(state.error_message);
        hoo_exception_throw(exc); // does not return
    }
    void* result = state.value;
    if (result && state.value_is_managed) hoo_retain(result);
    return result;
}

const char* hoo_future_get_error(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    std::lock_guard<std::mutex> lock(*impl->mutex);
    return impl->error_message;
}

HooFuture hoo_future_retain(HooFuture f) {
    return (HooFuture)hoo_retain(f);
}

void hoo_future_release(HooFuture f) {
    hoo_release(f);
}

#ifdef __cplusplus
}
#endif
