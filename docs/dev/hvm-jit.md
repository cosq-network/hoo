# How HVMJIT Works

**File:** `src/hvm/HVMJIT.h` / `.cpp` (~8224 lines)

`HVMJIT` is the LLVM ORC-based JIT execution engine. It loads compiled `HOModule` bytecode, translates it to LLVM IR, and executes it natively via LLVM ORC JIT. It also supports running modules via an interpreted fallback. With ~100+ `jit_hoo_*` runtime wrappers, it bridges Hoo built-in functions to C runtime implementations.

## What LLVM ORC Does

LLVM ORC is LLVM’s in-process, on-demand compilation and linking framework. In this codebase it provides three jobs:

1. Turn translated LLVM IR into native machine code.
2. Resolve symbols across multiple modules and runtime libraries.
3. Keep the JIT isolated from platform-specific linker details.

`HVMJIT` uses ORC instead of a direct static linker because Hoo modules are loaded dynamically. The runtime does not know up front which source files, runtime intrinsics, or imported modules will be available. ORC lets the engine compile pieces independently and link them lazily through a symbol table.

## Core ORC Objects

The JIT uses a small set of ORC types repeatedly:

- `llvm::orc::LLJIT`: the top-level JIT container
- `llvm::orc::JITDylib`: a named symbol namespace
- `llvm::orc::ThreadSafeContext`: shared LLVM context wrapper
- `llvm::orc::ThreadSafeModule`: a module plus owning context, safe to move across threads
- `llvm::orc::MangleAndInterner`: converts a symbol name into ORC’s internal lookup form

The important practical detail is that Hoo keeps separate `JITDylib` instances for modules and for runtime exports. That avoids accidental symbol collisions and makes lookup order explicit.

## Architecture

```cpp
class HVMJIT {
    // JIT infrastructure
    std::unique_ptr<llvm::orc::LLJIT> jit_;
    llvm::orc::ThreadSafeContext tsc_;
    std::unique_ptr<HooCompiler> sourceCompiler_;
    
    // Module management
    hvm::HVMModuleBundle bundle_;
    std::unordered_map<std::string, std::shared_ptr<hvm::HOModule>> loadedModules_;
    std::unordered_map<std::string, ModuleRegistryEntry> moduleRegistry_;
    std::unordered_map<std::string, LoaderState> moduleStates_;
    std::unordered_map<std::string, llvm::orc::JITDylib*> moduleDylibs_;
    
    // Memory
    std::vector<uint8_t> memory_;   // Virtual memory for interpreted mode
    uint64_t memoryTop_ = 0;
    
    // Runtime state
    IOProvider& io_;
    bool modulesMaterialized_ = false;
    bool modulesInitialized_ = false;
};
```

## HVMState — execution context

The engine uses an `HVMState` struct to represent VM state during interpreted execution:

```cpp
struct HVMState {
    int64_t regs[32]{};             // 32 general-purpose registers
    uint8_t* memory = nullptr;      // Virtual memory pointer
    IOProvider* io = nullptr;
    bool trapHit = false;
    int64_t loop_count = 0;
    int64_t loop_backedge = 0;
    int64_t vregs[32][8]{};        // Vector registers (for tensor ops)
    int64_t vl = 0;                // Vector length
    int64_t vtype = 0;             // Vector type
};
```

Register convention (from `argReg()` in HVMCodeGenerator):
- r0-r3, r5-r31: General purpose (r4 is the thread pointer, skipped)
- r29: Link register (LR)
- r30: Frame pointer (FP)
- r31: Stack pointer (SP)
- r1-r2: Argument/return value registers for runtime wrappers

## Module lifecycle

### Loader state machine

Each module transitions through states:

```
Discovered → Parsed → Validated → Registered → DependenciesResolved → Ready → (materialized)
                                                                        ↓
                                                                     Failed
```

### Entry points

| Method | Purpose |
|---|---|
| `loadInput(pathOrModuleName)` | Auto-detects source vs bytecode |
| `loadModule(path)` | Loads from `.hoo` binary file |
| `loadModule(unique_ptr<HOModule>)` | Loads from in-memory module |
| `loadSource(path)` | Compiles `.ho` source file then loads |
| `loadSourceCode(name, code)` | Compiles source string then loads |
| `loadBytecode(path)` | Loads pre-compiled bytecode |
| `run(entryPoint)` | Executes entry function and returns result |

### Load flow

```
loadInput() / loadSource() / loadBytecode()
    ↓
parseAndLoadModuleFromPath() — reads file, constructs HOModule
    ↓
registerModuleInBundle() — registers in the module bundle
    ↓
validateModule() — validates headers, sections, symbols
    ↓
buildModuleRegistryEntry() — indexes symbols, exports, imports by name
    ↓
resolveAndLoadDependencies() — recursively loads imported modules
    ↓
initializeDependencyGraphPostOrder() — topological sort
    ↓
bootstrapRuntimeModules() — loads runtime native libraries
registerRuntimeSymbolsInJITDylib() — registers jit_hoo_* symbols
    ↓
configureJITDylibs() — sets up search orders and visibility
    ↓
materializeModulesToJIT() — translates all modules to LLVM IR
    ↓
runPostLoadInitializers() — runs module_init functions
```

### ORC translation flow

Once a module reaches the materialization phase, the pipeline is:

1. Build an LLVM `Module` from HVM bytecode.
2. Wrap it in a `ThreadSafeModule`.
3. Add it to the correct module `JITDylib`.
4. Let ORC materialize the object lazily or immediately depending on lookup demand.
5. Resolve external calls through the runtime JITDylib.

That means codegen does not emit native code directly. It emits LLVM IR, and ORC becomes the linker and object manager.

### Symbol lookup

`getSymbolAddress(mangledName)` looks up a symbol across all loaded modules using `buildLookupCandidates()`:

```cpp
std::vector<std::string> buildLookupCandidates(const std::string& symbolName,
                                                const std::string& moduleName) {
    std::vector<std::string> candidates;
    appendUnique(candidates, symbolName);
    
    // Demangle and reconstruct both plain and member forms
    const auto demangled = SymbolMangler::demangleSymbol(symbolName);
    // ... builds both _F_ plain and _F_ ClassName versions
    return candidates;
}
```

This handles the mismatch between legacy test names and modern module-qualified names.

### How lookup works in practice

Symbol resolution in `HVMJIT` typically proceeds in this order:

1. Try the exact mangled symbol requested by codegen.
2. Try names reconstructed from demangling, including module-qualified and member-qualified variants.
3. Look in the primary module `JITDylib`.
4. Fall back to the main `JITDylib`.
5. Fall back to the runtime plain-symbol map for core helpers such as allocation and string utilities.

This layered lookup is needed because Hoo can produce:

- plain source-level calls
- module-qualified symbols
- built-in runtime calls
- legacy interpreter-era names

The lookup order preserves compatibility without forcing all call sites to use the same spelling.

## JIT compilation pipeline

### translateModule

The private `translateModule(hvm::HOModule&)` is the core JIT compiler. It:
1. Creates an LLVM module with the appropriate target triple
2. Allocates module-level globals (constants from `.rodata`, mutable data from `.data`)
3. Translates each function from HVM bytecode to LLVM IR
4. Sets up debug info if `HOOC_ENABLE_DWARF=1`
5. Returns a `ThreadSafeModule` for ORC

The returned `ThreadSafeModule` matters because ORC may materialize code on a worker thread. The context must remain valid while ORC owns or compiles the module, so `ThreadSafeModule` keeps the LLVM context and module paired together safely.

### JITDylibs and visibility

`HVMJIT` creates separate `JITDylib` instances for modules and for the shared `hoo` runtime namespace. This is important for two reasons:

- module-local symbols should not unintentionally shadow runtime helpers
- runtime helpers should remain visible to all loaded modules

In ORC, symbol resolution is scoped by `JITDylib` search order. `HVMJIT` uses that to make the runtime namespace visible while still keeping module ownership explicit.

### Runtime symbol table

All `jit_hoo_*` symbols are registered in the JIT dylib via `registerRuntimeSymbolsInJITDylib()`. These are `extern "C"` functions that follow the convention:

```cpp
uint64_t jit_hoo_<name>(void* state_ptr) {
    auto* state = reinterpret_cast<HVMJIT::HVMState*>(state_ptr);
    // Read args from state->regs[1], state->regs[2], etc.
    // Write result to state->regs[1] or return as uint64_t
}
```

`registerRuntimeSymbolsInJITDylib()` uses ORC’s symbol registration APIs to publish these wrappers into the `hoo` namespace. The code generator then emits mangled `CALL` targets that resolve against that namespace at runtime.

### IR lowering support

`isSupportedForIRLowering(op, func)` determines whether a given opcode can be lowered to native LLVM IR vs needing a runtime call. Most simple arithmetic, memory, and control flow ops are lowered directly.

The division of labor is:

- lower simple arithmetic and control-flow to LLVM IR when the semantics match directly
- call runtime wrappers for operations that depend on Hoo library behavior, object layout, or ARC
- fall back to the interpreter if ORC cannot materialize a function or the platform disables a lowering path

That is why the JIT can still execute a module even when part of the runtime surface is only available through a fallback path.

## ARC Use-Def optimization

Located at the top of `HVMJIT.cpp`, the `buildARCUseDefGraph()` function performs a dataflow analysis to eliminate redundant retain/release pairs:

```cpp
struct ARCUseDefGraph {
    std::unordered_set<uint64_t> skipPc;  // PCs to skip (no-op)
};

ARCUseDefGraph buildARCUseDefGraph(const hvm::Section& text, uint64_t entryPc) {
    // 1. Decode instruction stream starting from entryPc
    // 2. Build control flow graph (successor edges)
    // 3. Forward dataflow: track pending retain/release on reg2
    // 4. When retain+release (or release+retain) found on same reg
    //    without intervening writes, mark both as skip
}
```

Enable with `HOOC_ENABLE_ARC_USEDEF=1` (disabled by default).

Controlled by environment variables:

| Variable | Effect |
|---|---|
| `HOOC_ENABLE_ARC_USEDEF=1` | Enables ARC use-def optimization |
| `HOOC_ENABLE_ESCAPE_ALLOCA=1` | Enables escape-based alloca promotion |
| `HOOC_ENABLE_DWARF=1` | Enables DWARF debug info generation |

## Shadow exception handling

Exception handling is implemented via a shadow stack of handler frames:

```cpp
struct ShadowHandlerFrame {
    uint64_t handlerPc = 0;
    int64_t savedLr = 0;
    int64_t savedFp = 0;
    int64_t savedSp = 0;
};
std::unordered_map<void*, std::vector<ShadowHandlerFrame>> gShadowHandlers;
```

Key functions:
- `shadow_push_handler(state, handlerPc)` — Saves LR/FP/SP, pushes handler frame
- `shadow_pop_handler(state)` — Pops handler frame
- `shadow_throw_to_handler(state, exc, rethrow)` — Restores LR/FP/SP, returns handler PC, sets r1 to exception
- `shadow_clear_state(state)` — Cleans up on state destruction
- `shadow_should_stop_state(state_ptr)` — Checks `stopExecutionRequested_` flag

JIT wrappers:
- `jit_hoo_push_handler` / `jit_hoo_pop_handler` — Push/pop shadow handler
- `jit_hoo_throw` / `jit_hoo_rethrow` — Throw/rethrow with handler dispatch
- `jit_hoo_exception_runtime` / `jit_hoo_exception_clear` — Runtime exception creation/clearing

### Exception dispatch targets (`validPcs`)

When `translateModule` lowers `SYSCALL 9` (`kSysThrowToHandler`) and
`SYSCALL 10` (`kSysRethrowToHandler`), the returned handler PC is routed
through a generated switch whose cases are the instruction PCs that may
legally receive exception dispatch inside the current function. That target
set is computed per function as the decoded instruction boundaries from the
function entry PC up to (but not including) the next function's entry PC.

The scan is deliberately **not** terminated at the first `RET`: catch-handler
blocks emitted after an early `return` sit between that `RET` and the next
function, and must remain valid dispatch targets. Bounding the scan by the
next function's entry (a sorted list of `STT_FUNC` symbol values) keeps the
set inside the current function's text range instead. A handler PC outside the
set (no matching case) falls to the "unhandled" block, which returns `-1` and
lets `run()` fall back to the interpreter.

## Inbound trampoline dispatch

Allows external code to call into Hoo functions via a fixed set of trampoline slots (max 8):

```cpp
uint64_t hooc_hvm_inbound_trampoline_dispatch(size_t slot, uint64_t arg0);
uint64_t hooc_hvm_inbound_trampoline_dispatch2(size_t slot, uint64_t arg0, uint64_t arg1);
```

Pre-registered trampolines `hooc_hvm_inbound_trampoline_0` through `_7` (1-arg) and `hooc_hvm_inbound_trampoline2_0` through `_7` (2-arg) dispatch to `invokeInboundCallback(slot, args)`.

## Runtime symbol wrappers

The `jit_hoo_*` wrappers span ~25+ runtime library domains. Each reads arguments from `HVMState::regs[]` and returns results via `uint64_t` (or `memcpy` for doubles).

### Domain breakdown

| Domain | # Wrappers | Runtime library |
|---|---|---|
| Memory/ARC | 5 (`alloc`, `retain`, `release`, `refcount`, `type_id`) | `hoo_runtime.h` |
| String | 25+ (`from_cstr`, `from_int64`, `from_double`, `concat`, `length`, `to_upper`, `data`, `to_characters`, `join`, `from_object`, `from_any`, `from_bytes`, `from_bool`, `is_empty`, `to_lower`, `equals`, `contains`, `starts_with`, `trim`, `repeat`, `index_of`, and `jit_string_*` aliases) | `hoo_string.h` |
| Character | 7 (`from_utf8`, `from_utf8_string`, `from_codepoint`, `length`, `data`, `codepoint`, `print`, `release`) | `hoo_character.h` |
| Array | 12+ (`new`, `push_int64`, `get_int64`, `push_double`, `get_double`, `push_string`, `get_string`, `push_bool`, `get_bool`, `push_array`, `push_object`, `length`, `clear`, `empty`, `set_int64`, `push_vector_int64`) | `hoo_generic_array.h` |
| Map | 20+ (`new`, `set_int64_int64`, `get_int64_int64`, `set_int64_double`, `get_int64_double`, `set_int64_string`, `get_int64_string`, `set_int64_bool`, `get_int64_bool`, `set_string_double`, `get_string_double`, `set_string_string`, `get_string_string`, `set_string_bool`, `get_string_bool`, `set_int8_int64`, `get_int8_int64`, `length`, `contains`, `remove`, `clear`, `empty`, `key_type`, `value_type`) | `hoo_map.h` |
| AnyArray | 8 (`new`, `new_capacity`, `length`, `push`, `set`, `get_data`, `pop_data`, `clear`, `release`) | `hoo_anyarray.h` |
| HashMap | 7 (`new`, `count`, `set_fixed`, `get_fixed`, `set_any`, `get_any`, `remove`, `clear`, `release`) | `hoo_hashmap.h` |
| Math | 60+ — per-type overloads for `abs`, `min`, `max`, `sign` (int64/int8/byte/double/f8), `gcd`, `factorial`, `fibonacci`, `is_even`, `is_odd`, `is_prime`, `lcm`, `sqrt`, `get_pi`, `get_e`, `get_tau`, `get_inf`, `get_neg_inf`, `get_nan`, `pow`, `clamp`, `floor`, `cbrt`, `hypot`, `ceil`, `sin`, `cos`, `tan`, `atan2`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `exp`, `exp2`, `expm1`, `log`, `log10`, `log2`, `log1p`, `round`, `trunc`, `fract` | `hoo_math.h` |
| Tensor | 30+ (`new1`, `new2`, `new3`, `new`, `push_value`, `length`, `rank`, `element_type`, `dim`, `get_int64`, `get_double`, `add`, `sub`, `element_mul`, `element_div`, scalar add/subtract/scale/divide, `matmul`, `reshape`, `transpose`, `softmax`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `and`, `or`, `not`) | `hoo_tensor.h` |
| Buffer | 14 (`new`, `from_bytes`, `copy`, `length`, `capacity`, `byte_at`, `set_byte`, `append`, `append_buffer`, `clear`, `slice`, `data`, and `hoo_buffer.h` wrappers) | `hoo_buffer.h` |
| I/O | 4 (`print`, `println`, `readline`, `readchar`) | `hoo_io.h` |
| Filesystem | Fs functions (`hoo_fs.h`) —
| System | System functions (`hoo_system.h`) —
| DateTime | Date/time functions (`hoo_datetime.h`) —
| Regex | Regex functions (`hoo_regex.h`) —
| UUID | UUID functions (`hoo_uuid.h`) —
| Encoding | Base64/hex/URL encoding (`hoo_encoding.h`) —
| JSON | JSON serialize/deserialize (`hoo_json.h`) —
| Thread | Thread functions (`hoo_thread.h`) —
| CSV | CSV functions (`hoo_csv.h`) —
| Hashing | SHA256/SHA1/MD5/CRC32/HMAC (`hoo_hashing.h`) —
| Process | Process functions (`hoo_process.h`) —
| Compression | Compression functions (`hoo_compression.h`) —
| Args | Args functions (`hoo_args.h`) —
| Overload | Overload dispatch (`hoo_overload.h`) —
| Net | Networking functions (`hoo_net.h`) —
| Object | Field access (`jit_object_get_field`, `jit_object_set_field`) — `hoo_runtime.h` |

## C, C++, and Platform Boundaries

The JIT-facing runtime wrappers are declared `extern "C"` so ORC looks up stable unmangled names. The C++ compiler still compiles the wrapper bodies, but the exported symbol names stay predictable.

That stability matters for both Windows and Linux:

- On Windows, ORC resolves against PE/COFF exports and import libraries.
- On Linux, ORC resolves against ELF dynamic symbols and shared objects.

Hoo keeps the bridge layer in C ABI form even when the underlying implementation is C++, because ORC only needs a symbol name and an address. The wrapper itself can call C++ runtime classes or helper methods internally.

## System call interface

The runtime uses a syscall-like mechanism for low-level operations, accessed via `SYSCALL` opcode with an immediate `imm15` value:

| Constant | Value | Operation |
|---|---|---|
| `kSysAlloc` | 1 | Memory allocation |
| `kSysRetain` | 2 | Retain object |
| `kSysRelease` | 3 | Release object |
| `kSysRefcount` | 4 | Get reference count |
| `kSysTypeId` | 5 | Get type ID |
| `kSysExceptionRuntime` | 6 | Create runtime exception |
| `kSysPushHandler` | 7 | Push exception handler |
| `kSysPopHandler` | 8 | Pop exception handler |
| `kSysThrowToHandler` | 9 | Throw to handler |
| `kSysRethrowToHandler` | 10 | Rethrow to handler |
| `kSysStringData` | 11 | Get string data pointer |
| `kSysThreadCreate` | 12 | Create thread |
| `kSysThreadExit` | 13 | Exit thread |
| `kSysFutex` | 14 | Futex operation |
| `kSysGetTid` | 15 | Get thread ID |
| `kSysOpen` | 16 | Open file |
| `kSysRead` | 17 | Read file |
| `kSysWrite` | 18 | Write file |
| `kSysClose` | 19 | Close file |
| `kSysLseek` | 20 | Seek file |
| `kSysFstat` | 21 | File stat |
| `kSysClockGetTime` | 22 | Get clock time |
| `kSysGetRandom` | 23 | Get random bytes |

## Memory management

In interpreted mode, a `std::vector<uint8_t> memory_` serves as the virtual address space. `memoryTop_` tracks the next free byte. Module sections (`.text`, `.data`, `.rodata`) are mapped into this memory via `mapModuleSections()`.

Module memory layout (per-module `ModuleMemoryLayout`):
```cpp
struct ModuleMemoryLayout {
    uint64_t textBase = 0, rodataBase = 0, dataBase = 0, bssBase = 0;
    uint64_t textSize = 0, rodataSize = 0, dataSize = 0, bssSize = 0;
};
```

Allocation (`jit_hoo_alloc`) uses the C heap (`hoo_alloc(size, typeId)`) rather than the bump allocator, so allocated objects are real heap pointers.

## Inspector / debug support

```cpp
bool buildInspectorTrace(const std::string& entryPoint);
bool inspectorStep();
std::optional<InspectorSnapshot> getInspectorSnapshot() const;
void resetInspector();
std::array<int64_t, 32> getRegisters() const;
std::vector<uint8_t> readVirtualMemory(uint64_t addr, size_t size) const;
```

`InspectorSnapshot` captures:
```cpp
struct InspectorSnapshot {
    std::array<int64_t, 32> regs{};
    uint64_t pc = 0;
    std::string moduleName, functionName, opcode;
    bool halted = false;
};
```

A global `stopExecutionRequested_` atomic flag allows cooperative cancellation.

## Error handling

```cpp
enum class ErrorPhase { None, Parse, Validate, Resolve, Initialize, Execute };
enum class ErrorCode { None, IoReadFailed, ParseFailed, InvalidHeader, ... };

struct ErrorInfo {
    ErrorPhase phase;
    ErrorCode code;
    std::string moduleName, symbolName, path, message;
};
```

Errors are propagated via `lastError_` (string) and `lastErrorInfo_` (optional struct).

## Module registry

Each loaded module has a `ModuleRegistryEntry`:

```cpp
struct ModuleRegistryEntry {
    std::unordered_map<std::string, hvm::Symbol> symbolsByName;
    std::unordered_map<std::string, hvm::ExportEntry> exportsByName;
    std::vector<hvm::ImportEntry> imports;
    std::unordered_map<std::string, hvm::FunctionMetadata> functionMetaByName;
};
```

This is used for:
- `hasExportedOrDefinedSymbol()` — Symbol resolution across modules
- `ensureJITFunctionTable()` — JIT symbol registration
- `resolveImportModulePath()` — Import path resolution

## Post-load initializers

After all modules are materialized, `runPostLoadInitializers()` runs:
1. `runModuleInitializer(module)` — Module-level init functions (one-shot via `std::once_flag`)
2. `runModuleVTableInitializers(module)` — VTable setup (one-shot via `std::once_flag`)

These ensure module-level state is initialized exactly once, even if modules are loaded multiple times.

## Tests

Tests are split across:

| File | Purpose |
|---|---|
| `tests/jit/HVMJITLifecycleTest.cpp` | Basic lifecycle test (load then destroy) |
| `tests/jit/Hoo*JitTest.cpp` (28 files) | Domain-specific JIT tests (math, string, array, map, tensor, datetime, etc.) |
| `tests/hvm/HVMJITLoaderTest.cpp` | Module loader tests |
| `tests/hvm/HVMJITInstructionSemanticsTest.cpp` | Instruction-level semantics tests |

```cpp
TEST(HVMJITLifecycleTest, LoadSourceCodeThenDestroy) {
    auto io = std::make_unique<DefaultIOProvider>();
    auto jit = std::make_unique<HVMJIT>(*io);
    ASSERT_TRUE(jit->loadSourceCode("test", "func :int64 test() { return 42; }"))
        << jit->getLastError();
}
```
