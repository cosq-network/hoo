# ISSUE-047: Nullable/Optional Types Defined in Grammar and AST But Not Implemented in Codegen

## 1. Overview
The grammar and AST define `OptionalType` (nullable types using `?` suffix, e.g., `int64?`, `string?`, `User?`, `int64[]?`), and the parser builds `OptionalType` AST nodes. Historically the code generator collapsed every optional type to a hard-coded type ID of `100` (generic object), discarding the underlying `T`, and emitted **no** null-safety semantics (no null checks, no `T` vs `T?` distinction, no assignment/call compatibility validation).

This is now largely resolved: the codegen tracks nullability end-to-end, emits null-pointer checks before dereferencing nullable values, raises a catchable `NullPointerException`, validates `null`/nullable assignments at compile time, and mangles nullable signatures distinctly. See sections 4, 7, and 8.

## 2. Grammar → AST → Parser (IMPLEMENTED)
- **Grammar**: `src/parsing/Hooc.g4`
  - Line 38: `NULL: 'null';` literal token; line 364: `NULL` is a valid primary expression.
  - Line 207: `type: futureType | tensorType | hashMapType | mapType | decimalType | anyType | anyArrayType | optionalType;`
  - Line 209: `optionalType: arrayType QUESTION?;`
  - Line 211: `arrayType: baseType (LBRACKET RBRACKET)*;` — so scalars (`int64?`), arrays (`int64[]?`), and named types (`User?`) all route through `optionalType`.
- **AST**: `src/ast/Type.h:124-139` — `class OptionalType : public Type` wraps `std::unique_ptr<ArrayType> arrayType_` (holds base type + dimensions) plus `bool isOptional_`; accessors `getArrayType()`, `takeArrayType()`, `isOptional()`. `toString()` → `"OptionalType"` (`src/ast/ASTImpl.cpp:138-139`).
- **Parser**: `src/ast/SimpleASTBuilder.cpp`
  - `buildType` (218-266): when `?` is present, **always** wraps in `OptionalType(ArrayType(base, []))`; without `?` returns a bare `ArrayType` (if dimensions) or the plain `BaseType`.
  - `buildOptionalType` (268-284): dedicated builder; `isOptional = ctx->QUESTION() != nullptr`.
  - `buildPrimary` (1019-1021): `NULL` token → `NullLiteral` inside a `PrimaryExpression`.
- **Null literal AST**: `src/ast/Primary.h:142-145` — empty `NullLiteral` class; codegen lowers it to `emitConstant(0)` (`HVMCodeGenerator.cpp:2390-2392`), i.e. null == zero == NULL pointer.

## 3. Semantic / Type-Analysis Layer
- There is **no** separate semantic-analysis pass. `HooCompiler::compile` (`src/core/HooCompiler.cpp:35-96`) is a three-stage pipeline: parse → `SimpleASTBuilder::buildAST` → `HVMCodeGenerator::generateModule`. All type checking is ad-hoc, embedded in the codegen.
- `inferExpressionTypeInfo` (`HVMCodeGenerator.cpp`) now has a `NullLiteral` case: a `null` expression infers `typeId = 0` (`any`) **and** `isNullable = true`, so the codegen can reason about possibly-null values.
- `validateAssignmentNullSafety(targetNullable, targetTypeId, value, name)` (`HVMCodeGenerator.cpp`) is invoked from `visitAssignment` and `visitStatement` (var declarations) and rejects:
  - `null` flowing into a non-nullable **value-typed** slot (e.g. `var x: int64 = null`),
  - a nullable-typed value flowing into a non-nullable slot (reference or value type).

## 4. Codegen — `src/codegen/HVMCodeGenerator.cpp` (IMPLEMENTED)
- `typeIdFromDeclaredType` preserves the underlying type of `T?` instead of collapsing to `100`: a nullable scalar/object keeps its base type ID and class name; a nullable array (`int64[]?`) keeps array type ID `102`. Nullability is carried separately on the value (see below).
- Nullability is tracked on:
  - locals/params (`Local.isNullable`, via `reserveLocal(..., isNullable)` and `getLocalIsNullable`),
  - class fields (`ClassLayout.fieldIsNullable`),
  - expressions (`ExpressionTypeInfo.isNullable`), propagated through identifiers, array access, and field access,
  - overload signatures (`OverloadReturnInfo.parameterIsNullable`), so `f(int64?)` and `f(int64)` are distinct during `selectOverload`.
- Null-check emission: `emitNullCheck(valueReg)` emits `BNE valueReg, 0 → ok` and, on the null path, calls `_F_hoo_exception_null_pointer_p` (registered in the JIT symbol table), moves the handle into `r2`, and executes `SYSCALL 9` (`kSysThrowToHandler`) — the shadow-stack throw used by try/catch. A surrounding `try { } catch (e: Exception) { }` therefore catches the `NullPointerException`.
- Null checks are emitted before dereferencing a **nullable** receiver/base at every dereference site:
  - member access reads (`s.length()`),
  - method-call receivers,
  - array access reads and indexed assignment bases,
  - member-assignment objects.
- Mangling: `mangleTypeId(typeId, isNullable)` appends a `?` to the intermediate mangled type string (e.g. `i8?`, `ptr?`); `SymbolMangler::mangleType` (`src/core/SymbolMangler.cpp:777-781`) normalizes it to the `O` prefix (e.g. `Oi8`) in emitted symbol names, so overloaded signatures disambiguate `T?` from `T`.
- Module feature flag: when any null-checking code is emitted, `moduleUsesNullChecks_` is set and the generated module declares `HVM_NZ` via `HOModule::setRequiredFeatures`, so loaders accept the module.

## 5. Runtime — `src/runtime/lib/`
- Type ID constants (`hoo_runtime.h:24-53`): no nullable IDs; `HOO_TYPE_OBJECT = 100` is the generic object, so `T?` ≡ object at runtime.
- Overload scoring (`hoo_overload.cpp:63-78`): `expected == HOO_TYPE_OBJECT` → score `3` ("nullable/object-compatible"); `expected == 0` (`any`) → `20`. There is no dedicated nullable rule; nullable values are simply treated as objects.
- Null is the C `NULL` pointer / zero value. `hoo_retain`/`hoo_release` are null-safe (`hoo_runtime.h:76-111`, "can be NULL"), and printing a NULL string prints `null` (`hoo_io.h:23-30`).
- Exception creation: `hoo_exception_null_pointer(msg)` (`src/runtime/lib/hoo_exception.cpp:126`) creates a `NullPointerException` (type `HOO_EXCEPTION_NULL_POINTER`, id 1, `src/runtime/lib/hoo_exception.h:33`); the codegen's `_F_hoo_exception_null_pointer_p` bridge wraps it.

## 6. HVM ISA — null-check facility
- `LD.D.NZ` / `ld.d.nz`, opcode `0xD9`, I-format (`src/hvm/HVMInstruction.h:87`, `HVMInstruction.cpp:656`, `docs/hvm/instructions.md:142`): "Optional null-checking 64-bit load" — traps on null base.
- Interpreter: `HVMJIT.cpp` (`executeFunction`) — `if (addr == 0)` → "Null pointer dereference trap", `trapHit = true`, return `-1`.
- JIT codegen: `HVMJIT.cpp` (`translateModule`) — `icmp eq base, 0` → trap BB storing `trapHit` (state field 3) and `ret -1`; then misalignment check.
- Module feature flag: `HVM_NZ = 1ULL << 5` (`src/hvm/HOModule.h:64`), documented in `docs/hvm/ho-file-format.md:68` ("Module may contain null-checking load instructions"); now declared by the codegen whenever null-check branches are emitted.
- **Implementation note**: the codegen emits explicit `BNE`-based null-check branches that throw through the shadow-handler syscall path (section 4) rather than folding checks into `LD.D.NZ`. `LD.D.NZ` currently reports a VM trap (`trapHit`/`-1`) and does not enter the shadow-handler path, so folding it here would make `NullPointerException` uncaught. The load form remains available for future work after its exception semantics are aligned.

## 7. Impact (verified behavior)
- `int64?`, `string?`, `User?`, `int64[]?` retain their underlying type identity; nullability is tracked separately and does not corrupt overload resolution or mangling.
- Dereferencing a `null` nullable value (member access, method call, array access, or indexed/member assignment) raises a `NullPointerException` that is **catchable** by a surrounding `try/catch`, under both the interpreter and the JIT.
- Assigning `null` to a non-nullable value-typed variable (e.g. `var x: int64 = null`) is a compile-time error; assigning a nullable value into a non-nullable slot is likewise rejected with a message suggesting `T?` or an explicit null check.
- The HVM_NZ module feature flag is set when null-checking code is present.

## 8. Suggested Fix — Status of each item
1. **Track `T?` distinctly in codegen** — **DONE**: nullable flag carried on params, locals, fields, and expressions; underlying `T` preserved; `selectOverload` distinguishes `T?` from `T`.
2. **Emit null checks before dereference** — **DONE** (via explicit branches + `SYSCALL 9` shadow throw; see section 4). `LD.D.NZ` folding into the instruction's native null branch is also complete — interpreter and JIT IR null paths now route through the catchable `hooc_hvm_sys_throw_to_handler_state` dispatch.
3. **Compile-time null-safety validation** — **DONE**: `validateAssignmentNullSafety` covers var declarations and plain assignments; call-argument binding rejects nullable arguments for non-nullable parameters via `selectOverload`.
4. **Resolve ARC policy** for nullable object locals — **DONE**: nullable named reference locals in generic-object slots carry an explicit `arcManaged` cleanup bit and are released with `hoo_release`; non-ARC runtime classes remain excluded. Regression coverage is in `HVMCodeGeneratorTest.NullableUserClassLocal_EmitsScopeCleanupRelease`.

## 9. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED** (null checks, nullability tracking, assignment validation, arc cleanup, and catchable `LD.D.NZ` folding are complete)
- **Priority**: **HIGH**
- **Related**: `ISSUE-034` (ergonomics — proposes desugaring `T?` → `Option<T>`, `docs/issues/ISSUE-034_language_ergonomics_proposals.md:247,302-303,653`); `docs/ROADMAP.md:254`; CHANGELOG entry "implement implicit nullable conversions" (`docs/CHANGELOG.md:334`) refers to a prior inference/overload ergonomics pass, not null-safety.
