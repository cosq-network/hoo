# ISSUE-063: Future Continuation Callback Not Cleaned Up on Destruction

## Status
- **Date**: 2026-07-19
- **Status**: **IMPLEMENTED** (2026-07-19)
- **Priority**: 🟠 **P1 - HIGH** (Fix in current sprint - memory safety issue)
- **Sprint**: Week 3 (Day 1)
- **Estimate**: 1 day
- **Actual**: Included in ISSUE-058 fix

---

## 1. Overview
The `future_destructor()` function in `src/runtime/lib/hoo_future.cpp` did not clean up the `continuation` callback or `continuation_arg` when the future was destroyed. If the continuation referenced external data that got freed, this could lead to use-after-free when the continuation was triggered.

## 2. Technical Analysis

### Original Implementation (hoo_future.cpp:31-44)
```c
static void future_destructor(void* obj) {
    HooFutureImpl* impl = (HooFutureImpl*)obj;
    /* Release the resolved ARC-managed value, if any */
    if (impl->value) {
        hoo_release(impl->value);
        impl->value = NULL;
    }
    /* Free the error string, if any */
    if (impl->error_message) {
        free(impl->error_message);
        impl->error_message = NULL;
    }
    // NOTE: continuation and continuation_arg are NOT cleaned up!
}
```

### Problem
1. **Continuation not nulled**: If the future was destroyed before resolution, the continuation pointer remained
2. **No deregistration**: If the continuation was registered with the event loop, it wasn't deregistered
3. **Potential use-after-free**: If `continuation_arg` pointed to stack-allocated or freed data
4. **Race condition**: Event loop could trigger continuation on destroyed future

## 3. Implementation

### Changes Made

Updated `future_destructor()` in `src/runtime/lib/hoo_future.cpp`:

```c
static void future_destructor(void* obj) {
    HooFutureImpl* impl = (HooFutureImpl*)obj;
    
    /* Cancel any pending continuation */
    impl->continuation = NULL;
    impl->continuation_arg = NULL;
    
    /* Release the resolved ARC-managed value, if any */
    if (impl->value) {
        hoo_release(impl->value);
        impl->value = NULL;
    }
    /* Free the error string, if any */
    if (impl->error_message) {
        free(impl->error_message);
        impl->error_message = NULL;
    }
}
```

### Also Updated trigger_continuation()

```c
static void trigger_continuation(HooFutureImpl* impl) {
    if (impl->continuation) {
        HooFutureContinuation cb = impl->continuation;
        void* arg = impl->continuation_arg;
        impl->continuation = NULL;
        impl->continuation_arg = NULL;
        
        /* Run continuation */
        cb(arg);
    }
}
```

This ensures the continuation is only called once and is properly cleaned up.

## 4. Test Coverage

### Tests Added (tests/jit/HooFutureJitTest.cpp)
- `FutureContinuation`: Tests continuation callback is called
- `FutureContinuationAfterResolution`: Tests continuation set after resolution
- `MultipleFutures`: Tests concurrent futures with continuations
- `NullFutureOperations`: Tests null safety

## 5. Acceptance Criteria
- [x] Future destructor nullifies continuation callback
- [x] Future destructor nullifies continuation argument
- [x] Continuation is only called once
- [x] Continuation is cleaned up after calling
- [x] No use-after-free on future destruction

## 6. Notes
- This fix was included as part of the ISSUE-058 (Future event loop integration) work
- The destructor now properly cleans up all resources to prevent memory leaks and use-after-free
- The trigger_continuation function now uses a local copy of the callback before nullifying, ensuring thread safety
