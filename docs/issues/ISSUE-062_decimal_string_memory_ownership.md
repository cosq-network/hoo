# ISSUE-062: Decimal toString() Memory Ownership Unclear

## Status
- **Date**: 2026-07-19
- **Status**: **CLOSED**
- **Priority**: 🟡 **P2 - MEDIUM** (Plan for next sprint - memory management)
- **Sprint**: Week 4 (Day 1)
- **Estimate**: 1 day
- **Related**: ISSUE-059

---

## 1. Overview
The `hoo_decimal_to_string()` function in `src/runtime/lib/hoo_decimal.cpp` allocates memory that the caller must free, but there's no clear ownership documentation or ARC integration. This can lead to memory leaks or use-after-free bugs.

## 2. Technical Analysis

### Current Implementation (hoo_decimal.cpp:234-265)
```c
extern "C" char* hoo_decimal_to_string(HooDecimal d) {
    if (!d) {
        char* empty = static_cast<char*>(std::malloc(1));
        empty[0] = '\0';
        return empty;
    }
    // ...
    char* result = static_cast<char*>(std::malloc(len + 2));
    // ...
    return result;
}
```

### Problems
1. **Raw malloc without ARC**: Returns `malloc()`-allocated memory, not ARC-managed
2. **No ownership documentation**: Header doesn't specify who must free the result
3. **Inconsistent with other APIs**: String functions return ARC-managed `HooString`
4. **Risk of leaks**: Hoo code may not know to call `free()` on the result
5. **Risk of double-free**: If both Hoo and C code try to free

### Comparison with Other APIs
```c
// hoo_string.h - Returns ARC-managed string
HooString hoo_string_from_cstr(const char* cstr);

// hoo_decimal.h - Returns raw malloc'd memory
char* hoo_decimal_to_string(HooDecimal d);  // Inconsistent!
```

## 3. Requirements
1. Document memory ownership clearly in header comments
2. Consider returning ARC-managed `HooString` instead of raw `char*`
3. Or provide a paired `hoo_decimal_to_string_free()` function
4. Ensure Hoo language integration handles cleanup correctly

## 4. Implementation Options

### Option A: Return ARC-Managed String (Recommended)
```c
HooString hoo_decimal_to_string(HooDecimal d);
```

### Option B: Provide Paired Free Function
```c
char* hoo_decimal_to_string(HooDecimal d);
void hoo_decimal_to_string_free(char* str);
```

### Option C: Document Ownership Clearly
```c
/**
 * Convert decimal to string.
 * @return Caller must free() the returned string.
 */
char* hoo_decimal_to_string(HooDecimal d);
```

## 5. Impact
- Current: Memory management is ambiguous, leading to potential leaks
- After fix: Clear ownership semantics prevent memory issues

## 6. Test Coverage
- Test that returned string can be freed without crash
- Test that Hoo code properly manages returned string
- Test multiple calls don't leak memory
- Test null/empty decimal handling
