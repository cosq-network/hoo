# ISSUE-064: Managed Object Linked-List Traversal Is O(n) Performance Bottleneck

## Status
- **Date**: 2026-07-19
- **Status**: **OPEN**
- **Priority**: 🟠 **P1 - HIGH** (Fix in current sprint - performance bottleneck)
- **Sprint**: Week 3 (Days 4-5)
- **Estimate**: 2-3 days
- **Related**: ISSUE-046 (original issue)

---

## 1. Overview
The `hoo_is_managed_object()` function in `src/runtime/lib/hoo_runtime.c` performs an O(n) traversal of a linked list while holding a mutex. This becomes a performance bottleneck in applications with many managed objects.

## 2. Technical Analysis

### Current Implementation (hoo_runtime.c:118-130)
```c
int64_t hoo_is_managed_object(const void* obj) {
    if (!obj) return 0;
    hoo_mutex_lock(&g_managed_objects_mutex);
    ManagedObjNode* it = g_managed_objects;
    while (it) {
        if (it->obj == obj) {
            hoo_mutex_unlock(&g_managed_objects_mutex);
            return 1;
        }
        it = it->next;
    }
    hoo_mutex_unlock(&g_managed_objects_mutex);
    return 0;
}
```

### Performance Analysis
- **Time complexity**: O(n) where n = number of managed objects
- **Lock contention**: Mutex held for entire traversal
- **Scale issue**: With 100K objects, each check scans 100K nodes

### Usage Frequency
This function is called during:
- Memory leak detection (`hoo_print_memory_stats`)
- Debug assertions
- Type checking operations

## 3. Requirements
1. Replace linked list with a hash set for O(1) lookup
2. Use lock-free data structure or fine-grained locking
3. Consider making this debug-only functionality
4. Add metrics to measure impact

## 4. Implementation Options

### Option A: Hash Set (Recommended)
```c
#include <uthash.h>

typedef struct {
    void* obj;
    UT_hash_handle hh;
} ManagedObjEntry;

static ManagedObjEntry* g_managed_objects_hash = NULL;
```

### Option B: Bloom Filter
Use a probabilistic data structure for fast negative checks.

### Option C: Debug-Only
```c
#ifdef HOO_DEBUG_MEMORY
int64_t hoo_is_managed_object(const void* obj) { ... }
#else
int64_t hoo_is_managed_object(const void* obj) { return 0; }
#endif
```

## 5. Impact
- Current: O(n) lookup with mutex contention
- After fix: O(1) lookup with minimal locking

## 6. Test Coverage
- Benchmark with 1K, 10K, 100K objects
- Measure lock contention under concurrent access
- Test memory overhead of hash set vs linked list
