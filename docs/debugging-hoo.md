# Developing and Debugging Hooc

This guide provides a comprehensive walkthrough for setting up your development environment, configuring toolchains, and debugging the Hooc compiler on various platforms.

---

## 1. Development Environment Setup

### 1.1. macOS (Apple Silicon / Intel)
macOS users should primarily use **Homebrew** for dependency management.

1.  **Install Homebrew**: [brew.sh](https://brew.sh/)
2.  **Install Toolchain**:
    ```bash
    brew install cmake ninja llvm antlr4-cpp-runtime googletest
    ```
3.  **Path Configuration**: Homebrew LLVM is "keg-only". You may need to add it to your PATH for command-line use:
    ```bash
    export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
    ```

### 1.2. Linux (Ubuntu / Debian)
1.  **Update APT**: `sudo apt update`
2.  **Install Toolchain**:
    ```bash
    sudo apt install build-essential cmake ninja-build clang lldb gdb \
                     llvm-dev libantlr4-runtime-dev libgtest-dev java-common
    ```
3.  **LLVM Version**: If the default `llvm-dev` is too old (needs 15+), use the [LLVM Debian/Ubuntu nightly packages](https://apt.llvm.org/).

### 1.3. Linux (RedHat / Fedora / CentOS)
1.  **Install Toolchain via DNF**:
    ```bash
    sudo dnf groupinstall "Development Tools" "C Development Tools and Libraries"
    sudo dnf install cmake ninja-build clang lldb gdb llvm-devel \
                     antlr4-cpp-runtime-devel gtest-devel java-latest-openjdk
    ```

### 1.4. Windows (10 / 11)
1.  **Visual Studio 2022**: Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/).
    *   Select the **"Desktop development with C++"** workload.
    *   Ensure "C++ CMake tools for Windows" is checked.
2.  **LLVM for Windows**: Download the pre-built binaries from the [LLVM Releases page](https://github.com/llvm/llvm-project/releases).
3.  **Dependency Manager (vcpkg)**: Recommended for managing libraries like GoogleTest.
    ```powershell
    git clone https://github.com/microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
    ```

---

## 2. Configuring and Building

Hooc uses CMake Presets (CMake 3.20+) to simplify configuration.

### 2.1. Configuration Targets
*   **`hoo`**: The main compiler executable.
*   **`hoo-core`**: The primary logic library.
*   **`hoort`**: The Hoo Runtime library (required for execution).
*   **`hoo-tests`**: The unit test suite.

### 2.2. Building for Debugging
To enable full debug symbols and disable optimizations, use the `Debug` build type:

```bash
# Using Presets (e.g., macos)
cmake --preset macos-homebrew-ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset macos-homebrew-ninja

# Manual Configuration
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHOOC_BUILD_TESTS=ON
cmake --build build/debug --target hoo
```

---

## 3. Debugging with VS Code

VS Code is the recommended IDE for Hooc development.

### 3.1. Required Extensions
- **C/C++** (Microsoft): Provides IntelliSense and debugging.
- **CMake Tools** (Microsoft): Handles build configuration.
- **CodeLLDB** (Vadim Chugunov): Highly recommended for macOS and Linux users for a better LLDB experience.

### 3.2. launch.json Configurations

Create or update `.vscode/launch.json` in your workspace root:

#### macOS / Linux (using CodeLLDB)
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug hoo (LLDB)",
            "type": "lldb",
            "request": "launch",
            "program": "${command:cmake.launchTargetPath}",
            "args": ["${workspaceFolder}/tests/integration/hello.hoo"],
            "cwd": "${workspaceFolder}",
            "preLaunchTask": "CMake: build",
            "environment": [
                { "name": "LLVM_SYMBOLIZER_PATH", "value": "/opt/homebrew/opt/llvm/bin/llvm-symbolizer" }
            ]
        }
    ]
}
```

#### Linux (using GDB)
```json
{
    "name": "Debug hoo (GDB)",
    "type": "cppdbg",
    "request": "launch",
    "program": "${command:cmake.launchTargetPath}",
    "args": ["${workspaceFolder}/tests/integration/hello.hoo"],
    "cwd": "${workspaceFolder}",
    "MIMode": "gdb",
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        }
    ]
}
```

#### Windows (Visual Studio Debugger)
```json
{
    "name": "Debug hoo (MSVC)",
    "type": "cppvsdbg",
    "request": "launch",
    "program": "${command:cmake.launchTargetPath}",
    "args": ["${workspaceFolder}/tests/integration/hello.hoo"],
    "cwd": "${workspaceFolder}"
}
```

---

## 4. Command-Line Debugging

### 4.1. LLDB (macOS/Linux)
```bash
lldb -- ./build/debug/hoo path/to/file.hoo
(lldb) b hooc::HooCompiler::compile
(lldb) run
(lldb) v  # View local variables
(lldb) n  # Next line
```

### 4.2. GDB (Linux)
```bash
gdb --args ./build/debug/hoo path/to/file.hoo
(gdb) break hooc::HooCompiler::compile
(gdb) run
(gdb) info locals
(gdb) step
```

---

## 5. Debugging the JIT

Since Hooc uses LLVM ORC JIT, debugging the *generated* code can be tricky.
- **Debug Symbols in JIT**: The `HVMJIT` class handles symbol resolution. To debug the JIT-ed code itself, you may need to enable LLVM's JIT event listeners for debuggers.
- **Bytecode Inspection**: Use the `--dump-hvm` flag (if available) to see the HVM instructions being passed to the JIT.

---

## 6. Troubleshooting

- **Breakpoints not hitting**: Ensure you are using `CMAKE_BUILD_TYPE=Debug`. Some presets default to `RelWithDebInfo` which applies optimizations that can "skip" lines in the debugger.
- **Missing Includes**: Run `cmake --build build --target generate_parser` to ensure ANTLR4 headers are generated.
- **Library Not Found**: On Linux, you might need to update your library cache: `sudo ldconfig`. On Windows, ensure DLLs for LLVM and ANTLR4 are in the same folder as `hoo.exe` or in your `PATH`.
