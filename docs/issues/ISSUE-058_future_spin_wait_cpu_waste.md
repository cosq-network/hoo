# ISSUE-058: Future Spin-Wait Consumes 100% CPU Without Yielding

## Status
- **Date**: 2026-07-19
- **Status**: **IMPLEMENTED** (2026-07-19)
- **Priority**: 🔴 **P0 - CRITICAL** (Must fix immediately - blocks async functionality)
- **Sprint**: Week 1 (Days 3-5)
- **Estimate**: 3-5 days
- **Actual**: 1 day

---

## 1. Overview
The `hoo_future_get_value()` and `_F_hoo_future_await_unwrap_p_p()` functions in `src/runtime/lib/hoo_future.cpp` implemented a spin-wait loop that consumed 100% CPU while waiting for a future to resolve. This was unacceptable for production async workloads.

## 2. Technical Analysis

### Original Implementation (hoo_future.cpp:128-131)
```c
void* hoo_future_get_value(HooFuture f) {
    if (!f) return NULL;
    HooFutureImpl* impl = get_impl(f);
    /* Spin-wait until resolved (simple polling for now) */
    while (!impl->ready) {
        /* In a real event-loop integration, yield here */
    }
    return impl->value;
}
```

### Problems Identified
1. **CPU waste**: The tight loop consumed 100% CPU core while waiting
2. **No event loop integration**: Despite having libuv event loop (`hoo_event_loop`), futures didn't use it
3. **No yielding mechanism**: No `sched_yield()`, `uv_run()`, or any cooperative yield
4. **Potential livelock**: If the resolver runs on the same thread, the future may never resolve

## 3. Implementation

### Changes Made

1. **Added event loop integration helpers**:
   - `yield_to_event_loop()`: Runs the event loop in non-blocking mode and yields to OS scheduler
   - `wait_for_future()`: Waits with exponential backoff instead of spin-wait

2. **Implemented exponential backoff**:
   - Starts at 1ms delay
   - Doubles delay each iteration
   - Caps at 16ms (roughly one frame at 60fps)
   - Balances responsiveness and CPU usage

3. **Integrated with libuv event loop**:
   - Calls `uv_run(loop, UV_RUN_NOWAIT)` to process pending async work
   - Uses `uv_sleep()` for cooperative yielding

4. **Fixed Future destructor cleanup**:
   - Added nullification of continuation callbacks in destructor
   - Prevents use-after-free when future is destroyed before continuation fires

### Files Modified
- `src/runtime/lib/hoo_future.h` - Updated documentation
- `src/runtime/lib/hoo_future.cpp` - Added event loop integration and fixed destructor

### New Implementation
```c
static void yield_to_event_loop(void) {
    uv_loop_t* loop = hoo_event_loop_get();
    if (loop) {
        /* Run the event loop in non-blocking mode to process pending work */
        uv_run(loop, UV_RUN_NOWAIT);
    }
    
    /* Also yield to the OS scheduler to prevent CPU spinning */
    uv_sleep(1);  /* 1ms sleep */
}

static void wait_for_future(HooFutureImpl* impl) {
    int backoff_ms = 1;  /* Start with 1ms */
    int max_backoff_ms = 16;  /* Cap at 16ms (roughly one frame at 60fps) */
    
    while (!impl->ready) {
        yield_to_event_loop();
        
        /* Exponential backoff to reduce CPU usage */
        if (backoff_ms < max_backoff_ms) {
            backoff_ms *= 2;
        }
        uv_sleep(backoff_ms);
    }
}
```

## 4. Test Coverage

### New Tests Added (tests/jit/HooFutureJitTest.cpp)
- `FutureCreation`: Tests basic future creation
- `FutureImmediateResolution`: Tests immediate resolution
- `FutureErrorHandling`: Tests error propagation
- `FutureContinuation`: Tests continuation callback
- `FutureContinuationAfterResolution`: Tests continuation after resolution
- `FutureWaitWithEventLoop`: Tests waiting with event loop integration
- `AwaitUnwrapWithEventLoop`: Tests await unwrap function
- `AwaitUnwrapWithError`: Tests await unwrap with error
- `MultipleFutures`: Tests concurrent futures
- `FutureRetention`: Tests reference counting
- `NullFutureOperations`: Tests null safety

## 5. Performance Impact

### Before
- CPU usage: 100% while waiting on any future
- Battery impact: Severe drain on mobile/laptop devices
- Scalability: Cannot handle many concurrent futures

### After
- CPU usage: Minimal (1ms sleep + event loop processing)
- Battery impact: Minimal
- Scalability: Can handle many concurrent futures efficiently

## 6. Acceptance Criteria
- [x] Futures integrate with libuv event loop
- [x] Waiting yields control to event loop
- [x] CPU usage is minimal during wait
- [x] Continuation callbacks are properly cleaned up on destruction
- [x] Test coverage added for all scenarios

## 7. Notes
- The exponential backoff algorithm balances responsiveness (1ms initial) with CPU efficiency (16ms cap)
- The event loop integration allows other async operations to progress while waiting
- The `uv_sleep(1)` call yields to the OS scheduler, preventing busy-waiting
- Future destructor now properly nullifies continuation callbacks to prevent use-after-free
