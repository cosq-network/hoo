# Debugging Hooc

This document describes practical ways to debug the `hooc` compiler executable and related tests on macOS, Linux, and Windows.

## Build for Debugging

Use a build with debug symbols. The existing presets use `RelWithDebInfo`, which is usually enough for stepping through optimized-ish builds:

```bash
cmake --preset <preset>
cmake --build --preset <preset>
```

For a less optimized debug build, configure manually:

```bash
cmake -S . -B build/debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DHOOC_BUILD_TESTS=ON
cmake --build build/debug --target hooc
```

Useful executable paths:

```text
build/macos-homebrew-ninja/hooc
build/ubuntu-ninja/hooc
build/windows-ninja/hooc.exe
build/windows-vs-relwithdebinfo/RelWithDebInfo/hooc.exe
```

## What to Debug

Use these entry points for common debugging tasks:

| Area | Useful files |
|------|--------------|
| CLI flow | `src/main.cpp` |
| Source-to-LLVM pipeline | `src/HooCompiler.cpp` |
| Parser integration | `src/ProcessIsolatedParser.cpp` |
| AST building | `src/SimpleASTBuilder.cpp` |
| LLVM IR generation | `src/LLVMCodeGenerator.cpp` |
| JIT infrastructure | `src/HoocJIT.cpp` |
| Runtime memory | `src/rt/hoo_runtime.c` |
| Runtime strings/arrays | `src/rt/hoo_string.cpp`, `src/rt/hoo_generic_array.cpp` |

`hooc` currently compiles `.hoo` input to LLVM IR and prints the IR. JIT support is compiled into `hoo-compiler`, but command-line JIT execution is not fully wired into `src/main.cpp` yet.

## VS Code

Recommended extensions:

- CMake Tools
- C/C++
- CodeLLDB on macOS or Linux if using LLDB

Configure and build with CMake Tools:

```text
CMake: Select Configure Preset
CMake: Configure
CMake: Build
```

Then create `.vscode/launch.json` if needed.

### macOS or Linux with LLDB

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug hooc",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build/macos-homebrew-ninja/hooc",
      "args": ["${workspaceFolder}/path/to/file.hoo"],
      "cwd": "${workspaceFolder}",
      "stopOnEntry": false
    }
  ]
}
```

Change `program` to `build/ubuntu-ninja/hooc` on Linux.

### Linux with GDB

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug hooc with GDB",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/ubuntu-ninja/hooc",
      "args": ["${workspaceFolder}/path/to/file.hoo"],
      "cwd": "${workspaceFolder}",
      "MIMode": "gdb",
      "miDebuggerPath": "gdb",
      "stopAtEntry": false
    }
  ]
}
```

### Windows with Visual Studio Debugger

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug hooc on Windows",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}\\build\\windows-vs-relwithdebinfo\\RelWithDebInfo\\hooc.exe",
      "args": ["${workspaceFolder}\\path\\to\\file.hoo"],
      "cwd": "${workspaceFolder}",
      "stopAtEntry": false
    }
  ]
}
```

If runtime DLLs are required, add their directories to `PATH` in the debug environment.

## Eclipse CDT

Use Eclipse CDT with CMake support when available.

Recommended flow:

1. Configure and build with a preset from the terminal.
2. Import or open the repository in Eclipse.
3. Point Eclipse at the generated build directory, such as `build/ubuntu-ninja/`.
4. Create a C/C++ Application debug configuration.
5. Set the executable to `hooc`.
6. Set program arguments to a `.hoo` input file.

Example executable paths:

```text
build/ubuntu-ninja/hooc
build/macos-homebrew-ninja/hooc
build/windows-ninja/hooc.exe
```

On Linux, choose GDB as the debugger. On macOS, use LLDB if available in the Eclipse installation. On Windows, Eclipse CDT commonly uses GDB when configured with MinGW/Clang toolchains; for MSVC builds, Visual Studio is usually the better debugger.

## Command-Line Debuggers

### macOS: LLDB

LLDB is the default debugger on macOS.

```bash
lldb -- build/macos-homebrew-ninja/hooc path/to/file.hoo
```

Useful LLDB commands:

```text
breakpoint set --file src/main.cpp --line 45
breakpoint set --name hooc::HooCompiler::compile
run
bt
frame variable
next
step
continue
```

### Linux: GDB

GDB is the common default debugger on Linux.

```bash
gdb --args build/ubuntu-ninja/hooc path/to/file.hoo
```

Useful GDB commands:

```text
break src/main.cpp:45
break hooc::HooCompiler::compile
run
bt
info locals
next
step
continue
```

### Linux: LLDB

LLDB is also usable on Linux:

```bash
lldb -- build/ubuntu-ninja/hooc path/to/file.hoo
```

Use the same LLDB commands shown for macOS.

## Windows Debuggers

### Visual Studio

For the Visual Studio preset:

```powershell
cmake --preset windows-vs-relwithdebinfo
cmake --build --preset windows-vs-relwithdebinfo
```

Open the generated solution or open the folder in Visual Studio. Set `hooc` as the startup item and set the command argument to a `.hoo` file.

Executable:

```text
build\windows-vs-relwithdebinfo\RelWithDebInfo\hooc.exe
```

### WinDbg

WinDbg is useful for low-level crashes and postmortem debugging:

```powershell
windbg -- build\windows-vs-relwithdebinfo\RelWithDebInfo\hooc.exe path\to\file.hoo
```

Common commands:

```text
g
k
dv
bp hooc!main
```

### CDB

CDB is the command-line debugger from Windows Debugging Tools:

```powershell
cdb build\windows-vs-relwithdebinfo\RelWithDebInfo\hooc.exe path\to\file.hoo
```

## Debugging Tests

Build tests first:

```bash
cmake -S . -B build/debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DHOOC_BUILD_TESTS=ON
cmake --build build/debug --target hoo-tests
```

Run one test under GDB:

```bash
gdb --args build/debug/hoo-tests --gtest_filter=ArrayLiteralParsingTest.SimpleIntegerArrayLiteral
```

Run one test under LLDB:

```bash
lldb -- build/debug/hoo-tests --gtest_filter=ArrayLiteralParsingTest.SimpleIntegerArrayLiteral
```

In Visual Studio, set `hoo-tests` as the startup target and add:

```text
--gtest_filter=ArrayLiteralParsingTest.SimpleIntegerArrayLiteral
```

as the command argument.

## Useful Breakpoints

Start with these breakpoints:

```text
main
hooc::HooCompiler::compile
hooc::ProcessIsolatedParser::parseForAST
hooc::SimpleASTBuilder::buildAST
hooc::LLVMCodeGenerator::generateLLVMModule
hooc::HoocJIT::HoocJIT
hoo_alloc
hoo_retain
hoo_release
```

Use parser and AST breakpoints when debugging syntax or AST issues. Use `LLVMCodeGenerator` breakpoints when IR is missing or malformed. Use runtime breakpoints when debugging memory, string, or array behavior.

## Troubleshooting

If breakpoints are not hit:

- Verify you are debugging the executable from the same build directory you just built.
- Prefer `Debug` or `RelWithDebInfo` builds.
- Clean and rebuild if source paths look stale.

If symbols are missing:

- On Linux/macOS, verify the binary was not stripped.
- On Windows, verify `.pdb` files are beside the executable or discoverable by the debugger.

If shared libraries or DLLs fail to load:

- Add LLVM and ANTLR4 runtime library directories to `PATH` on Windows.
- Use `DYLD_LIBRARY_PATH` carefully on macOS if using nonstandard library locations.
- Use `LD_LIBRARY_PATH` on Linux if runtime libraries are outside standard search paths.
