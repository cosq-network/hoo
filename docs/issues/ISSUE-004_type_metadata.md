# ISSUE-004: Incomplete Type Metadata and Section Management

## 1. Overview
The HVM backend currently treats all variables as opaque 64-bit values. It does not utilize the type information provided by `ast::Type` nodes, and `const` variables are stored in mutable memory.

## 2. Technical Analysis
- **Const Allocation**: `ast::VariableDeclaration` with `isConstant() == true` should be allocated in the `.rodata` section to allow hardware protection.
- **Primitive Sizes**: `int8` and `byte` should ideally use `ST.B` and `LD.B` for memory efficiency, rather than always promoting to 64-bit `LD.D`.
- **Type Metadata**: `MapType` and `OptionalType` information is lost during codegen, preventing the JIT from performing advanced optimizations or safety checks.

## 3. Requirements & Lowering Suggestions
- Update `generateModule` to check `decl->isConstant()` and select the correct section (`.rodata` vs `.data`).
- Implement the `generateType` method to emit type metadata into a `SHT_TYPE` section in the HVM module.

## 5. Status
- **Date**: 2026-06-27
- **Status**: **COMPLETED**
- **Priority**: Medium
- **Audit 2026-06-27**: The code now successfully allocates `isConstant()` global variables to the `.rodata` section for hardware-level read-only protection. Additionally, the `generateType` method has been implemented to serialize type metadata as strings into a newly created `.types` section with the `SHT_TYPES` identifier.

Note: Native sub-word memory protection (`LD.B`/`ST.B`) implementation is tracked in ISSUE-027 and ISSUE-028.
