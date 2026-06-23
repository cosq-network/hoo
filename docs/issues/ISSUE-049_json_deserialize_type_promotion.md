# ISSUE-049: JSON Deserialization Does Not Reverse Serialization Type Promotion

## 1. Overview
The `@Serializable` code generator's `serializeFieldTypeId()` promotes certain field types to wider JSON-compatible representations, but `emitDeserializeMethod()` does not reverse these promotions. This causes round-trip corruption for `int8`, `byte`, `f8`, `bit`, and `buffer` fields.

## 2. Technical Analysis
- **Location**: `src/codegen/HVMCodeGenerator.cpp:4266-4491`

**Serialization promotion** (`serializeFieldTypeId`, line 4266):
| Field type | Promoted to | Rationale |
|------------|-------------|-----------|
| `int8` / `byte` | `HOO_TYPE_INT64` (1) | JSON only has number type |
| `f8` | `HOO_TYPE_FLOAT64` (2) | JSON only has number type |
| `bit` | `HOO_TYPE_BOOL` (3) | JSON uses true/false |
| `Buffer` | `HOO_TYPE_STRING` (101) | Base64-encoded in JSON |

**Deserialization** (`emitDeserializeMethod`, line 4388):
- Calls `hoo_hashmap_get_any_data_i8` to extract the raw value.
- Stores the result directly via `ST_D` into the field offset.
- No type conversion is performed.

## 3. Impact
- `int8` field serialized as `INT64` and stored back — produces wrong value range, possible overflow.
- `Buffer` field serialized as base64 `string` and stored back — the field will hold a string pointer instead of a Buffer, causing undefined behavior.
- `bit` field serialized as `bool` — the stored value is wrong.
- Any existing serialized JSON data will produce corrupted objects on deserialization.

## 4. Suggested Fix
1. In `emitDeserializeMethod`, emit a type check or conversion after extracting each field from the HashMap.
2. For `int8`/`byte` fields: truncate the deserialized `INT64` value to 8 bits.
3. For `f8` fields: convert the deserialized `FLOAT64` value to FP8 representation.
4. For `bit` fields: convert boolean to 0/1.
5. For `Buffer` fields: base64-decode the deserialized string back into a Buffer.
6. Add round-trip tests for all promoted field types.

## 5. Status
- **Date**: 2026-06-23
- **Status**: **PROPOSED**
- **Priority**: **HIGH**
