# ISSUE-007: Array Push API Regressed to a No-Op

## 1. Overview
The runtime array implementation was rewritten to a raw 64-bit layout, but `hoo_array_push()` was left as a permanent stub that always returns `-1`. That breaks every API path that depends on append semantics.

## 2. Technical Analysis
The current implementation in `src/runtime/lib/hoo_generic_array.cpp` still exposes a full dynamic array API surface:

- `hoo_array_push_int64`
- `hoo_array_push_double`
- `hoo_array_push_float`
- `hoo_array_push_bool`
- `hoo_array_push_char`
- `hoo_array_push_string`
- `hoo_array_push_object`
- `hoo_array_push_array`

However, every one of these wrappers delegates to `hoo_array_push()`, and `hoo_array_push()` now returns `-1` without modifying the array.

This regresses multiple existing call paths:

- `hoo_string_split()` appends split pieces with `hoo_array_push()`
- array phase tests expect append growth to succeed
- any future lowering that emits push-based collection building will fail at runtime

## 3. Impact
- `hoo_string_split()` produces an empty or partially initialized result.
- Any code path that builds arrays incrementally becomes unusable.
- The new array layout is not a drop-in replacement for the previous vector-like behavior.

## 4. Suggested Fixes
- Restore append semantics in `hoo_array_push()` using a growable backing store.
- If the raw fixed-size layout is intentional, introduce a separate constructor API for fixed arrays and keep the old push-based API behavior intact.
- If the runtime does not yet support `realloc`, use a capacity header and grow by allocating a larger block, copying existing elements, and releasing the old block.
- Add or update tests for:
  - `hoo_string_split()`
  - `hoo_array_push_*()` success cases
  - push on nested arrays

## 5. Status
- **Date**: 2026-05-25
- **Status**: **TODO (REGRESSION)**
- **Priority**: High
