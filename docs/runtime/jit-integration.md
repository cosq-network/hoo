# JIT Integration & Bridging

The `HVMJIT` dynamically translates HVM bytecode into LLVM IR. The runtime library acts as the foundational environment for this translated code. This document describes the specific integration mechanisms.

## 1. The Syscall Interface (`SYSCALL`)
The JIT lowers the `SYSCALL` instruction (`0xC0`) directly into highly optimized LLVM IR calls to the runtime library. 

| Syscall ID | Target Implementation | Register Usage | Description |
| :--- | :--- | :--- | :--- |
| `1` | `hooc_hvm_sys_alloc` | `rd = alloc(r2, r3)` | Heap allocation with ARC tracking. |
| `2` | `hooc_hvm_sys_retain` | `rd = retain(r2)` | Atomically increments refcount. |
| `3` | `hooc_hvm_sys_release` | `rd = release(r2)` | Atomically decrements refcount. |
| `4` | `hooc_hvm_sys_refcount` | `rd = refcount(r2)`| Returns current refcount. |
| `5` | `hooc_hvm_sys_typeid` | `rd = typeid(r2)`  | Returns object RTTI type. |
| `6` | `hooc_hvm_sys_exception_runtime` | `rd = exc()` | Emits a generic runtime exception object. |
| `7` | `hooc_hvm_sys_push_handler_state`| `rd = push(r2)`| Registers shadow stack frame (PC in r2). |
| `8` | `hooc_hvm_sys_pop_handler_state` | `rd = pop()` | Removes top shadow stack frame. |
| `9` | `hooc_hvm_sys_throw_to_handler_state` | `rd = throw(r2)` | Throws exc `r2`, returns target handler PC. |
| `10` | `hooc_hvm_sys_rethrow_to_handler_state`| `rd = rethrow()` | Rethrows, returns target handler PC. |
| `11` | `hooc_hvm_sys_string_data` | `rd = strdata(r2)` | Returns absolute host pointer to raw UTF-8. |

## 2. ARC Optimization Pass (`ARCUseDefGraph`)
To prevent severe performance degradation from excessive reference counting, the JIT executes an `ARCUseDefGraph` analysis pass over the instruction stream before IR translation.
- **Pattern Matching**: It searches for `SYSCALL 2` (retain) and `SYSCALL 3` (release) pairs that apply to the same object within a linear execution block.
- **Elimination**: If an object is retained and subsequently released with no branching or overriding writes to `r2` in between, the pass marks both instructions in the `skipPc` set, completely eliminating the ARC overhead at runtime.

## 3. Inbound FFI Trampolines
When a native C/C++ library needs to trigger a callback into Hooc-compiled code, it requires a raw function pointer. 
- **The Problem**: LLVM-compiled HVM functions expect an `HVMState*` as their first parameter, making their ABI incompatible with standard C callbacks.
- **The Solution**: The JIT provisions an array of predefined trampoline functions (`hooc_hvm_inbound_trampoline_N`). When `createInboundTrampoline` is called, it allocates an available trampoline slot and binds it to the requested HVM module/function pair. When the native library invokes the trampoline, it constructs a temporary `HVMState`, maps the C arguments into virtual registers (`r1`...`r7`), and initiates a re-entrant call into the JIT engine.

## 4. The `HVMState` Struct
In LLVM IR, the running state of the VM is passed around as a pointer to the `hvm.state` struct:
```llvm
%hvm.state = type { [32 x i64], i8*, i8* }
```
- `[32 x i64]`: The 32 general-purpose 64-bit registers.
- `i8*`: The base pointer to the emulated 16MB module memory (for `.text`, `.data`, `.rodata`).
