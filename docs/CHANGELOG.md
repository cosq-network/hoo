# Changelog

All notable changes to the Hoo project will be documented in this file.

The format follows [Semantic Versioning 2.0.0](https://semver.org/) and
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions.

Commit messages use the [Conventional Commits](https://www.conventionalcommits.org/) specification.

---

## Unreleased

- fix(codegen): release nullable user-object locals in generic type slots
  - Track ARC cleanup separately from the runtime type ID so nullable named
    references using type ID 100 are released safely at scope exit.

- fix(jit): dispatch exception handlers located after early returns
  - The JIT's exception-dispatch target set was computed by a linear scan
    that stopped at the first `RET`, so catch-handler blocks emitted after an
    early `return` (e.g. `try { ... return 0; } catch { ... }`) were excluded
    from the compiled throw/rethrow switch and the throw returned `-1`
  - Bound the scan by the next function's entry PC instead, keeping handler
    blocks valid throw targets while staying within the function's text range
  - Add a JIT regression test that dereferences a null nullable value inside a
    called function and catches the resulting exception after an early return

- feat(codegen): complete ISSUE-047 nullable deref null-safety
  - Preserve the underlying type of `T?` in `typeIdFromDeclaredType` instead of
    collapsing every optional type to generic object
  - Track nullability on locals, parameters, class fields, and expressions, and
    make overload selection distinguish `T?` from `T`
  - Emit null-pointer checks before dereferencing a nullable receiver/base at
    member access, method call, array access, and assignment sites; the check
    throws a catchable `NullPointerException` via the shadow-handler syscall path
  - Add `validateAssignmentNullSafety` compile-time diagnostics rejecting `null`
    or nullable values assigned into non-nullable slots
  - Mangle nullable types with the `O` prefix (`mangleTypeId` appends `?`,
    which `SymbolMangler::mangleType` normalizes to `O`) so overloaded
    signatures disambiguate
  - Set the `HVM_NZ` module feature flag when null-checking code is emitted
  - Register the `_F_hoo_exception_null_pointer_p` runtime bridge in the JIT
    symbol table and add interpreter/JIT regression coverage
  - Update ISSUE-047 status to PARTIALLY IMPLEMENTED (ARC policy for nullable
    object locals and `LD.D.NZ` folding remain open)

- fix(modules): complete ISSUE-036 dependency resolution
  - Traverse each visited module's own dependency edges for correct transitive
    topological ordering
  - Use explicit visiting/visited states for reliable cycle detection and keep
    bundle and per-module cycle results aligned
  - Remove the obsolete `checkCircularDependencies()` helper
  - Add chain, transitive dependency, cycle, and HVMJIT loader regression tests

- feat(serializable): complete ISSUE-035 declarative class serialization
  - Resolve generated static `deserialize()` and instance `serialize()` calls
    with modifier-aware symbols
  - Include inherited public fields in a deterministic base-first schema and
    recursively lower nested serializable fields
  - Preserve buffers as tagged Base64 objects and tensors as tagged
    shape/element-type/raw-bit objects
  - Add codegen and runtime round-trip regression tests; the full suite passes
    with 2062 tests and 2 disabled

- feat(tensor): complete ISSUE-030 scalar broadcasting
  - Add safe tensor-scalar add, subtract, scale, and divide for both operand
    orders without treating scalar values as tensor pointers
  - Preserve native integer, FP8, and f64 promotion semantics through JIT
    runtime wrappers
  - Add runtime and JIT regression coverage

- feat(tensor): complete ISSUE-025 tensor data type support
  - Add packed `tensor<bit>` and one-byte `tensor<int8>`, `tensor<byte>`, and
    canonical E4M3 `tensor<f8>` storage with promotion and native-width
    overflow semantics
  - Route low-precision tensor arithmetic through interpreter/JIT runtime
    wrappers while preserving wide-element vector lowering
  - Add reshape, transpose, and numerically stable softmax with JIT symbol
    registration and regression tests

- feat(hvm): implement HVM 1.5 scalar sub-word precision
  - Add `ARITH_B` (`0x11`), `LOGIC_B` (`0x22`), and `FLOAT_ARITH_B` (`0x31`)
    opcode families with registry, CSV, encoding, and decoding coverage
  - Add interpreter and LLVM JIT lowering for wrapping int8/byte arithmetic,
    normalized bit logic, and canonical E4M3 FP8 arithmetic
  - Add signed/unsigned division and remainder semantics with native-width
    overflow handling and compiler-controlled sign/zero extension
  - Preserve the f64 language ABI through FP8 encode/decode fallback shims
  - Add codegen and JIT regression tests; the current full suite passes with
    2062 tests

- fix(hvm): complete ISSUE-028 sub-word modulo and shifts
  - Dispatch binary and compound `int8`/`byte` modulo through `REM.B` and
    `REMU.B`
  - Add `SHIFT_B` (`0x12`) with wrapping left, logical right, and signed
    arithmetic right byte shifts across interpreter and LLVM JIT
  - Add standalone shift-expression grammar and AST support for `<<` and `>>`
    while preserving existing wide `SHIFT` behavior


- fix(codegen): complete ISSUE-019 register cleanup across loop control flow
  - Add register-state checkpoints to loop and switch control-flow scopes
  - Restore correct temporary-register state for `break`, `continue`, and loop
    fallthrough/exit joins
  - Add regression coverage for while, do-while, stepped range, and for-in
    loops

- fix(async): harden async/await and Future execution
  - Async functions now create and resolve `Future<T>`/`Future<void>` values
  - Await validates async context and Future operands, with primitive and
    ARC-managed result handling
  - Future waits use condition variables and cooperative libuv pumping instead
    of busy-spinning
  - Multiple continuations are detached and retained safely
  - HVM codegen no longer emits unsupported `llvm.coro.*` pseudo-calls; true
    stack-frame suspension remains future VM work
  - Added primitive round-trip and multiple-continuation JIT coverage

- feat(cli): allow `--repl` mode without an input file；add unit tests and update docs
  - `hoo --repl` now skips the required input_file validation
  - Add 3 CLI tests: accepts `--repl` alone, accepts it with a file, and ensures no missing-file error message
- feat(repl): complete REPL Phase 5 with fixed symbol mangling, comprehensive tests, and edge-case handling
  - Fix target symbol generation: use `SymbolMangler::mangleFunctionName()` instead of hardcoded `_any`/`_v` suffixes
  - Add `const` keyword to `isDeclaration()` detection
  - Reset parser state variables after each block evaluation
  - Add `HooReplAdvancedTest.cpp` to CMake test target (was missing)
  - 15 new tests: expression results, incremental function declarations, `var`/`const` persistence, assignment statements, deeply nested braces, block comments with braces, strings with braces, multi-line comments, error recovery across statements, stateful counter, reset clearing
- feat(array): add `sort()` and `reverse()` instance methods
- fix(hvm): add integer overflow checking for ARITH add/sub/mul/div/rem (ISSUE-051)
  - Interpreter: manual overflow checks for ADD/SUB/MUL, INT64_MIN/-1 guard for DIV/REM
  - JIT compiler: use LLVM sadd/ssub/smul_with_overflow intrinsics, guarded DIV/REM
  - 5 new tests covering all overflow edge cases

## 0.2.0 – 2026-06-21
- docs(issues): audit statuses and add HVM feature plans
- feat(issues/045): add comprehensive semantic-versioning, version-bump, Linux CI and GitHub release workflow
- feat(issues/039): refine List intrinsic design – no new HVM opcodes, reuse vector ops
- Fix HooIOTest to use HooCharacter and update includes; qualify HVMJIT methods with hooc:: namespace
- feat: complete REPL integration (Phase 12) – signal handling, cross‑platform input, nested brace matching, advanced tests
- feat: cross‑platform runtime fixes
- hoo.io: readchar now returns char (non‑blocking) and documentation updated
- docs: enrich Array documentation with type support, rules, syntax and examples
- docs: finalize HVM green‑compute spec & clean up system documentation
- feat: add full JIT tensor support and documentation
- hvm: Implement zero-overhead hardware loops (LOOP.SET / LOOP.DECBR)
- fix(hvm): resolve instruction decoding collisions, JIT register mapping, and add RETAIN/RELEASE support
- docs,hvm: consolidate system design book and expand ABI docs
- refactor: convert static methods to free functions across runtime, tests, and docs
- docs: refactor standard library APIs and finalize HVM Green Compute & Vector Proposal
- Refactor Character API to follow compiler and runtime design guidelines
- Refactor collections and runtime APIs to conform to compiler restrictions
- docs: enrich HVM proposal section 10 with codebase implementation mappings
- refactor: consolidate runtime documentation and add JIT helper functions
- docs: add and update HVM green compute and multicore specification proposal
- refactor: Convert Math, Hashing, System, and Encoding to free functions
- refactor: standardize hoort free functions to snake_case naming convention
- feat(repl): implement integration plan up to Phase 3 with unit tests
- Enhance REPL plan document with deep technical implementation details for each phase
- Update REPL integration plan document to specify HVMCodeGenerator and HVMJIT dependencies
- Enforce strict submodule-level import validation for hoort APIs
- fix(buffer): fix MSVC overloading error in extern C block
- refactor: convert DateTime static/factory methods to free functions
- docs: add language ergonomics proposals
- fix: harden serializable class modifier codegen and tests
- feat: add serializable class modifier and DateTime instantiable class redesign
- fix(portability): guard strdup with _MSC_VER in hoo_csv, use temp_directory_path in test
- docs(api): clean args.md of host references, expand index with full API listing
- docs(api/fs): remove C++ class reference, keep Hoo-only syntax
- docs(api/fs): expand C++ class docs per-method with full format
- docs(api/fs): add C++ class reference for Path, File, Directory
- Migrate Fs static-like class methods to free functions; fix hoo_fs.cpp bugs
- feat(csv): refactor to single-constructor, harden ARC, register array destructor
- Refactor Buffer API to single-constructor free-function model
- Refactor JSON runtime to free-function HashMap/AnyArray API
- feat: restrict \ny\ meta type and expand Map/HashMap/AnyArray test coverage
- Add native any, HashMap, and AnyArray intrinsic support
- Add Random math runtime coverage and HashMap planning docs
- Unify built-in runtime constructor syntax
- feat(json): add HooMap interop, string transformation, and float type support
- feat(csv): add DataFrame-like API with type validation exceptions
- docs(csv): update docs for ARC lifecycle, map-based API, and hoo_fs I/O
- feat(csv): use hoo_fs for file I/O and add HooMap-based parse API
- refactor(runtime): redesign CSV C-ABI to OOP-style ARC-managed API
- refactor(runtime): redesign Map C-ABI to OOP-style polymorphic API
- Merge branch 'main' of github.com:cosq-network/hoo
- feat: add managed Buffer type with full OOP API, JIT support, and cross-module integration
- test(fs): add getter, edge-case, and rename-path-update tests
- refactor(fs): convert static-only classes to proper OOP with instance methods
- merge(path): consolidate hoo.path into hoo::fs::Path
- fix(io): normalize null representation and rename doc to io.md
- Merge branch 'main' of github.com:cosq-network/hoo
- docs: update issue status based on recent f8, bit, and tensor implementations
- docs: identify and document new technical issues in compiler and ISA
- refactor(runtime): rewrite hoo.fs with object-oriented C++ class API
- docs: deepen HVM 1.5 plan with JIT mapping and quantitative advantages
- docs: add implementation plan for HVM 1.5 sub-word precision extension
- fix(tests): resolve HooCsvJitTest.WriteFile failure on Windows
- fix: make hoo_runtime.c compile on Windows by replacing direct pthread usage with portable wrapper
- feat: add tensor type with 1D/2D/3D support across parser, codegen, runtime, and JIT
- docs: add implementation plan for native ANN support
- Add scalar f8 and bit language support
- Fix HVM runtime pointer model regressions
- docs: enrich tensor plan with detailed syntax and operator specifications
- docs: expand tensor plan with f8 and bit type specifications
- docs: add implementation plan for tensor data type
- fix: resolve 2D array literal hang by switching to C-heap allocation
- refactor(csv): convert from singleton to instance-based OOP API
- refactor(compression): convert from singleton to instance-based OOP API
- docs: update status for macOS stabilization and threading support
- fix: resolve macOS build errors and security warnings in runtime library
- Fix Windows CI heap crash in parser tests
- Fix Windows CI test runtime setup
- fix: build GTest with static CRT (/MT) on Windows to prevent heap corruption
- fix: use single-line cmake command in Windows CI to avoid PowerShell caret issue
- docs: update build docs for Windows/MSVC compatibility and fix stale preset references
- fix: full Windows/MSVC compatibility for build, runtime, JIT, and tests
- fix: add MSVC / Windows compatibility across runtime and JIT
- Updating windows build
- Strip non-macOS presets, CI jobs, and trim Dockerfile; add ZLIB/OpenSSL deps to CMake
- feat: unify devcontainer and codespace into single Dockerfile with pre-built LLVM
- Merge branch 'main' of github.com:benoybose/hooc
- feat: add Docker/devcontainer/codespace development environments with CMake presets
- feat: add container-ninja and codespace-ninja CMake presets
- fix: add libzstd-dev to final stage (LLVM runtime dependency)
- feat: add lightweight Codespaces config with prebuilt LLVM binaries
- feat: multi-stage Dockerfile with LLVM 22.1.4 from source, ANTLR4 4.13.2, and zsh
- docs updated
- fix: print JIT execution result to stdout in CLI
- feat: implement scope-level release, singleton constructor, and array bounds checking
- docs: update stale test count in project layout
- feat: extend var type inference; remove char-keyed Map from Hoo layer
- refactor: reimplement Character API as instance methods
- feat: implement argparse-style API for Args class
- docs(runtime/api): generate API docs for all runtime modules
- docs: clean up Hoo developer-facing API docs
- refactor: migrate Hoo runtime API from snake_case to camelCase
- feat: add CLI integration tests and consolidate compiler tests
- feat: fix JIT entry point resolution with module-qualified mangled name
- feat: add Character class method dispatch with JIT support and documentation
- fix(hvm): correct singleton return types and narrow getTypeId fallthrough
- refactor(hvm): migrate standard library to class-method syntax
- docs: rewrite BUILDING.md and debugging-hoo.md with comprehensive beginner guides
- docs: update READMEs and add comprehensive issue tracking
- fix(hvm): resolve JIT crashes, symbol resolution, and class-based method dispatch
- docs: update root README and runtime lib README for SYSCALL 12-23 and tp register
- docs(runtime): update SYSCALL table and tp register docs
- feat(hvm): implement SYSCALLs 12-23, tp register, and memory-offset fix
- docs(hvm): extend ISA for C++ standard library port
- docs(hvm): add system profile for Linux kernel support
- fix(hvm): correct LUI shift, BREAK sentinel, and qualified-name edge cases
- fix(hvm): resolve new-expression crashes and wire JIT symbol resolution
- docs: add runtime API reference
- docs: add hoo source usage examples to 9 runtime module docs
- docs(runtime): synchronize documentation with expanded standard library
- feat: expand standard library and wire JIT symbol resolution for string/array/map/math/stdlib
- feat: enforce public/private access modifiers on class fields
- feat: enforce public/private access modifiers on class methods
- feat: implement singleton/immutable/final/service modifier enforcement and remove unused modifiers
- test: verify method call emits correctly mangled symbol name
- test: add variable scoping tests for HVMCodeGenerator
- feat: add variable scoping to HVMCodeGenerator (scope stack)
- refactor: remove scope statement, fix method mangling, address review findings
- fix: resolve CRITICAL and HIGH severity code review findings
- Added windows build compatibility (#1)
- docs(hvm): add QEMU simulator and production commercialization guides
- docs(hvm): add chip RTL project layout and simulation scaffold
- fix(hvm): make encoding explicit and align instruction decoding with docs
- docs: finalize regression tracking and implementation notes
- docs: clean up resolved issue tracking files
- fix: resolve regressions in array ARC, dynamic growth, and interpolation types
- docs: synchronize all normative documentation with current ISA and features
- feat: finalize literal lowering and runtime array enhancements
- feat: implement string interpolation support (ISSUE-002)
- feat: implement Character runtime type and literal lowering (ISSUE-002)
- Removed ffi
- docs: Remove ISSUE-001 as FFI feature was intentionally dropped
- Refactor: Completely remove FFI subsystem from language frontend
- docs: Expand ISSUE-001 with FFI syntax and lowering details
- Refactor: Remove unimplemented ARROW ('->') operator
- Refactor: Rename ProcessIsolatedParser, decouple HooCompiler, and improve error handling
- Refactor: Codebase cleanup, logic consolidation, and documentation overhaul
- refactor: rename build target hooc to hoo and update documentation
- docs: ensure compliance between docs and implementation files
- docs: add comprehensive grammar and runtime documentation suites
- docs: update READMEs to reflect recent phase 4-6 progress and HVM-only architecture
- Deleted some outdated documents.
- fix(hvmjit): require valid section index for function symbol resolution
- hvm: finish phase 6 hardening and stabilize full test suite
- hvm: advance phase-6 hardening with inspector/tlab scaffolding
- feat(hvmjit): complete Phase 4 bootstrap/init and advance Phase 5 FFI linkage + callback scaffolding
- fix: resolve all high/medium code review issues in codegen and JIT
- docs: normalize filenames to kebab-case, drop date from project-direction-summary
- refactor: merge hvm+hoo-compiler into hoo-core, clean up dead files, add AST unit tests
- refactor: migrate to HVM-only architecture and remove LLVM codegen backend
- hvmjit: advance Phase 2 core lowering and Phase 3 runtime bridge; expand tests/docs
- refactor(runtime): align object layout and docs with HVM v1.4 spec
- refactor(hvm): rename core classes and files for architectural consistency
- refactor: align HVM backend and mangling with v1.4 hardware-ready spec
- docs(hvm): add comprehensive LLVM ORC-based JIT Implementation Guide
- feat(hvm): finalize hardware-ready RISC ISA (v1.4) and aggressive lowering
- docs(hvm): add comparison between HVM and physical CPU architectures
- docs(hvm): add comparison between HVM, JVM, and .NET CLI
- docs(hvm): finalize hardware-ready documentation v1.4
- feat(hvm): refactor ISA to hardware-ready pure RISC core
- fix(hvm): align frame management and calls with lr (r29) convention
- docs(hvm): align canonical instruction set CSV with J-format calls
- docs: update documentation for HVM backend completion and AST hardening
- feat(hvm): achieve full feature parity and harden HVM backend
- feat(codegen): implement and harden HVMCodeGenerator for direct bytecode emission
- refactor: align HVM/AST with core spec and harden runtime integrity
- refactor(ast): remove orphaned nodes and implement strict validation in AST builder
- docs: consolidate and realign language + HVM documentation to minimal core spec
- hvm: introduce scalable escaped instruction encoding and update module decoding/tests
- feat(hvm): harden HoModule parsing/serialization and expand test coverage
- feat(parser-ast-hvm): complete grammar/AST integration and harden tooling/tests
- fix(ho): harden path validation and align CLI/test behavior for unimplemented modes
- docs: refresh and synchronize project documentation with current codebase
- Update .gitignore
- feat(hvm): implement ModuleBundle for HVM module management
- docs: consolidate language syntax and feature specifications
- Add binary file I/O support and integrate IOProvider into module system
- Add ho compiler executable and serialize/deserialize to HoModuleBase
- feat(hvm): add FFI module system with HoModuleBase abstraction
- hvm: align HoModule with HO_FILE_FORMAT.md v1.3 spec
- fix: remove conflicting stub declarations from Math runtime
- Implement Math and Network Standard Library Modules
- Parser: Remove deprecated -> return type syntax from grammar, now only supports func:ReturnType name(params) syntax where return type precedes the function name
- Add exception handling (try-catch-finally) and standard library design documentation
- Add SymbolMangler with demangling support for JIT and function overloading
- Add compound assignment, increment/decrement, multiline strings, and function modifiers
- Add IntegerTypesTest and SymbolMangler: 17 new tests, cross-module symbol resolution
- Modernize compiler infrastructure, implement global constants, and rebrand to 'hoo'
- Implement module-level constants and dynamic global initialization
- Modernize CLI, rebrand to 'hoo', and implement module-level variables
- Modernize CLI, stabilize JIT context management, and rebrand built-in namespace to 'hoo'
- Modernize HooCLI and fix JIT context mismatch segmentation fault
- Complete For-Range implementation: add step support and automatic direction detection
- Modernize language: Remove interfaces/unions, fix type unwrapping, and implement implicit nullable conversions
- CI: fix workflow syntax and successfully disable Windows build
- Update build-and-test.yml
- Fix Windows CI: provision fresh vcpkg in clean directory to resolve baseline errors
- Fixing Windows Build CI/CD issue
- Run lukka/run-vcpkg@v11 Prepare output directories ⏱ elapsed: 0.013 seconds Running command '"C:\Program Files\Git\bin\git.exe"' with args '^"submodule^",^"status^",^"D:\a\hooc\hooc\vcpkg^"' in current directory 'D:\a\hooc\hooc'. error: pathspec 'D:\a\hooc\hooc\vcpkg' did not match any file(s) known to git Setup to run on GitHub Action runners ⏱ elapsed: 0.003 seconds Retrieving the vcpkg Git commit id at: 'D:\a\hooc\hooc\vcpkg' ⏱ elapsed: 0.169 seconds Check whether vcpkg repository is up to date ⏱ elapsed: 0.001 seconds Download vcpkg source code repository ⏱ elapsed: 19.361 seconds Error: Last command execution failed with error code '128'.     at BaseUtilLib.throwIfErrorCode (D:\a\_actions\lukka\run-vcpkg\v11\dist\index.js:44357:19)     at VcpkgRunner.<anonymous> (D:\a\_actions\lukka\run-vcpkg\v11\dist\index.js:46661:28)     at Generator.next (<anonymous>)     at fulfilled (D:\a\_actions\lukka\run-vcpkg\v11\dist\index.js:46272:58)     at process.processTicksAndRejections (node:internal/process/task_queues:95:5) Error: run-vcpkg action execution failed: Last command execution failed with error code '128'.
- Fixing Windows Build issue
- Fix Windows CI: switch to vcpkg manifest mode and add caching
- Fixing Windows build by adding vcpkg path
- fixing windwos build
- Standardize function syntax, implement 'this' keyword, and remove generics
- Implement official support for 'this' keyword and clean up language documentation
- Add comprehensive diagnostics for Windows and Ubuntu CI/CD issues
- Fix: Revert AllTargets initialization, use native target only
- Fix: Initialize all LLVM targets for Ubuntu JIT tests
- Fix: Use pre-installed LLVM on Windows CI/CD runners
- Fix: Proper LLVM JIT target initialization for Ubuntu/Linux
- Fix: Dereference unique_ptr in NewObjectExpression::toString()
- Fix: Build ANTLR4 C++ runtime from source on Ubuntu
- Fix: Explicitly build test target in CI/CD workflow
- Remove old workflow file with underscores
- Fix: Add missing <memory> header to HInstruction.h
- Add comprehensive GitHub Actions workflow to build, test, and package the Hooc compiler across macOS, Ubuntu, and Windows platforms.
- Refactor: Extract CLI into separate testable components
- Add HVM library for object file format and instruction encoding
- Add HVM (Hooc Virtual Machine) library for object file format and instruction encoding
- docs: updated relevant documents
- Fix IO runtime registration and enable I/O functions
- Refactor LLVMCodeGenerator error handling and add I/O runtime support
- feat(ast): add missing scopeStatement and interpolatedString AST building
- refactor: reorganize source and test directories into logical subdirectories
- docs: update architecture docs with minified inline SVG diagrams
- fix: HoocJIT bugs, add unit tests, and improve compiler components
- Updating hooc hvm documentation
- HVM docs updated
- refactor: reorganize runtime library into clear separation of lib and llvm integration
- feat: inject runtime functions as class methods for string and array types
- reformating the code
- Update test documentation with accurate test counts
- A little more cleanup
- Updating array unit tests
- did some more cleanups
- did some code cleanup
- did some basic cleanup
- Updating test reports
- Implement for-in/for-range loop support for arrays
- Documentation improved
- Docs: Add project summary and feature status documents
- Update LLVMCodeGenerator.cpp
- Reworked on array registration
- Implementing runtime module system
- Docs: Add comprehensive Runtime Class Injection Framework documentation
- Fix: Declare hoo_string_new and hoo_array_new functions properly
- Refactor: Move runtime files from runtime/ to src/rt/
- Phase 7.5: Documentation and Final Cleanup
- Phase 7.4: Comprehensive Test Suite for HooArray Redesign
- Phase 7: Complete HooArray Redesign with std::list<std::any> Architecture
- Phase 6: Implement class array support
- Clean up legacy int64 and double array implementations
- Phase 5: Implement 32 integration tests for generic array operations
- Add Phase 4 Unit Tests - Generic Array Runtime Integration
- Update array refactoring status - Phase 4 complete
- Phase 4: Code Generation Updates - Generic Array Integration
- Implement generic array runtime library for hoort (Phase 1-3)
- Implement C#-style generics with monomorphization and complete documentation (v0.6)
- Added MCQ Seeding.
- Generated examples
- Adding array types
- Fix type conversion in function and constructor calls
- Implementing Auto-Generated runtime
- Integrating string implementation
- Update hooc-sample-programs.md
- Update settings.local.json
- Rename build targets for consistency and update all documentation
- Implement method calls on objects (v0.5 continued)
- Update documentation for member access implementation (v0.5)
- Implementing object reation in runtime.
- Supported fucntions with no return types
- Modified parameter definition styles
- Class declaration parsing is added
- Adding build action
- Added module level variable parsing
- Updated with Nullable type support.
- Updates docs
- Verified while loops
- Added unit tests for if else if
- Fixing unit tests
- Updated code generation unit tests
- Added abstraction on code generation
- Renamed CodeGenerator to LLVMCodeGenerator
- Added variable declaration parse unit test
- Added unit test for function call parsing
- docs: Update documentation to reflect working function call support
- feat: Implement array literal syntax with type inference and focused testing
- Updated documents
- Added unit tests for array
- feat: Add comprehensive char primitive type support and testing
- Added unit tests for bool
- Added unit tests for float
- Added unit test for byte
- Added unit tests
- Adding unit tests
- Modified main.cpp
- feat: Complete SimpleASTBuilder and CodeGenerator implementation
- Generated CodeGenerator using AI
- Created parser and jit with the help of AI
- Initial Message

## 0.1.0 – 2026-06-21

### Added
- ISSUE-039: List intrinsic data type design (uses existing vector opcodes, no new HVM instructions).
- ISSUE-041: Function overloading design and mangling strategy.
- ISSUE-042: Decimal fixed-precision intrinsic type design.
- ISSUE-043: Async/await integration via libuv with `Future<T>` design.
- ISSUE-045: Semantic versioning, Linux CI pipeline, and GitHub Release workflow (this release).
- `scripts/bump_version.py` – automated conventional-commit-aware version bumping.
- `scripts/generate_changelog.py` – release-notes generator from git history.
- `.github/workflows/linux-build.yml` – Linux (Ubuntu) CI build and test pipeline.
- `.github/workflows/release.yml` – automated GitHub Release workflow (Linux + macOS).
- Version and CI status badges in `README.md`.
- `HOO_VERSION` variables exported from `CMakeLists.txt`.

### Changed
- `hoo_readchar` now returns a managed `HooCharacter` instance.
- HVMJIT methods correctly qualified with `hooc::` namespace.
- REPL Phase 12 integration completed.

---

*Generated and maintained by Antigravity AI.*
