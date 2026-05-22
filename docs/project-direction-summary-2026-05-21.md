# Project Direction Summary

Date: 2026-05-22  
Context: consolidated direction after recent HVM/documentation refactors

## 1. Direction You Have Been Driving

The project direction is now clear and cohesive:

1. Keep Hooc language/compiler semantics grounded in `src/parsing/Hooc.g4`
2. Keep HVM architecture minimal, grammar-driven, and explicitly scoped
3. Separate optional VM capabilities into extension profiles rather than bloating core
4. Strengthen module/FFI/runtime boundaries and `.ho` artifact behavior

## 2. What Changed Strategically

- HVM moved from a broad “catalog ISA” mindset to a **core-minimalest profile**
- Core docs were synchronized across spec/CSV/reference/register/module-format docs
- Non-core families (SIMD/threading/interrupt/debug/string-op families) were removed from core narratives and pushed to extension scope
- Register/call semantics were normalized (notably `r29` as link register for `RET`)

## 3. Current Strategic Theme

**“Minimal core VM + explicit extension profiles + strong compiler/runtime alignment.”**

This lowers ambiguity, improves testability, and makes AOT/module work more predictable.

## 4. Likely Next High-Value Milestones

1. Finalize end-to-end AOT workflow (`.hoo -> .ho -> run`)
2. Add integration coverage for module loading/linking + FFI bridge behavior
3. Introduce capability-gated optional profiles only when required by grammar/runtime evolution
4. Continue standard-library growth via runtime and module APIs, not core ISA expansion

## 5. Practical Guardrails Going Forward

- Any grammar change should trigger synchronized AST/lowering/codegen/doc updates
- Any VM feature beyond current grammar needs should start as optional profile documentation
- Avoid introducing new core opcodes where lowering rules already satisfy semantics
