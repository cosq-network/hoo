# Hoo

[![Version](https://img.shields.io/badge/version-1.4.0-blue)](https://github.com/cosq-network/hoo/releases/tag/v1.4.0)
[![macOS Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-macos)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Linux Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-linux)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Windows Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-windows)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![License](https://img.shields.io/github/license/cosq-network/hoo)](LICENSE)

Last Updated: 2026-08-13

Hoo is a high-performance, statically-typed systems programming language and compiler ecosystem. It features an aggressive lowering pipeline that translates high-level object-oriented code into a pure, physical-silicon-ready 64-bit RISC architecture.

## 1. Architectural Vision: Hardware Purity

The Hoo ecosystem is built around the **HVM v1.6** specification. Unlike traditional virtual machines (JVM, Python) that rely on high-level semantic bytecode, HVM is a normative model for a physical processor.

- **ISA Purity**: No "magic" opcodes for objects or exceptions. The instruction set is limited to fundamental arithmetic, memory, and control-flow primitives.
- **Aggressive Lowering**: The compiler (`hoo`) performs all complex memory offset calculations, array scaling, and exception shadow-stack management at compile-time.
- **Unified Execution**: The JIT compiler (LLVM ORC-based) functions as a high-fidelity dynamic binary translator, mirroring physical silicon behavior with zero abstraction overhead.

## 2. Design Philosophy

The Hoo design philosophy is captured in five principles that govern every feature added to the language, compiler, and runtime:

- **Hardware Purity**: Every language feature must lower to HVM instructions that could run unmodified on real silicon. High-level constructs are never given "magic" opcodes; they are expressed in terms of the primitive ISA.
- **Aggressive Compile-Time Lowering**: All complex semantics — object layout, array indexing and scaling, virtual dispatch, exception handling — are resolved by the compiler before a single instruction is emitted. At runtime there is no metadata interpretation left to do.
- **ISA Minimalism**: HVM is a fixed-width 64-bit load/store RISC core (32 GPRs) whose instruction set covers only fundamental arithmetic, memory, and control flow. Objects, strings, allocation, and exceptions are software contracts layered above the ISA.
- **ARC-First Memory Management**: All managed objects are native 64-bit pointers carrying a 16-byte header (refcount + type ID). Lifetime is managed by deterministic, lock-free atomic reference counting — no tracing garbage collector, no stop-the-world pauses.
- **Specs-First Development**: Changes begin with the grammar (`Hoo.g4`), are formalized as lowering rules, and are implemented across codegen, backend, and runtime only after the normative documentation is synchronized. Every change must keep the full test suite at 100% pass-rate.

Because the `.ho` artifact is a normative model of a physical processor rather than high-level semantic bytecode, a single compiled module runs unmodified on a soft-core (FPGA/ASIC), a cycle-accurate simulator, an interpreter, or the LLVM-based JIT.

## 3. JIT Implementation

Hoo executes through `HVMJIT` (`src/hvm/HVMJIT.cpp`, ~10,000 lines), an LLVM ORC v2-based dynamic binary translator. It does not interpret bytecode at hot paths; instead it translates each HVM module to LLVM IR and lets ORC materialize native x86-64 / ARM64 code.

```text
.ho module → validate → resolve dependencies → register runtime symbols
  → translateModule (HVM bytecode → LLVM IR)
  → ORC materialization → native execution
```

- **LLVM ORC v2**: ORC compiles IR to native code, resolves symbols across modules and runtime libraries, and links them lazily via per-module `JITDylib`s.
- **Dual execution tiers**: Simple arithmetic, memory, and control-flow ops lower directly to LLVM IR; library-dependent operations (strings, arrays, maps, tensors, ARC, I/O) dispatch to ~100+ `jit_hoo_*` `extern "C"` runtime wrappers. If ORC cannot materialize a path, the engine falls back to the interpreter.
- **Shadow-stack exceptions**: `try/catch` lowers to `SYSCALL` 7-10 (`push_handler`, `pop_handler`, `throw`, `rethrow`) which maintain an explicit shadow stack of handler frames — the same technique used for C++ exceptions on physical hardware.
- **ARC use-def optimization**: A dataflow pass (`buildARCUseDefGraph`) eliminates redundant retain/release pairs before IR translation (opt-in via `HOOC_ENABLE_ARC_USEDEF=1`).
- **Module lifecycle**: Modules transition through a loader state machine (`Discovered → Parsed → Validated → Registered → DependenciesResolved → Ready`), are topologically ordered with cycle detection, and run one-shot post-load initializers.
- **Debugging support**: DWARF debug info (`HOOC_ENABLE_DWARF=1`), an instruction inspector, and inbound trampolines for calling Hoo functions from host code.

See `docs/dev/hvm-jit.md` and `docs/runtime/jit-integration.md` for the full implementation guide.

## 4. Comparison with Java and .NET

Java (JVM) and .NET (CLR) are high-level, stack-based virtual machines that provide managed safety through metadata, verification, and garbage collection. Hoo inverts that model: it is a register-based RISC ISA whose semantics are a normative contract for physical silicon.

| Feature | Hoo (HVM) | Java (JVM) | .NET (CLR) |
| :--- | :--- | :--- | :--- |
| Execution model | Register-based (32 GPRs), load/store RISC | Stack-based bytecode | Stack-based CIL |
| Abstraction level | Low-level, hardware-ready ISA | High-level semantic bytecode | High-level semantic bytecode |
| Encoding | Fixed 32-bit (8-byte extended ops) | Variable-length byte stream | Variable-length byte stream |
| Object creation | Lowered by compiler to `CALL` into runtime | Native `new` opcode | Native `newobj` opcode |
| Exceptions | Lowered to shadow-stack control flow | VM-managed exception tables (`athrow`) | CLR-managed exception blocks |
| Memory management | Deterministic ARC (atomic refcounts, 16-byte header) | Tracing GC (generational) | Tracing GC (generational) |
| JIT | LLVM ORC v2 dynamic binary translation | HotSpot (C1/C2/Graal) | RyuJIT |
| Verification | None needed — static, monomorphized type system | Mandatory bytecode verification | Mandatory metadata verification |
| Hardware readiness | Yes (silicon/FPGA target) | No (requires JRE) | No (requires .NET runtime) |
| Deployment artifact | `.ho` / `.ha` universal target | `.class` / `.jar` | IL / assemblies |

Key trade-offs:

- **Startup**: `.ha` archives are decompressed and linked up front in topological order, giving predictable, stutter-free execution; `.jar`-style lazy loading boots huge applications faster but pays "JIT warmup" latency later.
- **Performance**: Because all object and exception semantics are resolved at compile time, the JIT is a high-fidelity binary translator with zero abstraction overhead — closer to an AOT-style native executable than to a semantics-interpreting VM.
- **Safety**: Hoo trades runtime verification for a static, monomorphized type system and ARC ownership — safety is established at compile time rather than enforced by a runtime verifier.

For the archive-level `.ha` vs `.jar` comparison, see `docs/hvm/ha-archive-format.md`.

## 5. Build & Test

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

## 6. Project Layout

```text
src/
  parsing/    Hoo.g4 grammar + ANTLR4 generated artifacts.
  ast/        Typed Abstract Syntax Tree with lowering metadata.
  codegen/    HVM and LLVM IR generation backends.
  hvm/        ISA definitions, module serialization, and physical state model.
  runtime/    The 'hoort' library (ARC, Strings, Buffer, Arrays, Maps, Tensors, Exceptions, IO).
  core/       Symbol Mangler, CLI logic, and IO providers.
  repl/       REPL session implementation and interactive driver loop.
  tests/         Exhaustive unit and integration test suites (2637 tests in the current preset run).
docs/         Normative specifications and implementation guides.
```

## 7. HVM v1.6 Normative Reference

Current profile: **Silicon MVP** (Physical Silicon Ready)

| Document | Description |
| :--- | :--- |
| `docs/hvm/hvm-spec.md` | Normative ISA specification and execution model. |
| `docs/hvm/hvm_instruction_set.csv` | Machine-readable opcode/format table. |
| `docs/hvm/ho-file-format.md` | Binary container format for `.ho` modules. |
| `docs/runtime/jit-integration.md` | SYSCALL interface mapping and ARC optimization passes. |

## 8. Contributing

The Hoo project follows a "Specs First" methodology. When adding features:
1. Update the **Grammar** (`Hoo.g4`).
2. Define the **Lowering Rule** in `HVM_IMPLEMENTATION_ANALYSIS.md`.
3. Update the **HVM Backend** and **Runtime Intrinsics**.
4. Synchronize all **Normative Documentation**.
5. Achieve 100% test pass-rate.
