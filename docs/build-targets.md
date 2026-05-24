# Build Targets

This document reflects the current target layout in `CMakeLists.txt`.

## Configure

```bash
cmake -S . -B build
```

Key cache/options:

- `HOOC_BUILD_TESTS` (default: `OFF`)
- `HOOC_BUILD_SHARED_RUNTIME` (default: `OFF`)
- `ANTLR4_USE_STATIC_RUNTIME` (default: `OFF`)
- `ANTLR4_ROOT`
- `ANTLR4_JAR_PATH` (default: `tools/antlr-4.13.2-complete.jar`)
- `ANTLR4_GRAMMAR_FILE` (default: `src/parsing/Hooc.g4`)
- `ANTLR4_GENERATED_DIR` (default: `${CMAKE_BINARY_DIR}/generated/antlr4`)

## Quick Commands

```bash
cmake --build build
cmake --build build --target <target>
cmake --build build --target help
```

## Target Graph

```text
hoo
└── hoo-core
    ├── hoo-parser
    │   └── generate_parser
    │       └── download_antlr4 (only when jar is missing at configure time)
    ├── hoort
    └── LLVM (Core, OrcJIT, etc.)
```

## Primary Targets

### `hoo` (executable)

- Source: `src/core/main.cpp`
- Links: `hoo-core`
- Build:
  ```bash
  cmake --build build --target hoo
  ```

### `hoo-core` (library)

Core compiler + HVM + JIT library. Merged from the previous `hvm` and `hoo-compiler` targets.

- Sources under:
  - `src/ast/` — AST node definitions and builder
  - `src/codegen/` — HVM bytecode generator
  - `src/core/` — Compiler orchestrator, CLI, symbol mangler, I/O
  - `src/hvm/` — HVM module format, instructions, module bundle, JIT
  - `src/modules/` — Module system for qualified name resolution
  - `src/parsing/` — ProcessIsolatedParser (manual ANTLR4 wrapper)
- Links: `hoort`, `hoo-parser`, `${llvm_libs}`
- Build:
  ```bash
  cmake --build build --target hoo-core
  ```

### `hoo-parser` (library)

ANTLR-generated parser library.

- Sources: generated into `${ANTLR4_GENERATED_DIR}`
- Includes: `${ANTLR4_GENERATED_DIR}`, `${ANTLR4_INCLUDE_DIR}`
- Links: `${ANTLR4_LIBRARY}`
- Depends on: `generate_parser`
- Optional compile def: `ANTLR4CPP_STATIC` when `ANTLR4_USE_STATIC_RUNTIME=ON`
- Build:
  ```bash
  cmake --build build --target hoo-parser
  ```

### `hoort` (runtime library)

Runtime (`STATIC` by default, `SHARED` when `HOOC_BUILD_SHARED_RUNTIME=ON`).

- Sources under `src/runtime/lib/`
- No LLVM dependency
- Debug compile def: `HOO_DEBUG_MEMORY`
- Build:
  ```bash
  cmake --build build --target hoort
  ```

## Parser Utility Targets

### `generate_parser`

Generates ANTLR C++ sources from `src/parsing/Hooc.g4`.

```bash
cmake --build build --target generate_parser
```

### `download_antlr4` (conditional)

Created only if `ANTLR4_JAR_PATH` does not exist at configure time. `generate_parser` depends on it in that case.

```bash
cmake --build build --target download_antlr4
```

### `clean_generated`

Deletes and recreates `${ANTLR4_GENERATED_DIR}`.

```bash
cmake --build build --target clean_generated
```

## Test Targets (`HOOC_BUILD_TESTS=ON`)

When enabled, these are added:

- `hoo-tests` executable
- `HooUnitTests` via `add_test(NAME HooUnitTests COMMAND hoo-tests)`
- `run_tests` custom target running `ctest --verbose`

Build tests:

```bash
cmake -S . -B build -DHOOC_BUILD_TESTS=ON
cmake --build build --target hoo-tests
```

## Install Targets

```cmake
install(TARGETS hoo hoo-core hoo-parser hoort ...)
```

Install includes:

- `hoo`
- `hoo-core`
- `hoo-parser`
- `hoort`
- generated headers from `${ANTLR4_GENERATED_DIR}`
- headers from `src/`

```bash
cmake --build build --target install
```

Set prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/desired/prefix
```

## Notes

- `hoo-core` subsumes the former `hvm` and `hoo-compiler` targets, which were tightly coupled via the JIT's dependency on the compiler.
- `hoort` is a standalone C/C++ library with no LLVM or ANTLR4 dependency.
- CMake-generated maintenance targets (`all`, `clean`, `rebuild_cache`, etc.) depend on generator/platform.
