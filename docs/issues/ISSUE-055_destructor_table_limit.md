# ISSUE-055: Destructor Table Size Limited to 256 Type IDs

## 1. Overview
The destructor registration table is a fixed-size array of 256 entries. With type IDs ranging from 100 (generic object) up to 133 (UV handle for async), this imposes an artificial limit. If the number of unique managed types exceeds 256, destructor registration silently overwrites or becomes impossible.

## 2. Technical Analysis
- **Location**: `src/runtime/lib/hoo_runtime.c:81`
- **Current**: `#define HOO_DESTRUCTOR_TABLE_SIZE 256`
- **Usage**: The destructor table is indexed by `typeId` directly — type IDs above 255 cannot be stored.
- **Current type ID range**: 100–133 (2026-06-23), leaving only ~123 more slots.
- **Risk**: User-defined classes add a new type ID per class; even a moderate number of user classes will exhaust the table.

## 3. Impact
- Type IDs above 255 produce silent memory corruption or missed destructor calls.
- User-defined class types beyond slot 256 will leak managed resources.
- Future intrinsic types (async handles, additional built-ins) compete for the same limited space.

## 4. Suggested Fix
1. Use a dynamic data structure (e.g., `std::unordered_map<int64_t, DestructorFn>`) instead of a fixed array.
2. Alternatively, increase the table size significantly (e.g., 4096 or 65536) as a short-term fix.
3. Add a bounds check and clear error when registration exceeds capacity.

## 5. Status
- **Date**: 2026-06-23
- **Status**: **PROPOSED**
- **Priority**: **MEDIUM**
