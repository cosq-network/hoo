# ISSUE-008: Interpolated String Type Resolution Gaps

## 1. Overview
The `HVMCodeGenerator` currently only correctly identifies Type IDs for literal values in interpolated strings. When interpolation involves variables or parameters, it defaults to `HOO_TYPE_OBJECT` (100).

## 2. Technical Analysis
The `getTypeId` helper in `HVMCodeGenerator.cpp` needs to be more robust. While it has basic support for literals and some local variable lookup, it does not handle:
- Chained member access (e.g. `"${obj.field}"`).
- Function returns.
- Nested binary expressions.

## 3. Requirements
- Enhance `getTypeId` to perform basic semantic analysis or use existing frontend type metadata if available.
- Ensure primitives (`int64`, `double`, `bool`) are correctly mapped to their respective IDs (1, 2, 3) to trigger correct string conversion in the runtime.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **DONE**
- **Priority**: Medium

## 5. Implementation Notes
Implemented a robust `getTypeId` helper in `HVMCodeGenerator` that unwraps `PrimaryExpression` and performs local variable lookup to infer runtime Type IDs. This allows non-literal values (variables, parameters) to be correctly stringified during interpolation.
