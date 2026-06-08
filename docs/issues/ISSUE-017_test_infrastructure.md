# ISSUE-017: Test Infrastructure Issues

## 1. Overview
The CMake build configuration and test suite have several bugs and gaps that silently exclude tests, cause build failures, and produce weak coverage.

## 2. Issues

### 2.1 Duplicate `HooNetTest.cpp` entry (linker error)
- **Location**: `CMakeLists.txt` lines 372 and 385
- **Issue**: `tests/runtime/HooNetTest.cpp` is listed twice in the `hoo-tests` executable sources. This will cause duplicate-symbol linker errors.

### 2.2 `tests/jit/HooCompilerTest.cpp` excluded from build
- **Location**: `CMakeLists.txt` lines 339-422 (no entry for this file)
- **Issue**: The file exists on disk with 19 tests but is not included in CMakeLists.txt. These tests are silently excluded from all builds.

### 2.3 Duplicate test name `NewExpressionWithConstructorArgs`
- **Location**: `tests/jit/NewLanguageFeaturesTest.cpp` lines 262 and 264
- **Issue**: Two tests register with the same name — one `DISABLED_` stub (line 262, empty body) and one real implementation (line 264). If the `DISABLED_` prefix is removed, GTest will report a duplicate registration error.

### 2.4 Weak assertions across many JIT tests
- **Files**: `HooClassApiTest`, `HooDatetimeJitTest`, `HooHashingJitTest`, `HooJsonJitTest`, `HooStandardLibraryJitTest`, `HooPathJitTest`, `HooCompressionJitTest`
- **Issue**: Many tests use only `EXPECT_GT(r, 0)` or `EXPECT_NE(r, 0)` as their primary assertion. Bugs that return incorrect but non-zero values are not caught.

### 2.5 Debug `printf()` statements in test output
- **Files**: `tests/jit/NewLanguageFeaturesTest.cpp` (lines 237, 258), `tests/jit/HooStringJitTest.cpp` (line 35)
- **Issue**: Leftover debug `printf()` calls clutter stdout and interfere with structured test output parsers.

### 2.6 No runtime unit test for `hoo_json`
- **Issue**: `hoo_json` is the only runtime module without a corresponding unit test in `tests/runtime/`. It is only tested through JIT integration tests.

### 2.7 Hardcoded `/tmp/` paths
- **Files**: `HooHashingJitTest`, `HooCsvJitTest`, `HooCsvTest`
- **Issue**: Temp file paths use hardcoded `/tmp/` with `mkstemp()`. This is fragile on macOS (where `/tmp` is a symlink to `/private/tmp`) and incompatible with Windows.

### 2.8 `Double`/`int64_t` type-punning via memcpy
- **Files**: `HooArrayJitTest`, `HooMathJitTest`, `FloatingLiteralTest`
- **Issue**: Tests return `double` from functions declared to return `int64_t`, then use `memcpy` to reinterpret bits. This violates strict aliasing rules and is architecture-dependent.

## 3. Impact
- Build failures from duplicate source entries.
- 19 tests silently excluded from CI.
- Weak assertion coverage masks real bugs.
- Non-portable test code prevents Windows support.

## 4. Suggested Fixes
1. Remove duplicate `HooNetTest.cpp` from line 385 of CMakeLists.txt.
2. Add `tests/jit/HooCompilerTest.cpp` to CMakeLists.txt with a unique test suite name.
3. Delete the empty `DISABLED_NewExpressionWithConstructorArgs` stub.
4. Strengthen assertions to verify specific expected values where possible.
5. Replace `printf()` with `GTEST_MESSAGE_()` macros or remove.
6. Add a `tests/runtime/HooJsonTest.cpp` unit test.
7. Replace hardcoded `/tmp/` paths with `std::filesystem::temp_directory_path()`.
8. Use `std::bit_cast<double>()` (C++20) or a `union` for double reinterpretation.

## 5. Status
- **Date**: 2026-06-08
- **Status**: **TODO (UNIMPLEMENTED)**
- **Priority**: **HIGH**
