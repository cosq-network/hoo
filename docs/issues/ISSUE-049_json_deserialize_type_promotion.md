# ISSUE-049: JSON Deserialization Does Not Reverse Serialization Type Promotion

## 1. Overview
Historically, the `serializable` code generator promoted certain field types
to wider JSON-compatible representations without fully reversing them during
deserialization. This document records the issue and its completed fix.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp:4266-4491`

**Serialization promotion** (`serializeFieldTypeId`, line 4266):
| Field type | Promoted to | Rationale |
|------------|-------------|-----------|
| `int8` / `byte` | `HOO_TYPE_INT64` (1) | JSON only has number type |
| `f8` | `HOO_TYPE_FLOAT64` (2) | JSON only has number type |
| `bit` | `HOO_TYPE_BOOL` (3) | JSON uses true/false |
| `Buffer` | `HOO_TYPE_BUFFER` (113) | Tagged Base64 object in JSON |

**Deserialization** now calls `hoo_hashmap_get_any_data_i8` and applies the
declared-field conversion before `ST_D`: narrow integer truncation and sign
extension, bit masking, FP8 conversion, and dedicated buffer reconstruction.

## 3. Impact
- These corruptions were possible before the ISSUE-035 fix; regression tests now
  cover the corrected buffer path and the generated scalar conversion paths.
- Any existing serialized JSON data will produce corrupted objects on deserialization.

## 4. Suggested Fix
1. In `emitDeserializeMethod`, emit a type check or conversion after extracting each field from the HashMap.
2. For `int8`/`byte` fields: truncate the deserialized `INT64` value to 8 bits.
3. For `f8` fields: convert the deserialized `FLOAT64` value to FP8 representation.
4. For `bit` fields: convert boolean to 0/1.
5. For `Buffer` fields: base64-decode the deserialized string back into a Buffer.
6. Add round-trip tests for all promoted field types.

## 5. Status
- **Date**: 2026-08-06
- **Status**: **FIXED**
- **Priority**: **HIGH**

## 6. Resolution

Resolved as part of ISSUE-035. Generated deserialization now reverses the
`int8`/`byte`, `f8`, and `bit` promotions. Buffers use runtime type ID 113 and
tagged Base64 JSON instead of being misidentified as strings. Runtime tests
cover buffer round-trips, and the full suite passes with 2062 tests.
