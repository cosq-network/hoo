# Developing and Debugging Hoo

This guide covers setting up a development environment, building for debugging, and debugging the Hoo compiler across all supported platforms.

---

## 1. Development Environment Setup

### 1.1. macOS (Apple Silicon / Intel)

**Dependency Installation:**
```bash
# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install all dependencies
brew install cmake ninja llvm antlr4-cpp-runtime googletest
```

**PATH Configuration** (Homebrew LLVM is keg-only):
```bash
# Add to ~/.zshrc or ~/.bashrc:
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```
Reload: `source ~/.zshrc`

**Verify:**
```bash
clang++ --version
lldb --version
llvm-config --version   # Should be 15+
```

### 1.2. Linux (Ubuntu / Debian)

**Dependency Installation:**
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build clang lldb gdb \
  llvm-dev libgtest-dev uuid-dev wget unzip

# Build ANTLR4 C++ runtime from source (system package is often outdated)
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

### 1.3. Linux (RedHat / Fedora / CentOS)
```bash
sudo dnf groupinstall -y "Development Tools" "C Development Tools and Libraries"
sudo dnf install -y cmake ninja-build clang lldb gdb llvm-devel \
  antlr4-cpp-runtime-devel gtest-devel java-latest-openjdk
```

### 1.4. Dev Container & GitHub Codespaces

A single `Dockerfile` (multi-stage, downloads pre-built LLVM 22.1.4 binary, builds ANTLR4 4.13.2 C++ runtime) serves both local Docker and Codespaces. The `.devcontainer/devcontainer.json` is auto-detected in both environments.

```bash
# Local: Docker Desktop + VS Code Dev Containers extension → "Reopen in Container"
# Codespaces: GitHub repo → Code → Codespaces → Create codespace on main
# All dependencies (LLVM 22.1.4, ANTLR4 4.13.2, cmake, ninja) are pre-installed
```

Verify:
```bash
clang++ --version          # LLVM 22.1.4 clang
lldb --version             # LLDB 22.1.4
llvm-config --version      # 22.1.4
java -version              # JDK 21+
```

### 1.5. Windows (10 / 11)

**Install Visual Studio** (2022 or 18/2026):
- Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
- Select **"Desktop development with C++"** workload
- Ensure **"C++ CMake tools for Windows"** is checked

**Install full LLVM** (not just bundled Clang):
```powershell
winget install LLVM -v 19.1.7
```
Verify: `Test-Path "$env:ProgramFiles\LLVM\lib\cmake\llvm\LLVMConfig.cmake"` should be `True`.

**Install Java:**
```powershell
winget install EclipseAdoptium.Temurin.21.JDK
```

**Install vcpkg and dependencies:**
```powershell
# If not bundled with VS, clone it
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat

# Add to PATH
[System.Environment]::SetEnvironmentVariable("PATH", "$env:PATH;C:\dev\vcpkg", "User")

# Install dependencies
cd C:\Projects\hoo
vcpkg install --triplet x64-windows
```

---

## 2. Configuring and Building for Debugging

### 2.1. Build Targets

| Target | Description |
|--------|-------------|
| `hoo` | The main compiler executable |
| `hoo-core` | Core compiler + HVM + JIT library |
| `hoo-parser` | ANTLR-generated parser library |
| `hoort` | Hoo Runtime library (ARC, Strings, Maps, etc.) |
| `hoo-tests` | Unit test executable |

### 2.2. Debug Build with Presets

```bash
# macOS — override build type to Debug
cmake --preset macos-homebrew-ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset macos-homebrew-ninja

# Ubuntu / Linux (native)
cmake --preset ninja-relwithdebinfo -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DANTLR4_INCLUDE_DIR=/tmp/antlr4-runtime/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/tmp/antlr4-runtime/lib/libantlr4-runtime.a
cmake --build --preset ninja-relwithdebinfo

# Dev Container / Codespaces (use ninja-relwithdebinfo and override LLVM/ANTLR4 paths)
cmake --preset ninja-relwithdebinfo -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset ninja-relwithdebinfo

# Windows (Visual Studio)
cmake --preset windows-vs18-env -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset windows-vs18-env
```

### 2.3. Debug Build (Manual)

```bash
cmake -S . -B build/debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DHOO_BUILD_TESTS=ON
cmake --build build/debug
```

### 2.4. AddressSanitizer Build (for memory error detection)

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
  -DHOO_BUILD_TESTS=ON
cmake --build build-asan
```

When running with ASan, set `ASAN_SYMBOLIZER_PATH` for readable stack traces:
```bash
export ASAN_SYMBOLIZER_PATH=/opt/homebrew/opt/llvm/bin/llvm-symbolizer
./build-asan/hoo-tests --gtest_filter="*JIT*"
```

---

## 3. Debugging with VS Code

VS Code is the recommended IDE. The `.vscode/` directory is gitignored, so create these files locally — they won't be committed.

### 3.1. Required Extensions

- **C/C++** (ms-vscode.cpptools) — IntelliSense and MSVC/gdb debugging
- **CMake Tools** (ms-vscode.cmake-tools) — build integration
- **CodeLLDB** (vadimcn.vscode-lldb) — recommended for macOS/Linux (better LLDB support)

### 3.2. Create `.vscode/launch.json`

Create the directory and file at `<repo-root>/.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug hoo (LLDB) — macOS/Linux/Container",
            "type": "lldb",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["${workspaceFolder}/tests/examples/hello.hoo"],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "CMake: build",
            "environment": [
                { "name": "LLVM_SYMBOLIZER_PATH", "value": "/opt/homebrew/opt/llvm/bin/llvm-symbolizer" }
            ]
        },
        {
            "name": "Debug hoo (LLDB) — Dev Container / Codespaces",
            "type": "lldb",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["${workspaceFolder}/tests/examples/hello.hoo"],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "CMake: build",
            "environment": [
                { "name": "LLVM_SYMBOLIZER_PATH", "value": "/opt/llvm/bin/llvm-symbolizer" }
            ]
        },
        {
            "name": "Debug hoo (GDB) — Linux",
            "type": "cppdbg",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["${workspaceFolder}/tests/examples/hello.hoo"],
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        },
        {
            "name": "Debug hoo (MSVC) — Windows",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["${workspaceFolder}/tests/examples/hello.hoo"],
            "cwd": "${workspaceFolder}"
        },
        {
            "name": "Debug hoo-tests (LLDB) — macOS/Linux/Container",
            "type": "lldb",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["--gtest_filter=*JIT*", "--gtest_brief=1"],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "CMake: build tests"
        }
    ]
}
```

> **Note**: In the Dev Container / Codespaces, LLDB is already installed at `/opt/llvm/bin/lldb` and the `lldb` debug type works natively with the C/C++ extension. The LLVM symbolizer path should point to `/opt/llvm/bin/llvm-symbolizer` (not `/opt/homebrew/opt/llvm/...`).

### 3.3. Create `.vscode/settings.json`

```json
{
    "cmake.configureOnOpen": true,
    "cmake.preset": "macos-homebrew-ninja",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

For Dev Container / Codespaces, use `"cmake.preset": "ninja-relwithdebinfo"` instead.

### 3.4. Create `.vscode/tasks.json`

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "cmake",
            "label": "CMake: build",
            "command": "build",
            "group": "build"
        },
        {
            "type": "cmake",
            "label": "CMake: build tests",
            "command": "build",
            "args": ["--target", "hoo-tests"],
            "group": "build"
        }
    ]
}
```

### 3.5. Debugging Steps

1. Open the repo root in VS Code
2. Select your CMake preset from the status bar or via `Cmd+Shift+P` → "CMake: Select Configure Preset"
3. Set breakpoints (click in the gutter next to line numbers)
4. Press `F5` to start debugging with the active configuration
5. Use `F10` to step over, `F11` to step into, `Shift+F11` to step out

---

## 4. Visual Studio (Windows) Debugging

### 4.1. Native Visual Studio Debugging

1. Open the repo root as a folder in Visual Studio
2. Visual Studio auto-detects `CMakePresets.json`
3. Select the preset (e.g., `windows-vs18-env`) from the configuration dropdown
4. Set `hoo.exe` or `hoo-tests.exe` as the startup item
5. Set breakpoints and press `F5`

### 4.2. Launch Profiles

Visual Studio uses `launch.vs.json` for CMake projects. Create a `.vs/launch.vs.json` in the repo root:

```json
{
    "version": "0.2.1",
    "configurations": [
        {
            "type": "default",
            "project": "CMakeLists.txt",
            "projectTarget": "hoo.exe",
            "name": "Debug hoo.exe",
            "args": ["${workspaceFolder}\\tests\\examples\\hello.hoo"]
        },
        {
            "type": "default",
            "project": "CMakeLists.txt",
            "projectTarget": "hoo-tests.exe",
            "name": "Debug hoo-tests.exe",
            "args": ["--gtest_filter=*JIT*"]
        }
    ]
}
```

---

## 5. Command-Line Debugging

### 5.1. LLDB (macOS/Linux)

**Basic usage:**
```bash
lldb -- ./build/debug/hoo tests/examples/hello.hoo
(lldb) break set -n "hoo::HooCompiler::compile"
(lldb) run
(lldb) frame variable      # View locals
(lldb) thread step-over    # Next line (n)
(lldb) thread step-in      # Step into (s)
(lldb) thread step-out     # Finish function (f)
(lldb) continue            # Resume execution (c)
```

**Common LLDB commands:**
```bash
(lldb) break set -f HVMJIT.cpp -l 42     # Break at file:line
(lldb) break set -r ".*compile.*"         # Regex breakpoint
(lldb) expr myVar                          # Evaluate expression
(lldb) image lookup -vn "hoo::compile"    # Find symbol address
(lldb) bt                                  # Backtrace
(lldb) frame select 3                      # Switch to frame 3
```

### 5.2. GDB (Linux)

```bash
gdb --args ./build/debug/hoo tests/examples/hello.hoo
(gdb) break hoo::HooCompiler::compile
(gdb) run
(gdb) info locals
(gdb) step
```

### 5.3. Windows Debugging (command line)

Use Visual Studio's debugger (`devenv.exe`) or WinDbg:
```powershell
# Launch with VS debugger attached
devenv /debugexe build\Debug\hoo.exe tests\examples\hello.hoo

# Or use the MSVC command-line debugger
cd build\Debug
windbg hoo.exe ..\..\..\tests\examples\hello.hoo
```

---

## 6. Debugging the JIT

Hoo uses LLVM ORC JIT to compile HVM bytecode to native machine code at runtime. Debugging JIT-compiled code requires special setup.

### 6.1. Enable Debug Symbols in JIT Code

To get source-level debugging of JIT-compiled code, LLVM must emit DWARF debug info. This requires building with `-DDEBUG_JIT`:

```bash
cmake --preset macos-homebrew-ninja -DCMAKE_CXX_FLAGS="-DDEBUG_JIT"
cmake --build --preset macos-homebrew-ninja
```

### 6.2. Dump HVM Bytecode

To inspect what HVM instructions are being generated before JIT compilation, use the `--dump-hvm` flag:

```bash
./build/debug/hoo --dump-hvm tests/examples/hello.hoo
```

### 6.3. LLVM ORC JIT Diagnostics

Enable LLVM's internal JIT logging:

```bash
export LLVM_JIT_LOG_LEVEL=verbose
./build/debug/hoo tests/examples/hello.hoo
```

### 6.4. Debugging JIT Symbol Resolution

If you get "unknown symbol" errors from the JIT, the issue is typically that a runtime function hasn't been registered with the ORC engine. You can inspect registered symbols by adding a breakpoint in `HVMJIT.cpp` where symbols are looked up:

```
(lldb) break set -f HVMJIT.cpp -l 200
(lldb) run
(lldb) expr symbolName    # View the current symbol being resolved
```

### 6.5. Getting LLVM Stack Traces

When the JIT encounters an error, it may produce partial stack traces. To get full symbolicated traces, set the LLVM symbolizer path:

```bash
export LLVM_SYMBOLIZER_PATH=/opt/homebrew/opt/llvm/bin/llvm-symbolizer
./build/debug/hoo input.hoo
```

---

## 7. Running and Debugging Tests

### 7.1. Building Tests

```bash
# Build just the test executable (faster than full build)
cmake --build build/debug --target hoo-tests

# Using a build preset (macOS)
cmake --build --preset macos-homebrew-ninja-tests

# Dev Container / Codespaces
cmake --build --preset ninja-relwithdebinfo-tests
```

### 7.2. Running Specific Test Suites

```bash
# Run all tests with brief output
./build/debug/hoo-tests --gtest_brief=1

# Run a specific test suite
./build/debug/hoo-tests --gtest_filter="*NewLanguageFeatures*"

# Run a specific test
./build/debug/hoo-tests --gtest_filter="NewLanguageFeaturesTest.NewExpressionChainedMethodCalls"

# Run tests matching multiple patterns
./build/debug/hoo-tests --gtest_filter="*JIT*:*Parser*"

# List all test names without running them
./build/debug/hoo-tests --gtest_list_tests
```

### 7.3. Debugging a Specific Test

```bash
# Launch the test binary under LLDB with a specific filter
lldb -- ./build/debug/hoo-tests --gtest_filter="*NewLanguageFeatures*"
(lldb) break set -n "hoo::HVMCodeGenerator::visitFunctionDefinition"
(lldb) run
```

### 7.4. Running Tests with AddressSanitizer

```bash
# Build with ASan (see section 2.4)
export ASAN_SYMBOLIZER_PATH=/opt/homebrew/opt/llvm/bin/llvm-symbolizer
./build-asan/hoo-tests --gtest_filter="*HVM*" --gtest_brief=1
```

### 7.5. Running Tests via CTest

```bash
# macOS
ctest --preset macos-homebrew-ninja --output-on-failure

# Dev Container / Codespaces
ctest --preset ninja-relwithdebinfo --output-on-failure

# Run tests in parallel (4 jobs)
ctest --preset macos-homebrew-ninja -j4

# Run a specific test by name (regex)
ctest --preset macos-homebrew-ninja -R "JIT" --output-on-failure
```

---

## 8. Understanding the HVM JIT Pipeline

The Hoo compiler works in stages:

```
.hoo source file
      │
      ▼
┌─────────────────┐
│   Parser         │   ANTLR4-based → Parse Tree
│  (ANTLR4)        │
└─────────────────┘
      │
      ▼
┌─────────────────┐
│  AST Builder     │   SimpleASTBuilder → AST
└─────────────────┘
      │
      ▼
┌─────────────────┐
│ HVM CodeGen      │   HVMCodeGenerator → HVM instructions (bytecode)
└─────────────────┘
      │
      ▼
┌─────────────────┐
│  HVM Assembler   │   HVM → binary module
└─────────────────┘
      │
      ▼
┌─────────────────┐
│  HVM JIT         │   LLVM ORC JIT → native machine code
│  (LLVM ORC)      │
└─────────────────┘
      │
      ▼
   Execution
```

Key classes to set breakpoints on:
- `hoo::HooCompiler::compile()` — Entry point
- `hoo::HVMCodeGenerator::visitFunctionDefinition` — Function codegen
- `hoo::HVMJIT::compileModule` — JIT compilation
- `hoo::HVMJIT::lookup` — Symbol resolution

---

## 9. Troubleshooting Development Issues

### Breakpoints not hitting
Use `CMAKE_BUILD_TYPE=Debug`. `RelWithDebInfo` applies `-O2`, which inlines functions and reorders instructions. Switch to Debug:
```bash
cmake --preset macos-homebrew-ninja -DCMAKE_BUILD_TYPE=Debug
```

### "No source file named ..." in LLDB
Ensure you built with debug symbols (Debug or RelWithDebInfo). Run:
```bash
lldb -o "image list" -- ./build/debug/hoo 2>&1 | grep -i debug
```

### Missing ANTLR4 generated headers
The ANTLR parser sources must be generated before compilation. Run:
```bash
cmake --build build/debug --target generate_parser
```

### Test crashes with "unexpected instruction"
The JIT test suite includes expectations for HVM bytecode expansion. If you change the code generator, update the corresponding test expectations in `tests/hvm/` or `tests/jit/`. Run the specific failing test verbosely:
```bash
./build/debug/hoo-tests --gtest_filter="*NameOfFailingTest*" --gtest_print_time=1
```

### "LLVM ORC error: Symbol not found" at runtime
A runtime function (e.g., `hoo_string_new`) is not registered in the JIT symbol table. Check:
- `src/hvm/HVMJIT.cpp` — the `StaticHOModule` symbol registration section
- The function name must match the mangled symbol the code generator emits

### Windows DLLs not found at runtime
```powershell
$env:PATH = "C:\Program Files\LLVM\bin;$PWD\vcpkg_installed\x64-windows\bin;$env:PATH"
hoo.exe tests\examples\hello.hoo
```

### macOS: "Library not loaded: @rpath/libunwind.1.dylib"
Set `DYLD_LIBRARY_PATH`:
```bash
export DYLD_LIBRARY_PATH=/opt/homebrew/opt/llvm/lib:$DYLD_LIBRARY_PATH
./build/debug/hoo input.hoo
```

### Build failed: "fatal error: 'antlr4-runtime.h' file not found"
The ANTLR4 include path is not configured. On macOS, the `macos-homebrew-ninja` preset handles this automatically. On Linux, pass the path manually:
```bash
cmake --preset ninja-relwithdebinfo -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm -DANTLR4_INCLUDE_DIR=/tmp/antlr4-runtime/include/antlr4-runtime
```

### Dev Container / Codespaces first build is slow
- The `Dockerfile` builds LLVM from source — **30–60 min on first run**. Subsequent rebuilds use Docker layer caching.
- The first container build downloads pre-built LLVM binaries and builds ANTLR4 C++ runtime (~3 min total). Subsequent rebuilds use Docker layer caching.
- The ANTLR4 C++ runtime is always built from source (~1–2 min) in both configurations.

### "lldb: command not found" in Dev Container
LLDB is installed at `/opt/llvm/bin/lldb`, which is on the `PATH` via the container's `ENV` directive. Verify:
```bash
which lldb    # Should print /opt/llvm/bin/lldb
lldb --version
```
