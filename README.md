# Hoo

[![Version](https://img.shields.io/badge/version-1.4.0-blue)](https://github.com/cosq-network/hoo/releases/tag/v1.4.0)
[![macOS Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-macos)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Linux Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-linux)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![Windows Build](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml/badge.svg?job=build-windows)](https://github.com/cosq-network/hoo/actions/workflows/build-and-test.yml)
[![License](https://img.shields.io/github/license/cosq-network/hoo)](LICENSE)

Last Updated: 2026-08-12

Hoo is a high-performance, statically-typed systems programming language and compiler ecosystem. It features an aggressive lowering pipeline that translates high-level object-oriented code into a pure, physical-silicon-ready 64-bit RISC architecture.

## 1. Architectural Vision: Hardware Purity

The Hoo ecosystem is built around the **HVM v1.6** specification. Unlike traditional virtual machines (JVM, Python) that rely on high-level semantic bytecode, HVM is a normative model for a physical processor.

- **ISA Purity**: No "magic" opcodes for objects or exceptions. The instruction set is limited to fundamental arithmetic, memory, and control-flow primitives.
- **Aggressive Lowering**: The compiler (`hoo`) performs all complex memory offset calculations, array scaling, and exception shadow-stack management at compile-time.
- **Unified Execution**: The JIT compiler (LLVM ORC-based) functions as a high-fidelity dynamic binary translator, mirroring physical silicon behavior with zero abstraction overhead.

## 2. Build & Test

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
  tests/         Exhaustive unit and integration test suites (2637 tests in the current preset run).
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
