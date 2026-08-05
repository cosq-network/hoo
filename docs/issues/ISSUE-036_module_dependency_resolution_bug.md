# ISSUE-036: Module Dependency Resolution Can Produce Incorrect Order and False Cycle Reports

## 1. Overview
The historical implementation of `HOModuleBase::resolveDependencyOrder()` walked the dependency graph through the current module's local lookup path. That could produce incorrect transitive ordering and cycle results.

The defect was addressed by resolving each visited module name through the complete module map and traversing that module's own dependency list.

## 2. Technical Analysis
The relevant logic lives in `src/hvm/HOModuleBase.cpp` and
`src/hvm/HVMModuleBundle.cpp`.

The corrected traversal builds a local `module_map` from `all_modules` and
recursively visits the dependency list belonging to the module being visited:

```cpp
auto module_it = module_map.find(name);
if (module_it != module_map.end() && module_it->second) {
    for (const auto& dep : module_it->second->getDependencies()) {
        visit(dep.module_name);
    }
}
```

For a dependency chain like:

- `A` depends on `B`
- `B` depends on `C`

the corrected `visit("B")` inspects `B`'s dependency list. The traversal now uses explicit visiting/visited states, preventing:

- missing transitive edges,
- incorrect dependency order, and
- false-positive cycle reports.

The regression coverage is in `tests/hvm/HVMModuleBundleTest.cpp` and
`tests/hvm/HVMJITLoaderTest.cpp`.

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
- **Date**: 2026-08-06
- **Status**: **IMPLEMENTED**
- **Priority**: Medium (graph correctness and linker behavior)
- **Implementation**: Commits `8e9233b` and `ca2beb7` corrected per-module traversal, explicit cycle-state handling, bundle/per-module cycle agreement, and regression coverage. Commit `5db1684` removed the obsolete `checkCircularDependencies()` helper, leaving `resolveDependencyOrder()` as the single dependency-cycle implementation.
- **Verification**: Three-module chains, transitive-only dependencies, real three-module cycles, self-cycles, and HVMJIT loader rejection all pass. Focused verification: 9 tests passed.

### Remaining design notes

- The dependency order is topologically valid; ordering among unrelated modules
  follows the bundle's current container traversal and is not a serialized ABI.
- Missing optional dependencies are skipped by graph ordering; required-module
  presence is validated by the loader's dependency-resolution phase.
