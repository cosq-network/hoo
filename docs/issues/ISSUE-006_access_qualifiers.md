# ISSUE-006: Access Qualifier Implementation (Public/Private)

## 1. Overview
The Hoo language supports `public` and `private` access qualifiers for fields and methods. The implementation enforces access during compilation and preserves method visibility through HVM module generation and JIT linking.

## 2. Technical Analysis
The original implementation had three gaps, all now resolved:
1.  **Mangling**: `HVMCodeGenerator::beginFunction` propagates `PUBLIC` and `PRIVATE` into `MangledFunctionParams`, producing `_Pb` and `_Pv` symbol tags.
2.  **Metadata**: `FunctionPrologueInfo` carries private visibility through function generation.
3.  **Binding**: `HVMCodeGenerator::endFunction` emits private methods as `Symbol::STB_LOCAL` and public/default methods as `Symbol::STB_GLOBAL`.

## 3. Requirements & Lowering Suggestions
- **Compiler checks**: Reject reads/writes of private fields and calls to private methods outside the declaring class (or supported derived-class context).
- **Symbol lowering**: Preserve method modifiers in mangled names and emit private methods with local HVM binding.
- **JIT/linker enforcement**: Do not export local symbols, reject imports targeting them, ignore them during interpreter cross-module resolution, and use LLVM internal linkage during JIT translation.

## 4. Status
- **Date**: 2026-05-28
- **Status**: **IMPLEMENTED**
- **Priority**: Medium
- **Audit 2026-08-05**: Source-level checks and HVM/JIT module-boundary visibility enforcement are implemented and covered by regression tests.

## 5. Verification

- Compiler regression coverage verifies `_Pb`/`_Pv` mangling and `STB_GLOBAL`/`STB_LOCAL` bindings.
- Existing compiler tests cover public access, private access rejection, same-class access, derived-class access, and unrelated-class rejection.
- Full CTest suite passes with 0 failures.

## 6. Implementation Notes
- **Fields**: `isPrivate()`/`isPublic()` is read for every field during class layout; all read and write access sites are checked against `fieldAccess` and emit a compile error on violation.
- **Methods**: `fn->isPrivate()` is indexed into `ClassLayout::privateMethods`; call sites check this map and emit a compile error when a private method is invoked from outside its declaring class.
- **Linker/JIT enforcement**: Private methods are emitted with `STB_LOCAL` binding and public/default methods remain `STB_GLOBAL`. Local symbols are excluded from the module export registry, imports targeting them are rejected during dependency validation, interpreter cross-module resolution ignores them, and LLVM translation gives them internal linkage.
