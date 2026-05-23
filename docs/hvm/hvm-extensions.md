# HVM Extensions (Non-Core Profiles)

This document lists optional HVM instruction families that are **not part of the current core profile** in `docs/hvm/hvm-spec.md`.

Core profile is intentionally minimal and grammar-driven (`src/parsing/Hooc.g4`).
Use these extensions only when language/runtime requirements justify them.

## 1. Policy

- Core profile: minimal set required for current grammar.
- Extensions: optional sets that can be enabled as separate VM profiles.
- Extension adoption rule: add only when a grammar/runtime feature cannot be cleanly lowered through existing core + runtime calls.

## 2. Extension Profiles

## 2.1 String Opcode Family

Purpose:
- Dedicated string ops for performance and compact bytecode.

Representative instructions:
- `STRNEW`, `STRNEWB`, `STRLEN`, `STREMPTY`
- `STRGET`, `STRSET`, `STRAPPEND`, `STRPOP`
- `STRCMP`, `STRCMPN`, `STREQUAL`, `STRFIND`, `STRJOIN`, `STRSUB`
- `STRTOI`, `STRTOD`, `ITOSTR`, `DTOSTR`, `STRENCODE`, `STRDECODE`

When to enable:
- If compiler needs first-class string intrinsics instead of runtime library calls.

Notes:
- Current core profile supports strings via object model + runtime calls.

## 2.2 SIMD / Vector Profile

Purpose:
- Data-parallel execution for numeric kernels.

Representative instructions:
- `VADD`, `VSUB`, `VMUL`, `VDOT`
- `VLOAD`, `VSTORE`, `VSHUF`, `VSPLAT`
- `VEXTRACT`, `VINSERT`, `VCMPEQ`, `VCMPLT`, `VREDUCE`, `VFMA`

Register impact:
- Requires vector register file (`v0..v15`).

When to enable:
- Only after introducing language-level SIMD types/operations or auto-vectorizing backend.

## 2.3 Threading / Sync / Atomics / TLS Profile

Purpose:
- Native VM-level concurrency primitives.

Representative instructions:
- Threads: `THCREATE`, `THJOIN`, `THEXIT`, `THYIELD`, `THWAIT`
- Sync: `MUTEX*`, `COND*`, `SPIN*`, `BARR*`
- Atomics/TLS: `ATOM*`, `TLS*`

When to enable:
- If grammar/runtime introduces VM-managed threading semantics beyond host-runtime FFI calls.

## 2.4 Interrupt Profile

Purpose:
- Interrupt/trap style execution controls.

Representative instructions:
- `DI`, `EI`, `INT`, `IRET`, `SETINT`, `GETINT`, `MASKINT`, `UNMASKINT`

When to enable:
- Embedded/RT or VM hosting scenarios that need explicit interrupt handling model.

## 2.5 System/Debug Profile

Purpose:
- Low-level runtime and debugger integration.

Representative instructions:
- `SYSCALL`, `TRAP`, `DEBUG`, `RDCOUNT`, `BARRIER`, `BREAKPOINT`, `SINGLESTEP`, `GETREGS`, `SETREGS`, `GETFPOFF`

When to enable:
- Tooling-heavy workflows, VM introspection, low-level diagnostics.

## 2.6 Extended FFI Helpers

Purpose:
- Richer native interop orchestration beyond core bridge.

Representative instructions:
- `CALLHOSTV`, `PREPCALL`, `FINISHCA`, `FREELIB`
- pointer/function helpers: `I2PTR`, `PTR2I`, `REINTERP`, `ADDR2FUNC`, `FUNC2ADDR`

When to enable:
- If native ABI orchestration must be bytecode-visible rather than hidden inside runtime stubs.

## 3. Compatibility

- Extension opcodes must not alter core opcode semantics.
- Bytecode modules should declare required profile set (core + extension names).
- A VM implementation may reject modules requesting unsupported profiles.

## 4. Suggested Profile Names

- `core`
- `ext.string`
- `ext.simd`
- `ext.threading`
- `ext.interrupt`
- `ext.sysdebug`
- `ext.ffi.advanced`

## 5. Migration Guidance

If you later add grammar features that require these profiles:
1. Update `src/parsing/Hooc.g4`.
2. Update AST/builder/codegen lowering rules.
3. Add required instructions back to `docs/hvm/hvm_instruction_set.csv` under profile labels.
4. Expand `docs/hvm/hvm_register_set.csv` if profile requires new register classes.
5. Add VM capability checks and unit tests for profile-gated bytecode.
