#include "hoo_future.h"
#include "hoo_runtime.h"
#include "hoo_exception.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Internal layout of a HooFuture object.
 * Allocated via hoo_alloc so ARC header is prepended automatically.
 */
typedef struct {
    int64_t  elem_type_id;   /* type ID of the promised value          */
    void    *value;           /* resolved value (ARC-managed), or NULL  */
    char    *error_message;  /* non-NULL when resolved with an error   */
    int32_t  ready;          /* 0 = pending, 1 = resolved              */
    HooFutureContinuation continuation; /* callback to resume coroutine  */
    void*    continuation_arg; /* argument for the callback             */
} HooFutureImpl;

static HooFutureImpl* get_impl(HooFuture f) {
    return (HooFutureImpl*)f;
}

/* ------------------------------------------------------------------ */
/* Destructor – registered with the ARC engine                         */
/* ------------------------------------------------------------------ */
static void future_destructor(void* obj) {
    HooFutureImpl* impl = (HooFutureImpl*)obj;
    /* Release the resolved ARC-managed value, if any */
    if (impl->value) {
        hoo_release(impl->value);
        impl->value = NULL;
    }
    /* Free the error string, if any */
    if (impl->error_message) {
        free(impl->error_message);
        impl->error_message = NULL;
    }
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
    impl->error_message = NULL;
    impl->ready        = 0;
    impl->continuation = NULL;
    impl->continuation_arg = NULL;
    return (HooFuture)impl;
}

int64_t hoo_future_get_elem_type_id(HooFuture f) {
    if (!f) return 0;
    return get_impl(f)->elem_type_id;
}

int64_t hoo_future_is_ready(HooFuture f) {
    if (!f) return 0;
    return get_impl(f)->ready ? 1 : 0;
}

int64_t hoo_future_has_error(HooFuture f) {
    if (!f) return 0;
    HooFutureImpl* impl = get_impl(f);
    return (impl->ready && impl->error_message != NULL) ? 1 : 0;
}

static void trigger_continuation(HooFutureImpl* impl) {
    if (impl->continuation) {
        // Run continuation. Note: in a fully async environment, you might
        // post this to the event loop via uv_async_send instead of calling directly,
        // but for coroutines, direct call is often sufficient since the resolver
        // is already running on the event loop.
        impl->continuation(impl->continuation_arg);
        impl->continuation = NULL; // Run only once
    }
}

void hoo_future_set_value(HooFuture f, void* value) {
    if (!f) return;
    HooFutureImpl* impl = get_impl(f);
    if (impl->ready) return;  /* already resolved */
    impl->value = value ? hoo_retain(value) : NULL;
    impl->ready = 1;
    trigger_continuation(impl);
}

void hoo_future_set_error(HooFuture f, const char* error_message) {
    if (!f) return;
    HooFutureImpl* impl = get_impl(f);
    if (impl->ready) return;  /* already resolved */
    impl->error_message = error_message ? strdup(error_message) : NULL;
    impl->ready = 1;
    trigger_continuation(impl);
}

void hoo_future_set_continuation(HooFuture f, HooFutureContinuation callback, void* arg) {
    if (!f || !callback) return;
    HooFutureImpl* impl = get_impl(f);
    if (impl->ready) {
        callback(arg);
    } else {
        impl->continuation = callback;
        impl->continuation_arg = arg;
    }
}

void* hoo_future_get_value(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    /* Spin-wait until resolved (simple polling for now) */
    while (!impl->ready) {
        /* In a real event-loop integration, yield here */
    }
    return impl->value;
}

void* _F_hoo_future_await_unwrap_p_p(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    while (!impl->ready) {
        /* Spin-wait */
    }
    if (impl->error_message) {
        HooException exc = hoo_exception_runtime(impl->error_message);
        hoo_exception_throw(exc); // does not return
    }
    void* result = impl->value;
    if (result) hoo_retain(result);
    return result;
}

const char* hoo_future_get_error(HooFuture f) {
    if (!f) return NULL;
    return get_impl(f)->error_message;
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
