# Platform Build Guide

This document describes how to configure and build Hooc on macOS, Linux, and Windows.

The examples assume the platform already has these tools available in `PATH`:

- CMake
- `make` or Ninja
- Clang and Clang++
- Java
- On Windows, Visual Studio Community Edition with the Desktop development with C++ workload

Hooc also needs development packages for LLVM and the ANTLR4 C++ runtime. GoogleTest is optional for building the compiler, but it is required if unit tests should be configured.

## Dependency Summary

The compiler build depends on these components:

| Dependency | Required | Purpose |
|------------|----------|---------|
| CMake 3.16+ | Yes | Configures the build tree |
| Clang/Clang++ or MSVC | Yes | Compiles C and C++ sources |
| LLVM development package | Yes | Provides LLVM headers, libraries, and `LLVMConfig.cmake` |
| ANTLR4 C++ runtime | Yes | Runtime library for generated parser code |
| Java | Yes when regenerating parser sources | Runs the ANTLR jar |
| ANTLR jar | Yes when regenerating parser sources | Generates C++ parser sources from `src/Hooc.g4` |
| Ninja or Make | Yes for single-config builds | Executes the generated build files |
| Visual Studio Community C++ workload | Yes on Windows with the Visual Studio generator | Provides the MSVC compiler, linker, SDK, and IDE integration |
| GoogleTest | Optional | Enables `hoo-tests`, `test`, and `run_tests` targets |

The repository already includes:

```text
tools/antlr-4.13.2-complete.jar
```

Generated parser sources are checked into:

```text
antlr4/generated/
```

Even when generated sources already exist, CMake still needs the ANTLR4 C++ runtime headers and library to compile `hoo-parser`.

## Build Outputs

The main build target is `hooc`.

```bash
cmake --build build --target hooc
```

Depending on the generator, the executable is written to one of these locations:

- Single-config generators, such as Ninja or Unix Makefiles: `build/hooc`
- Visual Studio multi-config generators: `build/<Config>/hooc.exe`, for example `build/RelWithDebInfo/hooc.exe`

For the full target list, see [build-targets.md](build-targets.md).

## Shared Configuration Notes

The project uses C++17 and CMake 3.16 or newer.

The ANTLR grammar is `src/Hooc.g4`. Generated parser sources are written to `antlr4/generated/`. The repository includes `tools/antlr-4.13.2-complete.jar`, and CMake uses that path by default through `ANTLR4_JAR_PATH`.

If CMake cannot find LLVM automatically, pass `LLVM_DIR`:

```bash
cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
```

If CMake cannot find the ANTLR4 C++ runtime automatically, pass both variables:

```bash
cmake -S . -B build \
  -DANTLR4_INCLUDE_DIR=/path/to/antlr4-runtime/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/path/to/libantlr4-runtime.a
```

Use the platform-specific library filename on Windows, for example `antlr4-runtime.lib`.

## Dependency Installation Options

Use the package-manager path when available. Use manual installation when the package manager does not provide a new enough LLVM, does not provide the ANTLR4 C++ runtime, or installs files in paths CMake does not search automatically.

### macOS with Homebrew

Install dependencies:

```bash
brew install cmake ninja llvm antlr4-cpp-runtime googletest
```

Useful locations:

```text
/opt/homebrew/opt/llvm/lib/cmake/llvm
/opt/homebrew/include/antlr4-runtime
/opt/homebrew/lib/libantlr4-runtime.dylib
```

On Intel Macs, Homebrew commonly uses `/usr/local` instead of `/opt/homebrew`.

### Ubuntu 24.04 with APT

Install dependencies:

```bash
sudo apt update
sudo apt install cmake ninja-build clang llvm-dev libantlr4-runtime-dev libgtest-dev
```

Useful locations commonly look like:

```text
/usr/lib/llvm-18/lib/cmake/llvm
/usr/include/antlr4-runtime
/usr/lib/x86_64-linux-gnu/libantlr4-runtime.so
```

The LLVM version number and library architecture directory can vary. If CMake cannot locate LLVM or ANTLR4 automatically, pass explicit paths with `LLVM_DIR`, `ANTLR4_INCLUDE_DIR`, and `ANTLR4_LIBRARY`.

### Windows with Visual Studio and Manual Dependencies

Install:

- Visual Studio Community Edition
- Desktop development with C++ workload
- CMake tools for Windows, if not already installed separately
- LLVM for Windows
- ANTLR4 C++ runtime built or installed locally

Recommended dependency locations for the current CMake file:

```text
C:\Program Files\LLVM\lib\cmake\llvm
D:\antlr4\runtime\cpp\include\antlr4-runtime
D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib
```

The `D:\antlr4` layout matches the paths currently hard-coded in `CMakeLists.txt`. Other locations are fine when passed explicitly during configuration.

### Windows with vcpkg

The repository includes `vcpkg.json`, currently listing GoogleTest. vcpkg can be useful for optional test dependencies and can also be extended to manage more dependencies later.

Example configure command for Visual Studio with an existing vcpkg checkout:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm" `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
```

This keeps GoogleTest discovery aligned with the manifest while still using manually supplied LLVM and ANTLR4 runtime paths.

### Manual ANTLR4 C++ Runtime Installation

Manual installation is useful when the system package manager does not provide the runtime or installs a version that is incompatible with the generated parser.

Build or obtain the ANTLR4 C++ runtime, then record:

- The directory containing `antlr4-runtime.h`
- The static or import library file, such as `libantlr4-runtime.a`, `libantlr4-runtime.so`, or `antlr4-runtime.lib`
- Any runtime shared library or DLL directory needed when running `hooc`

Configure with explicit paths:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DANTLR4_INCLUDE_DIR=/path/to/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/path/to/libantlr4-runtime.a
```

On Windows:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
```

### Manual LLVM Installation

Manual LLVM installation is useful when the platform package manager installs an older LLVM or omits CMake package files.

The important path is the directory containing `LLVMConfig.cmake`.

Configure with:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
```

On Windows:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm"
```

## macOS: Apple Silicon, Sequoia

These steps are intended for Apple Silicon Macs, such as Apple M1 on macOS Sequoia.

Install the development dependencies if they are not already present:

```bash
brew install llvm antlr4-cpp-runtime googletest ninja
```

The project already adds `/opt/homebrew/opt/llvm` to `CMAKE_PREFIX_PATH`, which matches the default Homebrew prefix on Apple Silicon.

### Configure with Ninja

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++
```

Build the compiler:

```bash
cmake --build build --target hooc
```

### Configure with Unix Makefiles

```bash
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++
```

Build the compiler:

```bash
cmake --build build --target hooc
```

### macOS Troubleshooting

If Homebrew LLVM is installed somewhere else, pass `LLVM_DIR` explicitly:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm"
```

If Java cannot run the ANTLR jar, verify:

```bash
java -jar tools/antlr-4.13.2-complete.jar
```

## Linux: Ubuntu 24.04

Install the development dependencies if they are not already present:

```bash
sudo apt update
sudo apt install llvm-dev libantlr4-runtime-dev libgtest-dev ninja-build
```

The exact LLVM version depends on the package set installed on the machine. If multiple LLVM versions are installed, use the matching `LLVM_DIR` for the version you want CMake to use.

### Configure with Ninja

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
```

Build the compiler:

```bash
cmake --build build --target hooc
```

### Configure with Unix Makefiles

```bash
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
```

Build the compiler:

```bash
cmake --build build --target hooc
```

### Ubuntu Troubleshooting

If CMake cannot find LLVM, locate `LLVMConfig.cmake` and pass its directory:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
```

If CMake cannot find the ANTLR4 runtime, pass the include directory and library explicitly:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DANTLR4_INCLUDE_DIR=/usr/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/usr/lib/x86_64-linux-gnu/libantlr4-runtime.so
```

Adjust the library path if the package manager installed it under a different architecture directory.

## Windows 10/11: Visual Studio Community

Use a Developer PowerShell or Developer Command Prompt for Visual Studio so CMake can find the MSVC toolchain.

The project has Windows-specific CMake settings:

- If no build type is set, Windows defaults to `RelWithDebInfo`.
- MSVC builds use the release runtime library, `/MD`, to match LLVM libraries.
- Debug builds also define `_ITERATOR_DEBUG_LEVEL=0` to avoid runtime-library mismatches.

### Required Dependency Layout

The current CMake search paths include this ANTLR runtime location:

```text
D:\antlr4\runtime\cpp\include\antlr4-runtime
D:\antlr4\runtime\cpp\lib
```

If ANTLR4 is installed somewhere else, pass `ANTLR4_INCLUDE_DIR` and `ANTLR4_LIBRARY` during configuration.

LLVM for Windows normally provides `LLVMConfig.cmake` under a path like:

```text
C:\Program Files\LLVM\lib\cmake\llvm
```

Pass `LLVM_DIR` if CMake does not find it automatically.

### Configure with Visual Studio

From the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm" `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
```

Build the compiler:

```powershell
cmake --build build --config RelWithDebInfo --target hooc
```

The executable should be created at:

```text
build\RelWithDebInfo\hooc.exe
```

### Configure with Ninja on Windows

Use this option when Ninja is installed and you want a single-config build directory:

```powershell
cmake -S . -B build-ninja -G Ninja `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm" `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
```

Build the compiler:

```powershell
cmake --build build-ninja --target hooc
```

The executable should be created at:

```text
build-ninja\hooc.exe
```

### Windows Troubleshooting

If CMake reports that the ANTLR4 runtime is missing, verify that the include directory contains `antlr4-runtime.h` and that the library path points to the `.lib` file, not only the directory.

If the linker cannot find LLVM libraries, verify that `LLVM_DIR` points to the directory containing `LLVMConfig.cmake`.

If DLLs are required at runtime, put the matching LLVM and ANTLR runtime DLL directories in `PATH`, or copy the required DLLs beside `hooc.exe`.

## Build System Improvement Suggestions

The current build works, but a few changes would make it easier to configure consistently across platforms.

### Prefer CMake Packages Over Hard-Coded Paths

Current behavior:

- LLVM is found with `find_package(LLVM REQUIRED CONFIG)`, but the project prepends `/opt/homebrew/opt/llvm` to `CMAKE_PREFIX_PATH`.
- ANTLR4 runtime is found with hard-coded macOS and Windows paths.
- GoogleTest has several fallback paths, including a specific Homebrew Cellar version.

Suggested improvement:

- Let users provide `CMAKE_PREFIX_PATH`, `LLVM_DIR`, or a toolchain file instead of changing `CMAKE_PREFIX_PATH` inside the project.
- Add cache variables for dependency roots, such as `ANTLR4_ROOT`, and derive include/library paths from those roots.
- Prefer imported CMake targets when dependency packages provide them.

Example direction:

```cmake
set(ANTLR4_ROOT "" CACHE PATH "ANTLR4 C++ runtime installation prefix")
find_path(ANTLR4_INCLUDE_DIR antlr4-runtime.h
    HINTS "${ANTLR4_ROOT}/include/antlr4-runtime"
)
find_library(ANTLR4_LIBRARY antlr4-runtime
    HINTS "${ANTLR4_ROOT}/lib"
)
```

### Add CMake Presets

Suggested improvement:

- Add `CMakePresets.json` for common configurations.
- Include presets for macOS Homebrew Ninja, Ubuntu Ninja, Windows Visual Studio, and Windows Ninja.
- Keep platform-specific path assumptions in presets instead of requiring every user to remember long configure commands.

Example preset names:

```text
macos-ninja-release
ubuntu-ninja-release
windows-vs-relwithdebinfo
windows-ninja-relwithdebinfo
```

### Use Out-of-Source Generated Parser Files

Current behavior:

- Generated parser files are written to `antlr4/generated/` under the source tree.

Suggested improvement:

- Generate parser files under the build tree, for example `${CMAKE_BINARY_DIR}/generated/antlr4`.
- Include that generated directory from targets that need it.
- Avoid source-tree churn during local builds and CI builds.

This would make `clean` remove generated parser files as part of the build directory cleanup and reduce accidental commits of regenerated parser output.

### Make Test Configuration Explicit

Current behavior:

- GoogleTest is optional.
- If it is missing, CMake configures the compiler and skips test targets with a warning.

Suggested improvement:

- Add an option such as `HOOC_BUILD_TESTS`.
- When `HOOC_BUILD_TESTS=ON`, make missing GoogleTest a fatal configuration error.
- When `HOOC_BUILD_TESTS=OFF`, skip all GoogleTest discovery.

Example direction:

```cmake
option(HOOC_BUILD_TESTS "Build Hooc unit tests" ON)
if(HOOC_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif()
```

### Avoid Configuration-Specific Logic for Multi-Config Generators

Current behavior:

- Runtime debug memory tracking is enabled only when `CMAKE_BUILD_TYPE` equals `Debug`.

Issue:

- `CMAKE_BUILD_TYPE` is not used by Visual Studio multi-config builds in the same way it is used by Ninja or Makefile single-config builds.

Suggested improvement:

- Use generator expressions for configuration-specific definitions.

Example direction:

```cmake
target_compile_definitions(hoort PRIVATE
    $<$<CONFIG:Debug>:HOO_DEBUG_MEMORY>
)
```

### Install All User-Facing Artifacts

Current behavior:

- Install rules include `hoo-compiler`, `hoo-parser`, and generated parser headers.
- The `hooc` executable and `hoort` runtime library are not listed in the install target.

Suggested improvement:

- Install `hooc` and `hoort` if they are intended to be distributed.
- Install public headers from `src/` and `src/rt/` if downstream projects are expected to link against Hooc libraries.

Example direction:

```cmake
install(TARGETS hooc hoo-compiler hoo-parser hoort
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
```

### Keep Runtime Library Type Configurable

Current behavior:

- `hoort` is always built as a static library.

Suggested improvement:

- Respect `BUILD_SHARED_LIBS`, or add a project-specific option such as `HOOC_BUILD_SHARED_RUNTIME`.
- Document runtime DLL or shared-library search paths for Windows and Linux if shared builds are enabled.

### Add CI Matrix Coverage

Suggested improvement:

- Add CI jobs for macOS, Ubuntu, and Windows.
- Configure each platform with at least one generator.
- Build `hooc`, run `generate_parser`, and run tests when GoogleTest is available.

Recommended minimum matrix:

| Platform | Generator | Target |
|----------|-----------|--------|
| macOS Apple Silicon or macOS latest | Ninja | `hooc` |
| Ubuntu 24.04 | Ninja | `hooc` and `run_tests` |
| Windows 2022 | Visual Studio 17 2022 | `hooc` and `run_tests` |

## Parser Regeneration

Parser sources are regenerated automatically when needed by targets such as `hooc`, `hoo-compiler`, and `hoo-parser`.

To regenerate the parser directly:

```bash
cmake --build build --target generate_parser
```

To remove generated parser files and rebuild them:

```bash
cmake --build build --target clean_generated
cmake --build build --target generate_parser
```

On Windows Visual Studio builds, include the configuration for build steps that need it:

```powershell
cmake --build build --config RelWithDebInfo --target generate_parser
```

## Optional: Unit Tests

GoogleTest is optional at configure time. If CMake finds it, the project also creates the `hoo-tests`, `test`, and `run_tests` targets.

Run tests with:

```bash
cmake --build build --target run_tests
```

For Visual Studio builds:

```powershell
cmake --build build --config RelWithDebInfo --target run_tests
```

If GoogleTest is not found, CMake prints a warning and configures the compiler targets without unit-test targets.

## Clean Builds

For a normal cleanup:

```bash
cmake --build build --target clean
```

For a fresh reconfigure, delete the build directory and configure again:

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

On Windows PowerShell:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```
