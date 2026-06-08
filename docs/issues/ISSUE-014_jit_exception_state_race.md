# ISSUE-014: Unsynchronized Global State in JIT Exception Handling

## 1. Overview
The JIT exception handling subsystem uses multiple global data structures without adequate synchronization. The `gCurrentExceptionByState` map is accessed from JIT-compiled code without any lock, causing data races under concurrent execution.

## 2. Technical Analysis

### 2.1 Unlocked global map
- **Location**: `src/hvm/HVMJIT.cpp` lines 404, 481-486, 511
- **Issue**: `gCurrentExceptionByState` is a `std::unordered_map<void*, uint64_t>` accessed in `shadow_throw_to_handler` and `shadow_rethrow_to_handler` functions. These functions are called from JIT-compiled code which may execute on multiple threads.

```cpp
// Line 404: No mutex declared for this map
std::unordered_map<void*, uint64_t> gCurrentExceptionByState;

// Line 481-486: shadow_throw_to_handler reads/writes without lock
auto exIt = gCurrentExceptionByState.find(state);
if (exIt != gCurrentExceptionByState.end()) {
    // ... use existing exception ...
}
gCurrentExceptionByState[state] = exc;

// Line 511: shadow_rethrow_to_handler erases without lock
gCurrentExceptionByState.erase(state);
```

### 2.2 Contrast with protected globals
The adjacent `gShadowHandlers` map (line 403) IS protected by `gShadowHandlersMu` (line 385), and `gStateOwnerByPtr` (line 406) is protected by `gStateOwnerMu` (line 405). The unprotected `gCurrentExceptionByState` is an inconsistency.

## 3. Impact
- Data race when multiple JIT-compiled threads execute exception operations simultaneously.
- Corrupted exception state leading to wrong exception type, lost exceptions, or crashes.
- Undefined behavior under concurrent exception handling.

## 4. Suggested Fix
Add a dedicated mutex for `gCurrentExceptionByState`:

```cpp
std::mutex gCurrentExceptionMu;
std::unordered_map<void*, uint64_t> gCurrentExceptionByState;

// In shadow_throw_to_handler:
{
    std::lock_guard<std::mutex> lk(gCurrentExceptionMu);
    // ... access gCurrentExceptionByState ...
}

// In shadow_rethrow_to_handler:
{
    std::lock_guard<std::mutex> lk(gCurrentExceptionMu);
    gCurrentExceptionByState.erase(state);
}
```

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
