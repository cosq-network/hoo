# ISSUE-009: Runtime Header Misalignment and Bus Error

## 1. Overview
Refactoring `HooObjectHeader` to include a `capacity` field increased its size from 16 to 24 bytes. This breaks 16-byte alignment assumptions on ARM64 platforms (macOS), leading to `Bus error: 10` during memory access.

## 2. Technical Analysis
The payload of a managed object now starts at an offset of 24 bytes from the `malloc`-returned boundary. Many platforms and compilers assume that the returned object pointer is at least 16-byte aligned. If the payload is accessed using instructions that expect 16-byte alignment, the hardware triggers a bus error.

## 3. Requirements
- Re-align the `HooObjectHeader` to a multiple of 16 bytes.
- This can be done by adding padding to make the header exactly 32 bytes.
- Update `hoo_alloc` and `hoo_release` to account for the larger header.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **DONE**
- **Priority**: High

## 5. Implementation Notes
Resolved by expanding `HooObjectHeader` to 32 bytes (4 words) to maintain 16-byte alignment of the payload area. The `HooArray` layout was also synchronized to use a 4-word header (length, capacity, elem_type, reserved), ensuring that array elements start on a 16-byte boundary. All CodeGen paths were updated to emit this new layout.
