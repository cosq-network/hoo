# JIT Integration & Bridging

The `HVMJIT` dynamically translates HVM bytecode into LLVM IR. The runtime library acts as the foundational environment for this translated code. This document describes the specific integration mechanisms.

## 1. The Syscall Interface (`SYSCALL`)
The JIT lowers the `SYSCALL` instruction (`0xC0`) directly into highly optimized LLVM IR calls to the runtime library. 

| Syscall ID | Target Implementation | Register Usage | Description |
| :--- | :--- | :--- | :--- |
| `1` | `hoo_hvm_sys_alloc` | `rd = alloc(r2, r3)` | Heap allocation. `r2` = size, `r3` = type_id. |
| `2` | `hoo_hvm_sys_retain` | `rd = retain(r2)` | Atomically increments refcount. |
| `3` | `hoo_hvm_sys_release` | `rd = release(r2)` | Atomically decrements refcount. |
| `4` | `hoo_hvm_sys_refcount` | `rd = refcount(r2)`| Returns current refcount. |
| `5` | `hoo_hvm_sys_typeid` | `rd = typeid(r2)`  | Returns object RTTI type. |
| `6` | `hoo_hvm_sys_exception_runtime` | `rd = exc()` | Emits a generic runtime exception object. |
| `7` | `hoo_hvm_sys_push_handler_state`| `rd = push(r2)`| Registers shadow stack frame (PC in r2). |
| `8` | `hoo_hvm_sys_pop_handler_state` | `rd = pop()` | Removes top shadow stack frame. |
| `9` | `hoo_hvm_sys_throw_to_handler_state` | `rd = throw(r2)` | Throws exc `r2`, returns target handler PC. |
| `10` | `hoo_hvm_sys_rethrow_to_handler_state`| `rd = rethrow()` | Rethrows, returns target handler PC. |
| `11` | `hoo_hvm_sys_string_data` | `rd = strdata(r2)` | Returns absolute host pointer to raw UTF-8. |
| `12` | `hoo_hvm_sys_thread_create` | `rd = thread_create(r2, r3)` | Spawns a thread running function at offset `r2` with argument `r3`. |
| `13` | `hoo_hvm_sys_thread_exit` | `rd = thread_exit(r2)` | Exits current thread returning `r2`. |
| `14` | `hoo_hvm_sys_futex` | `rd = futex(r2, r3, r4, r5)` | Linux futex. Returns -1 on non-Linux (stub). |
| `15` | `hoo_hvm_sys_get_tid` | `rd = get_tid()` | Returns the calling thread's OS-level ID. |
| `16` | `hoo_hvm_sys_open` | `rd = open(r2, r3, r4)` | Opens file at HVM-memory path offset `r2` with flags `r3` and mode `r4`. |
| `17` | `hoo_hvm_sys_read` | `rd = read(r2, r3, r4)` | Reads up to `r4` bytes from fd `r2` into buffer at HVM offset `r3`. |
| `18` | `hoo_hvm_sys_write` | `rd = write(r2, r3, r4)` | Writes `r4` bytes from buffer at HVM offset `r3` to fd `r2`. |
| `19` | `hoo_hvm_sys_close` | `rd = close(r2)` | Closes file descriptor `r2`. |
| `20` | `hoo_hvm_sys_lseek` | `rd = lseek(r2, r3, r4)` | Seeks fd `r2` to offset `r3` relative to whence `r4`. |
| `21` | `hoo_hvm_sys_fstat` | `rd = fstat(r2, r3)` | Gets file status for fd `r2` into `struct stat` at HVM offset `r3`. |
| `22` | `hoo_hvm_sys_clock_gettime` | `rd = clock_gettime(r2, r3)` | Gets clock `r2` time into `struct timespec` at HVM offset `r3`. |
| `23` | `hoo_hvm_sys_getrandom` | `rd = getrandom(r2, r3)` | Fills buffer at HVM offset `r2` with `r3` random bytes. |

Statement exception lowering uses syscalls 7–10: it loads handler PCs as text
offsets with an integer immediate and passes them in `r2`, matching the
shadow-stack bridge ABI. This is required because throw and rethrow return a
handler PC and transfer control rather than behaving like ordinary calls.
Catch dispatch preserves the thrown handle across the handler-pop call and uses
the runtime type-compatibility helper before entering a catch clause.

The compiled throw/rethrow path routes the returned handler PC through a
switch whose cases are the function's valid exception-dispatch targets (see
`docs/dev/hvm-jit.md`, "Exception dispatch targets"). The target set spans the
current function's entire text range — bounded by the next function's entry —
so catch-handler blocks that follow an early `return` remain reachable; a
handler PC with no matching case is treated as unhandled and returns `-1`,
letting `run()` fall back to the interpreter.

**Pointer note**: Syscalls 16-18, 21-23 take HVM-memory offsets (not host virtual addresses) for buffer/string/path arguments. The runtime translates these offsets to real addresses at call time via an internal `g_hvm_memory` base pointer, so HVM code passes raw register values without manual address arithmetic.

**Platform note**: On Windows, syscalls 7-10 are not lowered to LLVM IR. `ensureJITFunctionTable()` returns `false` for these opcodes, causing the JIT to fall back to the interpreter for handler operations. The JIT bridge functions (`jit_hoo_throw`, `jit_hoo_rethrow`, `hoo_hvm_sys_throw_to_handler_state`, `hoo_hvm_sys_rethrow_to_handler_state`) use `hoo_exception_set_current()` instead of C++ try/catch on Windows, while macOS/Linux retain the original C++ exception-based path. Syscalls 12-15 (threading) are implemented via pthreads and are unavailable on Windows.

## 2. ARC Optimization Pass (`ARCUseDefGraph`)
To prevent severe performance degradation from excessive reference counting, the JIT executes an `ARCUseDefGraph` analysis pass over the instruction stream before IR translation.
- **Pattern Matching**: It searches for `SYSCALL 2` (retain) and `SYSCALL 3` (release) pairs that apply to the same object within a linear execution block.
- **Elimination**: If an object is retained and subsequently released with no branching or overriding writes to `r2` in between, the pass marks both instructions in the `skipPc` set, completely eliminating the ARC overhead at runtime.

## 3. Host Symbol Bridging
The JIT maintains a registry of host-native functions that can be called directly via the `CALL` instruction. During module loading, any `UNDEFINED` symbols (global binding with `section_index == -1`) are resolved against the `hoo` JITDylib. This allows HVM code to interact with runtime services like `print`, `println`, and string conversion intrinsics without using the slow `SYSCALL` path.

Defined function symbols now set `section_index = 0` (instead of leaving it uninitialized) to ensure consistent section-aware lookups in `HVMCodeGenerator::endFunction()`.

### 3.1 `any`, `HashMap`, and `AnyArray` Bridges

The heterogeneous collection intrinsics introduced by ISSUE-033 use ordinary HVM `CALL` lowering and do not add opcodes. `HashMap` and `AnyArray` handles are opaque 64-bit managed-object pointers. The shared `any` payload layout is two 64-bit fields: `type_id` and `data`.

Raw runtime APIs that return `any` write into an explicit out-buffer:

- `hoo_anyarray_get(array, index, HooAnyValue* out)`
- `hoo_anyarray_pop(array, HooAnyValue* out)`
- `hoo_hashmap_get_any_i8(map, key, HooAnyValue* out)`

Current expression-level JIT lowering also registers scalar data bridge helpers such as `_F_hoo_anyarray_get_data_i8_p_i8` and `_F_hoo_hashmap_get_any_data_i8_p_i8`. These helpers are JIT conveniences for payload reads in scalar expressions; they do not expose C++ container layout and must remain behaviorally equivalent to the out-buffer APIs for success/failure and ownership.

Arguments follow the normal HVM register convention: `r1`, `r2`, `r3`, then `r5` and above because `r4` is reserved as the thread pointer. Four-argument helpers such as `_F_hoo_anyarray_set_i8_p_i8_i8_i8` and `_F_hoo_hashmap_set_any_i8_p_i8_i8_i8` therefore read the fourth logical argument from `r5`.

### 3.2 Flexible Symbol Resolution
The JIT employs `buildLookupCandidates()` to resolve symbols that may have been compiled with different mangling conventions:

1. **Direct match**: The exact symbol name is tried first.
2. **Demangled reconstruction**: The symbol is demangled via `SymbolMangler::demangleSymbol()`, and both top-level function and class member forms are re-mangled with module path qualification.
3. **Legacy prefix match**: For `_F_`-prefixed symbols from interpreter bytecode, the base function name is extracted and searched with a prefix pattern.
4. **Fuzzy fallback**: A substring containment check is used as a last resort.

`lookupPlainRuntimeSymbolAddress()` provides a static map of common runtime symbols (`hoo_alloc`, `hoo_retain`, `hoo_release`, etc.) as a fast fallback in `getSymbolAddress()` when JIT and module lookups fail.

## 4. The `HVMState` Struct
In LLVM IR, the running state of the VM is passed around as a pointer to the `hvm.state` struct:
```llvm
%hvm.state = type { [32 x i64], i8*, i8* }
```
- `[32 x i64]`: The 32 general-purpose 64-bit registers. Register `r4` is reserved as `tp` (thread pointer) and is skipped during codegen argument allocation.
- `i8*`: The base pointer to the emulated 16MB module memory (for `.text`, `.data`, `.rodata`).
