#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooMutex;

int64_t   hoo_thread_spawn(int64_t (*func)(void*), void* arg);
int64_t   hoo_thread_join(int64_t thread_id);
int64_t   hoo_thread_self(void);
HooMutex  hoo_thread_mutex_create(void);
int64_t   hoo_thread_mutex_lock(HooMutex mutex);
int64_t   hoo_thread_mutex_unlock(HooMutex mutex);
int64_t   hoo_thread_mutex_destroy(HooMutex mutex);

#ifdef __cplusplus
}
#endif
