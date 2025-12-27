# HooArray Redesign Plan: std::list + std::any Architecture

## Goals

Redesign the generic array implementation to be more flexible, type-safe, and support advanced features:

1. **True Multi-Dimensional Support** - Seamlessly handle nested arrays
2. **Type-Agnostic Storage** - Use std::any instead of byte buffers
3. **Better Type Inference** - Support all primitive types and classes at compile time
4. **Future-Ready** - Support function types and interface types
5. **Maintain Compatibility** - Type-specific wrappers for code generator integration

## Current Architecture Issues

```
Current (Fixed-Size Buffer):
┌─────────────────────────────────────────────┐
│ HooArray                                    │
├─────────────────────────────────────────────┤
│ - void* data (raw bytes)                    │
│ - int64_t capacity (element count)          │
│ - int64_t length (element count)            │
│ - size_t element_size (fixed)               │
│ - int64_t refcount (ARC)                    │
├─────────────────────────────────────────────┤
│ Problems:                                   │
│ ❌ Fixed element size at creation           │
│ ❌ No type information at runtime            │
│ ❌ memcpy overhead for every operation      │
│ ❌ Difficult to support multi-dimensional   │
│ ❌ No native type safety                    │
└─────────────────────────────────────────────┘
```

## New Architecture (std::list + std::any)

```
New (Dynamic Container):
┌─────────────────────────────────────────────┐
│ HooArray                                    │
├─────────────────────────────────────────────┤
│ - std::list<std::any> elements              │
│ - std::type_info type (runtime type)        │
│ - int64_t refcount (ARC)                    │
├─────────────────────────────────────────────┤
│ Benefits:                                   │
│ ✅ Truly type-agnostic                      │
│ ✅ Support any C++ type                     │
│ ✅ Multi-dimensional arrays naturally       │
│ ✅ No element_size needed                   │
│ ✅ Proper C++ type safety                   │
│ ✅ Flexible sizing                          │
└─────────────────────────────────────────────┘
```

## Implementation Phases

### Phase 7.1: Redesign Header File

**File: `runtime/hoo_generic_array.h`**

```cpp
#pragma once

#include <stdint.h>
#include <memory>
#include <list>
#include <any>
#include <typeinfo>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HooArray - Generic Dynamic Array using std::list + std::any
// ============================================================================

typedef void* HooArray;  // Opaque handle to HooArray implementation

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a new empty generic array
 * @return New HooArray with refcount=1
 */
HooArray hoo_array_new(void);

/**
 * Create a generic array from a buffer
 * (Deprecated in favor of type-specific constructors)
 * @param data Pointer to data
 * @param length Number of elements
 * @return New HooArray
 */
HooArray hoo_array_from_buffer(const void* data, int64_t length);

/**
 * Create array with repeated element
 * @param value Pointer to value to repeat
 * @param count Number of repetitions
 * @return New HooArray
 */
HooArray hoo_array_repeat(const void* value, int64_t count);

// ============================================================================
// Basic Operations
// ============================================================================

/**
 * Get the number of elements in array
 * @param arr Array
 * @return Length or 0 if NULL
 */
int64_t hoo_array_length(HooArray arr);

/**
 * Get element at index by pointer
 * @param arr Array
 * @param index Index (0-based)
 * @param dest Destination buffer to copy element
 * @return 1 if success, 0 if out of bounds
 */
int64_t hoo_array_get(HooArray arr, int64_t index, void* dest);

/**
 * Set element at index from pointer
 * @param arr Array
 * @param index Index (0-based)
 * @param value Pointer to value to set
 * @return 1 if success, 0 if out of bounds
 */
int64_t hoo_array_set(HooArray arr, int64_t index, const void* value);

/**
 * Add element to end of array
 * @param arr Array
 * @param value Pointer to value to add
 * @return New length on success, -1 on failure
 */
int64_t hoo_array_push(HooArray arr, const void* value);

/**
 * Remove and return last element
 * @param arr Array
 * @param dest Destination buffer for removed element
 * @return 1 if success, 0 if array empty
 */
int64_t hoo_array_pop(HooArray arr, void* dest);

/**
 * Remove all elements
 * @param arr Array
 */
void hoo_array_clear(HooArray arr);

/**
 * Get number of elements (alias for hoo_array_length)
 * @param arr Array
 * @return Length
 */
int64_t hoo_array_size(HooArray arr);

/**
 * Check if array is empty
 * @param arr Array
 * @return 1 if empty, 0 otherwise
 */
int64_t hoo_array_empty(HooArray arr);

// ============================================================================
// Type Inference Operations
// ============================================================================

/**
 * Push int64 value
 * @param arr Array
 * @param value int64 value
 * @return New length on success
 */
int64_t hoo_array_push_int64(HooArray arr, int64_t value);

/**
 * Push double value
 * @param arr Array
 * @param value double value
 * @return New length on success
 */
int64_t hoo_array_push_double(HooArray arr, double value);

/**
 * Push float value
 * @param arr Array
 * @param value float value
 * @return New length on success
 */
int64_t hoo_array_push_float(HooArray arr, float value);

/**
 * Push bool value
 * @param arr Array
 * @param value bool value (1 or 0)
 * @return New length on success
 */
int64_t hoo_array_push_bool(HooArray arr, int64_t value);

/**
 * Push char value
 * @param arr Array
 * @param value char value
 * @return New length on success
 */
int64_t hoo_array_push_char(HooArray arr, char value);

/**
 * Push string (pointer)
 * @param arr Array
 * @param value Pointer to string
 * @return New length on success
 */
int64_t hoo_array_push_string(HooArray arr, const char* value);

/**
 * Push object pointer (class instance)
 * @param arr Array
 * @param value Pointer to object
 * @return New length on success
 */
int64_t hoo_array_push_object(HooArray arr, void* value);

/**
 * Push array (for multi-dimensional arrays)
 * @param arr Array
 * @param value Pointer to another HooArray
 * @return New length on success
 */
int64_t hoo_array_push_array(HooArray arr, HooArray value);

// ============================================================================
// Reference Counting
// ============================================================================

/**
 * Increment reference count
 * @param arr Array
 * @return Array handle
 */
HooArray hoo_array_retain(HooArray arr);

/**
 * Decrement reference count, free if zero
 * @param arr Array
 */
void hoo_array_release(HooArray arr);

/**
 * Get reference count
 * @param arr Array
 * @return Current refcount
 */
int64_t hoo_array_refcount(HooArray arr);

// ============================================================================
// Type Information
// ============================================================================

/**
 * Get array element type name
 * @param arr Array
 * @return Type name string (e.g., "int64_t", "double", "void*")
 */
const char* hoo_array_element_type(HooArray arr);

/**
 * Check if array contains elements of specific type
 * @param arr Array
 * @param type_name Type name to check
 * @return 1 if matches, 0 otherwise
 */
int64_t hoo_array_is_type(HooArray arr, const char* type_name);

#ifdef __cplusplus
}

// ============================================================================
// C++ Implementation Details (internal only)
// ============================================================================

namespace hooc {

class HooArrayImpl {
public:
    explicit HooArrayImpl();
    ~HooArrayImpl();

    // Type-safe operations
    template<typename T>
    void push(const T& value) {
        elements.push_back(std::any(value));
    }

    template<typename T>
    bool get(int64_t index, T& dest) const {
        if (index < 0 || index >= static_cast<int64_t>(elements.size())) {
            return false;
        }
        try {
            auto it = elements.begin();
            std::advance(it, index);
            dest = std::any_cast<T>(*it);
            return true;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }

    template<typename T>
    bool set(int64_t index, const T& value) {
        if (index < 0 || index >= static_cast<int64_t>(elements.size())) {
            return false;
        }
        try {
            auto it = elements.begin();
            std::advance(it, index);
            *it = std::any(value);
            return true;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }

    void clear() {
        elements.clear();
    }

    int64_t length() const {
        return static_cast<int64_t>(elements.size());
    }

    bool empty() const {
        return elements.empty();
    }

    void retain() {
        refcount++;
    }

    void release() {
        refcount--;
        if (refcount <= 0) {
            delete this;
        }
    }

    int64_t getRefcount() const {
        return refcount;
    }

    std::list<std::any> elements;
    const std::type_info* element_type = nullptr;

private:
    int64_t refcount = 1;
};

} // namespace hooc

#endif
```

### Phase 7.2: Implementation (CPP File)

**File: `runtime/hoo_generic_array.cpp`**

Key changes:
- Use std::list<std::any> instead of raw byte buffer
- Track type_info for runtime type checking
- Implement type-specific push operations
- Maintain reference counting
- Support multi-dimensional arrays naturally

### Phase 7.3: Code Generation Updates

**File: `src/LLVMCodeGenerator.cpp`**

Updates needed:
1. Modify array generation to use new hoo_array_push_* functions
2. Add type-specific push calls based on inferred element type
3. Support multi-dimensional arrays via nested arrays
4. Update type detection for: int64, double, float, bool, char, string, class pointers

### Phase 7.4: Testing

**New Test Suite: `tests/HooArrayRedesignTest.cpp`**

Test cases:
1. Basic primitive array operations (int64, double, float, bool, char)
2. String arrays
3. Object/class instance arrays
4. Multi-dimensional arrays (arrays of arrays)
5. Mixed-type array detection and errors
6. Type checking and inference
7. Large arrays and performance
8. Reference counting
9. Memory management
10. Future: Function type arrays, Interface type arrays

## Migration Strategy

### Step 1: Parallel Implementation
- Keep old implementation temporarily
- Build new implementation alongside
- Run both, compare outputs

### Step 2: Code Generator Transition
- Update code generator to detect element types
- Emit appropriate hoo_array_push_* calls
- Test with existing test suite

### Step 3: Verification
- All 630+ existing tests must pass
- Add 50+ new redesign tests
- Performance benchmarks

### Step 4: Cleanup
- Remove old byte-buffer-based code
- Update documentation
- Update examples

## Type Support Matrix

### Current (Phase 6)
- ✅ int64
- ✅ double
- ✅ class pointers (void*)
- ❌ float
- ❌ bool
- ❌ char
- ❌ string
- ❌ arrays of arrays

### After Redesign (Phase 7)
- ✅ int64
- ✅ double
- ✅ float
- ✅ bool
- ✅ char
- ✅ string
- ✅ class pointers
- ✅ arrays of arrays (multi-dimensional)
- 🔜 function types (future)
- 🔜 interface types (future)

## Code Generation Example

**Input hoo code:**
```hoo
func test() -> void {
    var ints = [1, 2, 3];
    var floats = [1.5, 2.5];
    var matrix = [[1, 2], [3, 4]];
    return;
}
```

**Generated LLVM (NEW):**
```llvm
define void @test() {
entry:
  %hoo_arr_ints = call ptr @hoo_array_new()
  call i64 @hoo_array_push_int64(ptr %hoo_arr_ints, i64 1)
  call i64 @hoo_array_push_int64(ptr %hoo_arr_ints, i64 2)
  call i64 @hoo_array_push_int64(ptr %hoo_arr_ints, i64 3)

  %hoo_arr_floats = call ptr @hoo_array_new()
  call i64 @hoo_array_push_double(ptr %hoo_arr_floats, double 1.5)
  call i64 @hoo_array_push_double(ptr %hoo_arr_floats, double 2.5)

  %hoo_arr_matrix = call ptr @hoo_array_new()

  %hoo_arr_row1 = call ptr @hoo_array_new()
  call i64 @hoo_array_push_int64(ptr %hoo_arr_row1, i64 1)
  call i64 @hoo_array_push_int64(ptr %hoo_arr_row1, i64 2)
  call i64 @hoo_array_push_array(ptr %hoo_arr_matrix, ptr %hoo_arr_row1)

  %hoo_arr_row2 = call ptr @hoo_array_new()
  call i64 @hoo_array_push_int64(ptr %hoo_arr_row2, i64 3)
  call i64 @hoo_array_push_int64(ptr %hoo_arr_row2, i64 4)
  call i64 @hoo_array_push_array(ptr %hoo_arr_matrix, ptr %hoo_arr_row2)

  ret void
}
```

## Benefits

1. **True Type Safety** - std::any provides type checking
2. **Multi-Dimensional** - Natural support via nested arrays
3. **Flexible Sizing** - No element size constraints
4. **Better Performance** - No memcpy overhead for each operation
5. **Future-Ready** - Can support function types, interface types
6. **Cleaner API** - Type-specific functions more intuitive
7. **Better Debugging** - Type information at runtime

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| std::list overhead (pointers) | Profile and optimize if needed; consider std::vector hybrid |
| std::any_cast performance | Use fast-path for common types |
| Breaking changes | Maintain wrapper functions with same names |
| Migration complexity | Phased approach with parallel testing |

## Timeline

- **Phase 7.1-7.2**: Header & implementation redesign (2-3 days)
- **Phase 7.3**: Code generator updates (2-3 days)
- **Phase 7.4**: Testing & verification (2-3 days)
- **Phase 7.5**: Documentation & cleanup (1-2 days)

## Success Criteria

- ✅ All 630+ existing tests pass
- ✅ 50+ new redesign tests pass
- ✅ Support float, bool, char, string, arrays in arrays
- ✅ No performance regression
- ✅ Clean compilation with no warnings
- ✅ Documented with examples

---

**Next Step**: Begin Phase 7.1 - Design and implement new header file
