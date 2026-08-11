# ISSUE-027: Reorganize Executable Integration Tests Under `tests/integration/`

**Status**: COMPLETED / RESOLVED

## 1. Overview

This issue defines a plan to reorganize the **executable-level integration
tests** — the GoogleTest cases that spawn the built `hoo` binary as a
subprocess and assert on its exit code and stdout/stderr — into a dedicated
top-level directory, `tests/integration/`, with typed subdirectories for each
coverage area.

The intent is to make the test tree self-describing:

- unit tests (parser, AST, codegen, runtime internals, JIT host APIs) live in
  per-domain directories under `tests/`;
- executable integration tests live under `tests/integration/` and are grouped
  by CLI concern rather than dumped into `tests/core/` next to the `HooCLITest`
  unit-level fake-IO tests.

This is a **pure reorganization issue**: no test behavior changes, no
production-code changes, and no new coverage is required. The only non-move
edit is a CMake source-list update plus the extraction of a shared subprocess
harness so future integration tests do not copy/paste `popen`/`system` glue.

## 2. Current State

As of 2026-08-10:

- Exactly **one** test file spawns the `hoo` executable:
  `tests/core/HooCLIIntegrationTest.cpp` (283 lines, 21 `TEST_F` cases).
  It is identified by its `#ifndef HOO_EXECUTABLE ... #error` guard and its use
  of `popen`/`system` (tests/core/HooCLIIntegrationTest.cpp:23-94).
- It is wired into the single `hoo-tests` executable via
  `CMakeLists.txt:608` (`tests/core/HooCLIIntegrationTest.cpp`).
- The executable path is injected at compile time with
  `HOO_EXECUTABLE="$<TARGET_FILE:hoo>"` (`CMakeLists.txt:672`); the
  `hoo-tests` target depends on the `hoo` target (`CMakeLists.txt:636`).
- The suite runs under a single CTest entry, `HooUnitTests`
  (`CMakeLists.txt:704`), alongside all unit tests.
- `tests/core/HooCLITest.cpp` is a **unit-level** test of `HooCLI` using
  `FakeIOProvider` and must **stay** in `tests/core/`.
- `tests/repl/*` test `REPLSession` in-process and do not spawn the binary;
  they also stay where they are.
- `docs/building-windows.md:324` documents Windows-specific workarounds for
  `tests/core/HooCLIIntegrationTest.cpp`; that reference must be updated.

### 2.1 Concerns covered by the current file

The single file mixes five distinct CLI concerns:

| Group | Tests |
| --- | --- |
| Flag/usage handling | `HelpFlag`, `ShortHelpFlag`, `VersionFlag`, `ShortVersionFlag`, `UnknownOption`, `NoInputFile`, `FileNotFound`, `InvalidExtension`, `MultipleInputFilesError`, `VerboseFlag` |
| `--exec` source run | `CompileAndRunSource`, `RunSourceFile`, `RunFailsOnSyntaxError`, `PassesArgumentsAfterDoubleDashToProgram` |
| Archive build | `CompileAndOutputArchive`, `CompileAndOutputArchiveWithEqualsSyntax`, `RejectsOptionAsOutputPath` |
| Archive run | `GeneratedArchiveRunnable`, `RunArchiveFile` |
| Cross-file imports | `CrossFileLocalImports` |

## 3. Proposed Layout

```text
tests/
├── test_main.cpp                      (unchanged, stays at root)
├── core/                              (unit tests; HooCLIIntegrationTest.cpp removed)
├── archive/
├── ast/
├── codegen/
├── examples/
├── hvm/
├── jit/
├── parsing/
├── repl/
├── runtime/
└── integration/                       (NEW: executable-level integration tests)
    ├── CMakeLists.txt                 (NEW: adds hoo-integration-tests target; optional)
    ├── support/
    │   ├── HooExecHarness.h           (NEW: extracted runHoo()/ExecResult/temp-file helpers)
    │   └── HooExecHarness.cpp         (NEW, optional; static-inline header alternative)
    ├── cli/
    │   └── HooCLIFlagsTest.cpp        (from tests/core/HooCLIIntegrationTest.cpp)
    ├── exec/
    │   └── HooExecSourceTest.cpp      (from tests/core/HooCLIIntegrationTest.cpp)
    ├── archive/
    │   ├── HooArchiveBuildTest.cpp    (from tests/core/HooCLIIntegrationTest.cpp)
    │   └── HooArchiveRunTest.cpp      (from tests/core/HooCLIIntegrationTest.cpp)
    └── imports/
        └── HooLocalImportTest.cpp     (from tests/core/HooCLIIntegrationTest.cpp)
```

### 3.1 Subdirectory purpose

- `tests/integration/support/` — the shared subprocess harness used by every
  integration test. Extract the `HooExecHarness`/`ExecResult`/
  `createTempFile` machinery from the current test fixture into a header-only
  utility (mirroring the project's use of the Windows compat header for
  `_WIN32` guards) so new integration tests never copy/paste process glue.
- `tests/integration/cli/` — pure argument-parsing and error-output behavior
  that does not require a working compilation (help, version, unknown options,
  missing/invalid input, multiple inputs, `--verbose`).
- `tests/integration/exec/` — `--exec` compile-and-run of `.hoo` source and
  `--` argument forwarding.
- `tests/integration/archive/` — building `.ha` archives (`-o`/`--output=`)
  and running previously built archives.
- `tests/integration/imports/` — cross-file local imports resolved by the CLI
  build planner.

Future executable-integration suites (e.g. spawning `hoo --repl` and driving
it over stdin, or a full `hoo compile`→`hoo run` pipeline) get their own
subdirectory under `tests/integration/`.

### 3.2 Naming convention

- Files: `Hoo<Domain>Test.cpp` matching the existing suite naming style.
- Fixtures: one `TEST_F` fixture class per subdirectory (or per file) that
  inherits from the shared harness and sets `hooExe` from `HOO_EXECUTABLE`.
- A CTest label `integration` should be added so
  `ctest -L integration` runs only these tests (see §5).

## 4. Build Integration

Two wiring options are proposed; **Option A is the recommendation**.

### 4.1 Option A — separate `hoo-integration-tests` executable (recommended)

Rationale: integration tests spawn the real binary and have different
link/run properties (no LLVM init needed in `test_main`, `popen`/`system`
usage, Windows `cmd.exe` output capture) than the in-process unit suite.
Keeping them in the same binary as the unit tests is what forced
`HooCLIIntegrationTest.cpp` to live with unrelated unit tests.

```cmake
# in tests/integration/CMakeLists.txt, added via add_subdirectory(tests/integration)
add_executable(hoo-integration-tests
    test_main_integration.cpp        # minimal gtest main, no LLVM init
    cli/HooCLIFlagsTest.cpp
    exec/HooExecSourceTest.cpp
    archive/HooArchiveBuildTest.cpp
    archive/HooArchiveRunTest.cpp
    imports/HooLocalImportTest.cpp
)

target_link_libraries(hoo-integration-tests GTest::gtest GTest::gtest_main)
add_dependencies(hoo-integration-tests hoo)
target_compile_definitions(hoo-integration-tests PRIVATE
    HOO_EXECUTABLE="$<TARGET_FILE:hoo>"
)

add_test(NAME HooIntegrationTests COMMAND hoo-integration-tests)
set_tests_properties(HooIntegrationTests PROPERTIES LABELS integration)
```

### 4.2 Option B — keep a single `hoo-tests` binary

Keep the current single-executable model and only relocate the source files:

- Remove `tests/core/HooCLIIntegrationTest.cpp` from `CMakeLists.txt:608`.
- Add the new per-subdirectory files to the `hoo-tests` source list.
- Keep `HOO_EXECUTABLE` definition and `add_dependencies(hoo-tests hoo)`.
- Optionally tag `HooUnitTests` with a label so both can be filtered.

Trade-off: simpler CMake diff, but the mixed-binary coupling that motivated
this issue remains. Choose **Option A** unless the CI matrix makes a second
GTest executable unattractive.

### 4.3 Windows notes

Preserve the existing Windows behavior for the moved tests: `NOMINMAX`
before `<windows.h>`, `_popen`/`_unlink`/`_stat` shims, `Z:\` path for the
file-not-found case, and `cmd.exe /S /C` with redirected capture files. These
must move into `support/HooExecHarness.h` so they are defined once. Update
`docs/building-windows.md:324` to point at `tests/integration/`.

## 5. Migration Steps

1. Create `tests/integration/{support,cli,exec,archive,imports}/`.
2. Extract the fixture machinery (`ExecResult`, `runHoo`, `createTempFile`,
   Windows guards) from `HooCLIIntegrationTest.cpp` into
   `support/HooExecHarness.h` (header-only, inline functions, guarded by
   `#ifndef _WIN32`).
3. Split the 21 `TEST_F` cases across the five subdirectory files per the
   table in §2.1. Do not alter test names, assertions, exit-code checks, or
   expected strings.
4. Create `tests/integration/test_main_integration.cpp` (Option A) with a
   minimal `InitGoogleTest` + `RUN_ALL_TESTS` main (no LLVM target init; the
   integration suite does not use HVMJIT).
5. Wire the new target in CMake (Option A) or extend the existing source list
   (Option B); remove the old `tests/core/HooCLIIntegrationTest.cpp` entry.
6. Update `docs/building-windows.md` and the README test-count line if it
   enumerates the integration suite.
7. Verify:
   - `cmake --build <build> --target hoo-integration-tests` builds;
   - `ctest -L integration` runs only the moved tests and all pass;
   - the full unit suite still passes with the same total test count as
     before the move (README.md:132 currently claims 2117 tests in the
     preset run).

## 6. Acceptance Criteria

- No production source under `src/` is modified.
- No test case is renamed, dropped, or weakened; total GTest case count is
  identical before and after.
- `tests/core/HooCLIIntegrationTest.cpp` no longer exists; all executable
  integration tests live under `tests/integration/` in typed subdirectories.
- The shared subprocess harness lives in `tests/integration/support/` and is
  the only place that touches `popen`/`system`/`HOO_EXECUTABLE`.
- `ctest -L integration` and the full test suite both pass on macOS/Linux and
  Windows (per the existing Windows compat documentation).

## 7. Out of Scope

- Adding new integration coverage (e.g. spawning `hoo --repl`).
- Changing the `hoo` CLI, archive format, or build planner.
- Moving `HooCLITest.cpp` (unit-level, fake-IO) or any other unit suite.
- Splitting the unit-test binary itself.
