# ISSUE-014: Unsynchronized Global State in JIT Exception Handling

## 1. Overview
The JIT exception handling subsystem uses multiple global data structures without adequate synchronization. The `gCurrentExceptionByState` map is accessed from JIT-compiled code without any lock, causing data races under concurrent execution.

## 2. Technical Analysis

### 2.1 Shared-mutex locking (no dedicated mutex)
- **Location**: `src/hvm/HVMJIT.cpp` lines 404, 481-486, 511
- **Issue**: `gCurrentExceptionByState` is a `std::unordered_map<void*, uint64_t>` accessed in `shadow_throw_to_handler` and `shadow_rethrow_to_handler` functions. All accesses are protected by `gShadowHandlersMu` (the same mutex that protects the shadow handler stack), but there is no dedicated mutex for exception state — it piggybacks on an unrelated lock.

```cpp
// Line 404: No dedicated mutex, relies on gShadowHandlersMu
std::unordered_map<void*, uint64_t> gCurrentExceptionByState;
```

### 2.2 Contrast with protected globals
`gShadowHandlers` (line 403) IS protected by `gShadowHandlersMu` (line 385) — that's its intended mutex. `gStateOwnerByPtr` (line 406) is protected by `gStateOwnerMu` (line 405). `gCurrentExceptionByState` borrows `gShadowHandlersMu` instead of having its own lock, making the locking relationship harder to reason about.

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
- **Status**: **PARTIALLY FIXED**
- **Priority**: **LOW**
- **Audit 2026-06-21**: Verified accesses remain protected through the shared shadow-handler mutex, but there is still no dedicated lock documenting ownership of `gCurrentExceptionByState`.
- **Note**: As of 2026-06-11, `gCurrentExceptionByState` IS protected by `gShadowHandlersMu` — it was never truly unlocked. The remaining concern is that no dedicated mutex exists, but there is no data race. Priority has been downgraded accordingly.
