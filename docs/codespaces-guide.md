# Developing Hoo with GitHub Codespaces

This guide walks you through setting up and using GitHub Codespaces to develop the Hoo compiler ecosystem entirely in your browser — no local toolchain required.

---

## 1. Quick Start

### 1.1. Create a Codespace

1. Navigate to the [Hoo repository](https://github.com/<org>/hooc) on GitHub
2. Click the green **Code** button → **Codespaces** tab → **Create codespace on main**
3. In the **"Dev container configuration"** dropdown, select **`codespace`** (`.devcontainer/codespace/devcontainer.json`)
4. Click **Create codespace**

The Codespace will build the container image using `Dockerfile.codespaces`:
- Downloads pre-built LLVM 22.1.4 binaries (~30 s)
- Builds ANTLR4 4.13.2 C++ runtime from source (~1–2 min)
- Installs all development tools (`cmake`, `ninja`, `clang`, `lldb`, `jdk`, etc.)

After creation finishes, you will have a full VS Code environment in your browser with a `zsh` terminal.

### 1.2. Verify the Environment

```bash
clang++ --version         # LLVM 22.1.4
lldb --version            # LLDB 22.1.4
llvm-config --version     # 22.1.4
cmake --version           # 3.28+
ninja --version           # 1.11+
java -version             # JDK 21+
echo $LLVM_DIR            # /opt/llvm/lib/cmake/llvm
echo $ANTLR4_ROOT         # /opt/antlr
```

### 1.3. Build the Project

```bash
# Configure
cmake --preset codespace-ninja

# Build the compiler
cmake --build --preset codespace-ninja

# Run the compiler on a test file
./build/codespace-ninja/hoo tests/examples/hello.hoo
```

### 1.4. Run Tests

```bash
# Build the test executable
cmake --build --preset codespace-ninja-tests

# Run via CTest
ctest --preset codespace-ninja --output-on-failure

# Or run the binary directly
./build/codespace-ninja/hoo-tests --gtest_brief=1

# Run a specific test suite
./build/codespace-ninja/hoo-tests --gtest_filter="*JIT*" --gtest_brief=1
```

---

## 2. Environment Details

### 2.1. Tool Locations

| Tool | Path |
|------|------|
| C/C++ Compiler | `/opt/llvm/bin/clang`, `/opt/llvm/bin/clang++` |
| Debugger | `/opt/llvm/bin/lldb` |
| LLVM CMake config | `/opt/llvm/lib/cmake/llvm/LLVMConfig.cmake` |
| ANTLR4 JAR | `/opt/antlr/antlr-4.13.2-complete.jar` |
| ANTLR4 C++ headers | `/opt/antlr/include/antlr4-runtime/` |
| ANTLR4 C++ library | `/opt/antlr/lib/libantlr4-runtime.a` |
| Build directory | `build/codespace-ninja/` |

### 2.2. Preset Configuration

The `codespace-ninja` CMake preset (defined in `CMakePresets.json`) sets:

```json
{
  "name": "codespace-ninja",
  "inherits": "ninja-relwithdebinfo",
  "cacheVariables": {
    "CMAKE_C_COMPILER": "clang",
    "CMAKE_CXX_COMPILER": "clang++",
    "CMAKE_PREFIX_PATH": "/opt/llvm;/opt/antlr",
    "LLVM_DIR": "/opt/llvm/lib/cmake/llvm"
  }
}
```

Inherits from `ninja-relwithdebinfo`: Ninja generator, `RelWithDebInfo` build type, tests enabled.

### 2.3. Pre-installed Packages

The container includes: `build-essential`, `cmake`, `ninja-build`, `default-jdk`, `libgtest-dev`, `libcurl4-openssl-dev`, `uuid-dev`, `git`, `zsh`, `python3`, `libedit-dev`, `libncurses-dev`, `libxml2-dev`, `libzstd-dev`.

---

## 3. Development Workflow

### 3.1. Build for Debugging

```bash
cmake --preset codespace-ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset codespace-ninja
```

### 3.2. Rebuild After Changing the Grammar

If you modify `src/parsing/Hooc.g4`, regenerate the ANTLR parser:

```bash
cmake --build build/codespace-ninja --target generate_parser
```

### 3.3. Clean and Full Rebuild

```bash
rm -rf build/codespace-ninja
cmake --preset codespace-ninja
cmake --build --preset codespace-ninja
```

### 3.4. Install Additional Packages

The Codespace runs as root. Install Ubuntu packages with `apt`:

```bash
apt update && apt install -y <package>
```

### 3.5. Using Git in Codespaces

The Codespace authenticates as you automatically via GitHub's OAuth. Standard git workflow:

```bash
git checkout -b my-feature
# ... make changes ...
git add .
git commit -m "feat: my feature"
git push -u origin my-feature
```

Then open a PR from the GitHub web UI.

---

## 4. Debugging

### 4.1. Debug with LLDB

```bash
lldb -- ./build/codespace-ninja/hoo tests/examples/hello.hoo
(lldb) break set -n "hoo::HooCompiler::compile"
(lldb) run
(lldb) bt
```

### 4.2. Debug a Test

```bash
lldb -- ./build/codespace-ninja/hoo-tests --gtest_filter="*JIT*"
(lldb) break set -f HVMJIT.cpp -l 200
(lldb) run
```

### 4.3. VS Code GUI Debugging

The Dev Container configuration (`codespace` config) includes the **C/C++** and **CMake Tools** extensions. To debug with the VS Code GUI:

1. Open the **Run and Debug** view (Ctrl+Shift+D)
2. Select **"Debug hoo (LLDB) — Dev Container / Codespaces"** from the dropdown
3. Set breakpoints by clicking in the gutter
4. Press **F5** to start debugging

> **Note**: The LLVM symbolizer path is `/opt/llvm/bin/llvm-symbolizer` in the Codespace.

### 4.4. AddressSanitizer Build

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
  -DHOO_BUILD_TESTS=ON
cmake --build build-asan
export ASAN_SYMBOLIZER_PATH=/opt/llvm/bin/llvm-symbolizer
./build-asan/hoo-tests --gtest_filter="*JIT*"
```

---

## 5. Performance Considerations

| Factor | Notes |
|--------|-------|
| **Container build** | First creation takes ~3 min (downloads LLVM binary + builds ANTLR4). Subsequent uses reuse the cached image. |
| **CMake configure** | ~30 s on first run, ~5 s on re-configuration |
| **Full compiler build** | ~2–3 min (ninja with parallel jobs) |
| **Test build** | ~1 min (`hoo-tests` target only) |
| **Full test suite** | ~5–10 min via `ctest` |
| **Idle timeout** | Codespaces stop after 30 min of inactivity. Work is saved. |
| **Storage** | ~4 GB used by the container image + build artifacts |

### 5.1. Speeding Up Iteration

- Build only the target you need: `cmake --build --preset codespace-ninja-tests` (instead of `codespace-ninja`)
- Use `--gtest_filter` to run a single test suite instead of all tests
- Keep the Codespace running between sessions to avoid rebuilds

---

## 6. Troubleshooting

### "cmake --preset codespace-ninja fails with LLVM not found"

Verify the environment variables are set:
```bash
echo $LLVM_DIR          # Should be /opt/llvm/lib/cmake/llvm
ls /opt/llvm/lib/cmake/llvm/LLVMConfig.cmake   # Should exist
```

### "ANTLR4 runtime not found"

The ANTLR4 C++ runtime should be at `/opt/antlr`. Verify:
```bash
ls /opt/antlr/include/antlr4-runtime/antlr4-runtime.h
ls /opt/antlr/lib/libantlr4-runtime.a
```

If missing, rebuild the Codespace from scratch.

### "No space left on device"

Codespaces have ~32 GB of storage. Clean build artifacts:
```bash
rm -rf build/*
```

### "lldb: command not found"

LLDB is at `/opt/llvm/bin/lldb`, which is on the `PATH`. Run:
```bash
which lldb
```

### "Port already in use" / "Can't preview"

If you run a server inside the Codespace, VS Code can forward ports for you. Click the **Ports** tab in the terminal panel, or use the notification that appears.

### Codespace won't start / container build fails

1. Check the build logs: click **"Open logs"** in the failed notification
2. Common causes: network timeout downloading LLVM binary, out of disk space
3. Try creating a new Codespace with a different region (GitHub → Settings → Codespaces)

---

## 7. Customizing the Codespace

### 7.1. VS Code Extensions

The default extensions are defined in `.devcontainer/codespace/devcontainer.json`:

```json
"extensions": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "ms-vscode.vscode-typescript-next"
]
```

Add more extensions by editing that file.

### 7.2. VS Code Settings

To set Codespace-specific VS Code settings, add to the `settings` block in `.devcontainer/codespace/devcontainer.json`:

```json
"settings": {
    "cmake.preset": "codespace-ninja",
    "editor.fontSize": 14,
    "files.autoSave": "onFocusChange"
}
```

### 7.3. Dotfiles

GitHub Codespaces can apply personal dotfiles (`.zshrc`, `.gitconfig`, etc.) from a GitHub repository. Configure this at **GitHub.com → Settings → Codespaces**.
