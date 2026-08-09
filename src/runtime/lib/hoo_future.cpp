#include "hoo_future.h"
#include "hoo_runtime.h"
#include "hoo_exception.h"
#include <string.h>
#include <stdlib.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "hoo_event_loop.h"

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

static int future_value_is_managed(const HooFutureImpl* impl, void* value) {
    if (!value) return 0;
    /* Primitive values are passed through the void* ABI as register bits. */
    if (impl->elem_type_id > 0 && impl->elem_type_id < 100) return 0;
    /* Type 100 is the unknown/object type; verify it before using ARC. */
    return hoo_is_managed_object(value) != 0;
}

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
    while (continuations) {
        HooFutureContinuationNode* next = continuations->next;
        HooFutureContinuation callback = continuations->callback;
        void* callback_arg = continuations->arg;
        free(continuations);
        if (callback) callback(callback_arg);
        /* Each pending continuation owns one reference while queued. */
        hoo_release((HooFuture)impl);
        continuations = next;
    }
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
        impl->error_message = error_message ? strdup(error_message) : NULL;
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
    if (!node) return;
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

static void wait_for_future(HooFutureImpl* impl) {
    std::unique_lock<std::mutex> lock(*impl->mutex);
    while (!impl->ready) {
        lock.unlock();
        hoo_event_loop_run_nowait();
        lock.lock();
        if (!impl->ready) {
            impl->cv->wait_for(lock, std::chrono::milliseconds(16));
        }
    }
}

void* hoo_future_get_value(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    wait_for_future(impl);
    return impl->value;
}

void* _F_hoo_future_await_unwrap_p_p(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    wait_for_future(impl);
    if (impl->error_message) {
        HooException exc = hoo_exception_runtime(impl->error_message);
        hoo_exception_throw(exc); // does not return
    }
    void* result = impl->value;
    if (result && impl->value_is_managed) hoo_retain(result);
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
