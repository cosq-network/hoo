#include "hoo_thread.h"
#include <cstdlib>
#include <map>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
struct HooMutexImpl {
    CRITICAL_SECTION cs;
};
struct ThreadStart {
    int64_t (*func)(void*);
    void* arg;
};
static std::mutex gThreadMapMutex;
static std::map<DWORD, HANDLE> gThreadHandles;

static DWORD WINAPI thread_wrapper(LPVOID lpParam) {
    auto* ts = static_cast<ThreadStart*>(lpParam);
    auto* func = ts->func;
    void* arg = ts->arg;
    delete ts;
    func(arg);
    return 0;
}
#else
#include <pthread.h>
struct HooMutexImpl {
    pthread_mutex_t mutex;
};
#endif

extern "C" {

int64_t hoo_thread_spawn(int64_t (*func)(void*), void* arg) {
#ifdef _WIN32
    auto* ts = new ThreadStart{func, arg};
    DWORD threadId;
    HANDLE h = CreateThread(nullptr, 0, thread_wrapper, ts, 0, &threadId);
    if (!h) { delete ts; return -1; }
    {
        std::lock_guard<std::mutex> lock(gThreadMapMutex);
        gThreadHandles[threadId] = h;
    }
    return static_cast<int64_t>(threadId);
#else
    pthread_t thread;
    int ret = pthread_create(&thread, nullptr,
        reinterpret_cast<void*(*)(void*)>(func), arg);
    if (ret != 0) return -1;
    return static_cast<int64_t>(thread);
#endif
}

int64_t hoo_thread_join(int64_t thread_id) {
#ifdef _WIN32
    DWORD tid = static_cast<DWORD>(thread_id);
    HANDLE h = nullptr;
    {
        std::lock_guard<std::mutex> lock(gThreadMapMutex);
        auto it = gThreadHandles.find(tid);
        if (it == gThreadHandles.end()) return -1;
        h = it->second;
        gThreadHandles.erase(it);
    }
    DWORD ret = WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    return (ret == WAIT_OBJECT_0) ? 0 : -1;
#else
    pthread_t thread = static_cast<pthread_t>(thread_id);
    void* result = nullptr;
    int ret = pthread_join(thread, &result);
    if (ret != 0) return -1;
    return reinterpret_cast<int64_t>(result);
#endif
}

int64_t hoo_thread_self(void) {
#ifdef _WIN32
    return static_cast<int64_t>(GetCurrentThreadId());
#else
    return static_cast<int64_t>(pthread_self());
#endif
}

HooMutex hoo_thread_mutex_create(void) {
    HooMutexImpl* impl = new HooMutexImpl();
#ifdef _WIN32
    InitializeCriticalSection(&impl->cs);
#else
    pthread_mutex_init(&impl->mutex, nullptr);
#endif
    return impl;
}

int64_t hoo_thread_mutex_lock(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
#ifdef _WIN32
    EnterCriticalSection(&impl->cs);
    return 0;
#else
    return pthread_mutex_lock(&impl->mutex) == 0 ? 0 : -1;
#endif
}

int64_t hoo_thread_mutex_unlock(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
#ifdef _WIN32
    LeaveCriticalSection(&impl->cs);
    return 0;
#else
    return pthread_mutex_unlock(&impl->mutex) == 0 ? 0 : -1;
#endif
}

int64_t hoo_thread_mutex_destroy(HooMutex mutex) {
    if (!mutex) return -1;
    HooMutexImpl* impl = static_cast<HooMutexImpl*>(mutex);
#ifdef _WIN32
    DeleteCriticalSection(&impl->cs);
#else
    pthread_mutex_destroy(&impl->mutex);
#endif
    delete impl;
    return 0;
}

} // extern "C"
