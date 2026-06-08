# Building Hooc

This document describes how to configure, build, and test the Hooc compiler ecosystem on macOS, Linux, and Windows.

---

## 1. Prerequisites & Dependencies

The compiler build depends on several key components. Below is a summary of everything you need, explained step by step for each platform.

### Core Requirements

| Dependency | Required | Purpose |
|------------|----------|---------|
| CMake 3.20+ | Yes | Configures the build tree (3.16+ works for manual builds; 3.20+ needed for Presets) |
| C++17 Compiler | Yes | Clang 15+ (recommended) or MSVC 2022 |
| LLVM 15+ dev headers/libs | Yes | LLVM core libraries, ORC JIT, Target, etc. |
| ANTLR4 C++ Runtime | Yes | Runtime library for the generated parser |
| Java 17+ | Yes | Runs the ANTLR generator jar |
| Ninja or Make | Yes | Executes the generated build files |
| GoogleTest | Optional | Enables unit tests (`hoo-tests` target) |

The repository includes the ANTLR generator jar at `tools/antlr-4.13.2-complete.jar`.

### How to Verify Each Dependency

Before building, check that your toolchain is installed:

```bash
cmake --version           # Must be 3.16+ (3.20+ for presets)

# Compiler
clang++ --version         # macOS/Linux — must be 15+
cl --version              # Windows — Visual Studio 2022+

# LLVM (must have dev headers, not just the compiler)
llvm-config --version     # macOS/Linux — must output 15+
llvm-config --includedir  # Should print a valid include path

# Java
java -version             # Must be 17+
```

---

### Platform-Specific Dependency Installation

#### macOS (Apple Silicon / Intel)

1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install all build dependencies**:
   ```bash
   brew install cmake ninja llvm antlr4-cpp-runtime googletest
   ```

3. **Configure PATH** (Homebrew LLVM is "keg-only" — not in default PATH):
   ```bash
   # Add this to your ~/.zshrc (or ~/.bashrc for bash users):
   export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
   export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
   export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
   ```
   Then reload: `source ~/.zshrc`

4. **Verify**:
   ```bash
   cmake --version
   clang++ --version
   llvm-config --version   # Should show 15+
   java -version
   ```

#### Ubuntu (Linux)

1. **Update package lists**:
   ```bash
   sudo apt update
   ```

2. **Install build tools and dependencies**:
   ```bash
   sudo apt install -y cmake ninja-build make clang llvm-dev \
     libgtest-dev uuid-dev wget unzip
   ```

3. **Build ANTLR4 C++ Runtime from source** (Ubuntu's `libantlr4-runtime-dev` package is often outdated):
   ```bash
   ANTLR_VERSION="4.13.2"
   wget -q "https://www.antlr.org/download/antlr4-cpp-runtime-${ANTLR_VERSION}-source.zip"
   unzip -q "antlr4-cpp-runtime-${ANTLR_VERSION}-source.zip" -d antlr4-src
   mkdir antlr4-build && cd antlr4-build
   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_INSTALL_PREFIX=/tmp/antlr4-runtime \
     -DANTLR_BUILD_SHARED=OFF \
     -DANTLR_BUILD_CPP_TESTS=OFF \
     ../antlr4-src
   ninja && ninja install
   cd .. && rm -rf antlr4-src antlr4-build antlr4-cpp-runtime-*-source.zip
   ```

4. **Install Java** (if not present):
   ```bash
   sudo apt install -y default-jdk
   ```

5. **Verify**:
   ```bash
   cmake --version
   clang++ --version
   llvm-config --version
   java -version
   ls /tmp/antlr4-runtime/lib/libantlr4-runtime.a  # Should exist
   ```

#### Fedora / RedHat / CentOS

1. **Install build tools**:
   ```bash
   sudo dnf groupinstall -y "Development Tools" "C Development Tools and Libraries"
   sudo dnf install -y cmake ninja-build clang lldb gdb llvm-devel \
     antlr4-cpp-runtime-devel gtest-devel java-latest-openjdk
   ```

#### Windows (10 / 11)

1. **Install Visual Studio 2022** (or Visual Studio 18 / 2026):
   - Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
   - Run the installer and select **"Desktop development with C++"** workload
   - Ensure **"C++ CMake tools for Windows"** is checked
   - For Visual Studio 2022, install **v17**; for the latest, install **v18 (2026)**

2. **Install LLVM** (full dev package, not just the Visual Studio bundled Clang):
   ```powershell
   winget install LLVM -v 19.1.7
   ```
   Or download from: [LLVM 19.1.7 Windows installer](https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-win64.exe)

   After install, verify:
   ```powershell
   Test-Path "$env:ProgramFiles\LLVM\lib\cmake\llvm\LLVMConfig.cmake"
   ```
   Should output `True`.

3. **Install Java** (Temurin 21 recommended):
   ```powershell
   winget install EclipseAdoptium.Temurin.21.JDK
   ```
   Or download from [adoptium.net](https://adoptium.net/).

4. **Install vcpkg** (dependency manager for ANTLR4, GoogleTest, LLVM on Windows):

   If you have Visual Studio 18 (2025+), vcpkg is bundled at:
   ```text
   C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg
   ```
   Otherwise, install it manually:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
   C:\dev\vcpkg\bootstrap-vcpkg.bat
   ```

   Add vcpkg to your `PATH`:
   ```powershell
   [System.Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";C:\dev\vcpkg", "User")
   ```

5. **Install vcpkg dependencies** (from the repo root):
   ```powershell
   cd C:\Projects\hooc
   vcpkg install --triplet x64-windows
   ```
   This installs `antlr4`, `gtest`, and `llvm[target-x86]` into `vcpkg_installed/x64-windows/`.

   > **Important**: The repository `.gitignore` excludes `vcpkg_installed/`, so each developer must run `vcpkg install` themselves.

6. **Verify**:
   ```powershell
   cmake --version
   cl.exe                  # Should show MSVC compiler info
   java -version
   Test-Path "C:\Program Files\LLVM\lib\cmake\llvm\LLVMConfig.cmake"
   ```

---

## 2. Understanding Build Types

Hooc supports three main CMake build types:

| Type | Optimizations | Debug Symbols | Use Case |
|------|:---:|:---:|----------|
| `Debug` | None (`-O0`) | Full | Development, stepping through code |
| `RelWithDebInfo` | Moderate (`-O2`) | Full | Daily development, testing |
| `Release` | Full (`-O3`) | None | Production / packaging |

For development, use `RelWithDebInfo` (default in presets) for speed, or `Debug` if you need single-stepping without optimizations skipping lines.

---

## 3. Quick Start: Building with CMake Presets

The project includes `CMakePresets.json` with pre-configured builds for all platforms. **This is the recommended way to build.**

### Step 1: List Available Presets
```bash
cmake --list-presets           # Configure presets
cmake --list-presets=build     # Build presets
cmake --list-presets=test      # Test presets
```

### Step 2: Choose Your Preset

| Preset | Platform / Purpose |
|--------|--------------------|
| `macos-homebrew-ninja` | macOS with LLVM from Homebrew |
| `ubuntu-ninja` | Ubuntu with Clang + Ninja |
| `windows-vs-relwithdebinfo` | Windows with Visual Studio 2022 |
| `windows-vs18-relwithdebinfo` | Windows with Visual Studio 18 / 2026 |
| `windows-vs18-local` | Windows VS 18 using repo-local deps from `vcpkg_installed/` |
| `windows-ninja` | Windows with Clang + Ninja |
| `ninja-relwithdebinfo` | Generic Ninja build (set LLVM/ANTLR4 paths manually) |
| `ninja-release-no-tests` | Generic Ninja release build without tests |

### Step 3: Configure and Build

**macOS:**
```bash
cmake --preset macos-homebrew-ninja
cmake --build --preset macos-homebrew-ninja
```
The binary will be at `build/macos-homebrew-ninja/hoo`.

**Ubuntu (Linux):**
```bash
cmake --preset ubuntu-ninja \
  -DANTLR4_INCLUDE_DIR=/tmp/antlr4-runtime/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/tmp/antlr4-runtime/lib/libantlr4-runtime.a
cmake --build --preset ubuntu-ninja
```
The binary will be at `build/ubuntu-ninja/hoo`.

**Windows (Visual Studio 18 / 2026 with local deps):**
```powershell
cmake --preset windows-vs18-local
cmake --build --preset windows-vs18-local
```
The binary will be at `build/windows-vs18-local/RelWithDebInfo/hoo.exe`.

**Windows (Visual Studio 2022):**
```powershell
cmake --preset windows-vs-relwithdebinfo `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build --preset windows-vs-relwithdebinfo --config RelWithDebInfo
```

**Windows (Clang + Ninja):**
```powershell
cmake --preset windows-ninja `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DLLVM_DIR="$env:ProgramFiles\LLVM\lib\cmake\llvm"
cmake --build --preset windows-ninja
```

---

## 4. Build Targets

The project is organized into these primary targets:

### Primary Executables & Libraries
- **`hoo`**: The main compiler executable (`src/core/main.cpp`)
- **`hoo-core`**: Core compiler + HVM + JIT library (merges former `hvm` and `hoo-compiler`)
- **`hoo-parser`**: ANTLR-generated parser library
- **`hoort`**: The Hoo Runtime library (ARC, Strings, Maps, JSON, Math, etc.) — static by default

### Utility Targets
- **`hoo-tests`**: Unit test executable (built when `HOOC_BUILD_TESTS=ON`)
- **`generate_parser`**: Regenerates C++ parser sources from `src/parsing/Hooc.g4` using the ANTLR jar
- **`clean_generated`**: Deletes and recreates the generated parser directory

### Dependency Graph
```text
hoo
└── hoo-core
    ├── hoo-parser
    │   └── generate_parser
    │       └── download_antlr4 (conditional)
    ├── hoort
    └── LLVM (Core, OrcJIT, Target, etc.)
```

To build a specific target:
```bash
cmake --build build/<preset> --target hoo
cmake --build build/<preset> --target hoort
cmake --build build/<preset> --target generate_parser  # Regenerate ANTLR sources
```

---

## 5. Manual Configuration (Advanced)

If CMake presets don't work for your setup (e.g., older CMake, custom paths), configure manually:

### Basic Manual Build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHOOC_BUILD_TESTS=ON
cmake --build build
```

With `make` instead of Ninja:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHOOC_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

### Overriding Dependency Paths
If CMake cannot find LLVM or ANTLR4, pass paths explicitly:
```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DANTLR4_INCLUDE_DIR=/path/to/antlr4/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/path/to/libantlr4-runtime.a \
  -DCMAKE_BUILD_TYPE=Debug \
  -DHOOC_BUILD_TESTS=ON
```

### Building Without Tests (Release)
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DHOOC_BUILD_TESTS=OFF
cmake --build build
```

---

## 6. Testing

Tests require `HOOC_BUILD_TESTS=ON` (enabled by default in most presets).

### Step 1: Build the Test Executable
```bash
# Using a build preset for tests (e.g., macos)
cmake --build --preset macos-homebrew-ninja-tests

# Or just build everything including tests
cmake --build build/macos-homebrew-ninja --target hoo-tests
```

### Step 2: Run Tests via CTest
```bash
ctest --preset macos-homebrew-ninja --output-on-failure
```

On Windows:
```powershell
ctest --preset windows-vs18-local --output-on-failure
```

### Direct Test Execution
Run the test binary directly (faster than ctest):
```bash
./build/macos-homebrew-ninja/hoo-tests --gtest_brief=1
```

### Running Individual Test Suites
```bash
./build/macos-homebrew-ninja/hoo-tests --gtest_filter="*NewLanguageFeatures*" --gtest_brief=1
./build/macos-homebrew-ninja/hoo-tests --gtest_filter="*JIT*" --gtest_brief=1
./build/macos-homebrew-ninja/hoo-tests --gtest_filter="*Parser*" --gtest_brief=1
```

### Getting Verbose Output
```bash
./build/macos-homebrew-ninja/hoo-tests --gtest_print_time=1
```

---

## 7. Working with the ANTLR Parser

The parser sources are generated from `src/parsing/Hooc.g4` using the ANTLR jar at `tools/antlr-4.13.2-complete.jar`.

- **On first build**: The `generate_parser` target runs automatically
- **If you modify `Hooc.g4`**: Re-run the generator:
  ```bash
  cmake --build build/<preset> --target generate_parser
  ```
- **To force regeneration**:
  ```bash
  cmake --build build/<preset> --target clean_generated
  cmake --build build/<preset> --target generate_parser
  ```

Generated files go to `<binary-dir>/generated/antlr4/`.

---

## 8. IDE Support

### VS Code

**Required Extensions:**
- **CMake Tools** (ms-vscode.cmake-tools) — detects `CMakePresets.json`
- **C/C++** (ms-vscode.cpptools) — IntelliSense and debugging

**Setup:**
1. Open the repo root in VS Code
2. CMake Tools will auto-detect presets
3. Select your preset from the status bar (bottom blue bar) or via Command Palette (`Cmd+Shift+P` → "CMake: Select Configure Preset")
4. Build with `Cmd+Shift+P` → "CMake: Build" or the status bar build button

### Visual Studio (Windows)
1. Open the repo root as a folder
2. Visual Studio will detect `CMakePresets.json`
3. Select the preset from the project configuration dropdown
4. Build with `Ctrl+Shift+B`

### CLion
1. Open the repo root
2. CLion auto-detects CMake and presets
3. Select the preset in **Settings → Build, Execution, Deployment → CMake**

---

## 9. Generated Files and Cache

- **Build output**: `build/<preset-name>/` (e.g., `build/macos-homebrew-ninja/`)
- **Generated parser**: `<binary-dir>/generated/antlr4/`
- **CMake cache**: `build/<preset-name>/CMakeCache.txt`

If something goes wrong, delete the build directory and reconfigure:
```bash
rm -rf build/macos-homebrew-ninja
cmake --preset macos-homebrew-ninja
```

---

## 10. Troubleshooting

### "Could NOT find LLVM" / "LLVMConfig.cmake missing"
- **macOS**: Run `brew list llvm` to verify LLVM is installed. If it is, LLVM is keg-only; set `LLVM_DIR` manually or use the `macos-homebrew-ninja` preset.
- **Linux**: Install `llvm-dev` (not just `llvm`). Verify `llvm-config --cmakedir` prints a valid path.
- **Windows**: Visual Studio's bundled Clang does NOT include LLVM dev headers. Install the full LLVM package from `winget` or the LLVM releases page. Verify `C:\Program Files\LLVM\lib\cmake\llvm\LLVMConfig.cmake` exists.

### "Could NOT find ANTLR4"
- **macOS**: `brew install antlr4-cpp-runtime`
- **Linux**: Build from source (see [Ubuntu section above](#ubuntu-linux)). The system `libantlr4-runtime-dev` is often too old.
- **Windows**: Run `vcpkg install --triplet x64-windows` from the repo root.

### "Java not found" / "ANTLR jar failed"
- Install Java 17+ (Temurin recommended). Verify with `java -version`.
- The jar should exist at `tools/antlr-4.13.2-complete.jar`. If missing, run:
  ```bash
  cmake --build build/<preset> --target download_antlr4
  ```

### "hoo: command not found" after build
The binary is in the build directory, not in your PATH. Run it with its full path:
```bash
./build/macos-homebrew-ninja/hoo
```
Or add it to your PATH temporarily:
```bash
export PATH="$PWD/build/macos-homebrew-ninja:$PATH"
hoo --help
```

### Missing DLLs on Windows (at runtime)
Ensure LLVM and ANTLR4 DLLs are in your PATH:
```powershell
$env:PATH = "C:\Program Files\LLVM\bin;$env:PATH"
$env:PATH = "$PWD\vcpkg_installed\x64-windows\bin;$env:PATH"
hoo.exe tests\examples\hello.hoo
```

### Test crashes / "unknown instruction"
Ensure you have the right LLVM version (15+) and that the LLVM target backend for your architecture is enabled (X86 on x86_64, AArch64 on Apple Silicon).

### "IO Error: Cannot open file" when running hoo
The input `.hoo` file path must be relative to your current working directory, or absolute. The compiler does not auto-resolve paths like a linker.
```bash
hoo /full/path/to/file.hoo      # OK
hoo tests/examples/hello.hoo    # OK (relative from repo root)
hoo hello.hoo                   # Only if hello.hoo is in the current directory
```

### Linker errors about duplicate symbols
Clean the build directory and reconfigure:
```bash
rm -rf build/<preset>
cmake --preset <preset>
```

### Breakpoints not hitting in the debugger
You are likely using `RelWithDebInfo` (which has optimizations). Switch to `Debug` build type for full single-stepping:
```bash
cmake --preset macos-homebrew-ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset macos-homebrew-ninja
```
