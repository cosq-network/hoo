# ISSUE-054: No Sort/Ordering Support for Generic Array

## 1. Overview
The generic Array runtime implements `push`, `get`, `set`, `insert`, `remove`, `clear`, and `length`, but has no sorting or ordering functions (`sort`, `reverse`, `shuffle`, `orderBy`). There is no comparator callback pattern exposed.

## 2. Technical Analysis
- **Location**: `src/runtime/lib/hoo_generic_array.h`, `src/runtime/lib/hoo_generic_array.cpp`
- **Existing operations**: `push`, `get`, `set`, `insert`, `remove`, `clear`, `length`, `capacity`, `pop`
- **Missing**:
  - `sort()` with optional comparator
  - `reverse()`
  - `shuffle()`
  - `orderBy(keyFn)` for structured data
  - `binarySearch()` for sorted arrays
  - `sortRange(start, end)` for partial sorting
- **CSV exception**: `Csv.sort()` is referenced in codegen (line 3994) but only for the CSV module's return type; not a general Array feature.

## 3. Impact
- Users must implement sorting manually using the limited comparison API.
- Common data-processing patterns (sort, reverse, shuffle) are unavailable.
- CSV operations that reference `sort` cannot use it generically.

## 4. Suggested Fix
1. Add `hoo_generic_array_sort(array, comparator_fn)` using an efficient sort (e.g., introsort).
2. Add `hoo_generic_array_reverse(array)` for in-place reversal.
3. Expose comparator callbacks as function pointers or closures.
4. Provide default comparison for `int64`, `double`, `string` element types.

## 5. Status
- **Date**: 2026-06-23
- **Status**: **COMPLETED**
- **Priority**: **LOW**
- **Sort**: `Array.sort()` — implemented in `hoo_generic_array.cpp:330-344` with type-aware `qsort` (int64 and IEEE 754 double comparison), registered in JIT as `_F_M_hoo_E_array_sort_v(_p)` (HVMJIT.cpp:3788-3789), supported in codegen type inference (HVMCodeGenerator.cpp:3854 returns 102/Array)
- **Reverse**: `Array.reverse()` — implemented in `hoo_generic_array.cpp:346-362` as in-place element swap, registered in JIT as `_F_M_hoo_E_array_reverse_v(_p)` (HVMJIT.cpp:3790-3791), supported in codegen type inference (HVMCodeGenerator.cpp:3854 returns 102/Array)
- **Shuffle**: `Array.shuffle()` — implemented in `hoo_generic_array.cpp` with Fisher-Yates shuffle, registered in JIT as `_F_M_hoo_E_array_shuffle_v_p`.
- **Sort Range**: `Array.sortRange(start, end)` — implemented in `hoo_generic_array.cpp` using bounds-checked sub-array sorting, registered in JIT as `_F_M_hoo_E_array_sortRange_v_p_p_p`.
- **Binary Search**: `Array.binarySearch(value)` — implemented in `hoo_generic_array.cpp` with int64 and double variants, registered in JIT as `_F_M_hoo_E_array_binarySearch_v_p_p`.
- **Tests**: Runtime tests in `tests/runtime/HooArrayPhase7Test.cpp` (SortEmptyArray through ReverseDoesNotReleaseArray, plus new SortRange, Shuffle, BinarySearch tests); JIT tests in `tests/jit/HooArrayJitTest.cpp` (ArraySortInt64 through ArrayReverseSingle, plus ArrayShuffle, ArraySortRange, ArrayBinarySearch).
- **API docs**: Updated in `docs/runtime/api/collections.md` and `docs/runtime/api/index.md`
- **Still missing**: `orderBy(keyFn)` and comparator callback support (requires closure or function pointer types in Hoo, deferred to future runtime enhancements).
