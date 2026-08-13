# How HOModule Is Laid Out

**File:** `src/hvm/HOModule.h` / `HOModule.cpp` (~1,300 lines)

`HOModule` is the binary container format for compiled bytecode. It supports serialization to/from a binary representation with LE (little-endian) encoding.

## Relation To ELF

`HOModule` is not an ELF file. ELF is a general-purpose executable/object/shared-library format used by operating systems and native toolchains. `HOModule` is a Hoo-specific container for bytecode, metadata, relocations, exports, imports, and runtime-facing symbol records.

The two formats are similar in structure, but they solve different problems:

- ELF is a native ABI container for machine code and dynamic linking.
- HOModule is a language/runtime container for HVM bytecode and module loading.

### What They Have In Common

Both formats use a section-oriented layout:

- a header identifies the file and declares basic properties
- section tables describe payloads by type, size, offset, and flags
- symbol and relocation metadata support linking across compilation units
- string tables store names indirectly through offsets
- little-endian encoding is common in the current Hoo toolchain

That similarity is intentional. It makes HOModule familiar to anyone who has worked with object files, and it gives the loader a clean way to reason about code, data, and metadata separately.

### What Is Different

ELF supports features that HOModule does not try to model directly:

- program headers for OS loader segments
- native CPU machine code and CPU-specific relocations
- shared-library ABI conventions
- dynamic loader metadata such as GOT/PLT machinery
- platform-defined entry behavior for executable binaries

HOModule instead focuses on the Hoo compilation pipeline:

- `.text` contains HVM bytecode, not native instructions
- relocations refer to Hoo symbols and module sections, not OS loader segments
- exports/imports are resolved by the Hoo compiler and JIT, not by a system dynamic loader
- metadata such as `SHT_FUNCMETA` and `SHT_TYPES` exists because the runtime needs language-level information that ELF does not carry

### Why Not Use ELF Directly?

ELF is a good fit for native object code, but Hoo needs a format that stays stable across the HVM toolchain:

- the same module must be loadable as source-compiled bytecode or prebuilt bytecode
- the JIT must be able to inspect module metadata without depending on host linker internals
- the runtime needs language-specific records such as function metadata and type descriptors
- the format must remain readable by the HVM loader even when no native linker is involved

So HOModule borrows the general shape of an object file without depending on ELF as the actual runtime container.

## Section types

The `SectionType` enum (in `HOModule.h`) currently defines 24 `SHT_*` constants. The core executable/loader sections are:

| Constant | Purpose |
|---|---|
| `SHT_NULL` | Unused / null section |
| `SHT_TEXT` | Executable bytecode |
| `SHT_DATA` | Initialized mutable data |
| `SHT_RODATA` | Read-only data (string literals, type descriptors) |
| `SHT_BSS` | Zero-initialized data |
| `SHT_SYMTAB` | Symbol table (exported symbols) |
| `SHT_STRTAB` | String pool referenced by offsets |
| `SHT_RELOC` | Relocation entries for external symbols |
| `SHT_EXPORT` | Public API exports |
| `SHT_IMPORT` | External module dependencies |
| `SHT_FUNCMETA` | Function metadata (args, locals, stack size) |
| `SHT_TYPES` | Type descriptor records |
| `SHT_NOTE` | Annotations |
| `SHT_TLS` | Thread-local storage data |
| `SHT_DEBUG_*` | DWARF debug sections (line, info, abbrev, str, frame, loc, ranges, macinfo) |
| `SHT_GROUP` | Section groups |

The core eight (`TEXT`, `DATA`, `RODATA`, `SYMTAB`, `RELOC`, `EXPORT`, `IMPORT`, `FUNCMETA`) are what the loader resolves and what codegen emits by default; the remaining types are reserved for metadata, DWARF, and future use.

## Binary format

Each section is written as:

```
[4 bytes: section_type][4 bytes: section_size][section_size bytes: payload]
```

The module header contains:
- Magic number
- Version (major/minor)
- File type, target architecture, endianness, and pointer size
- Flags
- Entry point RVA and base address
- Number of sections

## Serialization

```cpp
bool HOModule::serialize(std::vector<uint8_t>& output) const;
bool HOModule::deserialize(const std::vector<uint8_t>& input);
```

- **Write** — Iterates sections, writes header, then each section's type, size, and data.
- **Read** — Reads header, then loops over sections, dispatching to section-specific deserializers.

## Dependency resolution

`HOModuleBase::resolveDependencyOrder()` receives the complete module set and
builds a name-to-module map. Each DFS visit resolves the current module from
that map and traverses its own `getDependencies()` list. Dependencies are
emitted before their dependents, so a chain such as `A -> B -> C` produces
`C, B, A`. Explicit `Visiting` and `Visited` states identify real cycles
without confusing transitive edges for cycles. `HVMModuleBundle` and the HVMJIT
loader use the same graph semantics; missing required modules are rejected by
loader resolution while optional missing modules do not add graph edges.

## Symbol encoding

Symbols in `SHT_SYMTAB` store:
- Mangled name string
- Section index
- Offset within section
- Size
- Linkage (local/global/weak)
