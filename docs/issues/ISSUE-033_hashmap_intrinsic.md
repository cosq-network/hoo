# ISSUE-033: Implementation Plan for Native `HashMap`, `AnyArray`, and `any` Intrinsic Types

## 1. Overview
This document outlines the architectural and implementation plan for introducing native `HashMap` and `AnyArray` intrinsic types, plus a supporting virtual type `any`, to the Hoo language and core compiler module. The combination of these features allows for high-performance collections that can support heterogeneous value sets when needed.

Design constraint: these features are language, compiler, and runtime additions. They must not require new HVM opcodes, hidden object instructions, or JIT-only behavior. Hardware, interpreter, and JIT execution must observe the same ABI, memory ownership, error behavior, and type-tag representation.

### 1.1 The `any` Virtual Type
The `any` type is a first-class language intrinsic that can represent any data type in the Hoo system.
- **Scope**: It covers all primitive/intrinsic types (`int64`, `double`, `bit`, `f8`, `tensor`) and all managed class-based types (`String`, `Buffer`, `Array`, `Map`, etc.).
- **Representation**: At the HVM level, `any` is a "fat pointer" or "tagged value" consisting of a 64-bit type ID and a 64-bit data slot.
- **Purpose**: Primarily used for generic collections and flexible API boundaries where the specific type is determined at runtime.
- **Not an Object**: A standalone `any` value is not itself ARC-managed. ARC applies only when its `type_id` identifies a managed payload (`type_id >= 100`).
- **Register Form**: When passed through generated code, `any` occupies two 64-bit values: `type_id` first, `data` second. Helpers that cannot return two registers must write to an explicit out-buffer.

### 1.2 The `HashMap` Intrinsic
`HashMap` is a first-class compiler intrinsic optimized for hardware-speed key-value lookups.
1. **Implementation Location**: Core compiler and native runtime layers.
2. **Key Type Restrictions**: Limited to scalar numerical types: `int64`, `int8`, and `byte`.
3. **Value Type Flexibility**: Can be a specific type (e.g., `String`, `int64`) for homogeneous maps, or the `any` type for heterogeneous (mixed) maps.
4. **API Paradigm**: Purely Object-Oriented (OOP) style APIs.

### 1.3 The `AnyArray` Intrinsic
`AnyArray` is a first-class runtime-backed intrinsic type representing a variable-length array whose element type is `any`.
1. **Implementation Location**: Native runtime layer with compiler and JIT wrapper support.
2. **Element Type**: Always `any`; each slot stores both a runtime `type_id` and a 64-bit data payload.
3. **Value Flexibility**: Can contain primitives and intrinsic values (`int64`, `double`, `bit`, `f8`, `tensor`, etc.) and managed class-based values (`String`, `Buffer`, `Array`, `Map`, `HashMap`, class instances, etc.) in the same array.
4. **API Paradigm**: OOP-style APIs plus subscript syntax for indexed access.

---

## 1.4 Hardware and JIT Compatibility Requirements
The `any`, `HashMap`, and `AnyArray` plan must comply with the HVM hardware/JIT compatibility rule:

- **No ISA Extension Required**: All behavior lowers to existing HVM arithmetic, load/store, branch, `CALL`, and existing runtime allocation/ARC mechanisms.
- **Opaque Runtime Handles**: `HashMap` and `AnyArray` are managed runtime objects. HVM code sees only 64-bit handles, not C++ container layout.
- **Stable Tagged Layout**: The only exposed value layout is two little-endian 64-bit slots: `{ type_id: i64, data: u64 }`.
- **Software Fallback Required**: Every JIT bridge helper must have an interpreter/runtime fallback. Hardware may accelerate calls later, but acceleration must preserve the same ABI.
- **No Host Pointer Leakage**: Public HVM-facing helpers must not require guest code to understand host virtual addresses. Any out-buffer argument must point to HVM-addressable memory or be handled entirely inside the JIT bridge.
- **Deterministic Failure**: Bounds failures, missing map keys, invalid type casts, allocation failure, and null handles must have documented return codes or throw documented runtime exceptions. The JIT must not silently invent different behavior.

---

## 2. Grammar & Syntax Support

### 2.1 The `any` Keyword
Add `ANY` to the keywords in `Hooc.g4` and allow it in type positions:
```hoo
var x: any = 42;
x = "hello";
```

### 2.2 `HashMap` Type Declaration
```hoo
var map1: HashMap<int64, String>;             // Homogeneous
var map2: HashMap<byte, any>;                 // Heterogeneous (Mixed types)
```

### 2.3 `AnyArray` Type Declaration
`AnyArray` is accepted as an intrinsic type in normal type positions:
```hoo
var values: AnyArray;
var more = new AnyArray();
var sized = new AnyArray(16);                 // Optional initial capacity
```

The generic spelling `Array<any>` is not introduced by this plan. `AnyArray` is the canonical grammar-level syntax for a variable-length heterogeneous array.

### 2.4 Heterogeneous Array Literal Syntax
Array literals can be explicitly lowered to `AnyArray` with an `any` suffix:
```hoo
var values = [1, "two", 3.0, new Buffer()]any;
```

Without the `any` suffix, existing array literal inference rules continue to apply.

Grammar implementation target:
```antlr
ANY: 'any';
ANYARRAY: 'AnyArray';

baseType
    : primitiveType
    | ANY
    | ANYARRAY
    | qualifiedIdentifier
    ;

primary
    : LBRACKET expressionList? RBRACKET ANY   // AnyArray literal
    | ...
    ;
```

If lexer keyword ordering conflicts with `IDENTIFIER`, `ANY` and `ANYARRAY` must appear before `IDENTIFIER`.

### 2.5 Subscript Access
```hoo
var m = new HashMap<int64, any>();
m[1] = "Hoo String";
m[2] = 100;
m[3] = [1.0, 2.0]t;

var v = m[1]; // v is of type 'any'

var a = new AnyArray();
a.push("hello");
a.push(42);
a[2] = new Buffer();

var item = a[0]; // item is of type 'any'
```

---

## 3. AST & Type System Integration

### 3.1 `any` Type Implementation
- **Type ID**: Assigned `typeId 0`.
- **AST Node**: `AnyType` node in `src/ast/Type.h`.
- **Inference**: Any expression assigned to a variable of type `any` is valid. Retrieval from an `any` container returns `typeId 0`.
- **Stack Slots**: A local variable of type `any` reserves two adjacent 64-bit stack slots: one for `type_id`, one for `data`. It is not represented by a single pointer.
- **Casting/Unboxing**: Reading a concrete value from `any` requires a runtime type check. A failed checked cast must return a documented failure value or throw a typed runtime exception; unchecked reinterpretation is not allowed in generated code.
- **Current Surface**: Grammar, AST, type ID, mangling, runtime tagged value helpers, and collection packing are implemented. Full standalone two-slot local storage and checked unboxing remain the next semantic expansion when explicit unboxing/cast syntax is finalized.

### 3.2 `HashMap` Type Implementation
- **Type ID**: `typeId 117`. Do not use `105`, which is reserved for `Random`.
- **AST Node**: `HashMapType` node in `src/ast/Type.h` storing `keyType` and `valueType`.
- **Subscript Inference**: 
  - If `valueType` is a specific type `V`, result is `V`.
  - If `valueType` is `any`, result is `any`.

### 3.3 `AnyArray` Type Implementation
- **Type ID**: `typeId 118`. Do not use `106`, which is reserved for `URL`.
- **AST Node**: `AnyArrayType` node in `src/ast/Type.h`, or a compiler-recognized intrinsic class type named `AnyArray` if the AST continues using qualified type nodes for runtime-backed classes.
- **Literal Node**: Existing `ArrayLiteral` nodes can be annotated as `AnyArray` when parsed with the `any` suffix.
- **Subscript Inference**: `AnyArray[index]` always returns `any`.
- **Mutation Rules**: Any expression can be pushed into or assigned to an `AnyArray` slot after being packed into the tagged `any` representation.
- **Scope Release**: Local variables of type `AnyArray` are managed handles and participate in scope-level release. Their contained managed values are released by the `AnyArray` runtime destructor, not by scanning compiler locals.

---

## 4. Code Generation & Lowering

### 4.1 `any` Value Handling
- When passing a value to an `any` slot:
  - If the value is a primitive, the compiler emits code to pack the `typeId` and the value.
  - If the value is a managed object, the compiler extracts the `typeId` from the ARC header (at `ptr - 8`).
- Managed objects passed as `any` are automatically retained.
- Packing and unpacking must be centralized in codegen helpers, not duplicated per collection. Required helpers:
  - `emitPackAny(expr) -> {typeReg, dataReg}`
  - `emitStoreAny(typeReg, dataReg, baseReg, offset)`
  - `emitLoadAny(baseReg, offset) -> {typeReg, dataReg}`
  - `emitReleaseAny(typeReg, dataReg)` for compiler-owned temporary `any` values.
- Temporary `any` values that hold managed payloads must obey the same ARC rules as ordinary managed temporaries.

### 4.2 `HashMap` Lowering
- **Allocation**: `new HashMap<K, V>()` emits `_F_hoo_hashmap_new_p_i8_i8`.
- **Setter (`m[k] = v`)**:
  - If `V` is fixed, uses `_F_hoo_hashmap_set_fixed_i8_p_i8_i8`.
  - If `V` is `any`, uses `_F_hoo_hashmap_set_any_i8_p_i8_i8_i8` (passing `map`, `key`, `value_type_id`, `value_data`; the fourth logical argument is in `r5` because `r4` is `tp`).
- **Getter (`var v = m[k]`)**:
  - If `V` is fixed, raw ABI uses `hoo_hashmap_get_fixed_i8(map, key, out)` and current JIT expression lowering uses `_F_hoo_hashmap_get_fixed_data_i8_p_i8`.
  - If `V` is `any`, raw ABI uses `hoo_hashmap_get_any_i8(map, key, out)` and current JIT expression lowering uses `_F_hoo_hashmap_get_any_data_i8_p_i8` for scalar payload reads.

### 4.3 `AnyArray` Lowering
- **Allocation**: `new AnyArray()` emits `_F_hoo_anyarray_new_p`; `new AnyArray(capacity)` emits `_F_hoo_anyarray_new_capacity_p_i8`.
- **Literal**: `[expr1, expr2, ...]any` emits an `AnyArray` allocation followed by one append per element.
- **Push (`arr.push(v)`)**: Packs `v` into `(type_id, data)` and emits `_F_hoo_anyarray_push_i8_p_i8_i8`.
- **Setter (`arr[i] = v`)**: Packs `v` into `(type_id, data)` and emits `_F_hoo_anyarray_set_i8_p_i8_i8_i8`.
- **Getter (`var v = arr[i]`)**: Raw ABI uses `hoo_anyarray_get(array, index, out)` to write a tagged `any` value to an out-buffer. Current JIT expression lowering uses `_F_hoo_anyarray_get_data_i8_p_i8` for scalar payload reads.
- **Length (`arr.length()`)**: Emits `_F_hoo_anyarray_length_i8_p`.
- **Clear (`arr.clear()`)**: Emits `_F_hoo_anyarray_clear_v_p`.

### 4.4 ABI and Symbol Rules
Because HVM has 64-bit registers and ordinary calls return through `r1`, multiword `any` results need an explicit ABI rule:

1. **Preferred HVM ABI**: Getters that return `any` take an out-buffer pointing to two HVM-addressable 64-bit slots and return a status code in `r1`.
2. **JIT Bridge Optimization**: The JIT may internally lower a getter into a scalar payload bridge only where the language expression needs only the payload and the externally visible runtime behavior remains identical to the out-buffer ABI.
3. **No Hidden Host Pointers**: Runtime wrappers may use host pointers internally, but generated HVM code passes only handles, scalar values, and HVM-memory offsets.
4. **Mangled Names**: Symbols must use existing Hoo mangling conventions. Any pseudo-type name such as `any` must map to a stable mangle token and must not conflict with pointer (`p`) or primitive (`i8`, `f64`) encodings.
5. **Bounds and Missing Values**: Get operations return `1` on success and `0` on not found/out of bounds unless the language-level operation is specified to throw. Throwing forms must have separate helper names or compiler lowering paths.

---

## 5. Runtime Library Implementation (`hoort`)

### 5.1 `HashMap` Memory Layout
```cpp
struct AnyValue {
    int64_t type_id;
    uint64_t data;
};

struct HashMapImpl {
    // 16-byte ARC Header
    int64_t key_type_id;
    int64_t value_type_id; // If 0, it is a HashMap<K, any>
    int64_t count;
    void* native_map_ptr; 
};
```

The same tagged value layout is shared by `HashMap<K, any>` and `AnyArray`.

`AnyValue` is the normative runtime value layout. `HashMap<K, any>` entries and `AnyArray` elements must use this shape, even if the C++ implementation wraps it in helper classes.

### 5.2 Storage Strategy
The `native_map_ptr` points to a specialized `std::unordered_map` based on `key_type_id`:
1. **Fixed Value Type**: `std::unordered_map<Key, uint64_t>`.
   - Minimal overhead.
2. **`any` Value Type**: `std::unordered_map<Key, AnyValue>`.
   - Stores `type_id` alongside data for each entry.

### 5.3 ARC Management for `any`
When a value is stored in or removed from a `HashMap<K, any>`:
- The runtime checks `type_id`.
- If `type_id >= 100` (managed type), it performs `hoo_retain` on insertion and `hoo_release` on removal/overwrite/clear.
- This ensures full memory safety for mixed-type maps.

### 5.4 `AnyArray` Runtime Wrapper
The runtime wrapper exposes `AnyArray` as an ARC-managed opaque handle:
```cpp
struct AnyArrayImpl {
    std::vector<AnyValue> elements;
};
```

The host C ABI is intentionally small and mirrors the intrinsic lowering. JIT/HVM bridge wrappers may adapt pointer parameters into HVM-memory offsets, but the visible HVM contract remains the out-buffer ABI described above.
| Function | Signature | Returns | Description |
| :--- | :--- | :--- | :--- |
| `hoo_anyarray_new` | `()` | `AnyArray*` | Creates an empty `AnyArray`. |
| `hoo_anyarray_new_capacity` | `(int64_t capacity)` | `AnyArray*` | Creates an `AnyArray` with reserved capacity. |
| `hoo_anyarray_retain` | `(AnyArray*)` | `AnyArray*` | Retains the array handle. |
| `hoo_anyarray_release` | `(AnyArray*)` | `void` | Releases the array handle and all managed elements when refcount reaches zero. |
| `hoo_anyarray_length` | `(AnyArray*)` | `int64_t` | Returns element count. |
| `hoo_anyarray_push` | `(AnyArray*, int64_t type_id, uint64_t data)` | `int64_t` | Appends a tagged value. |
| `hoo_anyarray_set` | `(AnyArray*, int64_t index, int64_t type_id, uint64_t data)` | `int64_t` | Replaces an existing element. |
| `hoo_anyarray_get` | `(AnyArray*, int64_t index, AnyValue* out)` | `int64_t` | Writes a tagged value to `out`; returns 1 if found, 0 if out of bounds. |
| `hoo_anyarray_pop` | `(AnyArray*, AnyValue* out)` | `int64_t` | Removes the last element and writes it to `out`; returns 1 if present. |
| `hoo_anyarray_clear` | `(AnyArray*)` | `void` | Releases managed elements and clears the array. |

The C++ `std::vector` and `std::unordered_map` choices are implementation details of `hoort`. They are not part of the HVM ABI and must not be assumed by generated code, serialized modules, or future hardware.

### 5.5 ARC Management for `AnyArray`
When a value is stored in or removed from an `AnyArray`:
- The runtime checks `type_id`.
- If `type_id >= 100`, insertion and overwrite retain the new managed value.
- Overwrite, pop, clear, and final array release release the old managed value.
- Primitive and intrinsic scalar values are stored by value and require no ARC action.

### 5.6 Error and Boundary Semantics
Runtime wrappers must use stable, testable behavior:

- Null handle passed to a collection helper returns failure (`0` or `-1`) for non-throwing helpers.
- Negative indexes and indexes greater than or equal to length fail with `0` for `try`/raw helpers.
- Language-level subscript syntax may be lowered to throwing helpers after bounds checking policy is finalized.
- Allocation failure returns null from raw constructors or throws a runtime allocation exception from language-level `new`.
- Type mismatch during `any` unboxing must fail deterministically; it must not reinterpret bits as the requested type.
- `clear`, release, and destructor paths must be idempotent with respect to contained values and must not double-release overwritten or popped managed payloads.

---

## 6. Implementation Phases

### Phase 1: `any` Type Foundation
1. Add `any` keyword to grammar. **Done.**
2. Implement `AnyType` AST node. **Done.**
3. Update `HVMCodeGenerator` to pack concrete values into collection `any` slots. **Done for collection insertion.**
4. Add centralized pack/unpack/release helpers in codegen. **Partially done through runtime helpers; standalone local helpers remain pending.**
5. Add type-checking and unboxing rules with deterministic failures. **Pending explicit cast/unbox syntax.**

### Phase 2: `AnyArray` Core with `any` Support
1. Implement `AnyArrayImpl` as an ARC-managed runtime wrapper over `std::vector<AnyValue>`. **Done.**
2. Implement runtime logic for packed `any` values and managed-element ARC. **Done.**
3. Register `AnyArray` destructor callbacks with the runtime memory model. **Done.**
4. Add raw C ABI tests that do not depend on JIT behavior. **Done.**

### Phase 3: `HashMap` Core with `any` Support
1. Implement `HashMapImpl` with dual-mode storage (Fixed vs `any`). **Done.**
2. Implement runtime logic for `any` value ARC management. **Done.**
3. Use `AnyValue` for all `HashMap<K, any>` values. **Done.**
4. Add raw C ABI tests for set/get/remove/overwrite/clear. **Done.**

### Phase 4: Codegen and Symbol Bridge
1. Update `HVMCodeGenerator` to emit `AnyArray`, `HashMap`, and `any` intrinsics. **Done for constructors, literals, push, clear, count, remove, subscript set/get payload paths.**
2. Register `AnyArray` runtime wrappers and `_any` variant symbols in `HVMJIT.cpp`. **Done.**
3. Add grammar and AST support for `AnyArray` type positions and `[ ... ]any` literals. **Done.**
4. Verify every helper has interpreter/runtime fallback, not only JIT symbol registration. **Raw C ABI done; scalar `_get_data` helpers are JIT bridge conveniences.**
5. Verify helper signatures use handles, scalars, and HVM-memory out-buffers only. **Done for raw ABI; documented bridge exception for scalar payload reads.**

### Phase 5: Validation
1. Tests for `HashMap<int64, any>` and `HashMap<byte, any>` storing tagged values. **Done for primitive and managed-string runtime paths; Array/object payload expansion can reuse the same managed payload rule.**
2. Verify ARC correctness: ensure managed objects in an `any` map are released when the map is cleared. **Done.**
3. Tests for `AnyArray` storing a mix of primitive, intrinsic, and class values. **Done for primitive and managed-string runtime paths plus JIT primitive language paths.**
4. Verify ARC correctness: ensure managed objects in an `AnyArray` are retained on insertion and released on overwrite, clear, and array release. **Done.**
5. Cross-run the same language-level tests through parser/AST, HVM codegen, and JIT. **Done.**
6. Add compatibility tests for out-of-bounds, missing keys, and null handles. **Done for raw runtime APIs.**

### Phase 6: Hardware/JIT Compatibility Audit
Before marking this issue implemented:
1. Confirm no new opcode was introduced for `any`, `HashMap`, or `AnyArray`. **Done.**
2. Confirm generated HVM uses only documented calls and existing load/store/register behavior. **Done.**
3. Confirm runtime object layout remains opaque outside `hoort`. **Done.**
4. Confirm JIT lowering and hardware/simulator fallback produce identical visible results. **Done for raw ABI and current JIT bridge surface.**
5. Confirm all multiword `any` returns use the documented out-buffer ABI or a provably equivalent bridge. **Done for raw ABI; scalar payload bridges are documented as JIT conveniences.**
6. Update `docs/runtime/jit-integration.md`, `docs/grammar/types.md`, and runtime API docs with the final helper names and signatures. **Done.**

---

## 7. Status
- **Date**: 2026-06-19
- **Status**: **IMPLEMENTED - CORE RUNTIME, GRAMMAR, AST, CODEGEN, JIT BRIDGE, AND TEST COVERAGE**
- **Priority**: **HIGH** (Fundamental heterogeneous collection and type expansion)
- **Audit 2026-06-21**: Verified `HashMap`, `AnyArray`, and `any` support are present across grammar, AST, runtime, codegen, JIT bridge, and tests. Future polish can refine ABI documentation, but the core feature is implemented.

Implemented files include:
- Grammar/AST: `src/parsing/Hooc.g4`, `src/ast/Type.h`, `src/ast/Expression.h`, `src/ast/SimpleASTBuilder.cpp`.
- Runtime: `src/runtime/lib/hoo_any.*`, `src/runtime/lib/hoo_anyarray.*`, `src/runtime/lib/hoo_hashmap.*`.
- Codegen/JIT: `src/codegen/HVMCodeGenerator.*`, `src/hvm/HVMJIT.cpp`, `src/core/SymbolMangler.cpp`.
- Tests: parser, runtime ARC/ABI, and JIT language-level coverage under `tests/parsing`, `tests/runtime`, and `tests/jit`.

Remaining semantic expansion:
- Full standalone `any` local variables as two-slot stack values.
- Checked unboxing/casting syntax and deterministic typed failure behavior.
- Throwing language-level subscript helpers if the language chooses throwing collection access instead of non-throwing raw helpers.
