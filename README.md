# Hoo

[![Version](https://img.shields.io/badge/version-1.4.0-blue)](https://github.com/cosq-network/hoo/releases/tag/v1.4.0)
[![macOS Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-macos)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Linux Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-linux)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Windows Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-windows)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![License](https://img.shields.io/github/license/cosq-network/hoo)](LICENSE)

Last Updated: 2026-08-12

Hoo is a high-performance, statically-typed systems programming language and compiler ecosystem. It features an aggressive lowering pipeline that translates high-level object-oriented code into a pure, physical-silicon-ready 64-bit RISC architecture.

## 1. Architectural Vision: Hardware Purity

The Hoo ecosystem is built around the **HVM v1.5** specification. Unlike traditional virtual machines (JVM, Python) that rely on high-level semantic bytecode, HVM is a normative model for a physical processor.

- **ISA Purity**: No "magic" opcodes for objects or exceptions. The instruction set is limited to fundamental arithmetic, memory, and control-flow primitives.
- **Aggressive Lowering**: The compiler (`hoo`) performs all complex memory offset calculations, array scaling, and exception shadow-stack management at compile-time.
- **Unified Execution**: The JIT compiler (LLVM ORC-based) functions as a high-fidelity dynamic binary translator, mirroring physical silicon behavior with zero abstraction overhead.

## 2. Project Status & Focus

- [x] **Core ISA v1.4**: Finalized 64-bit RISC instruction set with bit-packed encoding.
- [x] **Aggressive Backend**: Functionally complete HVM code generator with manual object/array lowering.
- [x] **Runtime Library (`hoort`)**: ARC-managed core types (String, Buffer, Array, Map) with native C++ implementation and 16-byte headers; `hoo::fs` class-based API (File, Directory, Path) over `<filesystem>`.
- [x] **JIT Translator (`HVMJIT`)**: High-performance LLVM ORC v2 dynamic binary translator with per-module `JITDylib` isolation.
- [x] **Phase 4 (Bootstrap/Init)**: module post-load initialization, dependency-order init, vtable init ordering, and once-only guards implemented and covered by tests.
- [x] **Phase 5 (Literals & Interpolation)**: unicode-aware character support, 64-bit floating point lowering, and full string interpolation with automatic conversion implemented.
- [x] **Phase 6 (Hardening)**: refactored low-level array representation, implemented managed realloc/capacity tracking, and stabilized full test suite.
- [x] **Phase 7 (System Services)**: expanded SYSCALL table from 11 to 23 entries, adding OS-level file I/O, threading, clock, and random services; reserved `r4` as thread pointer (`tp`).
- [x] **Phase 8 (Class Method Dispatch)**: class-based method-call syntax (`Math.abs(x)`, `map.length()`) with full JIT support for all runtime modules; unified `new Class(...)` constructor syntax for built-in and user-defined classes (`new Map(1)`); consistent `"ptr"` mangling for all method parameters and return types.
- [x] **Access Qualifiers (ISSUE-006)**: `public` and `private` field/method access checks, modifier-aware method mangling (`_Pb`/`_Pv`), local HVM bindings for private methods, module export filtering, cross-module import rejection, and LLVM internal linkage during JIT translation.
- [x] **Phase 8.1 (Character Dispatch Fix)**: Added `Character` → `character` prefix mapping to `classToPrefix()`, registering JIT symbols for `_F_M_hoo_E_character_*` mangled names, enabling constructor syntax `new Character(...)` and factory `Character.fromUtf8()`, and instance methods `codepoint()`, `length()`, `data()`, `print()`, `release()` at the Hoo language level (no static methods).
- [x] **Phase 8.2 (Type Inference)**: Extended `getTypeId()` to infer return types for function calls, user-defined class methods, and array subscript access. Array literal element types inferred from uniform elements. For-in loop variables infer type from iterable's element type. Char-keyed Map operations removed from Hoo language layer (runtime-only C API).
- [x] **Phase 9 (Output Optimization)**: Silenced LLVM IR and JIT debug outputs during execution to significantly improve unit test performance.
- [x] **Phase 9.1 (macOS Stabilization)**: Resolved cross-platform build errors in `hoo_thread` and `hoo_uuid` modules; project now verified stable on macOS (Apple Silicon/Intel).
- [x] **Phase 10 (Tensor Type Support)**: Added tensor type (`tensor<T>[N]`) to grammar, AST, codegen, and runtime with full rank-1/rank-2/rank-3 support. Includes packed `bit`, one-byte `int8`/`byte`/canonical E4M3 `f8`, wide `int64`/`f64`, element-wise arithmetic, promotion, comparison and logical operators, matrix multiplication, reshape, transpose, softmax, and tensor-scalar broadcasting in both operand orders. Runtime, parsing, codegen, and JIT coverage is included.
- [x] **Phase 11 (Buffer Type Integration)**: Added managed mutable byte array (`Buffer`) with a single Hoo constructor (`new Buffer()`), free function byte-source creation (`buffer_fromBytes(...)`), ARC management, JIT wrappers, and symbol registration. Buffer handle points to `BufferImpl` (right after ARC header), matching the HooString memory layout. Includes copy, append, clear, slice, byte-level access, and Buffer-aware overloads in Fs, Encoding, Uuid, Hashing, and Compression modules. Runtime and JIT test coverage updated.
- [x] **Phase 11.1 (Map OOP API Redesign)**: Consolidated 70+ type-specific C-ABI functions into 15 polymorphic functions (`hoo_map_new`, `hoo_map_set`, `hoo_map_try_get`, `hoo_map_contains_key`, `hoo_map_remove`, `hoo_map_clear`, `hoo_map_count`, `hoo_map_is_empty`, etc.) with internal key/value type dispatch via template helpers. Key/value type-erased via `void*` at the C-ABI layer. JIT wrappers rewritten to delegate to the polymorphic C-ABI. 23 runtime tests + 21 JIT tests pass. Cross-platform fix: char map storage uses `int8_t` instead of `char` to avoid signedness differences on ARM64 Linux.
- [x] **Phase 11.2 (DateTime Redesign as Instantiable Class)**: Converted DateTime from a raw-int64 singleton API into an ARC-managed instantiable class (type ID 119) with `timestamp: int64` field, enabling future `serializable` modifier support. Added instance methods (`dt.format()`, `dt.addDays()`, `dt.compare()`, etc.), built-in class-qualified factory dispatch (`DateTime.now()`, `DateTime.parse()`), module-level free functions (`datetime_now()`, `datetime_new(ts)`), and full C-level free function wrappers (`hoo_datetime_now`, `hoo_datetime_format`, etc.). 15 JIT tests + 24 runtime tests pass.
- [x] **Phase 11.3 (Serializable Class Modifier)**: Added `serializable` class modifier to grammar, AST, symbol mangling, and code generation. Implements constructor/type/cycle validation, modifier-aware static and instance dispatch, inherited and nested field lowering, tagged buffer/tensor JSON round-trips, and regression coverage. Full preset verification passes (`2117 tests`, 2 disabled).
- [x] **Phase 12 (REPL Integration)**: Added interactive REPL shell mode via `--repl` flag, with nested multiline brace matching, built-in commands (`/exit`, `/quit`, `/help`, `/reset`), static library target `hoorepl`, and complete unit testing coverage.
- [x] **Phase 13 (Function Overloading)**: Implemented function and method overloading capabilities for both built-in core APIs and user-defined functions. Integrates name mangling strategies based on argument types, an `OverloadList` AST grouping node to gracefully handle multiple method declarations, and dynamic runtime dispatch leveraging `CALL_OVERLOADED` instructions directly into the LLVM ORC JIT. Includes robust ambiguity detection with `AmbiguousOverloadException`.
- [x] **Phase 14 (Archive Loading)**: Cross-file local imports and `.ha` archive format with Zstd compression, manifest, and multi-module JIT loading.
- [x] **ISSUE-036 (Module Dependency Resolution)**: Corrected per-module dependency graph traversal, transitive topological ordering, explicit cycle detection, bundle/JIT cycle agreement, and loader regression coverage. Removed the obsolete circular-dependency helper.
- [x] **Phase 15 (Async/Await via libuv)**: Native `async`/`await` syntax, `Future<T>` values (type ID 123), managed and primitive result handling, multiple continuations, and a mutex-protected libuv event loop with cooperative waiting. HVM async functions currently execute through the Future ABI and pump the event loop while awaiting; true VM stack-frame suspension requires future HVM suspend/resume instructions. Coverage includes `NewLanguageFeaturesTest.cpp` and `HooFutureJitTest.cpp`. Language docs: `docs/language/async_await.md`.
- [x] **HVM 1.5 Spec Compatibility (ISSUE-040)**: Full CSV parity, including the native `CMP_B` comparison family, all required CPU-profile instructions (ICACHE.RNG, LD.P/ST.P, LR.D/SC.D, ECALL/TRAPRET/CSRRW, SFENCE.VMA), advisory no-ops, RELEASE zero-flag semantics, ALLOC.BUMP TLAB fast path, module feature flags with loader validation, and complete HVM-V vector ISA expansion.
- [x] **Nullable Types (ISSUE-047 correctness complete)**: End-to-end `T?` tracking, catchable null checks for member/method/array dereferences, compile-time null-safety validation, distinct nullable overload mangling, and ARC cleanup for nullable named references stored in generic type-ID-100 slots. No required correctness work remains; optional `LD.D.NZ` folding is deferred because its current VM-trap path is not catchable.
- [x] **Tensor Precision and Broadcasting (ISSUE-025/030)**: Completed low-precision tensor storage, canonical FP8 fallback, native-width integer semantics, scalar promotion, safe tensor-scalar runtime/JIT lowering, and reshape/transpose/softmax utilities.
- [x] **Verification**: full preset test run passing (`2557 tests`, 0 failures).
- [ ] **Physical Hardware**: (Next Phase) FPGA Soft-Core implementation based on the HVM spec.

## Recent Changes

- **Args module hardening**: added validated typed parsing, required-argument
  support, negative numeric values, safe repeated parsing, retained argument
  snapshots, explicit and automatic handle cleanup, and executable CLI coverage.
- **Array API consistency**: Array operations are instance-only (`arr.pushInt64`,
  `arr.getString`, `arr.length`); static forms such as `Array.getString(arr, 0)`
  are rejected by the compiler.
- **Statement integration coverage**: added executable-target integration tests
  for `for-in` loops, range loops, stepped ranges, empty ranges, loop control,
  strings, arrays, maps, and real `print` execution.
- **While and do-while loop integration tests**: added CLI integration tests for
  `while` and `do..while` loops covering basic counting, array traversal,
  break/continue, nested loops, float/bool conditions, and print execution.
- **If-statement integration tests**: added CLI integration tests for `if`,
  `if..else`, `if..else if`, and `if..else if..else` covering comparison
  operators, logical operators, nested ifs, variable assignments, multiple
  statements per branch, return statements, float/char conditions, and deep
  else-if chains.
- **Exception-handling integration tests**: added CLI integration tests for
  `try/catch`, `try/finally`, `try/catch/finally`, `throw`, multiple catch
  clauses, nested try/catch, and uncaught exception behavior.
- **Switch-statement integration tests**: added CLI integration tests for
  `switch/case/default` covering basic matching, default fallback, fall-through
  semantics, break/continue behavior inside loops, nested switch, if/else and
  loops inside cases, variable declarations in cases, expression discriminants,
  int8/byte/bool/bit discriminant types, and string-discriminant rejection.

- **Networking and archive integration**: DNS-aware sockets now support
  configurable timeouts and TLS client/server handshakes. Archive imports
  preserve complete overload sets and user-defined return classes, while
  slice-aware encoding, hashing, compression, and construction are registered
  in the HVM JIT.

- **Runtime concurrency and networking**: added scoped native mutex locking,
  condition variables, semaphores, borrowed `HooByteSlice` views, and libuv
  TCP socket operations. Socket receives return owned `Buffer` handles.
- **Compiler metadata and HVM hardening**: archive imports now preserve external
  function signatures for chained inference and ranked overload conversion;
  LUI encoding uses a shared constant and JIT exception state has dedicated
  synchronization.

- **Runtime safety and dispatch hardening (ISSUE-055, ISSUE-015, ISSUE-031)**:
  destructor registration now supports arbitrary non-negative type IDs through a
  synchronized dynamic registry; method dispatch retains all class candidates,
  uses recursive receiver-aware inference, and rejects ambiguous unknown
  receivers.
- **Native byte comparisons (ISSUE-029)**: added HVM 1.5 `CMP_B` (`0x42`) with
  signed and unsigned 8-bit comparison semantics across codegen, interpreter,
  and LLVM ORC JIT.

- **Cross-File Local Imports & `.ha` Archives (Phase 14)**: Added native support for cross-file local imports, resolving and compiling local dependencies automatically. The CLI now outputs `.ha` (Hoo Archive) files, a ZIP-compatible, Zstd-compressed container with `manifest.json`, replacing the intermediate `.ho` files.
- **Archive Loading & CLI Updates**: Added `HooArchiveLoader` to seamlessly load multi-module `.ha` archives into the JIT. Introduced `--exec` to directly execute `.hoo` source files after building.
- **Concurrency & Performance**: Resolved thread-local allocation (TLAB) memory leaks (ISSUE-060) and implemented fine-grained mutex striping for the Managed Object hash set, eliminating a critical O(n) global bottleneck (ISSUE-064/046).
- **Access Qualifiers**: Completed compiler and HVM/JIT visibility enforcement for `public` and `private` fields and methods (ISSUE-006).
- **Nullable Type Safety**: Completed ISSUE-047 nullability tracking, catchable dereference checks, assignment validation, and nullable-object ARC cleanup. Explicit branches plus the shadow-handler path remain the production lowering. The only remaining work is optional `LD.D.NZ` optimization, deferred until its trap semantics support catchable exceptions.
- Updated `hoo --version` output to include tool name, brand name, version, and license information.
- Removed `--compile` / `-c` from the CLI; unknown flags now return a clear error.
- Improved REPL ergonomics so `--repl` no longer requires an input file, and preloads a `.hoo` file when provided.
- Updated CLI usage text and documentation to match the new flags, `--` program-argument delimiter, and behavior.


## 3. Build & Test

For detailed instructions on dependencies, platform-specific guides, and troubleshooting, see the [Building Guide](docs/BUILDING.md) and the [Developing and Debugging Guide](docs/debugging-hoo.md).

### Prerequisites
- CMake >= 3.20 for presets (`3.16+` only for manual configuration)
- C++17 compliant toolchain (Clang 15+ recommended)
- LLVM 22.1+ development headers
- ANTLR4 runtime
- OpenSSL development libraries (TLS socket support)

### Standard Workflow
```bash
cmake --preset ninja-relwithdebinfo
cmake --build --preset ninja-relwithdebinfo
cmake --build --preset ninja-relwithdebinfo-tests
ctest --preset ninja-relwithdebinfo
```

### Windows Workflow

On Windows, with LLVM 22.1.4 downloaded from GitHub releases and vcpkg in manifest mode:

```powershell
cmake --preset windows-vs18-env
cmake --build --preset windows-vs18-env
cmake --build --preset windows-vs18-env-tests
ctest --preset windows-vs18-env --output-on-failure
```

This preset reads `LLVM_DIR` and `VCPKG_ROOT` from environment variables. See [docs/building-windows.md](docs/building-windows.md) for full setup instructions.

## 4. Project Layout

```text
src/
  parsing/    Hoo.g4 grammar + ANTLR4 generated artifacts.
  ast/        Typed Abstract Syntax Tree with lowering metadata.
  codegen/    HVM and LLVM IR generation backends.
  hvm/        ISA definitions, module serialization, and physical state model.
  runtime/    The 'hoort' library (ARC, Strings, Buffer, Arrays, Maps, Tensors, Exceptions, IO).
  core/       Symbol Mangler, CLI logic, and IO providers.
  repl/       REPL session implementation and interactive driver loop.
  tests/         Exhaustive unit and integration test suites (2557 tests in the current preset run).
docs/         Normative specifications and implementation guides.
```

## 5. HVM v1.4 Normative Reference

Current profile: **core-minimalest** (Physical Silicon Ready)

| Document | Description |
| :--- | :--- |
| `docs/hvm/hvm-spec.md` | Normative ISA specification and execution model. |
| `docs/hvm/hvm_instruction_set.csv` | Machine-readable opcode/format table. |
| `docs/hvm/ho-file-format.md` | Binary container format for `.ho` modules. |
| `docs/runtime/jit-integration.md` | SYSCALL interface mapping and ARC optimization passes. |

## 6. Contributing

The Hoo project follows a "Specs First" methodology. When adding features:
1. Update the **Grammar** (`Hoo.g4`).
2. Define the **Lowering Rule** in `HVM_IMPLEMENTATION_ANALYSIS.md`.
3. Update the **HVM Backend** and **Runtime Intrinsics**.
4. Synchronize all **Normative Documentation**.
5. Achieve 100% test pass-rate.
