# Hooc

Last Updated: 2026-05-24

Hooc is a high-performance, statically-typed systems programming language and compiler ecosystem. It features an aggressive lowering pipeline that translates high-level object-oriented code into a pure, physical-silicon-ready 64-bit RISC architecture.

## 1. Architectural Vision: Hardware Purity

The Hooc ecosystem is built around the **HVM v1.4 (Hardware Ready)** specification. Unlike traditional virtual machines (JVM, Python) that rely on high-level semantic bytecode, HVM is a normative model for a physical processor.

- **ISA Purity**: No "magic" opcodes for objects or exceptions. The instruction set is limited to fundamental arithmetic, memory, and control-flow primitives.
- **Aggressive Lowering**: The compiler (`hoo`) performs all complex memory offset calculations, array scaling, and exception shadow-stack management at compile-time.
- **Unified Execution**: The JIT compiler (LLVM ORC-based) functions as a high-fidelity dynamic binary translator, mirroring physical silicon behavior with zero abstraction overhead.

## 2. Project Status & Focus

- [x] **Core ISA v1.4**: Finalized 64-bit RISC instruction set with bit-packed encoding.
- [x] **Aggressive Backend**: Functionally complete HVM code generator with manual object/array lowering.
- [x] **Runtime Library (`hoort`)**: ARC-managed core types (String, Array, Map) with native C++ implementation and 16-byte headers.
- [x] **JIT Translator (`HVMJIT`)**: High-performance LLVM ORC v2 dynamic binary translator with per-module `JITDylib` isolation.
- [x] **Phase 4 (Bootstrap/Init)**: module post-load initialization, dependency-order init, vtable init ordering, and once-only guards implemented and covered by tests.
- [x] **Phase 5 (Literals & Interpolation)**: unicode-aware character support, 64-bit floating point lowering, and full string interpolation with automatic conversion implemented.
- [x] **Phase 6 (Hardening)**: refactored low-level array representation, implemented managed realloc/capacity tracking, and stabilized full test suite.
- [x] **Verification**: full preset test run passing (`890+ tests`).
- [ ] **Physical Hardware**: (Next Phase) FPGA Soft-Core implementation based on the HVM spec.


## 3. Build & Test

For detailed instructions on dependencies, platform-specific guides, and troubleshooting, see the [Building Guide](docs/BUILDING.md) and the [Developing and Debugging Guide](docs/debugging-hoo.md).

### Prerequisites
- CMake >= 3.20 for presets (`3.16+` only for manual configuration)
- C++17 compliant toolchain (Clang 15+ recommended)
- LLVM 15+ development headers
- ANTLR4 runtime

### Standard Workflow
```bash
cmake --preset ninja-relwithdebinfo
cmake --build --preset ninja-relwithdebinfo
cmake --build --preset ninja-relwithdebinfo-tests
ctest --preset ninja-relwithdebinfo
```

### Windows Workflow

On Windows, the checked-in Visual Studio 18 preset with repo-local dependencies is:

```powershell
cmake --preset windows-vs18-local
cmake --build --preset windows-vs18-local
cmake --build --preset windows-vs18-local-tests
ctest --preset windows-vs18-local
```

This preset expects dependencies in `vcpkg_installed/x64-windows/` and uses the repo-local LLVM CMake package at `vcpkg_installed/x64-windows/share/llvm` when present.

## 4. Project Layout

```text
src/
  parsing/    Hooc.g4 grammar + ANTLR4 generated artifacts.
  ast/        Typed Abstract Syntax Tree with lowering metadata.
  codegen/    HVM and LLVM IR generation backends.
  hvm/        ISA definitions, module serialization, and physical state model.
  runtime/    The 'hoort' library (ARC, Strings, Exceptions, IO).
  core/       Symbol Mangler, CLI logic, and IO providers.
tests/        Exhaustive unit and integration test suites (899+ tests in the current preset run).
docs/         Normative specifications and implementation guides.
```

## 5. HVM v1.4 Normative Reference

Current profile: **core-minimalest** (Physical Silicon Ready)

| Document | Description |
| :--- | :--- |
| `docs/hvm/hvm-spec.md` | Normative ISA specification and execution model. |
| `docs/hvm/hvm_instruction_set.csv` | Machine-readable opcode/format table. |
| `docs/hvm/ho-file-format.md` | Binary container format for `.ho` modules. |
| `docs/hvm/jit-implementation-guide.md` | Blueprint for LLVM-based high-performance execution. |
| `docs/hvm/hvm-implementation-analysis.md`| Detailed assembly-level mapping of language features. |

## 6. Contributing

The Hooc project follows a "Specs First" methodology. When adding features:
1. Update the **Grammar** (`Hooc.g4`).
2. Define the **Lowering Rule** in `HVM_IMPLEMENTATION_ANALYSIS.md`.
3. Update the **HVM Backend** and **Runtime Intrinsics**.
4. Synchronize all **Normative Documentation**.
5. Achieve 100% test pass-rate.
