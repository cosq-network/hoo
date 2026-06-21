# ISSUE-010: Singleton Constructor Never Called

## 1. Overview
When a `singleton` class is accessed via `new`, the code generator loads the pre-allocated instance pointer from `.data` but never invokes the constructor. The constructor logic (parameter validation, field initialization) is silently skipped.

## 2. Technical Analysis

### 2.1 Load path (user code)
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 925-932
- **Issue**: When compiling `new Singleton(args...)`, the code loads the singleton pointer from `.data` and returns it directly without calling the constructor.

```cpp
uint8_t dest = allocateRegister();
emit(Opcode::LDA, OperandsI{dest, 0, static_cast<int16_t>(cl.singletonDataOffset)});
emit(Opcode::LD, ...);
// Constructor call is MISSING
return dest;
```

### 2.2 Init path (module initialization)
- **Location**: `src/codegen/HVMCodeGenerator.cpp` lines 1894-1916
- **Issue**: In `emitModuleInit`, the singleton instance is allocated via `hoo_alloc` but no constructor call is emitted. The instance is left zero-initialized.

## 3. Impact
- Singleton instances are always zero-initialized, regardless of constructor logic.
- Constructor parameters are silently ignored.
- Field validation in constructors is never performed for singletons.

## 4. Suggested Fix
After allocating the singleton instance in `emitModuleInit`, emit a `CALL` to the constructor function. Use a `std::call_once` guard to ensure the constructor runs only once:

```cpp
emitCall(Opcode::CALL, ctorMangledName);
```

The load path in the user code's `new` expression should still just load the pointer (the constructor was already called during module init).

## 5. Status
- **Date**: 2026-06-08 (opened), 2026-06-10 (fixed)
- **Status**: **FIXED**
- **Priority**: **HIGH**
- **Audit 2026-06-21**: Verified singleton constructor lowering remains implemented through module initialization/new-object handling; no regression was found in the reviewed code paths.
- **Fix**: `emitModuleInit` now emits `MOV r1, instanceReg` + `CALL <mangled_ctor_name>` after allocating and storing the singleton instance. Uses the same `MangledFunctionParams` pattern as the regular `new` expression path. Singletons are validated to have 0 constructor parameters at class definition (lines 288-294).
