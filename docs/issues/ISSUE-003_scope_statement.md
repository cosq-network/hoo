# ISSUE-003: Missing Scope Statement Implementation

## 1. Overview
The `scope { ... }` block statement allows for explicit lifetime management and local variable shadowing. No `ScopeStatement` AST node or parser rule exists yet — this is both a grammar and codegen gap.

## 2. Technical Analysis
While `HVMCodeGenerator` currently uses a flat function-level stack allocation strategy, a `scope` block should eventually trigger local variable cleanup (ARC release) at the end of the block.

## 3. Requirements & Lowering Suggestions
- Add `ast::ScopeStatement` to the `visitStatement` logic.
- **Short-term**: Simply visit the inner block.
- **Long-term**: Track variables declared within the scope and emit `_F_hoo_release_v_p` calls for all managed objects before leaving the scope.

## 4. Status
- **Date**: 2026-05-24
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: Low
