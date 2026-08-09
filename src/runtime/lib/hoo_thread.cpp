#include "hoo_thread.h"
#include "hoo_runtime.h"
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

int64_t hoo_thread_mutex_try_lock(HooMutex mutex) {
    if (!mutex) return -1;
    return uv_mutex_trylock((uv_mutex_t*)mutex) == 0 ? 0 : 1;
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
    uv_cond_wait((uv_cond_t*)condition, (uv_mutex_t*)mutex);
    return 0;
}

int64_t hoo_thread_condition_timed_wait(HooCondition condition, HooMutex mutex, uint64_t timeout_ns) {
    if (!condition || !mutex) return -1;
    return uv_cond_timedwait((uv_cond_t*)condition, (uv_mutex_t*)mutex, timeout_ns);
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
