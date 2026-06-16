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
    int64_t* result;
};
struct ThreadEntry {
    HANDLE handle;
    int64_t* result;
};
static std::mutex gThreadMapMutex;
static std::map<DWORD, ThreadEntry> gThreadMap;

static DWORD WINAPI thread_wrapper(LPVOID lpParam) {
    auto* ts = static_cast<ThreadStart*>(lpParam);
    *ts->result = ts->func(ts->arg);
    delete ts;
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
    auto* result = new int64_t(0);
    auto* ts = new ThreadStart{func, arg, result};
    DWORD threadId;
    HANDLE h = CreateThread(nullptr, 0, thread_wrapper, ts, 0, &threadId);
    if (!h) { delete ts; delete result; return -1; }
    {
        std::lock_guard<std::mutex> lock(gThreadMapMutex);
        gThreadMap[threadId] = {h, result};
    }
    return static_cast<int64_t>(threadId);
#else
    pthread_t thread;
    int ret = pthread_create(&thread, nullptr,
        reinterpret_cast<void*(*)(void*)>(func), arg);
    if (ret != 0) return -1;
#ifdef __APPLE__
    return reinterpret_cast<int64_t>(thread);
#else
    return static_cast<int64_t>(thread);
#endif
#endif
}

int64_t hoo_thread_join(int64_t thread_id) {
#ifdef _WIN32
    DWORD tid = static_cast<DWORD>(thread_id);
    int64_t* result = nullptr;
    HANDLE h = nullptr;
    {
        std::lock_guard<std::mutex> lock(gThreadMapMutex);
        auto it = gThreadMap.find(tid);
        if (it == gThreadMap.end()) return -1;
        h = it->second.handle;
        result = it->second.result;
        gThreadMap.erase(it);
    }
    DWORD ret = WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    int64_t retval = *result;
    delete result;
    if (ret != WAIT_OBJECT_0) return -1;
    return retval;
#else
#ifdef __APPLE__
    pthread_t thread = reinterpret_cast<pthread_t>(thread_id);
#else
    pthread_t thread = static_cast<pthread_t>(thread_id);
#endif
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
#ifdef __APPLE__
    return reinterpret_cast<int64_t>(pthread_self());
#else
    return static_cast<int64_t>(pthread_self());
#endif
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
