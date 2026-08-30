#include "runtime/lib/thread/hoo_thread.h"
#include "runtime/lib/runtime/hoo_runtime.h"
#include <uv.h>
#include <stdlib.h>
#include <stdint.h>

struct ThreadStart {
    int64_t (*func)(void*);
    void* arg;
    int64_t result;
};

struct ThreadContext {
    uv_thread_t thread;
    ThreadStart ts;
};

// On Windows libuv backs `uv_mutex_t` with an SRW lock, which lets the same
// thread re-acquire a lock it already holds; `uv_mutex_trylock` therefore
// reports "acquired" for a same-thread re-entry, unlike POSIX non-recursive
// mutexes. Track the owner so `hoo_thread_mutex_try_lock` can report "busy"
// for the calling thread's own held lock, giving POSIX-consistent semantics.
struct MutexImpl {
    uv_mutex_t lock;
#ifdef _WIN32
    uv_thread_t owner;
    bool owned;
#endif
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
    MutexImpl* impl = (MutexImpl*)malloc(sizeof(MutexImpl));
    if (!impl) return NULL;
    if (uv_mutex_init(&impl->lock) != 0) {
        free(impl);
        return NULL;
    }
#ifdef _WIN32
    impl->owned = false;
    impl->owner = (uv_thread_t)0;
#endif
    return (HooMutex)impl;
}

int64_t hoo_thread_mutex_lock(HooMutex mutex) {
    if (!mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
    uv_mutex_lock(&impl->lock);
#ifdef _WIN32
    impl->owner = uv_thread_self();
    impl->owned = true;
#endif
    return 0;
}

int64_t hoo_thread_mutex_try_lock(HooMutex mutex) {
    if (!mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
#ifdef _WIN32
    const uv_thread_t self = uv_thread_self();
    if (impl->owned && uv_thread_equal(&impl->owner, &self)) {
        return 1;
    }
    if (uv_mutex_trylock(&impl->lock) != 0) return 1;
    impl->owner = self;
    impl->owned = true;
    return 0;
#else
    return uv_mutex_trylock(&impl->lock) == 0 ? 0 : 1;
#endif
}

int64_t hoo_thread_mutex_unlock(HooMutex mutex) {
    if (!mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
#ifdef _WIN32
    impl->owned = false;
    impl->owner = (uv_thread_t)0;
#endif
    uv_mutex_unlock(&impl->lock);
    return 0;
}

int64_t hoo_thread_mutex_destroy(HooMutex mutex) {
    if (!mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
    uv_mutex_destroy(&impl->lock);
    free(impl);
    return 0;
}

HooCondition hoo_thread_condition_create(void) {
    uv_cond_t* condition = (uv_cond_t*)malloc(sizeof(uv_cond_t));
    if (!condition) return NULL;
    if (uv_cond_init(condition) != 0) {
        free(condition);
        return NULL;
    }
    return (HooCondition)condition;
}

int64_t hoo_thread_condition_wait(HooCondition condition, HooMutex mutex) {
    if (!condition || !mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
#ifdef _WIN32
    impl->owned = false;
    impl->owner = (uv_thread_t)0;
#endif
    uv_cond_wait((uv_cond_t*)condition, &impl->lock);
#ifdef _WIN32
    impl->owner = uv_thread_self();
    impl->owned = true;
#endif
    return 0;
}

int64_t hoo_thread_condition_timed_wait(HooCondition condition, HooMutex mutex, uint64_t timeout_ns) {
    if (!condition || !mutex) return -1;
    MutexImpl* impl = (MutexImpl*)mutex;
#ifdef _WIN32
    impl->owned = false;
    impl->owner = (uv_thread_t)0;
#endif
    int r = uv_cond_timedwait((uv_cond_t*)condition, &impl->lock, timeout_ns);
#ifdef _WIN32
    impl->owner = uv_thread_self();
    impl->owned = true;
#endif
    return r;
}

int64_t hoo_thread_condition_notify_one(HooCondition condition) {
    if (!condition) return -1;
    uv_cond_signal((uv_cond_t*)condition);
    return 0;
}

int64_t hoo_thread_condition_notify_all(HooCondition condition) {
    if (!condition) return -1;
    uv_cond_broadcast((uv_cond_t*)condition);
    return 0;
}

int64_t hoo_thread_condition_destroy(HooCondition condition) {
    if (!condition) return -1;
    uv_cond_destroy((uv_cond_t*)condition);
    free(condition);
    return 0;
}

HooSemaphore hoo_thread_semaphore_create(uint32_t initial_value) {
    uv_sem_t* semaphore = (uv_sem_t*)malloc(sizeof(uv_sem_t));
    if (!semaphore) return NULL;
    if (uv_sem_init(semaphore, initial_value) != 0) {
        free(semaphore);
        return NULL;
    }
    return (HooSemaphore)semaphore;
}

int64_t hoo_thread_semaphore_wait(HooSemaphore semaphore) {
    if (!semaphore) return -1;
    uv_sem_wait((uv_sem_t*)semaphore);
    return 0;
}

int64_t hoo_thread_semaphore_try_wait(HooSemaphore semaphore) {
    if (!semaphore) return -1;
    return uv_sem_trywait((uv_sem_t*)semaphore) == 0 ? 0 : 1;
}

int64_t hoo_thread_semaphore_post(HooSemaphore semaphore) {
    if (!semaphore) return -1;
    uv_sem_post((uv_sem_t*)semaphore);
    return 0;
}

int64_t hoo_thread_semaphore_destroy(HooSemaphore semaphore) {
    if (!semaphore) return -1;
    uv_sem_destroy((uv_sem_t*)semaphore);
    free(semaphore);
    return 0;
}

} // extern "C"
