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

## 4. Status
- **Date**: 2026-05-24
- **Status**: **PARTIALLY IMPLEMENTED**
- **Priority**: Medium

## 5. Updates
- **Update 2026-06-17**: The type system has been significantly expanded to support `f8`, `bit`, and `tensor` data types. 
- **Type Registration**: `HVMCodeGenerator::generateModule` now pre-registers top-level function return types and class names in `functionReturnTypes_` and `functionReturnClass_` maps, allowing for correct symbol mangling and type inference during cross-module and forward-referenced calls.
- **Symbol Mangling**: `SymbolMangler` now correctly encodes/decodes `f8` (e), `bit` (x), and `tensor` (t) types, preserving high-level type metadata in the HVM symbol table.
- **Remaining Gap**: The `SHT_TYPE` section and native sub-word memory protection (LD.B/ST.B for all scalar operations) are still pending (see ISSUE-027 and ISSUE-028).
