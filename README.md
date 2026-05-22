# Hooc

Last Updated: 2026-05-22

Hooc is a statically-typed language/compiler project with:

- ANTLR-based parsing
- typed AST construction
- LLVM-based code generation/JIT path
- HVM specification and module-format work for `.ho` artifacts

## 1. Current Focus

- keep grammar/AST/codegen aligned
- keep HVM core profile minimal and grammar-driven
- advance practical module/AOT workflows without inflating core ISA

See:

- `docs/features.md`
- `docs/grammar.md`
- `docs/implementation-status.md`
- `docs/hvm/HVM_SPEC.md`

## 2. Build

Prerequisites:

- CMake >= 3.16
- C++17 toolchain
- LLVM toolchain/development headers
- ANTLR4 runtime
- GoogleTest (for tests)

Typical local build:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -- -j8
```

## 3. Test

```bash
cd build
./hoo-tests
# or
ctest --output-on-failure
```

## 4. Project Layout

```text
src/
  parsing/    grammar + generated parser artifacts
  ast/        typed AST and builder
  codegen/    LLVM IR generation
  hvm/        HVM module/instruction infrastructure
  runtime/    runtime libraries and registries
tests/        unit/integration tests
docs/         language, architecture, HVM, roadmap docs
```

## 5. HVM Snapshot

Current HVM profile is **core-minimalest**.

- core spec: `docs/hvm/HVM_SPEC.md`
- opcodes: `docs/hvm/hvm_instruction_set.csv`
- registers: `docs/hvm/hvm_register_set.csv`
- reference: `docs/hvm/instructions.md`
- module format: `docs/hvm/HO_FILE_FORMAT.md`

Core excludes SIMD/threading/interrupt/debug families by default; those belong to optional extension profiles.

## 6. CLI Note

CLI/tooling behavior can evolve; use the currently built binaries and `--help` output as operational truth for your local build.

## 7. Contributing

When making language/runtime changes:

1. update grammar/implementation
2. update tests
3. update docs in the same change
4. ensure HVM docs remain internally consistent

Primary consistency set:

- `src/parsing/Hooc.g4`
- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`
- `docs/hvm/instructions.md`
