# ISSUE-006: Incomplete Access Qualifier Implementation (Public/Private)

## 1. Overview
The Hooc language supports `public` and `private` access qualifiers for functions and methods. While these are correctly parsed into `ast::FunctionModifier` nodes, the `HVMCodeGenerator` does not enforce these boundaries or correctly lower them to the HVM symbol table.

## 2. Technical Analysis
There are three critical failures in the current implementation:
1.  **Mangling**: `HVMCodeGenerator::beginFunction` does not extract modifiers from the AST node. Consequently, the `MangledFunctionParams` passed to `SymbolMangler` have an empty `functionModifiers` list, resulting in names missing the `_Pb` (Public) or `_Pv` (Private) tags.
2.  **Metadata Loss**: The `FunctionPrologueInfo` struct does not carry visibility information from the beginning to the end of the function generation process.
3.  **Hardcoded Binding**: `HVMCodeGenerator::endFunction` hardcodes all function symbols to `Symbol::STB_GLOBAL`. This prevents the HVM linker/JIT from enforcing internal-only visibility for private members.

## 3. Requirements & Lowering Suggestions
- **Update `beginFunction`**:
    - Iterate through `decl->getModifiers()` and populate `mp.functionModifiers`.
    - Detect if `FunctionModifier::PRIVATE` is present and store this state in `FunctionPrologueInfo`.
- **Update `endFunction`**:
    - Use the visibility state from `FunctionPrologueInfo` to set `sym.binding`.
    - If private, set `sym.binding = Symbol::STB_LOCAL` (if supported by the HVM spec) or ensure the mangled name sufficiently isolates it.
- **Enforcement**: The JIT/Linker should be updated to reject `CALL` instructions targeting `STB_LOCAL` symbols from external modules.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **TODO (PARTIALLY IMPLEMENTED / BROKEN)**
- **Priority**: Medium
