# How HOModule Is Laid Out

**File:** `src/module/HOModule.cpp` (~1200 lines)

`HOModule` is the binary container format for compiled bytecode. It supports serialization to/from a binary representation with LE (little-endian) encoding.

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
