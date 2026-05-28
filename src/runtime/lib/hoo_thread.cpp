#include "hoo_thread.h"
#include <cstdlib>
#include <pthread.h>
#include <stdint.h>

extern "C" {

struct HooMutexImpl {
    pthread_mutex_t mutex;
};

int64_t hoo_thread_spawn(int64_t (*func)(void*), void* arg) {
    pthread_t thread;
    int ret = pthread_create(&thread, nullptr,
        reinterpret_cast<void*(*)(void*)>(func), arg);
    if (ret != 0) return -1;
    return reinterpret_cast<int64_t>(thread);
}

int64_t hoo_thread_join(int64_t thread_id) {
    pthread_t thread = reinterpret_cast<pthread_t>(thread_id);
    void* result = nullptr;
    int ret = pthread_join(thread, &result);
    if (ret != 0) return -1;
    return reinterpret_cast<int64_t>(result);
}

int64_t hoo_thread_self(void) {
    return reinterpret_cast<int64_t>(pthread_self());
}

HooMutex hoo_thread_mutex_create(void) {
    HooMutexImpl* impl = new HooMutexImpl();
    pthread_mutex_init(&impl->mutex, nullptr);
    return impl;
}

int64_t hoo_thread_mutex_lock(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
    return pthread_mutex_lock(&impl->mutex) == 0 ? 0 : -1;
}

int64_t hoo_thread_mutex_unlock(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
    return pthread_mutex_unlock(&impl->mutex) == 0 ? 0 : -1;
}

int64_t hoo_thread_mutex_destroy(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
    int ret = pthread_mutex_destroy(&impl->mutex);
    delete impl;
    return ret == 0 ? 0 : -1;
}

} // extern "C"
