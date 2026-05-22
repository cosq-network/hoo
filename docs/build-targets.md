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

## Target Graph (non-test)

```text
hooc
└── hoo-compiler
    ├── hvm
    ├── hoo-parser
    │   └── generate_parser
    │       └── download_antlr4 (only when jar is missing at configure time)
    └── hoort

ho
└── ho-lib
```

## Primary Targets

### `hooc` (executable)

- Source: `src/core/main.cpp`
- Links: `hoo-compiler`
- Build:

```bash
cmake --build build --target hooc
```

### `hoo-compiler` (library)

Core compiler/JIT/runtime-registration library.

- Key sources under:
  - `src/core/`
  - `src/jit/`
  - `src/parsing/`
  - `src/codegen/`
  - `src/ast/`
  - `src/modules/`
  - `src/runtime/llvm/`
- Links: `hvm`, `hoo-parser`, `hoort`, `${llvm_libs}`
- Build:

```bash
cmake --build build --target hoo-compiler
```

### `hvm` (static library)

Hooc VM/module artifacts support.

- Sources:
  - `src/hvm/HoModuleBase.cpp`
  - `src/hvm/HoModule.cpp`
  - `src/hvm/HInstruction.cpp`
  - `src/hvm/ModuleBundle.cpp`
- Links: `${llvm_libs}`
- Build:

```bash
cmake --build build --target hvm
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

- Sources under:
  - `src/runtime/lib/`
  - `src/runtime/llvm/`
- Debug compile def: `HOO_DEBUG_MEMORY`
- Build:

```bash
cmake --build build --target hoort
```

### `ho` (executable)

- Source: `src/ho/main.cpp`
- Links: `ho-lib`
- Build:

```bash
cmake --build build --target ho
```

### `ho-lib` (library)

- Source: `src/ho/HoCLI.cpp`
- Build:

```bash
cmake --build build --target ho-lib
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

Configured install command:

```cmake
install(TARGETS hooc ho hoo-compiler hoo-parser hoort ...)
```

So install includes:

- `hooc`
- `ho`
- `hoo-compiler`
- `hoo-parser`
- `hoort`
- generated headers from `${ANTLR4_GENERATED_DIR}`
- headers from `src/`

Run install:

```bash
cmake --build build --target install
```

Set prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/desired/prefix
```

## Notes

- `hvm` is built as `STATIC` in current CMake.
- There is no standalone `jit` target; JIT implementation is part of `hoo-compiler`.
- CMake-generated maintenance targets (`all`, `clean`, `rebuild_cache`, etc.) depend on generator/platform.
