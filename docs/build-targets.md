# Build Targets

This document describes the CMake build targets used to build Hooc, excluding unit-test targets such as `hoo-tests`, `run_tests`, and `test`.

## Prerequisites

Configure the project before building individual targets:

```bash
cmake -S . -B build
```

The configuration expects:

- CMake 3.16 or newer
- A C++17 compiler
- LLVM development files
- ANTLR4 C++ runtime
- Java, when parser sources need to be regenerated from `src/Hooc.g4`

## Common Build Commands

Build the default target:

```bash
cmake --build build
```

Build a named target:

```bash
cmake --build build --target <target>
```

List targets available in the configured build tree:

```bash
cmake --build build --target help
```

## Primary Targets

### `hooc`

Builds the main Hooc compiler executable from `src/main.cpp`.

```bash
cmake --build build --target hooc
```

This target links against `hoo-compiler`, so it also builds the compiler library, runtime library, generated parser library, and generated parser sources as needed.

### `hoo-compiler`

Builds the compiler library. This contains the main compiler interface, AST construction, code generation, JIT support, module system, runtime registry, and runtime registration glue.

```bash
cmake --build build --target hoo-compiler
```

This target depends on:

- `hoo-parser`
- `hoort`
- LLVM libraries selected by CMake

### `hoort`

Builds the static Hooc runtime library. This includes reference-counting support, string support, and generic array support under `src/rt/`.

```bash
cmake --build build --target hoort
```

In Debug builds, this target is compiled with `HOO_DEBUG_MEMORY`.

### `hoo-parser`

Builds the generated ANTLR4 parser library.

```bash
cmake --build build --target hoo-parser
```

This target depends on `generate_parser`, so the generated parser sources are created first if they are missing or stale.

## Parser Generation Targets

### `generate_parser`

Generates the ANTLR4 C++ parser sources from `src/Hooc.g4` into `antlr4/generated/`.

```bash
cmake --build build --target generate_parser
```

The generated files include lexer, parser, visitor, and listener sources and headers. CMake uses `tools/antlr-4.13.2-complete.jar` by default through the `ANTLR4_JAR_PATH` cache variable.

To use a different ANTLR jar during configuration:

```bash
cmake -S . -B build -DANTLR4_JAR_PATH=/path/to/antlr.jar
```

### `download_antlr4`

Downloads an ANTLR jar into `tools/` when the configured jar path does not already exist.

```bash
cmake --build build --target download_antlr4
```

This target is only defined when `ANTLR4_JAR_PATH` is missing at configure time. `generate_parser` depends on it in that case.

### `clean_generated`

Removes and recreates the generated parser directory.

```bash
cmake --build build --target clean_generated
```

Run `generate_parser`, `hoo-parser`, `hoo-compiler`, or `hooc` afterward to regenerate parser sources.

## Install Targets

### `install`

Installs build artifacts and generated parser headers using the configured CMake install prefix.

```bash
cmake --build build --target install
```

The install rules currently include:

- `hoo-compiler`
- `hoo-parser`
- generated parser headers from `antlr4/generated/`

Use `CMAKE_INSTALL_PREFIX` at configure time to choose the destination:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/desired/prefix
cmake --build build --target install
```

### `install/local`, `install/strip`, and `list_install_components`

These are CMake-generated install helper targets. Availability and exact behavior depend on the configured generator.

- `install/local` installs without rebuilding dependencies first.
- `install/strip` installs stripped binaries when supported by the platform and toolchain.
- `list_install_components` prints install components known to CMake.

## CMake Maintenance Targets

### `all`

Builds the default target set for the configured generator.

```bash
cmake --build build --target all
```

This is the same build set used when no target is specified.

### `clean`

Removes generated build outputs from the build directory.

```bash
cmake --build build --target clean
```

This does not remove source-tree generated parser files under `antlr4/generated/`; use `clean_generated` for those.

### `rebuild_cache`

Regenerates CMake cache information for the existing build directory.

```bash
cmake --build build --target rebuild_cache
```

Use this when CMake configuration inputs changed and a full fresh configure is not necessary.

### `edit_cache`

Opens the CMake cache editor for generators and environments that support it.

```bash
cmake --build build --target edit_cache
```

For scripted changes, prefer passing `-D` options to `cmake -S . -B build`.

### `depend`

Refreshes dependency information for Makefile-style generators.

```bash
cmake --build build --target depend
```

This target is generator-specific and may not exist for every CMake backend.

## LLVM-Generated Helper Targets

Some configured trees expose LLVM utility targets such as `intrinsics_gen`, `target_parser_gen`, `acc_gen`, `omp_gen`, and `vt_gen`. These come from LLVM's CMake package and are not Hooc source targets.

Normally, build Hooc targets such as `hooc` or `hoo-compiler` instead of invoking these directly.

## Dependency Overview

The main non-test dependency chain is:

```text
hooc
└── hoo-compiler
    ├── hoo-parser
    │   └── generate_parser
    ├── hoort
    └── LLVM libraries
```

Building `hooc` is the most direct way to produce the compiler and all required non-test dependencies.
