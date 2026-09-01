# Building Hoo on Windows 11

This guide walks through setting up a Windows 11 development environment for Hoo using **Visual Studio 18 (2026)**, **LLVM 22.1.4** downloaded directly from GitHub, and **vcpkg manifest mode** for third-party dependencies.

---

## 1. Prerequisites

| Dependency | Required | Purpose |
|------------|----------|---------|
| Visual Studio 18 Community | Yes | MSVC toolchain (`cl.exe`) |
| CMake 3.20+ | Yes | Build system (bundled with VS or standalone) |
| LLVM 22.1.4 | Yes | LLVM core, ORC JIT, Target libraries |
| Java 17+ | Yes | Runs the ANTLR generator JAR |
| Ninja (optional) | No | Faster builds (recommended) |
| vcpkg | Yes | Package manager for GoogleTest, ZLIB, OpenSSL, curl, and optional ANTLR4 installs |

---

## 2. Install Visual Studio 18 Community

1. Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
2. Run the installer and select the **"Desktop development with C++"** workload
3. Ensure these individual components are checked:
   - **"C++ CMake tools for Windows"** (includes CMake)
   - **"Windows 10/11 SDK"**
4. After installation, open **"x64 Native Tools Command Prompt for VS 18"** from the Start Menu (this sets the environment for 64-bit builds).

Verify the compiler:

```cmd
cl /?
```

---

## 3. Download and Set Up LLVM 22.1.4

Download the pre-built LLVM binary for Windows (MSVC):

```cmd
curl -L -o llvm.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.4/clang+llvm-22.1.4-x86_64-pc-windows-msvc.tar.xz
```

Extract it to a convenient location (e.g., `C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc`). You can use 7-Zip, WinRAR, or `tar` if you have it:

```cmd
tar -xf llvm.tar.xz
```

Add LLVM's `bin` directory to your PATH:

```cmd
set PATH=C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\bin;%PATH%
```

> To make this permanent, add it to your system environment variables via **System Properties → Advanced → Environment Variables**.

Verify LLVM is usable:

```cmd
clang-cl --version
dir C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm\LLVMConfig.cmake
```

> **Known issue**: The pre-built LLVM 22.1.4 Windows package was built on a VS 2022 machine and has a hardcoded DIA SDK path in `LLVMExports.cmake`. If you get an error about `diaguids.lib`, edit that file to remove the DIA SDK reference. See [troubleshooting](#thediaguidslib-is-missing).

---

## 4. Install Java

Download **Eclipse Temurin 21** (or any JDK 17+):

```cmd
winget install EclipseAdoptium.Temurin.21.JDK
```

Or download manually from [adoptium.net](https://adoptium.net/).

Verify:

```cmd
java -version
```

---

## 5. Set Environment Variables

Set these **system/user environment variables** once — the `windows-vs18-env` preset reads them automatically.

| Variable | Value | Purpose |
|----------|-------|---------|
| `LLVM_DIR` | `C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm` | Tells CMake where to find `LLVMConfig.cmake` |
| `VCPKG_ROOT` | `C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg` | Tells CMake where to find the vcpkg toolchain file |

```cmd
setx LLVM_DIR "C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm"
setx VCPKG_ROOT "C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
```

Add LLVM's `bin` to your PATH (for DLLs at runtime):
```cmd
setx PATH "%PATH%;C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\bin"
```

> Restart your terminal after running `setx` for the changes to take effect.

---

## 6. Install Dependencies with vcpkg (Manifest Mode)

The repo includes `vcpkg.json` at the root. vcpkg manifest mode installs GoogleTest, ZLIB, OpenSSL, curl, and ANTLR4 package metadata during CMake configuration.

The Windows preset normally builds the ANTLR4 C++ runtime from source via CMake `FetchContent` unless `ANTLR4_ROOT` points at an installed ANTLR4 runtime. GoogleTest is consumed from vcpkg through `GTest::gtest` and `GTest::gtest_main`, so the test headers and libraries use the same version.

Visual Studio 18 bundles vcpkg at `C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe`, so no separate installation is needed. The `VCPKG_ROOT` environment variable (set in [section 5](#5-set-environment-variables)) already points to it.

If you don't have VS 18's bundled vcpkg, clone and bootstrap it manually:
```cmd
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
cd C:\dev\vcpkg
.\bootstrap-vcpkg.bat
```
Then set `VCPKG_ROOT=C:\dev\vcpkg` (adjust the path in [section 5](#5-set-environment-variables) accordingly). When you configure Hoo with the `windows-vs18-env` preset, CMake automatically installs manifest dependencies. By default vcpkg uses the build tree's `build\vcpkg_installed\` directory; GitHub Actions overrides this to `${{ github.workspace }}\vcpkg_installed`.

---

## 7. Configure and Build

> **IMPORTANT**: You **must** use **"x64 Native Tools Command Prompt for VS 18"**. The x86 command prompt will fail because LLVM 22.1.4 provides only **x64** libraries and cannot link against an x86 build.

Open **"x64 Native Tools Command Prompt for VS 18"** (or any terminal, if you set the env vars globally) and navigate to the Hoo repo root:

```cmd
cd C:\Projects\hoo
```

Configure with CMake using the dedicated Windows preset:

```cmd
cmake --preset windows-vs18-env
```

> This preset reads `LLVM_DIR` and `VCPKG_ROOT` from your environment variables — no need for manual `-D` flags. See [section 5](#5-set-environment-variables).

> If Ninja is not installed and you want to use MSBuild instead:
> ```cmd
> cmake --preset windows-vs18-env -DCMAKE_GENERATOR="Visual Studio 18 2026" -DCMAKE_GENERATOR_PLATFORM=x64
> ```

Build:

```cmd
cmake --build --preset windows-vs18-env
```

The binary will be at `build\hoo.exe`.

---

## 8. Testing

Build the test executable:

```cmd
cmake --build --preset windows-vs18-env-tests
```

Run all tests via CTest:

```cmd
ctest --preset windows-vs18-env --output-on-failure
```

The CTest preset registers only Hoo's `HooUnitTests` executable. The generated test environment prepends the active vcpkg `bin` directory, so DLLs such as curl, OpenSSL, zlib, and GoogleTest are resolved without manually editing `PATH`.

Run a specific test suite directly:

```cmd
build\hoo-tests --gtest_filter="*NewLanguageFeatures*" --gtest_brief=1
```

> If you get DLL load errors at runtime, ensure LLVM's `bin` is on your PATH (see [section 5](#5-set-environment-variables)).

---

## 9. Summary of Directory Layout

```
C:\
├── clang+llvm-22.1.4-x86_64-pc-windows-msvc\  # Pre-built LLVM (extracted from GitHub release)
│   ├── bin\                 #   clang-cl.exe, llvm-config.exe, etc.
│   └── lib\cmake\llvm\      #   LLVMConfig.cmake
└── Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\
    └── scripts\buildsystems\vcpkg.cmake   # vcpkg toolchain (bundled with VS 18)
```

---

## 10. Troubleshooting

### "Could NOT find LLVM"
Verify the path to `LLVMConfig.cmake`:
```cmd
dir C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm\LLVMConfig.cmake
```

### "Could NOT find ANTLR4"
The Windows preset builds ANTLR4 from source automatically when `ANTLR4_ROOT` is not set. If you intentionally use a vcpkg or external ANTLR4 install, ensure the vcpkg toolchain file is set correctly and the headers/library exist:
```cmd
dir "%VCPKG_ROOT%\installed\x64-windows\include\antlr4-runtime\antlr4-runtime.h"
dir "%VCPKG_ROOT%\installed\x64-windows\lib\antlr4-runtime.lib"
```

### "fatal error C1083: Cannot open include file: 'zlib.h'"
vcpkg toolchain not being picked up. Ensure `-DCMAKE_TOOLCHAIN_FILE` points to the correct vcpkg path.

### Linker errors (LNK2019 / LNK2001)
Usually caused by mismatched CRT linkage or missing library dependencies.

Ensure vcpkg libraries were built with the same triplet (`x64-windows`, not `x64-windows-static`). The project uses the dynamic MSVC runtime (`/MD`, CMake `MultiThreadedDLL`) to match the default `x64-windows` vcpkg triplet.

If GoogleTest symbols such as `testing::internal::MakeAndRegisterTestInfo` fail to link, clean and reconfigure. That usually means old FetchContent GoogleTest build files or stale headers from a previous configuration are still in `build\`.

If errors reference `__imp_deflate`, `__imp_inflate`, etc., zlib is not linked to `hoort`. Run `cmake --preset windows-vs18-env --fresh` to regenerate — the `CMakeLists.txt` links `ZLIB::ZLIB` to `hoort`.

If errors reference `__imp_curl_*` or `curl.lib` not found, the bare `curl` library name does not resolve to `libcurl.lib` on Windows. The `CMakeLists.txt` uses `find_package(CURL REQUIRED)` with the `CURL::libcurl` target instead.

### LLVM library architecture mismatch (LNK4272)
```
LLVMOrcJIT.lib : warning LNK4272: library machine type 'x64' conflicts with target machine type 'x86'
```
You are building for **x86** but LLVM only provides **x64** libraries. You must use the **"x64 Native Tools Command Prompt for VS 18"** and clean the build directory:
```cmd
rmdir /S /Q build
cmake --preset windows-vs18-env
```

### "fatal error C1083: Cannot open include file: 'pthread.h'"
MSVC does not provide `<pthread.h>`. The codebase uses platform-specific implementations:
- `hoo_thread.cpp` (`src/runtime/lib/concurrency/`) — uses Win32 `CreateThread` / `CRITICAL_SECTION` on Windows, pthread on macOS/Linux
- `HVMJIT.cpp` — uses `std::thread` on Windows, pthread on macOS/Linux

Run a clean build from the **x64 Native Tools Command Prompt**.

### "fatal error C1083: Cannot open include file: 'unistd.h'"
MSVC does not provide `<unistd.h>`. The codebase guards it with `#ifdef _WIN32` and uses `<io.h>` / `_open`, `_read`, `_write` etc. instead.

### "fatal error C1083: Cannot open include file: 'stdint.h'"
The MSVC standard library include paths are not being found. This usually means you are building outside a VS developer command prompt, or you are using the **x86** command prompt but the build is misconfigured. Use **"x64 Native Tools Command Prompt for VS 18"** and clean the build directory.

### "error C2589: '(': illegal token on right side of '::'" in LLVM headers
The Windows `min`/`max` macros conflict with `std::min`/`std::max` used in LLVM headers. The codebase defines `NOMINMAX` before including `<windows.h>`. Run a clean rebuild:
```cmd
rmdir /S /Q build
cmake --preset windows-vs18-env
cmake --build --preset windows-vs18-env
```

### "error C2039: 'clock_gettime': is not a member of 'global namespace'"
MSVC does not provide `clock_gettime`. The codebase implements it with `GetSystemTimePreciseAsFileTime` on Windows.

### "error C2039: 'getrandom': is not a member of 'global namespace'"
MSVC does not provide `getrandom`. The codebase uses `CryptGenRandom` on Windows.

### "diaguids.lib is missing"
The pre-built LLVM 22.1.4 Windows package has a hardcoded VS 2022 DIA SDK path in `LLVMExports.cmake`. Edit line 537 to remove the DIA library reference:
```cmd
powershell -Command "(Get-Content 'C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm\LLVMExports.cmake') -replace 'C:/Program Files/Microsoft Visual Studio/2022/Enterprise/DIA SDK/lib/amd64/diaguids.lib;', '' | Set-Content 'C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\lib\cmake\llvm\LLVMExports.cmake'"
```
Then clean and reconfigure: `rmdir /S /Q build && cmake --preset windows-vs18-env`.

### "The code execution cannot proceed because LLVM-C.dll was not found"
LLVM's `bin` directory is not on your PATH. Add it:
```cmd
set PATH=C:\clang+llvm-22.1.4-x86_64-pc-windows-msvc\bin;%PATH%
```

CTest adds the vcpkg runtime DLL directory automatically, but it does not add LLVM's `bin` directory. Keep LLVM's `bin` on `PATH` in your terminal or GitHub workflow.

### Slow builds
Install Ninja for parallel builds:
```cmd
winget install Ninja-build.Ninja
```
The `windows-vs18-env` preset uses Ninja by default.

### Environment variables not picked up
After running `setx`, restart your terminal or open a new **x64 Native Tools Command Prompt for VS 18**. CMake presets read env vars at process startup, not dynamically.

### vcpkg installs x86 packages instead of x64
You are using the **x86** Developer Command Prompt. Use **"x64 Native Tools Command Prompt for VS 18"** instead, then clean and reconfigure:
```cmd
rmdir /S /Q build
cmake --preset windows-vs18-env
```

### hoo-tests build fails with "fatal error C1083: Cannot open include file: 'unistd.h'"
`unistd.h` is a POSIX-only header. The forced-include header `build/generated/hoo_windows_test_compat.h` provides Windows replacements for the POSIX functions used in tests (`write`, `close`, `unlink`, `mkstemp`). The test files guard the include with `#ifndef _WIN32`. Build with:
```cmd
cmake --build --preset windows-vs18-env-tests
```

### hoo-tests crashes at runtime (exit code 0xc0000374) [RESOLVED]
This was caused by inconsistent Windows test linkage and CTest environment setup. The current fix uses `/MD` consistently, consumes GoogleTest from vcpkg, disables ANTLR's own C++ test registration, and escapes the generated CTest `PATH` so Windows semicolon separators are preserved. Run a clean rebuild:
```cmd
rmdir /S /Q build
cmake --preset windows-vs18-env
cmake --build --preset windows-vs18-env
cmake --build --preset windows-vs18-env-tests
ctest --preset windows-vs18-env --output-on-failure
```

If `build\hoo-tests.exe --gtest_brief=1` passes but `ctest --preset windows-vs18-env` fails, re-run CMake to regenerate `build\CTestTestfile.cmake`. Stale generated CTest files may contain an old or malformed `PATH`.

### Test files with Windows-specific changes
The following test files have `#ifdef _WIN32` guards for platform-specific behavior:

| File | Changes |
|------|---------|
| `tests/integration/jit/HooProcessJitTest.cpp` | `Capture` uses `cmd.exe /c echo` on Windows. Changed `SpawnAndWait` to use `cmd.exe /c exit 0` to prevent hanging in interactive shell |
| `tests/integration/jit/BugFixVerificationTest.cpp` | Read array refcount once before releasing to fix infinite loop reading use-after-free garbage data on Windows |
| `tests/runtime/HooIOTest.cpp` | Redirected stdin to a `tmpfile()` to prevent Windows from indefinitely blocking on empty redirected IO |
| `tests/integration/jit/HooDecimalJitTest.cpp` | Disabled exception-catching tests on Windows because OrcJIT lacks SEH unwinding support |
| `tests/integration/fs/FsModuleIntegrationTest.cpp` | Conditionally bypassed Unix-style path assertions on Windows and fixed expected line endings for file writes |
| `tests/integration/datetime/DatetimeCLIIntegrationTest.cpp` | Conditionally bypassed pre-1970 UTC datetime assertions since MSVC `gmtime` returns `NULL` |
| `tests/integration/**/*IntegrationTest.cpp` (28 files) | Fixed massive CLI test failures by wrapping `popen` arguments in extra quotes to prevent `cmd.exe` outer quote stripping, and normalized `std::filesystem::temp_directory_path()` backslashes to forward slashes to prevent `\U` compiler escape sequence errors |
| `tests/integration/jit/HooCsvJitTest.cpp` | Guarded `#include <unistd.h>` with `#ifndef _WIN32`; compat header provides `write`/`close`/`unlink`/`mkstemp` shims |
| `tests/integration/jit/HooHashingJitTest.cpp` | Same as above |
| `tests/integration/cli/HooCLIIntegrationTest.cpp` | Added `NOMINMAX` before `<windows.h>`, uses `_stat` on Windows, uses `Z:\` nonexistent path for FileNotFound test, and captures CLI output through `cmd.exe /S /C` with redirected temp files instead of `_popen` |
| `tests/runtime/HooCsvTest.cpp` | `ReadWriteFile` uses `GetTempPathA` for temp directory on Windows |
| `tests/runtime/HooHashingTest.cpp` | `Sha256File` creates a temp file instead of `/dev/null`; `Sha256FileNotFound` uses `Z:\` path |
| `tests/runtime/HooSystemTest.cpp` | `UserHome` checks for drive letter prefix; `SetCurrentDir` uses drive root instead of `/tmp` |
| `tests/runtime/HooPathTest.cpp` | `Separator` expects `\` on Windows; `ListSeparator` expects `;`; `IsAbsolute` uses `C:\` prefix; `HasRoot` uses `C:\` prefix |
| `tests/integration/jit/HooPathJitTest.cpp` | `Separator` expects `\` on Windows; `ListSeparator` expects `;`; `IsAbsolute` uses `C:\` prefix |
| `tests/runtime/HooProcessTest.cpp` | `echo`   `cmd.exe /c echo`; `false`   `cmd.exe /c exit 1`; `sleep`   `cmd.exe /c timeout` |