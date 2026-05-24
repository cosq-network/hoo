# HVM JIT Phase 3 Exit Checklist

Date: 2026-05-24

This checklist is the practical go/no-go gate for moving from Phase 3
("Runtime Bridge & Intrinsics") to Phase 4.

## Gate Criteria

- [x] Runtime bridge bootstraps with mandatory symbols available.
- [x] ARC intrinsics (`alloc/retain/release/refcount/typeid`) are callable and covered by unit tests.
- [x] `ST.D` retain/store/release sequence behavior is covered by unit tests.
- [x] Exception bridge path invokes runtime throw/rethrow hooks before control transfer.
- [x] Handler registration/unregistration and throw-to-handler transfer paths are covered by unit tests.
- [x] Extended runtime symbols for string/array/map bridge are exported and covered by loader tests.
- [x] JIT shadow exception state is cleaned up per-run to avoid cross-test contamination.
- [x] CTest preset run is deterministic in this environment (`ctest --preset macos-homebrew-ninja` is green).

## Current Decision

Status: **Pass**

Phase 3 exit criteria are satisfied in the current workspace, and preset-level
determinism is currently observed.
