# Build Targets

This document describes the CMake targets used to build Hooc. It focuses on non-test build targets and explains what each target builds, why it exists, and when to use it.

Unit-test targets such as `hoo-tests`, `run_tests`, and `test` are intentionally not covered in detail here.

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
- GoogleTest, when configured with `HOOC_BUILD_TESTS=ON`

## Common Commands

Build the default target set:

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

With presets, the equivalent compiler build is:

```bash
cmake --preset <preset>
cmake --build --preset <preset>
```

See [building-with-presets.md](building-with-presets.md) for the preset workflow.

## Target Graph

The primary non-test build graph is:

```text
hooc
└── hoo-compiler
    ├── hoo-parser
    │   └── generate_parser
    ├── hoort
    └── LLVM libraries
```

Build `hooc` when you want the compiler executable. CMake will build the required parser, runtime, compiler library, and LLVM-linked pieces first.

## Primary Targets

### `hooc`

Build command:

```bash
cmake --build build --target hooc
```

Purpose:

`hooc` is the main command-line compiler executable. It is the top-level product target for normal development builds.

Implementation:

- Source entry point: `src/main.cpp`
- Links to: `hoo-compiler`
- Indirectly pulls in: `hoo-parser`, `hoort`, generated ANTLR parser sources, ANTLR4 runtime, and LLVM libraries

Current behavior:

- Reads a `.hoo` source file from the command line.
- Uses `HooCompiler` to parse source code, build an AST, and generate an LLVM IR module.
- Prints the generated LLVM IR.
- Includes JIT-related headers and messaging, but full command-line JIT execution is not yet wired into `main.cpp`.

Usage:

```bash
cmake --build build --target hooc
./build/hooc path/to/file.hoo
```

With a preset:

```bash
cmake --preset macos-homebrew-ninja
cmake --build --preset macos-homebrew-ninja
./build/macos-homebrew-ninja/hooc path/to/file.hoo
```

Use this target when:

- You want to produce the actual compiler executable.
- You want to verify the full non-test build chain.
- You are changing CLI behavior in `src/main.cpp`.
- You are changing compiler internals and want to confirm the final executable still links.

### `hoo-compiler`

Build command:

```bash
cmake --build build --target hoo-compiler
```

Purpose:

`hoo-compiler` is the core compiler library. It contains the compilation pipeline used by the `hooc` executable and tests.

Implementation:

The target is built from compiler implementation files such as:

- `src/HooCompiler.cpp`
- `src/HoocJIT.cpp`
- `src/ProcessIsolatedParser.cpp`
- `src/LLVMCodeGenerator.cpp`
- `src/SimpleASTBuilder.cpp`
- `src/ast/ASTImpl.cpp`
- `src/ModuleSystem.cpp`
- `src/runtime/RuntimeRegistry.cpp`
- `src/rt/hoo_string_registration.cpp`
- `src/rt/hoo_array_registration.cpp`

It links to:

- `hoo-parser`
- `hoort`
- LLVM libraries selected by CMake

Responsibilities:

- High-level source-to-LLVM compilation through `HooCompiler`.
- Direct parser integration through `ProcessIsolatedParser`.
- AST construction through `SimpleASTBuilder`.
- LLVM IR generation through `LLVMCodeGenerator`.
- JIT infrastructure through `HoocJIT`.
- Module resolution support through `ModuleSystem`.
- Runtime registration glue for strings and arrays.

JIT note:

This is the target that compiles the JIT implementation. `HoocJIT.cpp` creates an LLVM ORC `LLJIT`, registers runtime symbols through `RuntimeRegistry`, and contains function lookup/execution helpers. There is no separate `hooc-jit` CMake target at the moment. JIT capability is part of the `hoo-compiler` library and becomes available to anything linking that library, including `hooc`.

Use this target when:

- You are changing compiler internals.
- You are changing JIT infrastructure.
- You are changing AST building, LLVM code generation, or module resolution.
- You want a faster validation than building every executable target.

### `hoo-parser`

Build command:

```bash
cmake --build build --target hoo-parser
```

Purpose:

`hoo-parser` builds the ANTLR-generated parser library. It isolates generated parser code from the handwritten compiler code.

Implementation:

The target is built from generated files under `${CMAKE_BINARY_DIR}/generated/antlr4` by default, including:

- `HoocLexer.cpp`
- `HoocParser.cpp`
- `HoocVisitor.cpp`
- `HoocListener.cpp`
- `HoocBaseVisitor.cpp`
- `HoocBaseListener.cpp`

It links to:

- ANTLR4 C++ runtime

It depends on:

- `generate_parser`

Responsibilities:

- Tokenize Hooc source text.
- Parse Hooc source text according to `src/Hooc.g4`.
- Provide ANTLR parse-tree contexts consumed by `ProcessIsolatedParser` and `SimpleASTBuilder`.
- Provide generated visitor/listener APIs for parser integration.

Use this target when:

- You changed `src/Hooc.g4`.
- You changed parser generation settings.
- You want to verify parser generation and compilation without building the full compiler.

### `hoort`

Build command:

```bash
cmake --build build --target hoort
```

Purpose:

`hoort` builds the Hooc runtime library. Generated code and JIT-compiled code rely on this runtime for low-level language services.

Implementation:

The target is built from runtime files under `src/rt/`, including:

- `src/rt/hoo_runtime.c`
- `src/rt/hoo_string.cpp`
- `src/rt/hoo_generic_array.cpp`

Runtime responsibilities:

- Reference-counted allocation through `hoo_alloc`.
- Retain/release support through `hoo_retain` and `hoo_release`.
- Runtime memory statistics and debugging helpers.
- String runtime functions.
- Generic array runtime functions.

Configuration:

By default, `hoort` is built as a static library.

```bash
cmake -S . -B build -DHOOC_BUILD_SHARED_RUNTIME=OFF
```

It can be built as a shared library:

```bash
cmake -S . -B build -DHOOC_BUILD_SHARED_RUNTIME=ON
```

In Debug builds, it is compiled with `HOO_DEBUG_MEMORY`.

Use this target when:

- You are changing ARC behavior.
- You are changing runtime strings or arrays.
- You want to verify runtime code independently from parser/compiler changes.
- You are debugging memory behavior.

## JIT-Related Build Behavior

There is currently no standalone CMake target named `jit`, `hooc-jit`, or similar.

JIT-related implementation is compiled into `hoo-compiler`:

- `src/HoocJIT.cpp`
- `src/HoocJIT.h`
- Runtime registration code in `src/rt/hoo_string_registration.cpp`
- Runtime registration code in `src/rt/hoo_array_registration.cpp`
- Registry support in `src/runtime/RuntimeRegistry.cpp`

The JIT implementation uses LLVM ORC `LLJIT` and runtime symbol registration. It is linked into `hooc` through the `hoo-compiler` library.

Important distinction:

- `hoo-compiler` contains JIT infrastructure.
- `hooc` links that infrastructure.
- `src/main.cpp` currently compiles source to LLVM IR and reports that command-line JIT execution integration is pending.

So if you are working on JIT internals, build:

```bash
cmake --build build --target hoo-compiler
```

If you want to verify the final executable still links with JIT code included, build:

```bash
cmake --build build --target hooc
```

## Parser Generation Targets

### `generate_parser`

Build command:

```bash
cmake --build build --target generate_parser
```

Purpose:

`generate_parser` runs ANTLR and creates the C++ parser sources from the Hooc grammar.

Implementation:

- Grammar input: `src/Hooc.g4`
- Default generated output: `${CMAKE_BINARY_DIR}/generated/antlr4`
- ANTLR jar: `tools/antlr-4.13.2-complete.jar` by default
- Java executable: discovered with `find_package(Java REQUIRED COMPONENTS Runtime)`

Generated output includes lexer, parser, visitor, listener, token, and interpreter files.

Use this target when:

- You changed `src/Hooc.g4`.
- You want to inspect generated parser output.
- You want to regenerate parser files without building the full compiler.

Override the ANTLR jar:

```bash
cmake -S . -B build -DANTLR4_JAR_PATH=/path/to/antlr.jar
```

Override the generated output directory:

```bash
cmake -S . -B build -DANTLR4_GENERATED_DIR=/path/to/generated/antlr4
```

### `download_antlr4`

Build command:

```bash
cmake --build build --target download_antlr4
```

Purpose:

`download_antlr4` downloads the ANTLR jar into `tools/` when the configured jar path does not exist at configure time.

Important details:

- This target is only defined when `ANTLR4_JAR_PATH` is missing during CMake configuration.
- `generate_parser` depends on it when it exists.
- If the jar is already present, this target will not appear in the build target list.

Use this target when:

- You configured the project without the ANTLR jar present.
- You want CMake to fetch the expected ANTLR jar instead of installing it manually.

### `clean_generated`

Build command:

```bash
cmake --build build --target clean_generated
```

Purpose:

`clean_generated` removes and recreates the configured generated parser directory.

Use this target when:

- Generated parser files appear stale.
- You changed parser generation settings.
- You changed `ANTLR4_GENERATED_DIR`.
- You want a clean parser regeneration without deleting the whole build directory.

Typical sequence:

```bash
cmake --build build --target clean_generated
cmake --build build --target generate_parser
cmake --build build --target hooc
```

## Install Targets

### `install`

Build command:

```bash
cmake --build build --target install
```

Purpose:

`install` copies built artifacts and public headers into the configured installation prefix.

Installed artifacts:

- `hooc`
- `hoo-compiler`
- `hoo-parser`
- `hoort`
- Generated parser headers from the configured `ANTLR4_GENERATED_DIR`
- Project headers from `src/`

Choose an install prefix at configure time:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/desired/prefix
cmake --build build --target install
```

Use this target when:

- You want a local install of the compiler executable and libraries.
- You are preparing artifacts for packaging.
- You want downstream projects to consume headers and libraries from a stable prefix.

### `install/local`, `install/strip`, and `list_install_components`

These are CMake-generated helper targets. Their exact behavior depends on the configured generator and platform.

- `install/local` installs without first rebuilding dependencies.
- `install/strip` installs stripped binaries when the platform and toolchain support it.
- `list_install_components` prints install components known to CMake.

## CMake Maintenance Targets

### `all`

Build command:

```bash
cmake --build build --target all
```

Purpose:

`all` builds the default target set for the configured generator. It is equivalent to running:

```bash
cmake --build build
```

Use this when you want the generator's default build behavior rather than one specific target.

### `clean`

Build command:

```bash
cmake --build build --target clean
```

Purpose:

`clean` removes generated build outputs from the build directory. Parser files generated under the configured `ANTLR4_GENERATED_DIR` can also be removed with `clean_generated`.

Use this when:

- You want to remove object files, libraries, and executables.
- You want to keep the CMake configuration but rebuild outputs.

For a full fresh configure, delete the build directory and run CMake again.

### `rebuild_cache`

Build command:

```bash
cmake --build build --target rebuild_cache
```

Purpose:

`rebuild_cache` asks CMake to regenerate cache information for the existing build directory.

Use this when:

- CMake inputs changed.
- You want to refresh configuration without manually deleting the build directory.

For dependency path changes, it is usually clearer to rerun:

```bash
cmake -S . -B build -DNAME=value
```

### `edit_cache`

Build command:

```bash
cmake --build build --target edit_cache
```

Purpose:

`edit_cache` opens the CMake cache editor for generators and environments that support it.

For scripted or repeatable configuration, prefer command-line `-D` options or CMake presets.

### `depend`

Build command:

```bash
cmake --build build --target depend
```

Purpose:

`depend` refreshes dependency information for Makefile-style generators.

This target is generator-specific and may not exist for Ninja, Visual Studio, or other CMake backends.

## LLVM-Generated Helper Targets

Some configured build trees expose LLVM utility targets such as:

- `intrinsics_gen`
- `target_parser_gen`
- `acc_gen`
- `omp_gen`
- `vt_gen`

These come from LLVM's CMake package and are not Hooc source targets. They support LLVM's generated headers and internal build metadata.

Normally, do not invoke these directly. Build Hooc targets such as `hooc`, `hoo-compiler`, or `hoo-parser` instead.

## Target Selection Guide

| Goal | Target |
|------|--------|
| Build the compiler executable | `hooc` |
| Build compiler internals only | `hoo-compiler` |
| Work on JIT internals | `hoo-compiler`, then `hooc` |
| Work on grammar/parser generation | `generate_parser`, then `hoo-parser` |
| Work on runtime memory, strings, or arrays | `hoort` |
| Install compiler and libraries | `install` |
| Clean generated parser files | `clean_generated` |
| Clean build outputs | `clean` |

For most development work, build `hooc`. It is the most direct way to confirm that the complete non-test compiler build chain still works.
