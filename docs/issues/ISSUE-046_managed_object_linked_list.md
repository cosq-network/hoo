# ISSUE-046: `hoo_is_managed_object` O(n) Linked-List Walk With Global Mutex

## 1. Overview
`hoo_is_managed_object()` walks a global linked list under a mutex to check whether a pointer is a managed object. Every allocation registers and every deallocation unregisters via this same O(n) structure, creating a global serialization bottleneck that degrades linearly with object count.

## 2. Technical Analysis
- **Location**: `src/runtime/lib/hoo_runtime.c:91-146`
- **Registration**: `managed_register()` (line 99) inserts into `g_managed_objects`.
- **Unregistration**: `managed_unregister()` (line 112) removes from the list.
- **Lookup**: `hoo_is_managed_object()` (line 133) walks the list under `g_managed_objects_mutex`.
- **Complexity**: O(n) where n = total live managed objects across all threads.

## 3. Impact
- Every ARC retain/release that needs managed-type checking contends on this mutex.
- Single-threaded workloads pay the O(n) cost with unnecessary lock overhead.
- Multi-threaded workloads serialize on every allocation and deallocation.
- Performance degrades linearly as the heap grows.

## 4. Suggested Fix
- Replace the linked list with a hash set (e.g., a concurrent skip-list or a lock-free hash table).
- Alternatively, use a pointer-address-based approach: allocate managed objects from a dedicated address range so the check becomes a bounds comparison.
- At minimum, switch from `std::list` to `std::unordered_set` to get O(1) expected lookups while keeping the global mutex.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **FIXED**
- **Priority**: **HIGH**

## 6. Resolution
Managed-object tracking now uses hashed buckets protected by striped mutexes
instead of one global linked list and mutex. Registration, removal, and lookup
remain synchronized while reducing expected lookup work and global contention.
