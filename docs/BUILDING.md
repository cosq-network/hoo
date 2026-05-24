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
Install **Visual Studio 2022 Community** with the "Desktop development with C++" workload.

#### Using vcpkg (Optional)
The repository includes `vcpkg.json` for managing dependencies like GoogleTest.
```powershell
cmake --preset windows-vs-relwithdebinfo `
  -DCMAKE_TOOLCHAIN_FILE="C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

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
- **Windows DLLs**: If running the compiler fails due to missing DLLs, ensure the LLVM and ANTLR4 bin directories are in your `PATH`.
- **Java Errors**: Verify Java is installed and can run the jar: `java -jar tools/antlr-4.13.2-complete.jar`.
