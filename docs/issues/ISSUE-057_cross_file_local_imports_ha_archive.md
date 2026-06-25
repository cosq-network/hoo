# ISSUE-057: Cross-File Local Imports and Hoo Archive (`.ha`) Format

## Status
- **Date**: 2026-06-25
- **Status**: **PROPOSED**
- **Priority**: **HIGH**

---

## 1. Overview
The `hoo` executable currently compiles only the source file passed on the command line. If `a.hoo` imports a local source file such as `b.hoo`, the executable does not resolve, compile, link, or package `b.hoo`.

This issue proposes a cross-file local import pipeline and a new Hoo Archive format, `.ha`, similar in role to Java ecosystem `.jar` files:

```bash
hoo a.hoo -o app.ha
```

Expected future behavior:

1. Compile `a.hoo` to an internal `.ho` module payload.
2. Resolve local imports recursively, for example `import b;` -> `b.hoo`.
3. Compile each imported local `.hoo` file into its own internal `.ho` module payload.
4. Package all generated `.ho` module payloads into one `.ha` archive.
5. Store a minified JSON metadata index for fast loading, symbol lookup, and dependency resolution.

---

## 2. Motivation
Local multi-file programs should be first-class:

- Developers should be able to split a program across files and directories.
- `hoo a.hoo -o app.ha` should produce one runnable/distributable artifact.
- The loader should not need to scan every `.ho` payload to find symbols.
- Archive loading should be fast enough for CLI execution and future package/module workflows.

---

## 3. Proposed File Format: Hoo Archive (`.ha`)

### 3.1 Container Choice
Use a ZIP-compatible archive container, then compress the whole archive as a single Zstandard-compressed blob.

Recommended vcpkg dependencies:

| Dependency | License | Purpose |
|------------|---------|---------|
| `libzip` | BSD-3-Clause | ZIP container reading/writing/modifying |
| `zstd` | BSD-3-Clause | Whole-archive compression/decompression |
| `nlohmann-json` | MIT | JSON manifest/index generation and parsing |

Rationale:

- ZIP provides a mature central-directory structure and random access to entries.
- Zstandard balances compression ratio and decompression speed better than deflate for compiled artifacts.
- Compression is applied to the whole `.ha` archive stream, not to individual `modules/**/*.ho` entries.
- BSD/MIT licensing matches the requested permissive-license requirement.
- vcpkg availability keeps cross-platform dependency management consistent.

Fallback option:

- `libarchive` (BSD-2-Clause) can be evaluated if it provides a cleaner implementation path for whole-archive Zstd compression.

### 3.2 Extension
Use:

```text
.ha
```

Meaning: **Hoo Archive**.

### 3.3 Archive Layout
Recommended internal layout:

```text
META-INF/hoo/archive.json
META-INF/hoo/archive.sha256
modules/main.ho
modules/pkg/math_utils.ho
modules/pkg/internal/parser.ho
```

Rules:

- Every compiled `.ho` file is stored under `modules/`.
- `.ho` files are internal payloads only. Users should not need to generate, pass, or consume `.ho` files directly through the `hoo` executable.
- Individual module entries are stored uncompressed inside the archive container. Only the whole `.ha` archive is compressed.
- Metadata is stored under `META-INF/hoo/`.
- The primary index is `META-INF/hoo/archive.json`.
- `archive.json` is stored as minified UTF-8 JSON text. Do not apply an additional `.zst` compression layer to the manifest.
- A checksum file may store the SHA-256 of the minified manifest and/or module payloads.

### 3.4 Manifest / Index
Use minified JSON text initially. BSON can be added later if measurements show JSON parse time is significant.

`archive.json` shape:

```json
{
  "format": "hoo-archive",
  "formatVersion": 1,
  "createdBy": {
    "tool": "hoo",
    "version": "1.4.0"
  },
  "entryPoint": {
    "source": "a.hoo",
    "module": "a",
    "symbol": "_F_M_a_E_main_i8"
  },
  "modules": [
    {
      "module": "a",
      "sourcePath": "a.hoo",
      "archivePath": "modules/a.ho",
      "sha256": "...",
      "imports": ["b"],
      "symbols": [
        {
          "name": "main",
          "mangled": "_F_M_a_E_main_i8",
          "kind": "function",
          "returnType": "int64",
          "visibility": "public"
        }
      ]
    },
    {
      "module": "b",
      "sourcePath": "b.hoo",
      "archivePath": "modules/b.ho",
      "sha256": "...",
      "imports": [],
      "symbols": []
    }
  ],
  "symbolIndex": {
    "_F_M_a_E_main_i8": {
      "module": "a",
      "archivePath": "modules/a.ho"
    }
  },
  "moduleIndex": {
    "a": "modules/a.ho",
    "b": "modules/b.ho"
  }
}
```

The loader should decompress the whole `.ha` archive, read the manifest, then load required internal `.ho` module payloads in dependency order.

---

## 4. Local Import Resolution Rules

When the executable receives a `.hoo` source file:

```bash
hoo path/to/a.hoo -o app.ha
```

The directory containing the main source file is the root of the module path. All local imports are resolved relative to that root.

### 4.1 Module Naming
Each local source file gets a module name based on its normalized relative path from the main source file's directory.

Examples:

| Source path | Relative path | Module name |
|-------------|---------------|-------------|
| `a.hoo` | `a.hoo` | `a` |
| `b.hoo` | `b.hoo` | `b` |
| `pkg/math-utils.hoo` | `pkg/math-utils.hoo` | `pkg.math_utils` |
| `pkg/internal/parser.hoo` | `pkg/internal/parser.hoo` | `pkg.internal.parser` |

Normalization:

- Remove `.hoo`.
- Normalize path separators to `/`.
- Treat each subdirectory as a module component.
- Convert each component to snake case.
- Replace invalid identifier characters with `_`.
- Reject paths that normalize outside the root source directory.

### 4.2 Import Mapping
Initial local import forms:

```hoo
import b;
import pkg.math_utils;
from pkg.internal.parser import Parser;
```

Resolution candidates:

| Import | Candidate file |
|--------|----------------|
| `import b;` | `<root>/b.hoo` |
| `import pkg.math_utils;` | `<root>/pkg/math_utils.hoo` |
| `from pkg.internal.parser import Parser;` | `<root>/pkg/internal/parser.hoo` |

No `--module-path` or `--source-path` flag should be added. The main source file's directory is always the only local module root.

---

## 5. Executable Behavior

### 5.0 Command-Line Contract
After this issue is implemented, `hoo` should support these inputs:

| Input | Default behavior |
|-------|------------------|
| `.hoo` | Build a `.ha` archive; do not execute `main` unless `--exec` is present |
| `.ha` | Load archive into `HVMJIT` and execute the selected archive entry point |

`.ho` is no longer a public executable input. It is an internal intermediate module payload used inside `.ha` archives.

Supported options:

| Option | Applies to | Behavior |
|--------|------------|----------|
| `-h`, `--help` | all | Print usage and exit `0` |
| `-v`, `--version` | all | Print version and exit `0` |
| `--verbose` | all execution/build modes | Print diagnostic progress to stderr |
| `-o <path>`, `--output <path>`, `--output=<path>` | `.hoo` build mode | Write build artifact to the given path |
| `--exec` | `.hoo` only | Compile source and local imports, then run `main` immediately |
| `--repl` | no input | Start interactive REPL mode |
| `--` | `.hoo --exec`, `.ha` | End driver options; remaining tokens become Hoo program args |

Invalid combinations:

| Command shape | Expected behavior |
|---------------|-------------------|
| `hoo --exec app.ha` | Error: `--exec` is only valid for `.hoo` source input |
| `hoo --exec app.ho` | Error: `.ho` is an internal intermediate format; use a `.ha` archive |
| `hoo app.hoo -o app.ho` | Error: source compilation always produces `.ha`; use `-o app.ha` |
| `hoo app.ha -o out.ha` | Error: `-o/--output` is only valid when compiling `.hoo` input |
| `hoo app.ho` | Error: `.ho` is an internal intermediate format; use a `.ha` archive |
| `hoo app.hoo -- first` without `--exec` | Build archive; program args are ignored or rejected because the program is not executed |
| `hoo --repl --exec app.hoo` | Error: `--repl` cannot be combined with `--exec` |
| `hoo --repl app.hoo` | Error: `--repl` does not accept input files |
| `hoo --repl app.ha` | Error: `--repl` does not accept input files |
| `hoo --repl app.hoo -o app.ha` | Error: `--output` cannot be combined with `--repl` |
| `hoo --repl -- first` | Error: program arguments cannot be passed to REPL mode |

Recommended handling for the last case:

```text
Error: program arguments require an execution mode; use --exec for .hoo source input
```

Examples:

| Command | Expected result |
|---------|-----------------|
| `hoo app.hoo` | Builds `app.ha`; does not run `main` |
| `hoo app.hoo -o dist/app.ha` | Builds `dist/app.ha`; does not run `main` |
| `hoo --exec app.hoo` | Builds in memory, loads into `HVMJIT`, runs `main` |
| `hoo --exec app.hoo -- one two` | Runs source and exposes `one`, `two` through `hoo.args` |
| `hoo --repl` | Starts an empty interactive REPL |
| `hoo app.ha` | Loads archive and runs selected `main` |
| `hoo app.ha -- one two` | Runs archive and exposes `one`, `two` through `hoo.args` |

### 5.1 Source Input
Current:

```bash
hoo a.hoo -o a.ho
```

Future behavior:

- `.hoo` input is treated as a build/archive input by default.
- `hoo` always emits `.ha` for `.hoo` source input, even for a single source file with no imports.
- If local imports exist, compile all local source files and package their internal `.ho` module payloads into `.ha`.
- The executable directly runs a `.hoo` source file and calls its `main` function only when `--exec` is provided.

Output naming:

```bash
hoo a.hoo
```

Builds a `.ha` archive using the default output name:

```text
a.ha
```

Direct source execution requires:

```bash
hoo --exec a.hoo
```

Expected behavior:

1. Resolve and compile `a.hoo` plus local imports.
2. Load the compiled module set into an `HVMJIT` instance.
3. Resolve the selected `main` entry point.
4. Call `main`.
5. Initialize `hoo.args` from arguments after `--`.

```bash
hoo a.hoo -o app.ha
```

Writes:

```text
app.ha
```

### 5.2 Output Parameter
Rules:

- If `-o` or `--output` is provided, use that exact path.
- The path must end in `.ha`.
- If the path ends in `.ho`, emit a clear error:

```text
Error: source compilation always produces .ha output; use -o app.ha
```

Recommended default:

- For source files, always produce `.ha` output once cross-file imports are implemented.
- `.ho` files are internal intermediate payloads only.
- Direct execution of `.hoo` source is opt-in through `--exec`.

### 5.3 Archive Execution
Add:

```bash
hoo app.ha
```

Expected behavior:

1. Open archive.
2. Read `META-INF/hoo/archive.json`.
3. Resolve entry point from manifest.
4. Load required `.ho` modules in dependency order.
5. Execute `main`.

The `hoo` executable must accept `.ha` files as a first-class input type alongside `.hoo`.

Runtime behavior:

- Create an `HVMJIT` instance.
- Load the `.ha` archive through `HooArchiveLoader`.
- Use the archive manifest's `entryPoint` when present.
- If no explicit `entryPoint` is present, scan the manifest symbol index for exported `main` functions.
- If exactly one `main` function exists, call it.
- If no `main` function exists, fail with a clear error:

```text
Error: archive 'app.ha' does not define a main function
```

- If multiple `main` functions exist and the manifest does not identify one entry point, fail with a clear error:

```text
Error: archive 'app.ha' contains multiple main functions; entry point is ambiguous
```

- If the selected main symbol cannot be loaded or resolved by `HVMJIT`, fail with a clear error naming the missing symbol.

Argument behavior:

```bash
hoo app.ha -- first second
```

The executable must initialize `hoo.args` with sanitized program arguments, the same way it does for `.hoo --exec` execution:

```text
programName = app.ha
positional args = ["first", "second"]
```

Driver flags such as `--verbose`, `-o`, and `--output` must not leak into `hoo.args`. Only arguments after the first `--` delimiter are visible to the Hoo program.

### 5.4 Internal `.ho` Payloads
`.ho` remains the internal compiled module representation, but it is not a public executable input or output format for `hoo`.

```bash
hoo module.ho
```

Expected behavior:

```text
Error: .ho is an internal intermediate format; use a .ha archive
```

Rationale:

- `hoo` should expose one distributable build artifact: `.ha`.
- `.ho` payloads remain useful for compiler internals, archive construction, and low-level tests.
- End users should not need to coordinate multiple `.ho` files manually.

### 5.5 REPL Mode
The `hoo` executable must continue to support interactive REPL mode.

```bash
hoo --repl
```

Expected behavior:

1. Start an interactive `REPLSession`.
2. Do not build a `.ha` archive.
3. Do not run a source or archive `main`.
4. Do not require an input file.
5. Reject any input file.

REPL mode is mutually exclusive with build output and execution modes:

- `--repl` cannot be combined with `--exec`.
- `--repl` cannot be combined with `-o` or `--output`.
- `--repl` does not accept `.hoo`, `.ha`, or `.ho` input files.
- `--repl` does not accept program arguments after `--`.

REPL behavior:

- Start with a welcome message and an interactive prompt.
- Maintain session state across entered declarations, imports, variables, functions, and classes.
- Evaluate expressions and statements immediately where possible.
- Support multiline input by tracking balanced braces, parentheses, brackets, strings, and comments.
- Report parse, compile, and runtime errors without terminating the session.
- Use the same compiler, AST builder, code generator, and `HVMJIT` execution path as normal source execution.
- Do not create `.ha` files or expose intermediate `.ho` payloads.
- Do not initialize `hoo.args`, because there is no program `main` invocation.
- Keep REPL commands separate from Hoo source syntax.

Required REPL commands:

| Command | Behavior |
|---------|----------|
| `/help` | Show REPL command help |
| `/reset` | Clear accumulated session declarations/state |
| `/exit` | Exit the REPL |
| `/quit` | Exit the REPL |

### 5.6 Help Text
The future help text should make build-vs-execute behavior explicit:

```text
Usage: hoo [options] <input> [-- program_args...]

Inputs:
  <file>.hoo      Source file. Builds a .ha archive by default.
  <file>.ha       Hoo archive. Executes archive entry point.

Options:
  -h, --help            Display this help message
  -v, --version         Display version information
  -o, --output <file>   Write .ha output when compiling .hoo input
  --exec                Execute .hoo source after compiling it
  --repl                Start interactive REPL mode
  --verbose             Enable verbose logging
  --                    End hoo options; remaining args are passed to the Hoo program

Examples:
  hoo app.hoo                 # Build app.ha
  hoo app.hoo -o dist/app.ha  # Build dist/app.ha
  hoo --exec app.hoo          # Compile and run app.hoo
  hoo --repl                  # Start interactive REPL
  hoo app.ha                  # Run archive
  hoo app.ha -- one two       # Run archive with program args
```

---

## 6. Required Classes

Add archive-management classes under a suitable namespace, for example `hooc::archive`.

### 6.1 `HAArchive`
Represents an opened archive.

Responsibilities:

- Open `.ha` from file or memory.
- Read archive metadata.
- List module entries.
- Extract/load `.ho` payloads.
- Validate checksums.

### 6.2 `HAArchiveBuilder`
Creates new archives.

Responsibilities:

- Add compiled `HOModule` objects or serialized `.ho` byte vectors.
- Add manifest/index metadata.
- Store the manifest as minified JSON text.
- Store module entries uncompressed inside the archive container.
- Apply compression only to the completed `.ha` archive stream.
- Write final `.ha` file atomically.

### 6.3 `HAArchiveEditor`
Modifies existing archives.

Responsibilities:

- Add, replace, or remove module entries.
- Rebuild manifest indexes.
- Preserve or update checksums.
- Write modified archive atomically.

### 6.4 `HAManifest`
Structured representation of `archive.json`.

Responsibilities:

- Parse/generate JSON.
- Validate required fields and version.
- Provide quick lookups by module and symbol.
- Track dependencies and entry point.

### 6.5 `LocalImportResolver`
Resolves source imports.

Responsibilities:

- Parse imports from AST.
- Resolve local module paths to `.hoo` files.
- Detect duplicate normalized module names.
- Detect import cycles.
- Produce a dependency graph.

### 6.6 `HooArchiveLoader`
Integrates `.ha` with `HVMJIT`.

Responsibilities:

- Load manifest.
- Resolve dependency order.
- Load each `.ho` module.
- Resolve cross-module symbols through manifest indexes.
- Select the archive entry point.
- Detect missing or ambiguous `main` functions.
- Report missing modules/symbols clearly.

### 6.7 `HooBuildPlanner`
Plans a source build before compilation.

Responsibilities:

- Accept the root `.hoo` path and CLI build options.
- Normalize the root directory.
- Invoke `LocalImportResolver`.
- Produce an ordered list of source modules to compile.
- Decide output kind: `.ha` file or in-memory execution archive.
- Reject invalid CLI combinations before compiling.

### 6.8 `HooArchiveCompiler`
Coordinates compilation of a multi-file program.

Responsibilities:

- Compile each planned source file with the normalized module name.
- Preserve source path and module name metadata.
- Collect serialized `.ho` bytes.
- Collect exports, imports, and selected entry point metadata.
- Return an in-memory archive model or write through `HAArchiveBuilder`.

### 6.9 `HAEntryPointResolver`
Finds the runnable entry point.

Responsibilities:

- Prefer `manifest.entryPoint` when present.
- Otherwise scan for exported `main` functions.
- Reject missing main.
- Reject ambiguous multiple main functions.
- Return the exact mangled symbol to pass to `HVMJIT::run`.

---

## 7. Implementation Details

### 7.1 CLI Parsing
Extend `HooCLI::Options` with:

```cpp
bool exec = false;
bool repl = false;
enum class InputKind { Source, Archive };
InputKind inputKind;
std::vector<std::string> programArgs;
```

Parsing rules:

- Parse driver options until `--`.
- After `--`, store all tokens in `programArgs` without interpreting them.
- Determine input kind from the input file extension.
- Validate option/input combinations before any file reads.
- Initialize `hoo_args` only after validation succeeds and only for execution modes:
  - `.hoo` with `--exec`
  - `.ha`
- Do not initialize `hoo_args` for `--repl` mode because no Hoo program `main` is executed.
- For `.hoo` build mode without `--exec`, do not initialize runtime args because no Hoo program is executed.

### 7.2 Source Discovery
`LocalImportResolver` should use the AST rather than ad-hoc text scanning.

Algorithm:

1. Parse root source and collect imports.
2. For each import, decide whether it is local or runtime/builtin.
3. Resolve local imports to candidate `.hoo` paths.
4. Read and parse imported source files.
5. Repeat recursively.
6. Build a dependency graph.
7. Detect cycles and missing files.
8. Return a deterministic topological order.

Builtin imports such as `hoo.args`, `hoo.io`, and `hoo.math` should not resolve to local files unless an explicit local module shadowing policy is later introduced.

### 7.3 Module Name Derivation
Add a helper such as:

```cpp
std::string moduleNameFromRelativePath(std::filesystem::path relativePath);
```

Rules:

- Require `.hoo` extension.
- Strip extension.
- Normalize separators.
- Convert each path component to snake case.
- Join components with `.` for the compiler module name.
- Use the same module name for symbol mangling.

Example:

```text
pkg/Math-Utils.hoo -> pkg.math_utils
```

Archive entry path:

```text
modules/pkg/math_utils.ho
```

### 7.4 Compilation
For each planned source module:

1. Read source.
2. Compile with `HooCompiler::compile(moduleName, source)`.
3. Serialize the resulting `HOModule` into bytes.
4. Extract exported symbols from the module's symbol table.
5. Record source path, archive path, imports, exports, checksum, and byte size.

Compilation should fail if two source files normalize to the same module name.

### 7.5 Archive Writing
`HAArchiveBuilder` should write atomically:

1. Write to a temporary file in the destination directory.
2. Add all `modules/**/*.ho` entries.
3. Generate minified `archive.json`.
4. Add `META-INF/hoo/archive.json` as plain UTF-8 JSON text.
5. Add checksum metadata.
6. Close archive.
7. Rename temporary file to final output path.

If any step fails, delete the temporary file and leave the previous output unchanged.

### 7.6 Archive Loading
`HooArchiveLoader` should:

1. Open archive.
2. Read `META-INF/hoo/archive.json`.
3. Validate `format`, `formatVersion`, checksums, and required fields.
4. Resolve the entry point.
5. Load `.ho` modules into `HVMJIT` in dependency order.
6. Return the selected entry symbol.

Execution path:

```cpp
HVMJIT jit(*ioProvider_);
HooArchiveLoader loader(*ioProvider_);
auto entry = loader.loadArchive(jit, archivePath);
auto result = jit.run(entry.symbol);
```

### 7.7 Error Messages
Required diagnostics:

| Scenario | Error |
|----------|-------|
| Missing local import | `Error: cannot resolve local import 'pkg.foo' from 'src/app.hoo'` |
| Import cycle | `Error: local import cycle detected: a -> b -> a` |
| Duplicate module name | `Error: multiple source files normalize to module 'pkg.foo'` |
| `.ho` output requested | `Error: source compilation always produces .ha output; use -o app.ha` |
| `.ho` input provided | `Error: .ho is an internal intermediate format; use a .ha archive` |
| `.hoo` args without `--exec` | `Error: program arguments require an execution mode; use --exec for .hoo source input` |
| `--repl` with `--exec` | `Error: --repl cannot be combined with --exec` |
| `--repl` with output | `Error: --output cannot be combined with --repl` |
| `--repl` with any input file | `Error: --repl does not accept input files` |
| `--repl` with program args | `Error: program arguments cannot be passed to REPL mode` |
| `.ha` no main | `Error: archive 'app.ha' does not define a main function` |
| `.ha` multiple mains | `Error: archive 'app.ha' contains multiple main functions; entry point is ambiguous` |
| Unsupported archive version | `Error: unsupported Hoo archive format version N` |

### 7.8 Build Artifacts
By default, intermediate `.ho` files are not written next to sources. They are serialized in memory and placed into the `.ha` archive.

Debugging support should be added through test helpers or developer-only diagnostics, not public command-line flags in the first implementation.

No `--module-path`, `--source-path`, `--emit-ho-dir`, or `--keep-temp` flag is part of this issue.

---

## 8. Implementation Plan

1. Add vcpkg dependencies:
   - `libzip`
   - `zstd`
   - `nlohmann-json`
2. Add archive data model:
   - `HAManifest`
   - manifest JSON schema tests
3. Add CLI contract changes:
   - add `--exec`
   - preserve `--repl`
   - accept `.ha` input
   - validate option/input combinations
   - update usage text
4. Add `.ha` writer:
   - `HAArchiveBuilder`
   - write metadata and `.ho` entries
5. Add `.ha` reader:
   - `HAArchive`
   - validate and extract modules
6. Add local import resolver:
   - resolve `import foo.bar;` to local source paths
   - normalize paths to module names
   - build dependency graph
7. Add build planner/compiler orchestration:
   - `HooBuildPlanner`
   - `HooArchiveCompiler`
   - compile root source
   - compile imported local sources
   - preserve module names from normalized relative paths
8. Extend `hoo` CLI:
   - `.hoo` input can emit `.ha`
   - `.hoo` input runs `main` only with `--exec`
   - `--repl` starts an empty REPL and rejects input files
   - `.ha` input can execute archive
   - `.ho` input/output is rejected as an internal intermediate format
   - `.ha` execution initializes `hoo.args` with archive path as program name and arguments after `--`
   - ambiguous or missing archive `main` functions produce clear errors
9. Extend `HVMJIT` loading:
   - load multiple `.ho` modules from archive
   - use manifest index for symbol lookup
10. Add integration tests around real executable behavior.

---

## 9. Tests

### Unit Tests
- Manifest serialization/deserialization.
- Archive build/read round trip.
- Minified manifest round trip.
- Whole-archive Zstd compression/decompression round trip.
- Module-name normalization from relative paths.
- Import graph construction.
- Import cycle detection.
- Duplicate module-name detection.

### Integration Tests

Given:

```text
tmp/
  a.hoo
  b.hoo
  pkg/math_utils.hoo
```

Test:

```bash
hoo tmp/a.hoo
```

Expected:

- Builds `tmp/a.ha`.
- Does not execute `main`.

Test direct source execution:

```bash
hoo --exec tmp/a.hoo -- first second
```

Expected:

- Compiles `tmp/a.hoo` and local imports.
- Loads the compiled module set into `HVMJIT`.
- Calls the selected `main`.
- `hoo.args` sees two positional arguments: `first` and `second`.

Test:

```bash
hoo tmp/a.hoo -o tmp/app.ha
```

Expected:

- `tmp/app.ha` exists.
- Archive contains:
  - `META-INF/hoo/archive.json`
  - `modules/a.ho`
  - `modules/b.ho`
  - `modules/pkg/math_utils.ho`
- Manifest indexes all modules and exported symbols.

Test:

```bash
hoo tmp/app.ha
```

Expected:

- Loads archive.
- Executes root `main`.
- Returns expected result.

Test archive program arguments:

```bash
hoo tmp/app.ha -- first second
```

Expected:

- `hoo.args` sees `programName() == "tmp/app.ha"`.
- `args_count()` returns `2`.
- `args_get(0)` returns `"first"`.
- `args_get(1)` returns `"second"`.

Test ambiguous entry point:

```bash
hoo tmp/ambiguous.ha
```

Where the archive manifest indexes multiple exported `main` functions and has no explicit `entryPoint`.

Expected:

```text
Error: archive 'tmp/ambiguous.ha' contains multiple main functions; entry point is ambiguous
```

Test missing entry point:

```bash
hoo tmp/no_main.ha
```

Expected:

```text
Error: archive 'tmp/no_main.ha' does not define a main function
```

Test invalid `.ho` output:

```bash
hoo tmp/a.hoo -o tmp/a.ho
```

Expected:

```text
Error: source compilation always produces .ha output; use -o tmp/a.ha
```

Test invalid `.ho` input:

```bash
hoo tmp/a.ho
```

Expected:

```text
Error: .ho is an internal intermediate format; use a .ha archive
```

Test empty REPL:

```bash
hoo --repl
```

Expected:

- Starts interactive REPL.
- Does not require an input file.
- Does not build `.ha`.
- Does not execute `main`.

Test REPL rejects input files:

```bash
hoo --repl tmp/a.hoo
```

Expected:

```text
Error: --repl does not accept input files
```

Test invalid REPL combinations:

```bash
hoo --repl --exec tmp/a.hoo
hoo --repl tmp/a.hoo -o tmp/a.ha
hoo --repl tmp/app.ha
hoo --repl tmp/a.hoo -- first
```

Expected:

- Each command fails with the corresponding clear diagnostic from section 7.7.

---

## 10. Open Questions

- Should the manifest include full debug/source maps in v1, or reserve them for a future archive version?
- Should archive signatures be added later for package distribution?

---

## 11. Acceptance Criteria

1. `hoo a.hoo` builds `a.ha` and does not execute `main`.
2. `hoo --exec a.hoo` compiles source, loads the module set into `HVMJIT`, and runs the selected `main`.
3. `hoo a.hoo -o app.ha` compiles `a.hoo` and all local `.hoo` imports into one archive.
4. `.ha` archives contain a minified JSON manifest with module and symbol indexes.
5. Module names are derived from normalized snake-case relative paths.
6. `hoo app.ha` loads the archive into `HVMJIT` and runs the selected archive entry point.
7. `hoo app.ha -- args...` initializes `hoo.args` with sanitized program arguments.
8. Archives with no `main` or ambiguous multiple `main` functions fail with clear diagnostics.
9. Local import cycles and missing imports produce clear diagnostics.
10. `.ho` input and output are rejected by `hoo` because `.ho` is an internal intermediate format.
11. `hoo --repl` starts an interactive REPL without accepting any input file.
12. Invalid `--repl` combinations with `--exec`, `--output`, `.ha`, `.ho`, or program args fail clearly.
13. Unit tests cover manifest, archive read/write, import resolution, dependency ordering, and CLI option validation.
14. Integration tests cover the real `hoo` executable for `.hoo -> .ha`, `--exec` source execution, `.ha` execution, archive args, missing main, ambiguous main, and REPL behavior.
