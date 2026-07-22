#include "hoo_thread.h"
#include "hoo_runtime.h"
#include <uv.h>
#include <stdlib.h>

struct ThreadStart {
    int64_t (*func)(void*);
    void* arg;
    int64_t result;
};

struct ThreadContext {
    uv_thread_t thread;
    ThreadStart ts;
};

static void uv_thread_wrapper(void* arg) {
    ThreadStart* ts = (ThreadStart*)arg;
    ts->result = ts->func(ts->arg);
    hoo_tlab_reset_thread_cache();
}

extern "C" {

int64_t hoo_thread_spawn(int64_t (*func)(void*), void* arg) {
    ThreadContext* ctx = (ThreadContext*)malloc(sizeof(ThreadContext));
    if (!ctx) return -1;
    ctx->ts.func = func;
    ctx->ts.arg = arg;
    ctx->ts.result = 0;
    
    int ret = uv_thread_create(&ctx->thread, uv_thread_wrapper, &ctx->ts);
    if (ret != 0) {
        free(ctx);
        return -1;
    }
    
    return (int64_t)ctx;
}

int64_t hoo_thread_join(int64_t thread_id) {
    if (thread_id == 0) return -1;
    ThreadContext* ctx = (ThreadContext*)thread_id;
    int ret = uv_thread_join(&ctx->thread);
    if (ret != 0) return -1;
    int64_t result = ctx->ts.result;
    free(ctx);
    return result;
}

int64_t hoo_thread_self(void) {
    return (int64_t)uv_thread_self();
}

HooMutex hoo_thread_mutex_create(void) {
    uv_mutex_t* mutex = (uv_mutex_t*)malloc(sizeof(uv_mutex_t));
    if (!mutex) return NULL;
    if (uv_mutex_init(mutex) != 0) {
        free(mutex);
        return NULL;
    }
    return (HooMutex)mutex;
}

int64_t hoo_thread_mutex_lock(HooMutex mutex) {
    if (!mutex) return -1;
    uv_mutex_lock((uv_mutex_t*)mutex);
    return 0;
}

int64_t hoo_thread_mutex_unlock(HooMutex mutex) {
    if (!mutex) return -1;
    uv_mutex_unlock((uv_mutex_t*)mutex);
    return 0;
}

int64_t hoo_thread_mutex_destroy(HooMutex mutex) {
    if (!mutex) return -1;
    uv_mutex_destroy((uv_mutex_t*)mutex);
    free(mutex);
    return 0;
}

} // extern "C"
