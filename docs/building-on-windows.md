# Building hooc on Windows

This guide provides step-by-step instructions for building the **hooc** compiler (for the **hoo** programming language) on Windows.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Installation Steps](#installation-steps)
- [Building the Project](#building-the-project)
- [Running Tests](#running-tests)
- [VS Code Integration](#vs-code-integration)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

The following tools and libraries are required to build **hooc** on Windows:

### 1. Visual Studio 2022 (or 2019)
- **Download**: [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
- **Required Components**:
  - Desktop development with C++
  - C++ CMake tools for Windows
  - Windows 10/11 SDK
  - MSVC v143 (or v142) build tools

### 2. CMake (3.16 or later)
- **Download**: [CMake for Windows](https://cmake.org/download/)
- **Installation**: Use the installer and select "Add CMake to the system PATH"
- **Verify**: Open Command Prompt and run:
  ```cmd
  cmake --version
  ```

### 3. Ninja Build System
- **Download**: [Ninja releases](https://github.com/ninja-build/ninja/releases)
- **Installation**: 
  1. Download `ninja-win.zip`
  2. Extract `ninja.exe` to a folder (e.g., `C:\Tools\ninja`)
  3. Add the folder to your system PATH
- **Verify**:
  ```cmd
  ninja --version
  ```

### 4. vcpkg (Package Manager)
- **Installation**:
  ```cmd
  cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC"
  git clone https://github.com/Microsoft/vcpkg.git
  cd vcpkg
  .\bootstrap-vcpkg.bat
  ```
- **Note**: Adjust the path based on your Visual Studio installation location
- **Alternative location**: You can install vcpkg anywhere, just update the toolchain path accordingly

### 5. Java Runtime Environment (JRE)
- **Required for**: ANTLR4 parser generation
- **Download**: [Java SE Runtime](https://www.oracle.com/java/technologies/downloads/)
- **Minimum version**: Java 8 or later
- **Verify**:
  ```cmd
  java -version
  ```

### 6. LLVM
- **Download**: [LLVM Pre-built Binaries](https://releases.llvm.org/download.html)
- **Recommended version**: LLVM 15.0 or later
- **Installation**: 
  1. Download the Windows installer (e.g., `LLVM-15.0.0-win64.exe`)
  2. Run installer and select "Add LLVM to the system PATH"
- **Verify**:
  ```cmd
  llvm-config --version
  ```

### 7. ANTLR4 C++ Runtime
- **Installation via vcpkg** (Recommended):
  ```cmd
  cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
  .\vcpkg install antlr4-cpp-runtime:x64-windows
  ```
- **Alternative manual installation**: Download from [ANTLR4 repository](https://github.com/antlr/antlr4/tree/master/runtime/Cpp) and build from source

### 8. Google Test (GTest)
- **Installation via vcpkg**:
  ```cmd
  .\vcpkg install gtest:x64-windows
  ```
- This is already specified in `vcpkg.json` and will be installed automatically when using the vcpkg toolchain

---

## Installation Steps

### Step 1: Install Visual Studio 2022

1. Download and run the Visual Studio installer
2. Select "Desktop development with C++"
3. Ensure these individual components are selected:
   - MSVC v143 - VS 2022 C++ x64/x86 build tools
   - Windows 10 SDK (or Windows 11 SDK)
   - C++ CMake tools for Windows
   - Git for Windows (optional but recommended)

### Step 2: Install CMake and Ninja

1. Install CMake using the Windows installer
2. Download Ninja and add to PATH
3. Restart your terminal/Command Prompt

### Step 3: Set Up vcpkg

```cmd
cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC"
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

**Note**: The path should match your Visual Studio installation. Adjust if needed.

### Step 4: Install Dependencies via vcpkg

```cmd
cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
.\vcpkg install antlr4-cpp-runtime:x64-windows
.\vcpkg install gtest:x64-windows
```

### Step 5: Install LLVM

1. Download LLVM pre-built binaries for Windows
2. Run the installer
3. **Important**: Check "Add LLVM to the system PATH for all users"
4. Verify installation:
   ```cmd
   llvm-config --version
   ```

### Step 6: Install Java

1. Download and install Java Runtime Environment (JRE) or JDK
2. Verify:
   ```cmd
   java -version
   ```

---

## Building the Project

### Step 1: Clone the Repository

```cmd
git clone <repository-url>
cd hooc
```

### Step 2: Configure with CMake

Open **Developer Command Prompt for VS 2022** (important for proper environment setup):

```cmd
cd D:\Projects\hooc
mkdir build
cd build

cmake -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="D:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  ..
```

**Configuration Explanation**:
- `-G "Ninja"`: Use Ninja as the build system (faster than MSBuild)
- `-DCMAKE_TOOLCHAIN_FILE`: Path to vcpkg's CMake toolchain file
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`: Build with optimizations + debug symbols
- `..`: Path to the source directory (parent of build/)

**Alternative Build Types**:
- `Debug`: No optimizations, full debug info
- `Release`: Full optimizations, no debug info
- `RelWithDebInfo`: Optimizations + debug info (recommended)
- `MinSizeRel`: Optimize for size

### Step 3: Build the Project

```cmd
ninja
```

Or to see verbose output:
```cmd
ninja -v
```

### Step 4: Build Output

After successful build, you'll find:
- `hooc.exe`: Main compiler executable
- `hoo-compiler.lib`: Compiler library
- `hoo-parser.lib`: Parser library
- `hoo-tests.exe`: Unit test executable (if GTest is found)

---

## Running Tests

### Run All Tests

```cmd
cd D:\Projects\hooc\build
.\hoo-tests.exe
```

### Run Tests via CTest

```cmd
ctest --verbose
```

### Run Specific Test

```cmd
.\hoo-tests.exe --gtest_filter=CodeGeneratorTest.*
```

---

## VS Code Integration

### Step 1: Install Extensions

Install these VS Code extensions:
1. **CMake Tools** (ms-vscode.cmake-tools)
2. **C/C++** (ms-vscode.cpptools)
3. **CMake** (twxs.cmake)

### Step 2: Configure VS Code

Create `.vscode/settings.json`:

```json
{
    "cmake.generator": "Ninja",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureArgs": [
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
    ],
    
    // Windows-specific settings
    "[windows]": {
        "cmake.configureArgs": [
            "-DCMAKE_TOOLCHAIN_FILE=D:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake",
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
        ]
    }
}
```

**Adjust the vcpkg path** to match your installation.

### Step 3: Build from VS Code

1. Open Command Palette (Ctrl+Shift+P)
2. Run: **CMake: Configure**
3. Run: **CMake: Build**

Or use the CMake status bar:
- Click **Build** button in the status bar
- Or press **F7** (default build shortcut)

### Step 4: Run and Debug

1. Open Command Palette (Ctrl+Shift+P)
2. Run: **CMake: Debug**
3. Or click the debug icon in the status bar

---

## Troubleshooting

### Issue 1: CMake Can't Find LLVM

**Solution**: Make sure LLVM is in PATH or set `LLVM_DIR`:
```cmd
set LLVM_DIR=C:\Program Files\LLVM\lib\cmake\llvm
cmake -G "Ninja" ...
```

### Issue 2: ANTLR4 Runtime Not Found

**Solution**: Reinstall via vcpkg:
```cmd
cd "D:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg"
.\vcpkg remove antlr4-cpp-runtime:x64-windows
.\vcpkg install antlr4-cpp-runtime:x64-windows --recurse
```

### Issue 3: Java Not Found

**Solution**: Ensure Java is in PATH:
```cmd
java -version
```

If not found, add Java bin directory to PATH:
```
C:\Program Files\Java\jre1.8.0_XXX\bin
```

### Issue 4: Ninja Not Found

**Solution**: 
1. Verify Ninja is in PATH: `ninja --version`
2. If not, add Ninja location to system PATH
3. Restart Command Prompt

### Issue 5: vcpkg Toolchain File Not Found

**Solution**: Update the path in the CMake command to match your vcpkg installation:
```cmd
# Find vcpkg.cmake
dir /s "D:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"

# Use the correct path in cmake command
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="<correct-path>" ...
```

### Issue 6: Link Errors with LLVM

**Error**: Runtime library mismatch

**Solution**: The project is configured to use `/MD` (Release runtime) even in Debug builds to match LLVM. Use `RelWithDebInfo` instead of `Debug`:
```cmd
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo ...
```

### Issue 7: Parser Files Not Generated

**Solution**: Make sure ANTLR4 JAR exists:
```cmd
dir tools\antlr-4.13.2-complete.jar
```

If missing, download manually:
```cmd
mkdir tools
cd tools
curl -L -o antlr-4.13.2-complete.jar https://www.antlr.org/download/antlr-4.13.2-complete.jar
cd ..
```

### Issue 8: Permission Denied Errors

**Solution**: Run Developer Command Prompt as Administrator

---

## Quick Reference

### Rebuild from Scratch

```cmd
cd D:\Projects\hooc
rmdir /s /q build
mkdir build
cd build

cmake -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="D:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  ..

ninja
```

### Clean Build

```cmd
ninja clean
ninja
```

### Regenerate Parser Only

```cmd
ninja generate_parser
```

### Build and Test

```cmd
ninja && ctest --verbose
```

---

## Project Structure

```
hooc/
├── antlr4/
│   └── generated/        # Auto-generated ANTLR4 parser files
├── build/                # Build output directory
├── docs/                 # Documentation
├── src/                  # Source code
│   ├── main.cpp         # Main compiler entry point
│   ├── HooCompiler.cpp  # Compiler implementation
│   ├── CodeGenerator.cpp # LLVM code generation
│   ├── Hooc.g4          # ANTLR4 grammar
│   └── ast/             # Abstract Syntax Tree
├── tests/               # Unit tests
│   └── examples/        # Test .hoo programs
├── tools/               # Build tools (ANTLR4 JAR)
├── CMakeLists.txt       # CMake build configuration
└── vcpkg.json           # vcpkg dependencies
```

---

## Additional Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/en/docs/README.html)
- [LLVM Documentation](https://llvm.org/docs/)
- [ANTLR4 Documentation](https://github.com/antlr/antlr4/blob/master/doc/index.md)
- [Visual Studio Developer Tools](https://docs.microsoft.com/en-us/cpp/)

---

## Summary

With all dependencies installed, building **hooc** on Windows is straightforward:

```cmd
# One-time setup
cd D:\Projects\hooc
mkdir build && cd build

# Configure
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg.cmake>" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Build
ninja

# Test
.\hoo-tests.exe

# Run compiler
.\hooc.exe --help
```

For VS Code users, simply configure the settings.json and use the CMake Tools extension for a seamless development experience.
