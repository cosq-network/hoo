# Platform Build Guide

This document describes how to configure and build Hooc on macOS, Linux, and Windows.

The examples assume the platform already has these tools available in `PATH`:

- CMake
- `make` or Ninja
- Clang and Clang++
- Java
- On Windows, Visual Studio Community Edition with the Desktop development with C++ workload

Hooc also needs development packages for LLVM and the ANTLR4 C++ runtime. GoogleTest is optional for building the compiler, but it is required if unit tests should be configured.

The project includes `CMakePresets.json` for common configure and build flows. Presets require CMake 3.20 or newer. Developers with older CMake versions can use the explicit `cmake -S . -B ...` commands shown below.

## Dependency Summary

The compiler build depends on these components:

| Dependency | Required | Purpose |
|------------|----------|---------|
| CMake 3.16+ | Yes | Configures the build tree |
| Clang/Clang++ or MSVC | Yes | Compiles C and C++ sources |
| LLVM development package | Yes | Provides LLVM headers, libraries, and `LLVMConfig.cmake` |
| ANTLR4 C++ runtime | Yes | Runtime library for generated parser code |
| Java | Yes when regenerating parser sources | Runs the ANTLR jar |
| ANTLR jar | Yes when regenerating parser sources | Generates C++ parser sources from `src/parsing/Hooc.g4` |
| Ninja or Make | Yes for single-config builds | Executes the generated build files |
| Visual Studio Community C++ workload | Yes on Windows with the Visual Studio generator | Provides the MSVC compiler, linker, SDK, and IDE integration |
| GoogleTest | Optional | Enables `hoo-tests`, `test`, and `run_tests` targets |

The repository already includes:

```text
tools/antlr-4.13.2-complete.jar
```

Generated parser sources are written to the build tree by default:

```text
build/<build-dir>/generated/antlr4/
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

The ANTLR grammar is `src/parsing/Hooc.g4`. Generated parser sources are written to `build/<preset>/generated/antlr4` by default and are no longer committed to the repository. The repository includes `tools/antlr-4.13.2-complete.jar`, and CMake uses that path by default through `ANTLR4_JAR_PATH`.

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

Example dependency locations:

```text
C:\Program Files\LLVM\lib\cmake\llvm
D:\antlr4\runtime\cpp\include\antlr4-runtime
D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib
```

Any local ANTLR4 runtime location is fine when passed explicitly during configuration with `ANTLR4_ROOT`, or with `ANTLR4_INCLUDE_DIR` and `ANTLR4_LIBRARY`.

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

The `macos-homebrew-ninja` preset points CMake at the default Apple Silicon Homebrew prefixes for LLVM and ANTLR4.

### Configure with the Preset

```bash
cmake --preset macos-homebrew-ninja
cmake --build --preset macos-homebrew-ninja
```

### Configure with Ninja

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/llvm;/opt/homebrew"
```

Build the compiler:

```bash
cmake --build build --target hooc
```

### Configure with Unix Makefiles

```bash
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/llvm;/opt/homebrew"
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

### Configure with the Preset

```bash
cmake --preset ubuntu-ninja
cmake --build --preset ubuntu-ninja
```

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

When using manual dependency paths, keep track of the ANTLR runtime include directory and library file. For example:

```text
D:\antlr4\runtime\cpp\include\antlr4-runtime
D:\antlr4\runtime\cpp\lib
```

If ANTLR4 is installed somewhere else, pass `ANTLR4_ROOT`, or pass `ANTLR4_INCLUDE_DIR` and `ANTLR4_LIBRARY` during configuration.

LLVM for Windows normally provides `LLVMConfig.cmake` under a path like:

```text
C:\Program Files\LLVM\lib\cmake\llvm
```

Pass `LLVM_DIR` if CMake does not find it automatically.

### Configure with Visual Studio

From the repository root:

```powershell
cmake --preset windows-vs-relwithdebinfo `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm" `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
cmake --build --preset windows-vs-relwithdebinfo
```

Equivalent explicit configuration:

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
cmake --preset windows-ninja `
  -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm" `
  -DANTLR4_INCLUDE_DIR="D:\antlr4\runtime\cpp\include\antlr4-runtime" `
  -DANTLR4_LIBRARY="D:\antlr4\runtime\cpp\lib\antlr4-runtime.lib"
cmake --build --preset windows-ninja
```

Equivalent explicit configuration:

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

## Build System Notes

The build now uses these cross-platform conventions:

- Dependency paths are provided through `CMAKE_PREFIX_PATH`, `LLVM_DIR`, `ANTLR4_ROOT`, `ANTLR4_INCLUDE_DIR`, `ANTLR4_LIBRARY`, or a toolchain file.
- `CMakePresets.json` defines common macOS, Ubuntu, Windows Visual Studio, and Windows Ninja flows.
- ANTLR4 parser output is generated under the build tree by default.
- `HOOC_BUILD_TESTS` controls GoogleTest discovery and test target creation.
- Debug-only runtime memory tracking uses a generator expression, so it works with single-config and multi-config generators.
- Install rules include `hooc`, `hoo-core`, `hoo-parser`, `hoort`, generated parser headers, and project headers.
- `HOOC_BUILD_SHARED_RUNTIME` can switch `hoort` from a static runtime library to a shared runtime library.

Remaining improvement to consider:

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

Parser sources are regenerated automatically when needed by targets such as `hooc`, `hoo-core`, and `hoo-parser`.

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

Unit tests are controlled by `HOOC_BUILD_TESTS`, which defaults to `OFF` for manual CMake configuration. Test-oriented presets set it to `ON`. When tests are enabled, GoogleTest is required and CMake fails during configuration if it cannot be found.

Enable tests when configuring manually:

```bash
cmake -S . -B build -G Ninja -DHOOC_BUILD_TESTS=ON
```

When tests are enabled, the project creates the `hoo-tests`, `test`, and `run_tests` targets.

Run tests with:

```bash
cmake --build build --target run_tests
```

For Visual Studio builds:

```powershell
cmake --build build --config RelWithDebInfo --target run_tests
```

If GoogleTest is not installed and you only need the compiler, configure with `-DHOOC_BUILD_TESTS=OFF`.

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
