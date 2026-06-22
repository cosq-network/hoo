# How HOModule Is Laid Out

**File:** `src/module/HOModule.cpp` (~1200 lines)

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

| Constant | Purpose |
|---|---|
| `SHT_TEXT` | Executable bytecode |
| `SHT_DATA` | Initialized mutable data |
| `SHT_RODATA` | Read-only data (string literals, type descriptors) |
| `SHT_SYMTAB` | Symbol table (exported symbols) |
| `SHT_RELOC` | Relocation entries for external symbols |
| `SHT_EXPORT` | Public API exports |
| `SHT_IMPORT` | External module dependencies |
| `SHT_FUNCMETA` | Function metadata (args, locals, stack size) |

## Binary format

Each section is written as:

```
[4 bytes: section_type][4 bytes: section_size][section_size bytes: payload]
```

The module header contains:
- Magic number
- Version
- Entry point offset
- Number of sections

## Serialization

```cpp
void HOModule::serialize(Buffer &buf) const;
void HOModule::deserialize(const Buffer &buf);
```

- **Write** — Iterates sections, writes header, then each section's type, size, and data.
- **Read** — Reads header, then loops over sections, dispatching to section-specific deserializers.

## Symbol encoding

Symbols in `SHT_SYMTAB` store:
- Mangled name string
- Section index
- Offset within section
- Size
- Linkage (local/global/weak)
