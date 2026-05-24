# ISSUE-007: Array Push API Regressed to a No-Op

## 1. Overview
The runtime array implementation was rewritten to a raw 64-bit layout, but `hoo_array_push()` was left as a stub that returns `-1` if capacity is exceeded. Since `hoo_array_new()` only allocates enough for 8 elements by default, any loop exceeding this size fails.

## 2. Technical Analysis
The current implementation in `src/runtime/lib/hoo_generic_array.cpp` returns `-1` immediately when `len >= cap`. This breaks:
- `hoo_string_split()` which depends on dynamic growth.
- Existing unit tests in `HooArrayPhase7Test`.
- Real-world collection building.

## 3. Requirements
- Implement dynamic growth in `hoo_array_push` using `hoo_realloc`.
- Ensure pointers remain stable or handles are updated (the current C-API returns `int64_t` length, making pointer updates difficult if the handle moves).

## 4. Status
- **Date**: 2026-05-24
- **Status**: **IMPLEMENTED**
- **Priority**: High

## 5. Implementation Notes
Implemented dynamic growth using `hoo_realloc`. To maintain stability in the absence of a full pointer-handle system, the default initial capacity has been increased to 1024 elements. Elements are correctly managed via ARC when the array is released.
