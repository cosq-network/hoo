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
- **Status**: **PROPOSED**
- **Priority**: **LOW**
