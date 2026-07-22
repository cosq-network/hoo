# ISSUE-061: Async/Await Implementation Is Incomplete

## Status
- **Date**: 2026-07-19
- **Status**: **IMPLEMENTED** (2026-07-19)
- **Priority**: 🔴 **P0 - CRITICAL** (Must fix immediately - feature advertised but non-functional)
- **Sprint**: Week 2 (Days 1-5)
- **Estimate**: 1-2 weeks
- **Actual**: 2 days

---

## 1. Overview
The async/await feature (Phase 15, ISSUE-043) had a partially implemented codegen and runtime but lacked proper Future wrapping, event loop integration, and complete test coverage.

## 2. Technical Analysis

### Original Issues Found

1. **Codegen for Async Function Return** (Critical)
   - Async functions should return `Future<T>`, not `T`
   - Return values should be wrapped in a Future object
   - The codegen just emitted coroutine intrinsics without wrapping

2. **Codegen for Await Expression** (Critical)
   - `await(future)` should suspend until future resolves
   - Current implementation called `llvm.coro.suspend` without proper integration

3. **Missing JIT Wrappers** (Critical)
   - No JIT wrappers for `hoo_future_new`, `hoo_future_set_value`, etc.

## 3. Implementation

### Changes Made

#### 1. Codegen Changes (HVMCodeGenerator.cpp)

**Added hidden local for Future in async functions:**
```cpp
// In header file
int32_t asyncFutureOffset_ = 0; // Stack offset for the hidden Future in async functions
```

**Updated beginFunction() to create Future:**
```cpp
if (decl->isAsync()) {
    // Create a Future object for the async function
    uint32_t elemTypeId = 1; // default to int64
    if (decl->getReturnType()) {
        if (auto futureType = dynamic_cast<const ast::FutureType*>(decl->getReturnType())) {
            elemTypeId = typeIdFromDeclaredType(&futureType->getElementType());
        }
    }
    
    // Create the Future
    uint8_t elemTypeReg = emitConstant(static_cast<int64_t>(elemTypeId));
    emit(Opcode::MOV, OperandsR{1, elemTypeReg, 0, 0});
    emitCall(Opcode::CALL, "_F_hoo_future_new_i64");
    freeRegister(elemTypeReg);
    
    // Store the Future in a hidden local variable
    asyncFutureOffset_ = reserveLocal("__async_future__", 123, "Future");
    emit(Opcode::ST_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
    
    // Emit coroutine intrinsics
    emitCall(Opcode::CALL, "llvm.coro.id");
    emitCall(Opcode::CALL, "llvm.coro.size.i64");
    emitCall(Opcode::CALL, "llvm.coro.begin");
}
```

**Updated return statement handling for async functions:**
```cpp
if (currentFunctionIsAsync_ && asyncFutureOffset_ != 0) {
    // For async functions, set the Future's value and return the Future pointer
    if (ret->hasExpression()) {
        uint8_t reg = visitExpression(*ret->getExpression());
        // Load the Future pointer
        emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
        // Move the return value to r2
        emit(Opcode::MOV, OperandsR{2, reg, 0, 0});
        // Call hoo_future_set_value(future, value)
        emitCall(Opcode::CALL, "_F_hoo_future_set_value_v_p_p");
        freeRegister(reg);
    } else {
        // Void async function - set NULL value
        emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
        emit(Opcode::MOV, OperandsR{2, 0, 0, 0}); // NULL
        emitCall(Opcode::CALL, "_F_hoo_future_set_value_v_p_p");
    }
    // Return the Future pointer
    emit(Opcode::LD_D, OperandsI{1, 30, static_cast<int16_t>(asyncFutureOffset_)});
    emitScopeCleanup(scopeStack_.size(), 0);
    emitCall(Opcode::CALL, "llvm.coro.end");
    emitCall(Opcode::CALL, "llvm.coro.free");
    emit(Opcode::LEAVE, OperandsR{0, 0, 0, 0});
    emit(Opcode::RET, OperandsR{0, 0, 0, 0});
}
```

**Updated function return type handling:**
```cpp
// Async functions always return a pointer to Future
if (decl && decl->isAsync()) {
    mp.returnType = "ptr";
}
```

**Updated function return type storage:**
```cpp
if (decl.isAsync()) {
    // Async functions return a pointer to Future
    functionReturnTypes_[decl.getName()] = 123; // Future type ID
    functionReturnClass_[decl.getName()] = "Future";
}
```

**Updated await expression handling:**
```cpp
if (auto awaitExpr = dynamic_cast<const ast::AwaitExpression*>(&expr)) {
    uint32_t futureTypeId = inferExpressionTypeId(awaitExpr->getFuture());
    if (futureTypeId != 123 && futureTypeId != 100 && futureTypeId != 0) {
        addError(std::string("await expression must be used with a Future type"));
        return 0;
    }
    uint8_t futureReg = visitExpression(awaitExpr->getFuture());
    // Load the future pointer into r1 for the await call
    emit(Opcode::MOV, OperandsR{1, futureReg, 0, 0});
    // Call _F_hoo_future_await_unwrap_p_p which handles event loop integration
    emitCall(Opcode::CALL, "_F_hoo_future_await_unwrap_p_p");
    // The result is in r1
    uint8_t dest = allocateRegister();
    emit(Opcode::MOV, OperandsR{dest, 1, 0, 0});
    emit(Opcode::RELEASE, OperandsR{futureReg, 0, 0, 0});
    freeRegister(futureReg);
    return dest;
}
```

#### 2. JIT Wrappers (HVMJIT.cpp)

**Added JIT wrapper functions:**
```cpp
uint64_t jit_hoo_future_new(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    int64_t elemTypeId = state->regs[1];
    HooFuture fut = hoo_future_new(elemTypeId);
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(fut));
}

uint64_t jit_hoo_future_set_value(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    HooFuture fut = reinterpret_cast<HooFuture>(state->regs[1]);
    void* value = reinterpret_cast<void*>(state->regs[2]);
    hoo_future_set_value(fut, value);
    return 0;
}

uint64_t jit_hoo_future_set_error(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    HooFuture fut = reinterpret_cast<HooFuture>(state->regs[1]);
    const char* msg = reinterpret_cast<const char*>(state->regs[2]);
    hoo_future_set_error(fut, msg);
    return 0;
}

uint64_t jit_hoo_future_get_value(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    HooFuture fut = reinterpret_cast<HooFuture>(state->regs[1]);
    void* result = hoo_future_get_value(fut);
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
}

uint64_t jit_hoo_future_await_unwrap(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    HooFuture fut = reinterpret_cast<HooFuture>(state->regs[1]);
    void* result = _F_hoo_future_await_unwrap_p_p(fut);
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(result));
}
```

**Registered JIT functions:**
```cpp
{"_F_hoo_future_new_i64", reinterpret_cast<void*>(&jit_hoo_future_new)},
{"_F_hoo_future_set_value_v_p_p", reinterpret_cast<void*>(&jit_hoo_future_set_value)},
{"_F_hoo_future_set_error_v_p_p", reinterpret_cast<void*>(&jit_hoo_future_set_error)},
{"_F_hoo_future_get_value_p_p", reinterpret_cast<void*>(&jit_hoo_future_get_value)},
{"_F_hoo_future_await_unwrap_p_p", reinterpret_cast<void*>(&jit_hoo_future_await_unwrap)},
```

#### 3. Test Updates (NewLanguageFeaturesTest.cpp)

**Updated async/await tests to verify execution:**
```cpp
TEST_F(NewLanguageFeaturesTest, AsyncAwait_SimpleExecution) {
    std::string code = R"(
        import hoo;
        
        async func:Future<int64> getVal() {
            return 42;
        }
    )";

    ASSERT_TRUE(jit->loadSourceCode("test", code)) << jit->getLastError();

    // Call getVal(). It should return a Future<int64>.
    int64_t futPtr = jit->run("_F_getVal_a_Future");
    HooFuture fut = reinterpret_cast<HooFuture>(futPtr);
    ASSERT_NE(fut, nullptr);
    
    // The future should be immediately resolved since getVal() is synchronous
    EXPECT_EQ(hoo_future_is_ready(fut), 1);
    EXPECT_EQ(hoo_future_has_error(fut), 0);
    
    // Get the value - should be 42
    void* value = hoo_future_get_value(fut);
    // For int64, the value is stored directly in the pointer
    int64_t intValue = reinterpret_cast<int64_t>(value);
    EXPECT_EQ(intValue, 42);
}
```

## 4. Test Coverage

### Tests Added/Updated (NewLanguageFeaturesTest.cpp)
- `AsyncAwait_SimpleExecution`: Tests basic async function returning Future
- `AsyncAwait_VoidFunction`: Tests async void function
- `AsyncAwait_ChainedCalls`: Tests await with chained async calls

## 5. Acceptance Criteria
- [x] Async functions return properly wrapped Future objects
- [x] Return values are set on the Future
- [x] Await extracts value from resolved Future
- [x] JIT wrappers exist for all Future functions
- [x] Tests verify actual execution, not just compilation

## 6. Progress Summary

| Component | Status | Notes |
|-----------|--------|-------|
| Grammar | ✅ Complete | async, await, Future<T> |
| AST | ✅ Complete | FunctionDeclaration, AwaitExpression, FutureType |
| Runtime | ✅ Complete | HooFuture with event loop integration |
| JIT Passes | ✅ Complete | CoroEarlyPass, CoroSplitPass, CoroCleanupPass |
| JIT Wrappers | ✅ Complete | Future functions registered |
| Codegen - Async Return | ✅ Complete | Future wrapping implemented |
| Codegen - Await | ✅ Complete | Calls await_unwrap with event loop integration |
| Tests | ✅ Complete | 3 tests verifying actual execution |

**Overall Progress**: 100% complete

## 7. Notes

1. **Future Creation**: Async functions now create a Future object at entry and store it in a hidden local variable `__async_future__`.

2. **Return Handling**: Return statements in async functions now set the Future's value before returning the Future pointer.

3. **Await Integration**: The await expression now calls `_F_hoo_future_await_unwrap_p_p` which integrates with the event loop (via ISSUE-058).

4. **JIT Wrappers**: All Future functions have JIT wrappers that properly pass arguments via registers.

5. **Type System**: Async functions are now correctly typed as returning `Future<T>` (type ID 123) rather than the raw return type.

---

*Last Updated: 2026-07-19*
