# ISSUE-003: Missing Scope Statement Implementation

## 1. Overview
The `scope { ... }` block statement allows for explicit lifetime management and local variable shadowing. No `ScopeStatement` AST node or parser rule exists — scope semantics are implemented at the JIT execution level via existing `Block` handling.

## 2. Implementation
- **Approach**: Scope variable release is handled directly in `visitStatement` for `Block` nodes, not via a separate grammar rule or AST node type.
- **Mechanism**: Before popping a scope stack entry, iterate over all locals in that scope and emit `_F_hoo_release_v_p` for any managed-type variable (typeId >= 100: strings, arrays, maps, class instances, etc.).
- **No grammar changes**: The `statement` grammar rule was not extended; existing `{ }` blocks automatically get scope-level release.

## 3. Limitations
- Variables declared in the outermost function scope (function parameters) are not released — requires proper ARC tracking per ISSUE-007.
- Return statements inside blocks skip scope releases for that block's locals (dead code after RET). Full ARC (ISSUE-007) would resolve this.
- Reassigning a managed variable does not release the old value — requires proper ARC on assignment (ISSUE-007).

## 4. Related HashMap/`any` Ownership Plan
The native `HashMap` implementation plan is tracked separately in ISSUE-033. That plan already uses `any` as the value type for heterogeneous maps (`HashMap<K, any>`), with `any` represented as a tagged `(type_id, data)` value.

Scope-level release from this issue applies to local variables whose declared/inferred type is managed (`typeId >= 100`), including local `HashMap` handles when that type is implemented. Ownership of values stored inside `HashMap<K, any>` is handled by the HashMap runtime plan itself: managed `any` values are retained on insertion and released on removal, overwrite, clear, or map release.

## 5. Status
- **Date**: 2026-05-24 (opened), 2026-06-10 (fixed)
- **Status**: **FIXED** (scope-level release in Block visitor)
- **Priority**: Low
- **Audit 2026-06-21**: Verified block-scope managed-local release is still implemented in `HVMCodeGenerator` block handling. Remaining ownership gaps are still tracked under ISSUE-007 and ISSUE-012.
