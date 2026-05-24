# ISSUE-009: Raw Array Rewrite Drops ARC Ownership Semantics

## 1. Overview
The raw array rewrite removed the previous element-ownership behavior from `HooArray`. Arrays now store `int64_t` slots directly, but managed handles are written into those slots without retain/release bookkeeping.

## 2. Technical Analysis
The current array implementation no longer preserves the old semantic contract:

- `hoo_array_release()` no longer releases contained elements.
- `hoo_string_to_characters()` stores `HooCharacter` handles directly into the raw array.
- `hoo_array_push_array()` retains nested arrays before appending, but there is no matching element release path on array destruction.
- The array API still exposes object-oriented operations such as `hoo_array_push_object()` and `hoo_array_get_object()`, but the implementation now treats all elements as untyped 64-bit values.

This means the runtime can no longer guarantee:

- nested arrays are released correctly
- character objects are released after conversion
- object handles remain balanced across append/remove lifetimes

## 3. Impact
- Memory leaks are introduced for arrays containing managed objects.
- Nested arrays can retain indefinitely after parent array destruction.
- The raw-array format undermines the ARC model used elsewhere in the runtime.
- Conversions such as string-to-characters become leak-prone even when the caller behaves correctly.

## 4. Suggested Fixes
- Restore typed ownership tracking in the array representation.
- Store an element kind or ownership policy in the array header so release can recurse safely.
- If the array is intentionally raw, then remove or clearly separate managed-handle array APIs from plain `int64_t` arrays.
- Make `hoo_string_to_characters()` either:
  - retain each `HooCharacter` before storage and release them during array destruction, or
  - return a typed primitive representation instead of managed handles.
- Add leak-focused tests for:
  - `hoo_string_to_characters()`
  - `hoo_array_push_array()`
  - releasing arrays containing objects, strings, and nested arrays

## 5. Status
- **Date**: 2026-05-25
- **Status**: **TODO (REGRESSION)**
- **Priority**: Medium
