# Hoo Project Roadmap

This document provides a high-level overview of the current status of all tracked issues and proposals.

## Completed / Fixed / Implemented

- **[ISSUE-003_scope_statement.md](issues/ISSUE-003_scope_statement.md)**
  - **Title**: # ISSUE-003: Missing Scope Statement Implementation
  - **Status**: FIXED** (scope-level release in Block visitor)
  - **Priority**: Low

- **[ISSUE-005_class_modifiers.md](issues/ISSUE-005_class_modifiers.md)**
  - **Title**: # ISSUE-005: Unimplemented Class Modifiers
  - **Status**: IMPLEMENTED (EXCEPTIONAL JIT-FLOW VERIFICATION PENDING)
  - **Priority**: Medium

- **[ISSUE-006_access_qualifiers.md](issues/ISSUE-006_access_qualifiers.md)**
  - **Title**: # ISSUE-006: Incomplete Access Qualifier Implementation (Public/Private)
  - **Status**: IMPLEMENTED (SOURCE-LEVEL), LINKER BINDING GAP REMAINS
  - **Priority**: Medium

- **[ISSUE-008_short_circuit_eval.md](issues/ISSUE-008_short_circuit_eval.md)**
  - **Title**: # ISSUE-008: Missing Short-Circuit Evaluation for `&&` and `||`
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-009_string_concat_operator.md](issues/ISSUE-009_string_concat_operator.md)**
  - **Title**: # ISSUE-009: `+` on String Operands Produces Pointer Arithmetic, Not Concatenation
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-010_singleton_constructor_never_called.md](issues/ISSUE-010_singleton_constructor_never_called.md)**
  - **Title**: # ISSUE-010: Singleton Constructor Never Called
  - **Status**: FIXED
  - **Priority**: HIGH

- **[ISSUE-011_bounds_checking.md](issues/ISSUE-011_bounds_checking.md)**
  - **Title**: # ISSUE-011: Missing Runtime Bounds Checking for Array Access
  - **Status**: FIXED
  - **Priority**: HIGH

- **[ISSUE-012_finally_block_safety.md](issues/ISSUE-012_finally_block_safety.md)**
  - **Title**: # ISSUE-012: Finally Block Not Guaranteed to Execute
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-016_jit_memory_corruption.md](issues/ISSUE-016_jit_memory_corruption.md)**
  - **Title**: # ISSUE-016: JIT Memory Safety Issues
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-023_missing_return_check.md](issues/ISSUE-023_missing_return_check.md)**
  - **Title**: # ISSUE-023: Missing Return Value Validation for Non-Void Functions
  - **Status**: FIXED
  - **Priority**: MEDIUM

- **[ISSUE-024_char_literal_vs_raw_char.md](issues/ISSUE-024_char_literal_vs_raw_char.md)**
  - **Title**: # ISSUE-024: Character Literals Passed as Raw Char Parameters Create Object Pointers
  - **Status**: FIXED** (char key support removed from Hoo language layer)
  - **Priority**: LOW

- **[ISSUE-032_float_modulo_bug.md](issues/ISSUE-032_float_modulo_bug.md)**
  - **Title**: # ISSUE-032: Invalid Modulo Lowering for Floating Point Types
  - **Status**: FIXED
  - **Priority**: Medium (Bug in mathematical correctness)

- **[ISSUE-033_hashmap_intrinsic.md](issues/ISSUE-033_hashmap_intrinsic.md)**
  - **Title**: # ISSUE-033: Implementation Plan for Native `HashMap`, `AnyArray`, and `any` Intrinsic Types
  - **Status**: IMPLEMENTED - CORE RUNTIME, GRAMMAR, AST, CODEGEN, JIT BRIDGE, AND TEST COVERAGE
  - **Priority**: HIGH** (Fundamental heterogeneous collection and type expansion)

- **[ISSUE-038_repl_integration_plan.md](issues/ISSUE-038_repl_integration_plan.md)**
  - **Title**: # ISSUE-038: Interactive REPL Integration Plan
  - **Status**: IMPLEMENTED
  - **Priority**: Medium

- **[ISSUE-041_function_overloading.md](issues/ISSUE-041_function_overloading.md)**
  - **Title**: # ISSUE-041 Function Overloading Support
  - **Status**: IMPLEMENTED
  - **Priority**: High

- **[ISSUE-045_semantic_versioning.md](issues/ISSUE-045_semantic_versioning.md)**
  - **Title**: # ISSUE-045 Semantic Versioning, Version Bumping, Linux Build Pipelines & GitHub Release Workflow
  - **Status**: IMPLEMENTED
  - **Priority**: UNKNOWN

- **[ISSUE-051_integer_overflow_not_checked.md](issues/ISSUE-051_integer_overflow_not_checked.md)**
  - **Title**: # ISSUE-051: Integer Overflow Not Checked in JIT Arithmetic
  - **Status**: RESOLVED
  - **Priority**: MEDIUM

- **[ISSUE-056_csv_methods_missing_from_runtime.md](issues/ISSUE-056_csv_methods_missing_from_runtime.md)**
  - **Title**: # ISSUE-056: CSV Methods Referenced in Codegen But Missing From Runtime
  - **Status**: FIXED** — all methods are fully implemented and wired through runtime, JIT, and codegen.
  - **Priority**: MEDIUM

- **[ISSUE-057_cross_file_local_imports_ha_archive.md](issues/ISSUE-057_cross_file_local_imports_ha_archive.md)**
  - **Title**: # ISSUE-057: Cross-File Local Imports and Hoo Archive (`.ha`) Format
  - **Status**: COMPLETED
  - **Priority**: HIGH

- **[ISSUE-054_array_sort_missing.md](issues/ISSUE-054_array_sort_missing.md)**
  - **Title**: # ISSUE-054: No Sort/Ordering Support for Generic Array
  - **Status**: COMPLETED
  - **Priority**: LOW

- **[ISSUE-004_type_metadata.md](issues/ISSUE-004_type_metadata.md)**
  - **Title**: # ISSUE-004: Incomplete Type Metadata and Section Management
  - **Status**: COMPLETED
  - **Priority**: Medium

## Completed / Fixed / Implemented

- **[ISSUE-007_memory_leaks.md](issues/ISSUE-007_memory_leaks.md)**
  - **Title**: # ISSUE-007: ARC Memory Leaks in Generated Code
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-014_jit_exception_state_race.md](issues/ISSUE-014_jit_exception_state_race.md)**
  - **Title**: # ISSUE-014: Unsynchronized Global State in JIT Exception Handling
  - **Status**: FIXED — exception-state map has dedicated synchronization and concurrent regression; lock-ordering invariant documented
  - **Priority**: LOW

- **[ISSUE-015_method_dispatch_conflict.md](issues/ISSUE-015_method_dispatch_conflict.md)**
  - **Title**: # ISSUE-015: Method Name Resolution Uses Single-Class Map, Causing Wrong Dispatch
  - **Status**: FIXED — receiver-aware dispatch, multi-class indexing, and safe ambiguity diagnostics
  - **Priority**: HIGH

- **[ISSUE-017_test_infrastructure.md](issues/ISSUE-017_test_infrastructure.md)**
  - **Title**: # ISSUE-017: Test Infrastructure Issues
  - **Status**: PARTIALLY IMPLEMENTED
  - **Priority**: HIGH

- **[ISSUE-018_lui_shift.md](issues/ISSUE-018_lui_shift.md)**
  - **Title**: # ISSUE-018: LUI Shift Value Mismatch with Encoding
  - **Status**: FIXED — shared constant and encoding regression coverage
  - **Priority**: MEDIUM

- **[ISSUE-019_register_leaks.md](issues/ISSUE-019_register_leaks.md)**
  - **Title**: # ISSUE-019: Register Leaks on Control Flow Breaks
  - **Status**: IMPLEMENTED
  - **Priority**: MEDIUM

- **[ISSUE-020_class_modifiers_pruned.md](issues/ISSUE-020_class_modifiers_pruned.md)**
  - **Title**: # ISSUE-020: Removed Class Modifiers and Legacy Grammar Dead Code
  - **Status**: FIXED — dead grammar and AST helper remnants removed
  - **Priority**: LOW

- **[ISSUE-021_missing_statement_types.md](issues/ISSUE-021_missing_statement_types.md)**
  - **Title**: # ISSUE-021: Statement Grammar, Lowering, and Execution Completeness
  - **Status**: IMPLEMENTED
  - **Priority**: MEDIUM

- **[ISSUE-022_type_inference_limits.md](issues/ISSUE-022_type_inference_limits.md)**
  - **Title**: # ISSUE-022: Type Inference for `var` Declarations
  - **Status**: IMPLEMENTED for the supported compile-time type model
  - **Priority**: MEDIUM

- **[ISSUE-025_tensor_data_type.md](issues/ISSUE-025_tensor_data_type.md)**
  - **Title**: # ISSUE-025: Implementation Plan for `tensor` Data Type
  - **Status**: IMPLEMENTED
  - **Priority**: HIGH** (for AI/ML target workloads)
  - **Notes**: Complete rank-1/2/3 support with tensor literals, packed bit and one-byte int8/byte/f8 storage, canonical E4M3 FP8 fallback, promotion, element-wise arithmetic/comparison/logic, matrix multiplication, reshape, transpose, softmax, tensor-scalar broadcasting, and JIT/runtime coverage. Higher ANN/autograd kernels remain tracked by ISSUE-026.

- **[ISSUE-027_hvm_1_5_subword_precision.md](issues/ISSUE-027_hvm_1_5_subword_precision.md)**
  - **Title**: # ISSUE-027: HVM 1.5 Sub-Word Precision Extension Implementation
  - **Status**: IMPLEMENTED for the HVM 1.5 scalar E4M3/int8/byte/bit profile
  - **Priority**: HIGH** (Prerequisite for high-performance AI/ML support)

- **[ISSUE-031_chained_inference.md](issues/ISSUE-031_chained_inference.md)**
  - **Title**: # ISSUE-031: Advanced Return Type Inference for Method Chains
  - **Status**: IMPLEMENTED — recursive propagation, archive overload metadata, and generic `any[]` element propagation are complete; dynamic `any`-return receivers stay conservative until runtime dispatch exists
  - **Priority**: Medium (Affects code ergonomics and dispatch safety)

- **[ISSUE-035_serializable_class_modifier.md](issues/ISSUE-035_serializable_class_modifier.md)**
  - **Title**: # ISSUE-035: `serializable` Class Modifier — Declarative Serialization
  - **Status**: IMPLEMENTED
  - **Priority**: MEDIUM** (Feature enhancement — no correctness impact on existing code)
  - **Notes**: Grammar, AST, modifier-aware symbol mangling and source dispatch, constructor/type/cycle validation, inherited and nested field lowering, tagged buffer/tensor JSON round-trips, and runtime/codegen regression coverage. Phase 11.3 complete.


## Open / Backlog

- **[ISSUE-013_unicode_escapes.md](issues/ISSUE-013_unicode_escapes.md)**
  - **Title**: # ISSUE-013: Missing Unicode and Hex Escape Sequences in String Literals
  - **Status**: FIXED
  - **Priority**: HIGH

- **[ISSUE-026_native_ann_support.md](issues/ISSUE-026_native_ann_support.md)**
  - **Title**: # ISSUE-026: Native Neural Network and AI Data Support
  - **Status**: IN PROGRESS — versioned tensor/AI ABI foundation implemented; autograd, `hoo.nn`, data pipelines, model formats, and training remain planned
  - **Priority**: CRITICAL** (Core differentiator for the Hoo ecosystem)

- **[ISSUE-028_subword_modulo_shift.md](issues/ISSUE-028_subword_modulo_shift.md)**
  - **Title**: # ISSUE-028: Inconsistent Sub-word Modulo and Shift Semantics
  - **Status**: IMPLEMENTED
  - **Priority**: Medium (Correctness issue for low-precision types)

- **[ISSUE-029_unsigned_comparisons.md](issues/ISSUE-029_unsigned_comparisons.md)**
  - **Title**: # ISSUE-029: Missing Unsigned Comparison Instructions in HVM
  - **Status**: FIXED — wide unsigned comparisons and native HVM 1.5 `CMP_B` comparisons are implemented
  - **Priority**: High (Crucial for `byte` type correctness)

- **[ISSUE-030_tensor_scalar_arithmetic.md](issues/ISSUE-030_tensor_scalar_arithmetic.md)**
  - **Title**: # ISSUE-030: Tensor-Scalar Mixed Arithmetic Support
  - **Status**: IMPLEMENTED
  - **Notes**: Safe scalar broadcasting for add, subtract, scale, and divide in both operand orders, with runtime/JIT coverage.
  - **Priority**: Medium (Essential for training loops/optimizers)

- **[ISSUE-034_language_ergonomics_proposals.md](issues/ISSUE-034_language_ergonomics_proposals.md)**
  - **Title**: # ISSUE-034: Language Ergonomics Proposals from Perl, Lua, and Rust Influences
  - **Status**: PROPOSED
  - **Priority**: MEDIUM** (Ergonomics enhancement — no correctness bugs but significant developer experience improvement)

- **[ISSUE-036_module_dependency_resolution_bug.md](issues/ISSUE-036_module_dependency_resolution_bug.md)**
  - **Title**: # ISSUE-036: Module Dependency Resolution Can Produce Incorrect Order and False Cycle Reports
  - **Status**: IMPLEMENTED
  - **Priority**: Medium (graph correctness and linker behavior)
  - **Notes**: Per-module graph traversal, transitive ordering, explicit cycle-state tracking, HVMJIT loader rejection, and regression coverage are complete. The obsolete circular-dependency helper was removed.

- **[ISSUE-037_network_runtime_socket_support.md](issues/ISSUE-037_network_runtime_socket_support.md)**
  - **Title**: # ISSUE-037: Runtime Networking API Is Missing Full Socket Support
  - **Status**: IMPLEMENTED — DNS-aware TCP, timeouts, and TLS client support are available
  - **Priority**: Medium (runtime capability gap)

- **[ISSUE-039_list_intrinsic.md](issues/ISSUE-039_list_intrinsic.md)**
  - **Title**: # ISSUE-039 List Intrinsic Data Type
  - **Status**: PROPOSED
  - **Priority**: Medium

- **[ISSUE-040_hvm_spec_compatibility.md](issues/ISSUE-040_hvm_spec_compatibility.md)**
  - **Title**: # ISSUE-040: HVM Implementation Is Not Fully Compatible With HVM 1.5 Specification
  - **Status**: IMPLEMENTED (Phases 1-7 complete: CSV parity + JIT IR lowering + LD.P/ST.P hardening + RELEASE zero-flag + ALLOC.BUMP TLAB + feature flags + HVM-V vector expansion)
  - **Priority**: High

- **[ISSUE-042_decimal_intrinsic.md](issues/ISSUE-042_decimal_intrinsic.md)**
  - **Title**: # ISSUE-042 Decimal Intrinsic Data Type
  - **Status**: REVISED DESIGN REQUIRED
  - **Priority**: Medium

- **Async/Await via libuv**
  - **Title**: # ISSUE-043 Async/Await Integration via libuv
  - **Status**: IMPLEMENTED for cooperative HVM Future execution
  - **Priority**: HIGH
  - **Notes**: Native `async`/`await` syntax, `Future<T>` (type ID 123),
    condition-variable waiting, multiple continuations, and a mutex-protected
    libuv event loop. True HVM stack-frame suspension remains future work.

- **[ISSUE-046_managed_object_linked_list.md](issues/ISSUE-046_managed_object_linked_list.md)**
  - **Title**: # ISSUE-046: `hoo_is_managed_object` O(n) Linked-List Walk With Global Mutex
  - **Status**: IMPLEMENTED — hashed tracking with striped mutexes replaced the global linked-list walk
  - **Priority**: HIGH

- **[ISSUE-047_nullable_types_missing_codegen.md](issues/ISSUE-047_nullable_types_missing_codegen.md)**
  - **Title**: # ISSUE-047: Nullable/Optional Types Defined in Grammar and AST But Not Implemented in Codegen
  - **Status**: IMPLEMENTED (null checks, nullability tracking, assignment validation, ARC cleanup, and catchable LD.D.NZ folding are complete)
  - **Priority**: HIGH

- **[ISSUE-048_string_interpolation_placeholder.md](issues/ISSUE-048_string_interpolation_placeholder.md)**
  - **Title**: # ISSUE-048: String Interpolation Is a Placeholder Only
  - **Status**: FIXED — AST parsing, concatenation lowering, and JIT coverage are implemented
  - **Priority**: MEDIUM

- **[ISSUE-049_json_deserialize_type_promotion.md](issues/ISSUE-049_json_deserialize_type_promotion.md)**
  - **Title**: # ISSUE-049: JSON Deserialization Does Not Reverse Serialization Type Promotion
  - **Status**: FIXED — narrow integer, FP8, bit, and Buffer round trips are covered
  - **Priority**: HIGH

- **[ISSUE-050_raii_scoped_lock.md](issues/ISSUE-050_raii_scoped_lock.md)**
  - **Title**: # ISSUE-050: No RAII Scoped Lock for Mutex
  - **Status**: IMPLEMENTED — native `ScopedLock` and mutex try-lock are available
  - **Priority**: MEDIUM

- **[ISSUE-052_condition_variables_thread_notification.md](issues/ISSUE-052_condition_variables_thread_notification.md)**
  - **Title**: # ISSUE-052: No Condition Variables or Thread Notification Mechanism
  - **Status**: IMPLEMENTED — condition variables and semaphores are available in the C ABI
  - **Priority**: MEDIUM

- **[ISSUE-053_byte_slice_type.md](issues/ISSUE-053_byte_slice_type.md)**
  - **Title**: # ISSUE-053: Missing First-Class `byte[]` / Slice Type Causes Runtime API Duplication
  - **Status**: IMPLEMENTED — `slice<byte>` syntax and slice-aware encoding/hash/compression/socket APIs are available
  - **Priority**: MEDIUM

- **[ISSUE-055_destructor_table_limit.md](issues/ISSUE-055_destructor_table_limit.md)**
  - **Title**: # ISSUE-055: Destructor Table Size Limited to 256 Type IDs
  - **Status**: FIXED — synchronized dynamic destructor registry supports arbitrary non-negative type IDs
  - **Priority**: MEDIUM

## Recommended Next Work

1. **Finish ISSUE-031 advanced inference**
   - Add explicit return metadata for external/module signatures.
   - Define generic type propagation and overload-set metadata across archives.
   - Keep ambiguous dynamic receivers as compile-time errors until runtime type dispatch is designed.

   Completed: archive overload sets, user-defined return classes, and generic
   `any[]` element propagation. Dynamic `any`-return receivers remain
   compile-time errors until runtime type dispatch is designed.

2. **Complete small correctness and specification hardening**
    - ~~Finish ISSUE-018 by centralizing the LUI shift constant across codegen, interpreter, JIT, and specification tests.~~ ✅ COMPLETE — constant `hvm::kLuiImmediateShift` used by interpreter and JIT IR, pinned by `LuiRespectsSharedShiftConstant` test; codegen does not emit LUI so no codegen-side consumer exists.
    - ~~Add a dedicated concurrent regression for ISSUE-014's exception-state ownership and document the shared-lock invariant.~~ ✅ COMPLETE — `ConcurrentExceptionThrowCatchIsSafeAcrossThreadsAndStates` in `HVMJITLoaderTest.cpp`; lock-ordering invariant documented in `ISSUE-014`.
   - Remove remaining dead grammar and AST helper code from ISSUE-020.

3. **Design concurrency APIs together**
   - Implement ISSUE-050 mutex ownership and scoped-lock semantics first.
   - Build ISSUE-052 condition variables and notification primitives on that stable ABI.

4. **Define the byte-view ABI before expanding networking**
   - Add source-level/JIT wrappers for every slice-aware encoding, hashing, and compression operation.
   - Add protocol-level asynchronous socket events and certificate rotation.

5. **Treat ISSUE-026 ANN support as a product phase**
   - Stabilize tensor shape/type inference, serialization, and profiling APIs before adding `grad`, optimizer, and accelerator work.
