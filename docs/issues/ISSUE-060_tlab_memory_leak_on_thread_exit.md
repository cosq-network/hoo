# ISSUE-060: Thread-Local Allocation Buffers Not Freed on Thread Exit

## Status
- **Date**: 2026-07-19
- **Status**: **OPEN**
- **Priority**: 🟠 **P1 - HIGH** (Fix in current sprint - memory leak)
- **Sprint**: Week 3 (Days 2-3)
- **Estimate**: 1-2 days

---

## 1. Overview
Thread-Local Allocation Buffers (TLABs) in `src/runtime/lib/hoo_runtime.c` are allocated per-thread but never freed when a thread exits. This causes memory leaks in multithreaded applications.

## 2. Technical Analysis

### Current Implementation (hoo_runtime.c)
```c
static _Thread_local ThreadTLAB g_thread_tlab = {NULL};
static _Thread_local TLABObjNode* g_tlab_objects = NULL;

// TLAB blocks are allocated in tlab_alloc_block()
static TLABBlock* tlab_alloc_block(size_t min_payload_size) {
    TLABBlock* block = (TLABBlock*)malloc(sizeof(TLABBlock) + block_cap);
    block->next = g_thread_tlab.head;
    g_thread_tlab.head = block;
    return block;
}

// Cleanup function exists but is never called
static void tlab_reset_thread_cache_impl(void) {
    TLABBlock* it = g_thread_tlab.head;
    while (it) {
        TLABBlock* next = it->next;
        free(it);
        it = next;
    }
    g_thread_tlab.head = NULL;
}
```

### Problem
1. `tlab_reset_thread_cache_impl()` is defined but never called
2. No thread exit handler is registered to call cleanup
3. TLAB blocks accumulate and leak when threads terminate

### Memory Leak Analysis
For each thread that uses TLAB allocation:
- Each TLAB block is 64KB (`HOO_TLAB_BLOCK_SIZE`)
- A thread making many allocations could accumulate multiple blocks
- When the thread exits, `_Thread_local` storage is destroyed but `free()` is never called

## 3. Requirements
1. Register a thread exit handler to call `tlab_reset_thread_cache_impl()`
2. Clean up both TLAB blocks and TLAB object tracking nodes
3. Handle cleanup for threads created by both `hoo_thread_spawn()` and external thread creation

## 4. Implementation Notes

### Option A: pthread_key_t (POSIX)
```c
static pthread_key_t tlab_key;
static pthread_once_t tlab_key_once = PTHREAD_ONCE_INIT;

static void tlab_destructor(void* arg) {
    tlab_reset_thread_cache_impl();
    // Also free g_tlab_objects
}

static void create_tlab_key() {
    pthread_key_create(&tlab_key, tlab_destructor);
}
```

### Option B: Thread Spawn Integration
Add cleanup call at end of `uv_thread_wrapper()` in `hoo_thread.cpp`:
```c
static void uv_thread_wrapper(void* arg) {
    ThreadStart* ts = (ThreadStart*)arg;
    ts->result = ts->func(ts->arg);
    tlab_reset_thread_cache_impl();  // Add this
}
```

### Option C: Atexit Handler (Limited)
Register cleanup with `atexit()`, but this only handles main thread.

## 5. Impact
- Current: Memory grows unbounded with thread creation/destruction
- After fix: TLAB memory is properly reclaimed on thread exit

## 6. Test Coverage
- Create multiple threads that allocate objects
- Join threads and verify memory is reclaimed
- Check that main thread TLAB is also cleaned up
- Measure memory usage before and after thread exits
