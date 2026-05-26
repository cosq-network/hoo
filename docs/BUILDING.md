# Building Hooc

This document describes how to configure, build, and test the Hooc compiler ecosystem on macOS, Linux, and Windows.

---

## 1. Prerequisites & Dependencies

The compiler build depends on several key components. The examples assume these tools are available in your `PATH`.

### Core Requirements
- **CMake 3.20+**: (3.16+ for manual builds, but 3.20+ is required for Presets)
- **C++17 Compiler**: Clang 15+ (recommended) or MSVC 2022
- **LLVM 15+**: Development headers and libraries
- **ANTLR4 C++ Runtime**: Required for the generated parser
- **Java**: Required to run the ANTLR generator
- **Ninja or Make**: Build systems

### Dependency Summary

| Dependency | Required | Purpose |
|------------|----------|---------|
| CMake 3.20+ | Yes | Configures the build tree and handles presets |
| Clang/Clang++ or MSVC | Yes | Compiles C and C++ sources |
| LLVM Development | Yes | Provides LLVM headers, libraries, and `LLVMConfig.cmake` |
| ANTLR4 C++ Runtime | Yes | Runtime library for generated parser code |
| Java | Yes | Runs the ANTLR jar (re-generating parser sources) |
| ANTLR jar | Yes | Generates C++ parser sources from `src/parsing/Hooc.g4` |
| Ninja or Make | Yes | Executes the generated build files |
| GoogleTest | Optional | Enables unit tests (`hoo-tests` target) |

The repository includes the ANTLR generator jar at `tools/antlr-4.13.2-complete.jar`.

---

## 2. Quick Start: Building with CMake Presets

The project includes `CMakePresets.json` for common platforms. This is the recommended way to build.

### List Available Presets
```bash
cmake --list-presets
cmake --list-presets=build
cmake --list-presets=test
```

### Common Presets

| Preset | Platform / Purpose |
|--------|--------------------|
| `macos-homebrew-ninja` | macOS with LLVM from Homebrew |
| `ubuntu-ninja` | Ubuntu with Clang + Ninja |
| `windows-vs-relwithdebinfo` | Windows with Visual Studio 2022 |
| `windows-vs18-relwithdebinfo` | Windows with Visual Studio 18 / 2026 |
| `windows-vs18-local` | Windows VS 18 using repo-local dependencies from `vcpkg_installed/x64-windows/` |
| `windows-ninja` | Windows with Clang + Ninja |
| `ninja-relwithdebinfo` | Generic Ninja build (RelWithDebInfo) |
| `ninja-release-no-tests` | Generic Ninja release build without tests |

### Build Example (macOS)
```bash
cmake --preset macos-homebrew-ninja
cmake --build --preset macos-homebrew-ninja
```

The build outputs will be located in `build/<preset-name>/`. For example: `build/macos-homebrew-ninja/hoo`.

---

## 3. Platform-Specific Guides

### macOS (Apple Silicon / Intel)
Install dependencies via Homebrew:
```bash
brew install cmake ninja llvm antlr4-cpp-runtime googletest
```
Recommended Preset: `macos-homebrew-ninja`.

### Ubuntu (Linux)
Install dependencies via APT:
```bash
sudo apt update
sudo apt install cmake ninja-build clang llvm-dev libantlr4-runtime-dev libgtest-dev
```
Recommended Preset: `ubuntu-ninja`.

### Windows

#### Prerequisites

Install **Visual Studio 2022 Community** or **Visual Studio 18 / 2026** with the "Desktop development with C++" workload. This provides the MSVC toolchain and a bundled Clang.

If you are using Visual Studio 18 / 2026, prefer the `windows-vs18-relwithdebinfo` preset. The older `windows-vs-relwithdebinfo` preset remains for machines that still target the Visual Studio 2022 generator.

#### 1. Install LLVM Development Package

Visual Studio's bundled Clang only includes the compiler driver — it does **not** ship LLVM development headers or libraries. Install the full LLVM release via winget:

```powershell
winget install LLVM -v 19.1.7
```

Alternatively, download the installer from:
[https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-win64.exe](https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-win64.exe)

After installation, verify that `LLVMConfig.cmake` is findable:

```powershell
Test-Path "$env:ProgramFiles\LLVM\lib\cmake\llvm\LLVMConfig.cmake"
```

#### 2. Install Dependencies via vcpkg

The repository includes a `vcpkg.json` manifest. If you have Visual Studio 18 (2025+), vcpkg is bundled at:

```
C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg
```

Otherwise, install vcpkg from [https://vcpkg.io](https://vcpkg.io).

Install the manifest dependencies:

```powershell
vcpkg install --triplet x64-windows
```

The current manifest includes:
- `antlr4`
- `gtest`
- `llvm[target-x86]`

If you install dependencies into the repository-local `vcpkg_installed/x64-windows/` tree, the checked-in Windows fallback logic and `windows-vs18-local` preset can pick up `ANTLR4` and LLVM from there automatically. This is the Windows path verified in this repository.

#### 3. Build

For Visual Studio 18 / 2026 with the repo-local dependency preset:

```powershell
cmake --preset windows-vs18-local
cmake --build --preset windows-vs18-local
```

This preset resolves `ANTLR4` from `vcpkg_installed/x64-windows/` and prefers the repo-local LLVM package at:

```text
C:\Projects\hooc\vcpkg_installed\x64-windows\share\llvm
```

If the repo-local LLVM package is not present, the build falls back to:

```text
C:\Program Files\LLVM\lib\cmake\llvm
```

This Windows preset has been validated for target builds of:
- `hoo`
- `hoo-core`
- `hoo-parser`
- `hoort`
- `hoo-tests`

For Ninja builds, configure and build using the `windows-ninja` preset, pointing CMake to vcpkg and the installed LLVM:

```powershell
cmake --preset windows-ninja `
  -DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>\scripts\buildsystems\vcpkg.cmake" `
  -DLLVM_DIR="$env:ProgramFiles\LLVM\lib\cmake\llvm"

cmake --build --preset windows-ninja
```

Replace `<vcpkg-root>` with the path to your vcpkg installation (e.g. `C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg`).

For Visual Studio MSVC builds instead of Clang/Ninja, use one of these presets:
- `windows-vs18-relwithdebinfo` for Visual Studio 18 / 2026
- `windows-vs-relwithdebinfo` for Visual Studio 2022

```powershell
cmake --preset windows-vs18-relwithdebinfo `
  -DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>\scripts\buildsystems\vcpkg.cmake" `
  -DLLVM_DIR="$env:ProgramFiles\LLVM\lib\cmake\llvm"
cmake --build --preset windows-vs18-relwithdebinfo
```

If `LLVMConfig.cmake` is missing, the configure step will fail even if `clang.exe` is present. Visual Studio's bundled Clang is not sufficient by itself; the full LLVM development package must provide `LLVMConfig.cmake`.

---

## 4. Build Targets

The project is organized into several primary targets:

### Primary Executables & Libraries
- **`hoo`**: The main compiler executable.
- **`hoo-core`**: The core compiler + HVM + JIT library. Subsumes former `hvm` and `hoo-compiler` targets.
- **`hoo-parser`**: ANTLR-generated parser library.
- **`hoort`**: The Hoo Runtime library (ARC, Strings, etc.). Static by default.

### Utility Targets
- **`generate_parser`**: Generates C++ sources from `Hooc.g4`.
- **`clean_generated`**: Deletes and recreates the generated parser directory.
- **`install`**: Installs executables, libraries, and headers to the `CMAKE_INSTALL_PREFIX`.

### Target Graph
```text
hoo
└── hoo-core
    ├── hoo-parser
    │   └── generate_parser
    │       └── download_antlr4 (conditional)
    ├── hoort
    └── LLVM (Core, OrcJIT, etc.)
```

---

## 5. Manual Configuration (Advanced)

If you have older CMake or custom dependency paths, you can configure manually.

### Basic Manual Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

### Overriding Dependency Paths
If CMake cannot find LLVM or ANTLR4 automatically, pass the paths explicitly:
```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DANTLR4_INCLUDE_DIR=/path/to/antlr4/include \
  -DANTLR4_LIBRARY=/path/to/libantlr4-runtime.a
```

---

## 6. Testing

Tests require `HOOC_BUILD_TESTS=ON` (enabled by default in most presets).

### Running Tests via CTest
```bash
# Build tests
cmake --build --preset <preset>-tests

# Run tests
ctest --preset <preset>
```

On Windows with the checked-in Visual Studio 18 preset:

```powershell
cmake --build --preset windows-vs18-local-tests
ctest --preset windows-vs18-local --output-on-failure
```

The Windows preset configures the required runtime `PATH` for `ctest` and Visual Studio test launches automatically.

### Direct Test Execution
```bash
./build/<preset>/hoo-tests --gtest_brief=1
```

---

## 7. IDE Support

### VS Code
- Required Extensions: **CMake Tools**, **C/C++**.
- CMake Tools will automatically detect `CMakePresets.json`.
- Select your preset from the status bar or Command Palette.

### Eclipse
1. Configure via terminal using a preset.
2. Import as an existing CMake project.
3. Point Eclipse at the `build/<preset-name>/` directory.

---

## 8. Troubleshooting

- **Missing ANTLR4 Headers**: Ensure `ANTLR4_INCLUDE_DIR` points to the directory containing `antlr4-runtime.h`.
- **LLVM Mismatch**: If you have multiple LLVM versions, ensure `LLVM_DIR` points to the specific version required (15+).
- **LLVM Installed But Not Usable**: Verify `C:\Program Files\LLVM\lib\cmake\llvm\LLVMConfig.cmake` exists. If the directory only contains `LLVMConfigExtensions.cmake`, reinstall LLVM with the full development package.
- **Prefer Repo-Local LLVM on Windows**: If you use the manifest-based Windows flow, prefer `vcpkg_installed/x64-windows/share/llvm` over a system LLVM install.
- **Windows DLLs**: If running the compiler fails due to missing DLLs, ensure the LLVM and ANTLR4 bin directories are in your `PATH`.
- **Java Errors**: Verify Java is installed and can run the jar: `java -jar tools/antlr-4.13.2-complete.jar`.
