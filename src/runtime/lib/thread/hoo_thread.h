#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HooMutex;
typedef void* HooCondition;
typedef void* HooSemaphore;

int64_t   hoo_thread_spawn(int64_t (*func)(void*), void* arg);
int64_t   hoo_thread_join(int64_t thread_id);
int64_t   hoo_thread_self(void);
HooMutex  hoo_thread_mutex_create(void);
int64_t   hoo_thread_mutex_lock(HooMutex mutex);
int64_t   hoo_thread_mutex_try_lock(HooMutex mutex);
int64_t   hoo_thread_mutex_unlock(HooMutex mutex);
int64_t   hoo_thread_mutex_destroy(HooMutex mutex);

HooCondition hoo_thread_condition_create(void);
int64_t      hoo_thread_condition_wait(HooCondition condition, HooMutex mutex);
int64_t      hoo_thread_condition_timed_wait(HooCondition condition, HooMutex mutex, uint64_t timeout_ns);
int64_t      hoo_thread_condition_notify_one(HooCondition condition);
int64_t      hoo_thread_condition_notify_all(HooCondition condition);
int64_t      hoo_thread_condition_destroy(HooCondition condition);

HooSemaphore hoo_thread_semaphore_create(uint32_t initial_value);
int64_t      hoo_thread_semaphore_wait(HooSemaphore semaphore);
int64_t      hoo_thread_semaphore_try_wait(HooSemaphore semaphore);
int64_t      hoo_thread_semaphore_post(HooSemaphore semaphore);
int64_t      hoo_thread_semaphore_destroy(HooSemaphore semaphore);

#ifdef __cplusplus
}

namespace hoo::thread {

// ScopedLock is intentionally non-copyable so one mutex acquisition has one
// clear owner. The C ABI remains available for generated Hoo code.
class ScopedLock {
public:
    explicit ScopedLock(HooMutex mutex) : mutex_(mutex), owns_(mutex && hoo_thread_mutex_lock(mutex) == 0) {}
    ~ScopedLock() { release(); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

    ScopedLock(ScopedLock&& other) noexcept : mutex_(other.mutex_), owns_(other.owns_) {
        other.mutex_ = nullptr;
        other.owns_ = false;
    }

    ScopedLock& operator=(ScopedLock&& other) noexcept {
        if (this == &other) return *this;
        release();
        mutex_ = other.mutex_;
        owns_ = other.owns_;
        other.mutex_ = nullptr;
        other.owns_ = false;
        return *this;
    }

    bool owns_lock() const { return owns_; }
    void release() {
        if (owns_) {
            hoo_thread_mutex_unlock(mutex_);
            owns_ = false;
        }
    }

private:
    HooMutex mutex_ = nullptr;
    bool owns_ = false;
};

} // namespace hoo::thread
#endif
