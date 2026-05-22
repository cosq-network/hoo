# HVM Implementation Considerations (Core-Minimalest)

This guide is aligned to:

- `docs/hvm/HVM_SPEC.md`
- `docs/hvm/hvm_instruction_set.csv`
- `docs/hvm/hvm_register_set.csv`
- `docs/hvm/instructions.md`
- grammar in `src/parsing/Hooc.g4`

It intentionally excludes optional extension families from the core implementation plan.

## 1. Primary Design Choice: Interpreter First, JIT Optional

Recommended order:

1. Implement a correct interpreter for the core profile
2. Add tracing/profiling and differential checks
3. Add JIT only after semantic parity is stable

Reason:
- Core set is intentionally small; correctness risk is mostly in control flow, object model, exceptions, and FFI boundary.

## 2. Encoding and Decode Path

Requirements:

- Support base 32-bit formats (`R/I/RI/B/J`) for opcodes `0x00..0x7F`
- Support escape-prefixed extended opcode decoding for core opcodes `>= 0x100` (`TRY`, `THROW`, `CALLHOST`, etc.)
- Enforce sign-extension and branch offset semantics exactly as defined in `HVM_SPEC.md`

Validation checks:

- malformed/truncated instruction stream rejection
- illegal opcode rejection
- immediate range handling consistency

## 3. Register and Calling Convention Semantics

Must enforce:

- `r0` immutable zero
- return value in `r1`
- return address in `r29` and `RET -> pc = r29`
- stack discipline via `r31`, frame conventions via `ENTER/LEAVE/FRAME`

Common bug sources:

- accidentally treating `r1` as link register
- mutating `r0`
- inconsistent stack-pointer update ordering in `ENTER/LEAVE`

## 4. Minimality-Preserving Lowering

Lower these, do not add new core opcodes:

- `SUBI` -> `ADDI rd, rs, -imm`
- `NEG` -> `SUB rd, r0, rs`
- `CMPGT/CMPGE` -> swapped `CMPLT/CMPLE`
- `BGT/BGE` -> swapped `BLT/BLE`
- constant materialization via `MOVZ`/`LUI` (+ arithmetic/logic)

Keep lowering centralized so parser/AST/codegen behavior stays deterministic.

## 5. Control-Flow Implementation

Grammar-driven features:

- `if/else`
- `while`
- `for in ... range ... by ...`
- `break` / `continue`

Implementation notes:

- normalize all conditions to compare + conditional branch
- use explicit loop context stacks for break/continue targets
- preserve branch offset calculation in instruction units (not bytes)

## 6. Objects and Arrays

Core object/array opcodes:

- `NEW`, `NEWA`
- `LDF`, `STF`
- `LDELEM`, `STELEM`
- `ARRAYLEN`

Runtime boundary:

- object layout and array storage are runtime-managed
- VM should keep field/element access checks explicit (null and bounds where defined)

## 7. Exception Model

Core exception opcodes:

- `TRY`, `THROW`, `CATCH`, `FINALLY`, `RETHROW`, `ENDFIN`

Recommended execution model:

- maintain explicit handler stack
- push handler context on `TRY`
- unwind to matching handler on `THROW`
- guarantee `FINALLY` execution on normal and exceptional paths
- represent rethrow as propagation of current exception context

Regression hotspots:

- nested `try/finally` ordering
- `rethrow` without active exception
- control transfer across finally blocks

## 8. FFI and Runtime Bridge

Core bridge opcodes:

- `CALLHOST`, `CALLNATIVE`, `LOADLIB`, `GETSYM`

Guidelines:

- isolate ABI marshalling/unmarshalling in one subsystem
- validate symbol resolution failure paths cleanly
- provide deterministic error reporting for missing libs/symbols

Security baseline:

- do not expose unrestricted host symbols by default
- allowlist host-call IDs for `CALLHOST`
- treat dynamic loading as privileged capability in production modes

## 9. Module and Binary Format

Use `HoModule` and `HO_FILE_FORMAT.md` as binary truth:

- header: 64 bytes
- section entries: 40 bytes
- little-endian only
- metadata section schemas fixed (`symtab`, `reloc`, `export`, `import`, `funcmeta`)

Critical checks:

- bounds/overflow checks on all offsets and counts
- reject malformed metadata section lengths
- preserve string-table offset validity

## 10. Testing Strategy

Required layers:

1. Unit tests per opcode family
2. Grammar-to-lowering tests (source -> expected instruction patterns)
3. Exception and FFI negative-path tests
4. Module roundtrip tests (`serialize` <-> `parse`)

Recommended invariants:

- interpreter state equivalence before/after non-observable instructions
- deterministic results across repeated runs
- no dependence on optional extension opcodes in core tests

## 11. Performance Guidance Without Breaking Minimality

- optimize dispatch (jump table / threaded interpreter)
- cache decoded instruction forms
- keep runtime calls explicit instead of adding specialized opcodes prematurely

Performance work should not expand the core ISA unless grammar evolution requires it.

## 12. Extension Policy

If new grammar features require extra instruction families:

1. keep core unchanged unless mandatory
2. define feature profile in `docs/hvm/HVM_EXTENSIONS.md`
3. gate by capability flag/versioning
4. add exhaustive compatibility tests

Core profile should remain the smallest complete set for current `Hooc.g4`.
