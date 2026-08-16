# HVM Application Binary Interface (HVM-ABI)

Version: `1.6` (silicon-ready revision)

Normative sources:
- `docs/hvm/hvm-spec.md` (ISA, register roles, syscall contract)
- `docs/hvm/ho-file-format.md` (`.ho` module container, symbols, metadata)
- `docs/hvm/hvm_register_set.csv`
- `src/hvm/HVMJIT.cpp` (interpreter / LLVM JIT conformance)
- `src/codegen/HVMCodeGenerator.cpp` (Hoo compiler conventions)

This document is the **normative ABI contract** between a producer (the Hoo
compiler), a consumer (the HVM interpreter, the LLVM JIT, or physical silicon),
and the `hoort` runtime library. It covers the calling convention, stack and
frame layout, data representation, symbol naming, ARC, exceptions, syscalls,
and the native bridge. It intentionally does **not** cover the instruction
encodings (see `docs/hvm/instructions.md`) or the `.ho` container layout (see
`docs/hvm/ho-file-format.md`).

HVM is RISC-V-inspired but is **not** binary-, privilege-, trap-,
memory-model-, or ABI-compatible with RISC-V. In particular the HVM calling
convention is not the RISC-V LP64 convention (`x10-x17`); the LP64 reference
only informs the C/C++ data model (64-bit `long`/pointers).

## 1. Data Model

- 64-bit registers, little-endian, byte-addressable memory.
- LP64-style C/C++ data model: 64-bit pointers, 64-bit `long`/`size_t`.
- `pc` is a byte address; instructions are 4-byte aligned (base32) or begin on
  a 4-byte-aligned boundary (escape32 8-byte lines).
- The stack grows toward lower addresses. The standard HVM ABI requires **16
  bytes** of stack-pointer alignment at every public call boundary.
- Integer and pointer values occupy the full 64-bit register. A 64-bit
  floating-point value is carried as its IEEE-754 binary64 bit pattern in a
  general-purpose register (HVM has no separate FP register file).
- HVM object references are native 64-bit pointers in the public ABI. Compact
  object references are permitted only inside a platform/runtime profile that
  explicitly defines their conversion at ABI boundaries.
- The ABI does **not** define C++ name mangling, DWARF unwinding, or object
  layout; those are compiler/runtime contracts layered above this document
  (see `docs/dev/symbol-mangler.md` for the Hoo mangling scheme).

## 2. Register Roles

| Register | Mnemonic | Role |
|----------|----------|------|
| `r0`  | `zero` | Hardwired zero; reads return 0, writes discarded. |
| `r1`  | `arg1/ret` | First argument and scalar/pointer return-value register. |
| `r2`  | `arg2` | Second argument register. |
| `r3`  | `arg3` | Third argument register. |
| `r4`  | `tp` | Thread pointer. Used for TLS. **Never an argument register** and callee-saved. |
| `r5`  | `arg4` | Fourth argument register. |
| `r6`  | `arg5` | Fifth argument register. |
| `r7`  | `arg6` | Sixth argument register. |
| `r8`  | `arg7` | Seventh argument register. |
| `r9..r15`  | `temp1..7` | Caller-saved temporaries. |
| `r16..r28` | `saved1..13` | Callee-saved registers. |
| `r29` | `lr` | Link register; holds the return address for `RET` (`pc = r29`). |
| `r30` | `fp` | Frame pointer. Required by `ENTER`/`LEAVE`/`FRAME`. |
| `r31` | `sp` | Stack pointer. |

Vector registers (`v0..v31`, `vl`, `vtype`, `vstate`) exist only when
`feature0.Vector` is set; see §11.

## 3. Calling Convention

### 3.1 Argument passing

The first seven scalar or pointer arguments are passed in registers in this
order:

| Argument index | Register |
|---:|----------|
| 0 | `r1` |
| 1 | `r2` |
| 2 | `r3` |
| 3 | `r5` |
| 4 | `r6` |
| 5 | `r7` |
| 6 | `r8` |

`r4` is reserved for the thread pointer and is skipped by the compiler's
argument mapper (`argReg`: `reg = first + index`, then `++reg` when `reg >= 4`).

- **Free functions**: arguments occupy `r1, r2, r3, r5, r6, r7, r8` (max 7).
- **Instance methods and generative constructors**: `this` is passed in `r1`;
  the explicit parameters occupy `r2, r3, r5, r6, r7, r8` (max 6).
- Arguments beyond the register budget are passed in ascending 8-byte stack
  slots at the **caller-owned outgoing argument area** (see §4).

### 3.2 Return values

- A scalar or pointer return value is delivered in `r1`.
- A 64-bit float is returned as its IEEE-754 binary64 bit pattern in `r1`.
- A managed object reference is returned as a native pointer in `r1`.
- `void` functions return no value; `r1` is not significant after return.
- On a native `RET`, the interpreter/JIT returns `r1` to the caller; on silicon
  `RET` performs `pc = r29` with `r1` already holding the result.

### 3.3 Preserved and scratch registers

| Class | Registers |
|-------|-----------|
| Callee-saved (must preserve) | `r16..r28`, `r30` (`fp`), `r31` (`sp`), `r4` (`tp`) |
| Link register | `r29` (`lr`) — a callee saves it when it makes a nested call |
| Caller-saved (scratch) | `r1..r3`, `r5..r15` |

A callee must restore the incoming `sp` before returning. `fp`/`sp` are
restored by the `LEAVE` sequence in ordinary compiled functions.

### 3.4 Stack discipline

- The stack pointer must be 16-byte aligned at every public call boundary.
- The stack grows toward lower addresses.
- The caller owns the outgoing argument area; callees use frame-relative
  addressing for their own locals.

## 4. Frame Layout

Compiled functions begin with `ENTER` and end with `LEAVE; RET`.

### 4.1 `ENTER imm15`

```
1.  sp -= 16
2.  mem[sp + 0] = r29        // save link register (return address)
3.  mem[sp + 8] = r30        // save caller's frame pointer
4.  r30 = sp                 // new frame pointer (fp)
5.  sp -= imm15              // allocate the locals area (frameSize)
```

The interpreter/JIT implement exactly this sequence; physical silicon must
match it. The compiler emits `imm15` = `frameSize`, computed as
`align_to_16(-currentStackOffset)`, so `sp` stays 16-byte aligned.

### 4.2 `LEAVE` / `RET`

```
LEAVE:
1.  sp = r30
2.  r29 = mem[sp + 0]        // restore link register
3.  r30 = mem[sp + 8]        // restore caller's fp
4.  sp = fp + 16
RET:
5.  pc = r29                 // (hosted: native return of r1)
```

### 4.3 Layout diagram

Growing down (lower addresses at the bottom):

```
                ┌──────────────────────┐
 higher addr    │  incoming stack args │   caller-owned outgoing area
                ├──────────────────────┤   <- caller sp at call site
                │  saved FP (r30)      │   fp + 8
                │  saved LR (r29)      │   fp + 0   <- fp (r30)
                ├──────────────────────┤   fp - 16   (top of locals area)
                │  locals / spills     │   fp - 16 - frameSize ... fp - 16
                │  (negative fp-relative offsets)
                ├──────────────────────┤   <- sp (r31) after ENTER
 lower addr     └──────────────────────┘
```

- Locals are addressed **relative to `fp` (`r30`)** with negative `imm15`
  offsets. The compiler spills incoming argument registers into local slots
  with `ST_D <argReg>, 30, <offset>` and loads them back with
  `LD_D <reg>, 30, <offset>`.
- `FRAME rd, fp, imm` computes `rd = fp + imm` and is the standard way to reach
  caller-owned outgoing argument slots.
- `PUSH`/`POP`/`ADJSP` adjust `sp` for temporary storage without touching the
  `fp`-anchored frame.

## 5. Data Representation

### 5.1 Scalar types

| Hoo type | HVM representation |
|----------|--------------------|
| `int64` | full 64-bit signed value in a register |
| `int8` | low byte in a register; sign-extended at ABI boundaries |
| `byte` / `bit` | low byte / low bit; zero-extended to 64 bits at ABI boundaries |
| `bool` | `0` or `1` in the full register |
| `char` | Unicode code point as a 64-bit integer |
| `f64` / `double` | IEEE-754 binary64 bit pattern in a GPR |
| `f8` | canonical E4M3 FP8 byte, encoded/decoded at the `f64` boundary (software-compatible shim when the host lacks native FP8) |

Sub-word results produced by the HVM 1.6 scalar profile (`ARITH_B`,
`SHIFT_B`, `LOGIC_B`, `CMP_B`, `FLOAT_ARITH_B`) are normalized by codegen to
this 64-bit ABI representation before they reach a call boundary.

### 5.2 Reference types

- Every managed object (String, Array, Map, Dict, List, Buffer, user object) is
  prefixed by a hidden **16-byte header**:

  | Offset | Size | Field |
  |-------:|-----:|-------|
  | `-16` | 8 | atomic reference count |
  | `-8`  | 8 | `type_id` (RTTI) |

  `hoo_alloc` returns a pointer **past** the header to the user-data payload;
  that payload pointer is what HVM registers carry.
- A **null reference is `0`**. `LD.D.NZ` (`HVM-NZ`) and `CHK.B` (`HVM-Cap`)
  provide precise null/bounds-checked access that traps with `scause = 20`
  before writing the destination register.
- Borrowed byte slices (`slice<byte>`) are a pointer to a
  `HooByteSliceHandle` (`{ptr, length}`) that does **not** own the backing
  buffer; the producer must keep the backing object alive for the slice
  lifetime.

### 5.3 `any` tagged values

`any` (type ID `0`) is not a managed header type. It denotes the two-slot
`{ type_id, data }` value shape used by heterogeneous containers.

### 5.4 Compiler type IDs

The compiler assigns each Hoo type a stable 64-bit `type_id` used for
`hoo_alloc`, runtime RTTI, exception dispatch, and JSON serialization:

| type_id | Meaning |
|--------:|---------|
| `0` | `any` (tagged value) |
| `1` | `int64` |
| `2` | `f64` / `double` / `float` |
| `3` | `bool` |
| `4` | `void` |
| `5` | `int8` |
| `6` | `byte` |
| `7` | `char` |
| `8` | `bit` |
| `9` | `f8` |
| `100` | generic Object / unknown (may be a raw pointer, so it is excluded from ARC cleanup) |
| `101` | `String` |
| `102` | `Array` |
| `103` | `Map` |
| `104` | `Tensor` |
| `105` | `Random` |
| `106` | `URL` |
| `107` | `HttpResponse` |
| `108` | `HttpClient` |
| `109` | `Character` |
| `110` | `Args` |
| `111` | `Compression` |
| `113` | `Buffer` |
| `114` | `Csv` |
| `117` | `Dict` |
| `118` | `List` |
| `119` | `DateTime` |
| `120` | `Regex` |
| `121` | `Mutex` |
| `122` | `Uuid` |
| `123` | `Future` |
| `125` | `Decimal` |
| `127` | `Socket` |
| `128` | `Condition` |
| `129` | `Semaphore` |
| `130` | borrowed byte slice (`slice<byte>`), pointer ABI |

User-defined classes receive a compiler-assigned type ID; `100` is the default
for unknown/raw cases. Nullability is tracked separately by the compiler (see
§5.5) and does not change the type ID of the underlying type.

### 5.5 Nullability

- `T?` preserves the underlying type ID and class name of `T`; the compiler
  tracks nullability on locals, parameters, fields, and expressions.
- In mangled function symbols a nullable type gets a `?` suffix
  (e.g. `int64?` mangles to `Oi8`).
- At runtime, nullability is enforced with explicit zero checks and, when the
  `HVM_NZ` feature is required, `LD.D.NZ` (trap `scause = 20` on a null
  address).

## 6. Hoo Language ABI (Compiler Conventions)

These are the exact conventions the `HVMCodeGenerator` emits. They are the
primary contract a silicon implementation or a reimplementation must match.

### 6.1 Symbol naming

| Prefix | Meaning |
|--------|---------|
| `_F_` | function symbol |
| `_H_` | module-level symbol (type descriptor, global, object) |
| `_F_module_init_v` | module initializer (runs singleton allocation + constructors) |

Function symbols encode module path, class/base class, modifiers, return type,
and parameter types via `SymbolMangler` (see `docs/dev/symbol-mangler.md`).
Type codes used inside mangled names:

| Code | Type | Code | Type |
|------|------|------|------|
| `i8` | `int64` | `i1` | `int8` |
| `d`  | `double` / `f64` | `e`  | `f8` |
| `f`  | `float` | `x`  | `bit` |
| `b`  | `bool` | `c`  | `char` |
| `s`  | `string` | `v`  | `void` |
| `p`  | `ptr` | `t`  | `tensor` |
| `u1` | `byte` | `y`  | `any` |
| `o`  | unknown (fallback) | `a`  | array (legacy) |

The compiler's `typeIdToMangleType` maps type IDs onto these names
(`1→int64`, `2→double`, `3→bool`, `4→void`, `5→int8`, `6→byte`, `7→char`,
`8→bit`, `9→f8`, `101→string`, `104→tensor`, `123→ptr`, default→`ptr`), and
`mangleTypeId` appends `?` for nullable types.

### 6.2 Prologue and argument spilling

```
ENTER 0                     // imm15 patched to frameSize at function end
ST_D  <argReg>, 30, <off>   // spill each incoming argument to its local slot
ST_D  1, 30, <thisOff>      // methods: spill 'this' (already in r1)
```

The compiler reserves a local slot per parameter (up to the register budget)
and a `this` slot for methods. Async functions additionally reserve an
`__async_future__` local and call `_F_hoo_future_new_native_i64`.

### 6.3 Call sites

- Arguments are evaluated into temporary registers, then moved into argument
  registers via the `argReg` mapping (`r1..r3,r5..r8`; `r2..r8` for methods
  after `this`).
- `CALL` is emitted with a placeholder displacement and recorded in the
  module's symbol-fixup list; the layout pass patches the displacement to the
  callee's encoded position (see `ho-file-format.md` §11).
- `CALL_OVERLOADED` is used at overload dispatch sites so the JIT can resolve
  the call through `hoo_resolve_overload`.
- Calls to undefined (import) symbols with `section_index == -1` are resolved
  through the runtime state-ABI table (§9).

### 6.4 Return sequence

For a non-`void` return, the compiler emits:

```
MOV  1, <resultReg>     // result in r1
RETAIN 1, 1, 0, 0       // retain managed return values (ARC)
<scope cleanup>         // release ARC-managed locals
LEAVE
RET
```

The `RETAIN r1,r1` balances the callee's eventual release of its own
reference, so the returned object survives long enough for the caller to
consume it.

### 6.5 Module initialization

`_F_module_init_v` is the module constructor. It runs `ENTER`, allocates each
pending singleton (`hoo_alloc`), stores the singleton pointer into a `.data`
slot via `LDA` with `rs=1` (data base), calls the class constructor, and ends
with `LEAVE; RET`. Its symbol carries `STT_FUNC`/`STB_GLOBAL`.

### 6.6 Data and string addressing

`LDA rd, rs, imm` computes `rd = rs + sign_extend(imm)` with two reserved base
conventions implemented by both the interpreter and the JIT:

| `rs` | Base | Meaning |
|------|------|---------|
| `0` | `.rodata` base | read-only data / string literals |
| `1` | `.data` base | mutable module data |

Rules:
- When `rs == 0` the offset must be in range for the `.rodata` section; larger
  `.rodata` offsets are built with chained `ADDI` steps from a bounded `LDA`.
- When `rs == 1` the offset is applied to the `.data` base.
- Any other `rs` is an ordinary register-relative address.
- String literals are placed NUL-terminated in `.rodata` and materialized by
  `LDA rd, 0, offset`, loading the literal length into the following argument
  register, then calling the `String.fromBytes` bridge
  (`_F_M_hoo_E_String_fromBytes_static_p_p_p`). The explicit length preserves
  embedded NUL bytes; the older `String.fromCStr` bridge
  (`_F_M_hoo_E_String_fromCStr_static_p_p`) truncates at the first NUL and is
  only used where the caller already has a C string.
- Text-section offsets (e.g. exception handler PCs) must be materialized as
  integer immediates (`ADDI reg, 0, off`) so `LDA`'s `rs=0` rodata special
  case cannot reinterpret them as data pointers.

### 6.7 Type metadata

- The `.types` section (`SHT_TYPES`) stores NUL-terminated type-descriptor
  strings produced by `generateType`.
- `STT_TYPE` symbols reference type records; `STT_OBJECT` symbols reference
  data objects.

## 7. ARC (Automatic Reference Counting)

- Managed objects carry an atomic reference count in their 16-byte header.
- `hoo_alloc(size, type_id)` returns an object with `refcount = 1`.
- `hoo_retain(obj)` increments and returns `obj`; `hoo_release(obj)` decrements
  and frees at zero.
- The compiler emits `RETAIN`/`RELEASE` instructions (HVM 1.6 green-compute
  core) for ARC-managed values, and calls `_F_hoo_release_v_p` during scope
  cleanup for every ARC-managed local that was not explicitly released.
- ARC applies to type IDs `>= 100` with explicit exclusions for types that
  manage their own lifecycle (`Args`, `Compression`, `Regex`, `Mutex`, `Uuid`,
  `Exception`, `Condition`, `Semaphore`) and for `any` (`100`), which may hold
  raw pointers or primitives.
- Managed stores (JIT `ST.D` instrumentation) retain the new value and release
  the old value when both are non-null.

## 8. Exception Handling ABI

`try/catch/finally` and `throw` compile to `SYSCALL`-based handler-stack
operations plus ordinary control flow:

| Operation | Emission | Contract |
|-----------|----------|----------|
| Push handler | `MOV r2, <handlerPc>`; `SYSCALL 7` | `r2` = handler PC (a text offset); shadow handler stack in the runtime |
| Pop handler | `SYSCALL 8` | normal path |
| Throw | `MOV r2, <exc>`; `SYSCALL 9` | `r2` = thrown exception handle; runtime rewinds to the nearest handler |
| Rethrow | `SYSCALL 10` | re-throw the in-flight exception |

- On the catch path the runtime places the exception handle in `r1`.
- Catch-clause dispatch calls `_F_hoo_exception_matches_type_i8_p_i8`
  (`r1` = exception, `r2` = expected type ID; returns match in `r1`) followed
  by `BEQ r1, 0`.
- `Exception` (type ID `100` match rule) is the open base type compatible with
  every runtime exception.
- Runtime-created exceptions: `_F_hoo_exception_null_pointer_p` (null-check
  path) and `_F_hoo_exception_runtime_p`.
- The runtime functions are `hoo_push_handler(void*)`, `hoo_pop_handler()`,
  `hoo_throw_handler(void*)`, `hoo_rethrow_handler()`; see
  `src/runtime/lib/exception/hoo_exception.cpp`.

## 9. SYSCALL Contract and Runtime Bridge

### 9.1 `SYSCALL`

`SYSCALL imm15` dispatches to the platform/runtime service table. Arguments are
passed in `r2` (and `r3` for two-argument, `r4` for three-argument services);
the result is written to the instruction's `rd`. The full normative table
(runtime 1–11, OS 12–23) is in `docs/hvm/hvm-spec.md` §7.

### 9.2 Native runtime bridge (state-ABI)

Runtime bridge functions are declared `extern "C"` and exported under the exact
mangled names the compiler emits. Each bridge has the signature:

```c
uint64_t jit_hoo_<name>(void* state_ptr);   // state = HVM execution state
```

- Arguments are passed **through the HVM register file** (`state->regs[1]`,
  `state->regs[2]`, ...), exactly as the HVM calling convention dictates.
- The return value is delivered as the function's return value, which the
  interpreter/JIT writes into `r1`.
- The interpreter dispatches `CALL` to an import symbol through
  `invokeStateAbiSymbol(calleeName, state, ...)`; the JIT resolves the same
  names through its runtime symbol table.

Key runtime symbols emitted by the compiler:

| Symbol | Registers read | Returns in |
|--------|----------------|------------|
| `_F_hoo_alloc_p_i8_i8` | `r1` = size, `r2` = type_id | `r1` = pointer |
| `_F_hoo_release_v_p` | `r1` = object | — |
| `_F_hoo_retain_p_p` | `r1` = object | `r1` = object |
| `_F_hoo_get_refcount_i8_p` | `r1` = object | `r1` = count |
| `_F_hoo_get_type_id_i8_p` | `r1` = object | `r1` = type_id |
| `_F_hoo_exception_matches_type_i8_p_i8` | `r1` = exc, `r2` = type_id | `r1` = 0/1 |
| `_F_hoo_exception_null_pointer_p` | — | `r1` = exception |
| `_F_hoo_future_new_native_i64` | `r1` = element type_id | `r1` = Future |
| `_F_hoo_future_set_value_native_v_p_p` | `r1` = Future, `r2` = value | — |
| `_F_hoo_future_set_error_native_v_p_p` | `r1` = Future, `r2` = error message | — |
| `_F_hoo_future_await_unwrap_native_p_p` | `r1` = Future | `r1` = value, `r2` = error flag, `r3` = exception handle |
| `_F_hoo_push_handler_v_p` | `r2` = handler PC | — |

Allocation (`hoo_alloc`) uses a thread-local allocation buffer (64 KiB blocks,
objects ≤ 2048 bytes bump-allocated) with a `malloc` fallback; see
`docs/runtime/memory-model.md`.

#### 9.2.1 The `await` bridge ABI

`_F_hoo_future_await_unwrap_native_p_p` is the bridge the compiler emits for an
`await` expression. It blocks until the Future is ready while pumping the libuv
event loop (condition-variable wait with cooperative pumping, never a
busy-spin) and **must not unwind C++ stacks**. Its results are delivered
through the register file:

- `r1` — resolved value. ARC-managed values are retained for the caller;
  primitive values arrive as raw register bits.
- `r2` — error flag: `1` when the Future was rejected, `0` on success.
- `r3` — exception handle (refcount 1, owned by the call site) when `r2 == 1`,
  otherwise undefined.

On success the compiler consumes `r1` directly. On rejection it emits
`SYSCALL kSysThrowToHandler` (9) with the handle in `r2` so control transfers to
the nearest registered handler (enclosing `try/catch`) exactly as a `throw`
statement would, in both the interpreter and the JIT.

## 10. FFI Boundary (C ABI)

- Runtime library entry points are `extern "C"` and versioned for stability
  (`HOO_API` on Windows).
- `void* hoo_alloc(size_t, int64_t type_id)` and the ARC family
  (`hoo_retain`, `hoo_release`, `hoo_get_refcount`, `hoo_get_type_id`) are the
  C ABI core (see `docs/runtime/memory-model.md` §4).
- `hoo_register_destructor(int64_t type_id, HooDestructor)` maintains the
  runtime destructor registry; any non-negative type ID is valid and the
  registry grows on demand.
- The C ABI must not throw C++ exceptions across the Hoo/HVM boundary.
- A shared-library consumer resolves the exported symbol by the **exact
  mangled string** the compiler emits; `extern "C"` prevents the C++ compiler
  from altering it.

## 11. Vector ABI (HVM-V)

Present only when `feature0.Vector` is set. The hosted HVMJIT profile fixes
`VLEN = 64` so `VLMAX = 8`.

- `v0` — vector mask register and vector return/temporary register.
- `v1..v7` — caller-saved vector temporaries.
- `v8..v15` — callee-saved in ABI profiles that enable preserved vector state.
- `vl`, `vtype` — caller-saved; `vstate` ∈ {Off, Initial, Clean, Dirty}.
- Implementations that expose HVM-V must save/restore vector state per this
  profile (see `hvm-spec.md` §5.5.1).

## 12. Module ABI

- Native HVM64 modules set `target_arch = HVM64 (0x02)`, `pointer_size = 8`,
  little-endian (see `ho-file-format.md` §3).
- Required ISA features are mirrored in header `flags[12:0]` and stated
  authoritatively in the `.note` section; a loader must reject a module whose
  required feature bits the target CPU/profile does not implement
  (`hvm-spec.md` §10.4).
- Function metadata (`.funcmeta`) records entry RVA, code size, local size,
  parameter/return type offsets, flags, and source line.
- Exported functions are `STB_GLOBAL`/`STT_FUNC`; private functions are
  `STB_LOCAL`. Data objects are `STT_OBJECT`; type records are `STT_TYPE`.
- `_F_module_init_v` is the entry point executed at module load.

## 13. Alignment and Traps Quick Reference

| Access | Required alignment |
|--------|--------------------|
| `LD.H` / `ST.H` | 2 bytes |
| `LD.W` / `ST.W` | 4 bytes |
| `LD.D` / `ST.D`, `LR.D` / `SC.D`, `LD.P` / `ST.P` | 8 bytes |
| `LD.B` / `ST.B`, `LDA` | none |
| `JAL` / `JMP` / `JALR` targets | 4 bytes (misaligned `JALR` sum traps `scause = 0`) |

Misaligned memory access raises the corresponding address-misaligned trap;
`LD.D.NZ`/`CHK.B` null/bounds faults use `scause = 20` and are precise (the
destination register is not written).

## 14. Conformance

The interpreter and the LLVM JIT are execution engines for the same
architecture and the same ABI; they are not separate ABIs. A conforming
implementation must:

- use the register roles and calling convention in §2–§3;
- produce and consume the frame layout in §4;
- represent data as specified in §5;
- resolve compiler-emitted symbols and runtime bridge names as in §6 and §9;
- preserve the ARC, exception, and syscall contracts in §7–§9;
- return the same hosted execution result for normal completion, unhandled
  traps, and explicit termination (`hvm-spec.md` §8.1).

Physical silicon implementing the Silicon MVP profile must additionally honor
the trap-entry and CSR contracts in `hvm-spec.md` §9–§10.
