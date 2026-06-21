# ISSUE-036: Module Dependency Resolution Can Produce Incorrect Order and False Cycle Reports

## 1. Overview
The current implementation of `HOModuleBase::resolveDependencyOrder()` appears to walk the dependency graph incorrectly for modules that are not the current one. The algorithm relies on `findDependency(name)` against the current module instead of consulting the actual module object for the dependency name, which can produce wrong topological orderings and incorrect cycle detection results.

This was explicitly called out in the test suite: the module bundle tests note that `resolveDependencyOrder` has a known issue with cycle detection that causes false positives for simple dependency chains.

## 2. Technical Analysis
The relevant logic lives in `src/hvm/HOModuleBase.cpp`.

The current implementation builds a local `module_map` from `all_modules`, but the recursive `visit()` routine does this:

```cpp
auto dep = findDependency(name);
if (dep) {
    visit(dep->module_name);
}
```

That logic is suspicious because `findDependency(name)` checks `dependencies_` of the current module, not the module whose name is being visited. For a dependency chain like:

- `A` depends on `B`
- `B` depends on `C`

`visit("B")` will inspect `A`'s dependency list instead of `B`'s list unless `B` happens to be a dependency of `A` with the same name. This can lead to:

- missing transitive edges,
- incorrect dependency order, and
- false positives when the routine thinks it has found a cycle.

The issue is visible in the test comments for `tests/hvm/HVMModuleBundleTest.cpp`:

- `HOModuleBase::resolveDependencyOrder` has a known issue with cycle detection.
- The tests avoid triggering the bug by using self-dependency cases.

## 3. Requirements
1. Rework dependency traversal so that each visited module name resolves to the corresponding module object and its own dependency list.
2. Ensure cycle detection uses a proper graph walk over the full dependency graph, not the current module's local list.
3. Add regression tests that cover:
   - a simple three-module chain (`A -> B -> C`),
   - a real cycle (`A -> B -> C -> A`), and
   - a module that is referenced only transitively.
4. Verify that `hasCircularDependency()` and `resolveDependencyOrder()` agree on all cases.

## 4. Impact
- Incorrect module loading order in the HVM loader/bundle path.
- Potentially missing or duplicate symbols during resolution.
- False cycle reports that can block valid programs.

## 5. Status
- **Date**: 2026-06-19
- **Status**: **PROPOSED**
- **Priority**: Medium (graph correctness and linker behavior)
- **Audit 2026-06-21**: Verified `HOModuleBase::resolveDependencyOrder` still resolves dependency names through the current module lookup path, and the bundle test still documents the known unresolved dependency-order bug.
