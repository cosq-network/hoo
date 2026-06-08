# ISSUE-016: JIT Memory Safety Issues

## 1. Overview
The JIT layer has several memory safety issues including a dangling raw pointer to the module memory buffer, potential bump-allocator integer overflow, and risk of C++ stack overflow from recursive interpreter calls.

## 2. Issues

### 2.1 Dangling `g_hvm_memory` pointer
- **Location**: `src/hvm/HVMJIT.cpp` lines 1665-1667, 2208
- **Issue**: `g_hvm_memory` stores a raw pointer to `memory_.data()` (an `std::vector<uint8_t>`). If the vector is ever resized, the pointer dangles. The POSIX syscall wrappers (lines 1770-1820) dereference this pointer directly.

```cpp
static uint8_t* g_hvm_memory = nullptr;
void hvm_set_memory_base(uint8_t* base) { g_hvm_memory = base; }

// In constructor:
memory_.resize(16 * 1024 * 1024, 0);
hvm_set_memory_base(memory_.data());
```

### 2.2 Bump allocator integer overflow
- **Location**: `src/hvm/HVMJIT.cpp` lines 588-611
- **Issue**: `gHvmBumpNext.fetch_add(size)` uses atomic fetch-and-add. If `size` is very large (near `UINT64_MAX`), the addition wraps around, allowing the post-add bounds check `offset + size > kHvmHeapLimit` to pass erroneously.

### 2.3 Unbounded C++ recursion for CALL
- **Location**: `src/hvm/HVMJIT.cpp` lines 3636-3647
- **Issue**: The interpreter loop handles `CALL` by recursively calling `executeFunction` on the C++ call stack. Deeply nested calls will overflow the C++ stack.

```cpp
rv = executeFunction(module, calleeName, state); // Recursive call, no depth limit
```

## 3. Impact
- Use-after-free if `memory_` is ever resized.
- Heap corruption from wrap-around allocation.
- Crash from stack overflow on deeply nested calls.

## 4. Suggested Fixes
1. Replace the raw pointer with a pointer + size pair that is recent on each access:
   - Store `g_hvm_memory_base` and `g_hvm_memory_size`.
   - Validate all offsets against the stored size before dereferencing.
2. Add overflow check before `fetch_add`:
   - Check `size <= kHvmHeapLimit - gHvmBumpNext.load()` before fetching.
3. Add a recursion depth counter in `executeFunction`:
   - Return an error if depth exceeds a limit (e.g., 1024).
   - Use a trampoline or explicit stack for iterative calls.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
