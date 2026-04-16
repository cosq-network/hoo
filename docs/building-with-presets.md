# Building with CMake Presets

Use this guide when CMake 3.20 or newer is available.

## List Presets

```bash
cmake --list-presets
cmake --list-presets=build
cmake --list-presets=test
```

## Build the Compiler

Configure once, then build:

```bash
cmake --preset <preset>
cmake --build --preset <preset>
```

Common presets:

This repository defines a small set of reusable CMake presets for common platforms and build scenarios. Use the configure preset name directly with `cmake --preset` and the matching build preset name with `cmake --build --preset`.

| Preset | Platform / Purpose | Notes |
|--------|--------------------|-------|
| `macos-homebrew-ninja` | macOS with LLVM from Homebrew | Inherits `ninja-relwithdebinfo` and sets `CMAKE_PREFIX_PATH` for Homebrew LLVM |
| `ubuntu-ninja` | Ubuntu with Clang + Ninja | Inherits `ninja-relwithdebinfo` and sets `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` to Clang |
| `windows-vs-relwithdebinfo` | Windows with Visual Studio 2022 | Uses the VS generator and `RelWithDebInfo` configuration |
| `windows-ninja` | Windows with Clang + Ninja | Inherits `ninja-relwithdebinfo` and sets Clang compilers |
| `ninja-relwithdebinfo` | Generic Ninja build | Builds with `RelWithDebInfo` and tests enabled |
| `ninja-release-no-tests` | Generic Ninja release build without tests | Builds only the `hooc` target and disables `HOOC_BUILD_TESTS` |

Example:

```bash
cmake --preset macos-homebrew-ninja
cmake --build --preset macos-homebrew-ninja
```

The build presets above default to compiling the `hooc` target.

Build presets for running the test target are available too:

| Build Preset | Purpose |
|--------------|---------|
| `macos-homebrew-ninja-tests` | Build `hoo-tests` using `macos-homebrew-ninja` configuration |
| `ubuntu-ninja-tests` | Build `hoo-tests` using `ubuntu-ninja` configuration |
| `windows-vs-relwithdebinfo-tests` | Build `hoo-tests` using the VS configuration |
| `windows-ninja-tests` | Build `hoo-tests` using `windows-ninja` configuration |
| `ninja-relwithdebinfo-tests` | Build `hoo-tests` using generic `ninja-relwithdebinfo` configuration |

## Build Without Tests

Use the no-test preset:

```bash
cmake --preset ninja-release-no-tests
cmake --build --preset ninja-release-no-tests
```

Or override any configure preset:

```bash
cmake --preset macos-homebrew-ninja -DHOOC_BUILD_TESTS=OFF
cmake --build --preset macos-homebrew-ninja
```

## Run Tests

Tests require a preset configured with `HOOC_BUILD_TESTS=ON` and a discoverable GoogleTest installation.

```bash
cmake --preset <preset>
cmake --build --preset <preset>-tests
ctest --preset <preset>
```

Example:

```bash
cmake --preset ubuntu-ninja
cmake --build --preset ubuntu-ninja-tests
ctest --preset ubuntu-ninja
```

## Custom Dependency Paths

Pass dependency paths while configuring:

```bash
cmake --preset <preset> \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DANTLR4_ROOT=/path/to/antlr4
```

If needed, pass ANTLR4 paths separately:

```bash
cmake --preset <preset> \
  -DANTLR4_INCLUDE_DIR=/path/to/include/antlr4-runtime \
  -DANTLR4_LIBRARY=/path/to/libantlr4-runtime.a
```

On Windows, use the `.lib` file for `ANTLR4_LIBRARY`.

## Output Location

Preset build directories are created under:

```text
build/<preset-name>/
```

For example:

```text
build/macos-homebrew-ninja/hooc
build/windows-vs-relwithdebinfo/RelWithDebInfo/hooc.exe
```

## VS Code

Recommended extensions:

- CMake Tools
- C/C++

Open the repository root in VS Code. CMake Tools detects `CMakePresets.json`.

Use the command palette:

```text
CMake: Select Configure Preset
CMake: Configure
CMake: Build
```

Choose the preset that matches the platform, for example `macos-homebrew-ninja`, `ubuntu-ninja`, or `windows-vs-relwithdebinfo`.

To run tests:

```text
CMake: Run Tests
```

If dependency paths differ from the preset defaults, configure from the VS Code terminal with overrides:

```bash
cmake --preset <preset> \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DANTLR4_ROOT=/path/to/antlr4
cmake --build --preset <preset>
```

Then continue using CMake Tools with the same preset.

## Eclipse

Use Eclipse CDT with CMake support, or import the project as an existing CMake project if the installed Eclipse distribution supports it.

Preferred flow:

1. Configure the project from a terminal with a preset.
2. Open or import the repository in Eclipse.
3. Point Eclipse at the generated build directory under `build/<preset-name>/`.
4. Build the `hooc` target from Eclipse, or continue building from the terminal.

Example:

```bash
cmake --preset ubuntu-ninja
cmake --build --preset ubuntu-ninja
```

Use this build directory in Eclipse:

```text
build/ubuntu-ninja/
```

For Windows Visual Studio builds, use:

```powershell
cmake --preset windows-vs-relwithdebinfo
cmake --build --preset windows-vs-relwithdebinfo
```

Build directory:

```text
build/windows-vs-relwithdebinfo/
```

If Eclipse does not read CMake presets directly, keep CMake configure/build commands in the terminal and use Eclipse for editing, indexing, and launching the built executable.
