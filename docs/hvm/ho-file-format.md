# HVM Object File Format (HO)

Version: `1.6`
Extension: `.ho`  
Endianness: little-endian only

This document is the normative binary format for `src/hvm/HOModule.h` and `src/hvm/HOModule.cpp`.
It is aligned with the current hardware-ready core ISA profile in `docs/hvm/hvm-spec.md`.

## 1. File Kinds

- `0x01` Executable
- `0x02` Shared Object
- `0x03` Object File

## 2. Top-Level Layout

1. Fixed header (`64` bytes)
2. Section table (`40 * section_count` bytes)
3. Section payloads (aligned per section entry)

```
┌──────────────────────────────────┐
│ 0x00  Header (64 bytes)          │
├──────────────────────────────────┤
│ 0x40  Section table              │
│       (40 * section_count bytes) │
├──────────────────────────────────┤
│       Section payloads           │  <- each aligned to its entry's alignment
│       (.text, .rodata, .data,    │     (.symtab, .reloc, .export, .import,
│        .types, ...)              │      .funcmeta, .note, then .strtab last)
└──────────────────────────────────┘
```

`HOModule::serialize()` builds this layout; `HOModule::parse()` walks it back
with bounds and overflow checks (see §10).

## 3. Header (64 bytes)

All fields are little-endian.

| Offset | Size | Field | Description |
|---|---:|---|---|
| `0x00` | 4 | `magic` | `0x484F4F43` (mnemonic `"HOOC"`; on-disk LE bytes `43 4F 4F 48`) |
| `0x04` | 2 | `version_major` | format major (`1`) |
| `0x06` | 2 | `version_minor` | format minor (`6`) |
| `0x08` | 1 | `file_type` | executable/shared/object |
| `0x09` | 1 | `target_arch` | `0x00` x86_64 (legacy host), `0x01` arm64 (legacy host), `0x02` **HVM64** (native ISA / silicon), `0xFF` any |
| `0x0A` | 1 | `endianness` | must be `0x01` |
| `0x0B` | 1 | `pointer_size` | currently `8` |
| `0x0C` | 4 | `flags` | multiplexed feature mirror + module attribute bits (see §3.1) |
| `0x10` | 8 | `entry_point` | RVA for executables |
| `0x18` | 8 | `base_address` | preferred base |
| `0x20` | 8 | `section_count` | number of section table entries |
| `0x28` | 8 | `symtab_offset` | compat: serializer writes the byte offset just past the section table (start of the first payload); the parser ignores it and locates metadata via the section table |
| `0x30` | 4 | `symtab_entry_count` | symbol count |
| `0x34` | 4 | `reloc_count` | relocation count |
| `0x38` | 4 | `export_count` | export count |
| `0x3C` | 4 | `import_count` | import count |

Notes:
- Parser validates magic, header size, section table bounds, and little-endian mode.
- `symtab_offset` is written by serializer and kept for compatibility, but parser discovers metadata via section table.
- The four 32-bit counts at `0x30`-`0x3C` (symbol/reloc/export/import) are informational only; the parser derives entry counts from metadata section payload sizes.
- HVM 1.6 modules remain 64-bit. `pointer_size` must be `8` for native HVM64 code.
- HVM 1.5 readers must reject 1.6 modules unless they explicitly opt into forward-compatible parsing of unknown feature flags.

### 3.1 The `flags` Field

The 32-bit `flags` field is a multiplexed field:

- The **low 13 bits mirror the HVM required-feature bits** (`HVMFeature` enum) via `setRequiredFeatures()` / `addRequiredFeatures()`.
- The **same field also carries module attribute bits** (optimization level, PIE, stripped, type info, debug info) via `setOptimizationLevel()`, `setPIE()`, `setStripped()`, `setTypeInfo()`, `setDebugInfo()`.

The authoritative feature set for a serialized module lives in the `.note` section (see §7.6). The header mirror exists so silicon loaders that only inspect the fixed header can still see the MVP contract.

A loader must reject a module if any required feature bit is set and the target CPU/simulator does not expose the corresponding HVM feature.

#### 3.1.1 HVM Required Feature Bits

| Bit | Header mask | `HVMFeature` enum | Meaning |
|---:|---:|---|---|
| 0 | `0x00000001` | `HVM_C` | Module may contain HVM-C compressed encodings |
| 1 | `0x00000002` | `HVM_ARC` | Module may contain `RETAIN` or `RELEASE` |
| 2 | `0x00000004` | `HVM_ICACHE` | Module may contain `ICACHE.RNG` |
| 3 | `0x00000008` | `HVM_L` | Module may contain HVM-L hardware-loop instructions |
| 4 | `0x00000010` | `HVM_MEM` | Module may contain pair memory operations or memory hints |
| 5 | `0x00000020` | `HVM_V` | Module may contain HVM-V vector instructions |
| 6 | `0x00000040` | `HVM_A` | Module may contain HVM-A accelerator doorbell instructions |
| 7 | `0x00000080` | `HVM_Alloc` | Module may contain `ALLOC.BUMP` |
| 8 | `0x00000100` | `HVM_ObjRef` | Module expects compact object-reference runtime support |
| 9 | `0x00000200` | `HVM_Cap` | Module may contain capability/bounds-check instructions |
| 10 | `0x00000400` | `HVM_Prof` | Module may contain `RDPROF` |
| 11 | `0x00000800` | `HVM_NZ` | Module may contain null-checking load instructions |
| 12 | `0x00001000` | `HVM_RT` | Module is built for the deterministic RT subset |

`setRequiredFeatures()` mirrors these into `flags_ & 0x1FFF`. The same layout is repeated verbatim in the `.note` `requiredFeatures` descriptor.

#### 3.1.2 Module Attribute Bits

| Bits | Mask | Accessor | Meaning |
|---:|---:|---|---|
| 8-11 | `0x0F00` | `setOptimizationLevel()` / `getOptimizationLevel()` | 4-bit optimization level |
| 12 | `0x1000` | `setPIE()` / `isPIE()` | Position-independent executable / relocatable base |
| 13 | `0x2000` | `setStripped()` / `isStripped()` | Symbol/string metadata stripped |
| 14 | `0x4000` | `setTypeInfo()` / `hasTypeInfo()` | Type metadata present |
| 15 | `0x8000` | `setDebugInfo()` / `hasDebugInfo()` | Debug metadata present |

> Implementation note: bits 8-12 are shared between the feature mirror (§3.1.1) and the attribute bits. Because both encodings multiplex the same field, a module that sets both an optimization level (or `PIE`) and HVM features 8-12 cannot be distinguished in the header alone. Silicon loaders must treat the `.note` section as the authoritative feature source. Bits 16-31 are currently unused.

### 3.2 Construction Defaults

`HOModule` is constructed with these defaults (see `HOModule::HOModule()`):

| Field | Default |
|---|---|
| `version_major` / `version_minor` | `1` / `6` |
| `file_type` | `ObjectFile` (`0x03`) |
| `target_arch` | `Any` (`0xFF`) |
| `endianness` | `Little` (`0x01`) |
| `pointer_size` | `8` |
| `flags` | `0` |
| `entry_point` / `base_address` | `0` |
| string pool | single NUL byte (offset 0 = empty string) |

The compiler front-end (`HVMCodeGenerator`) creates a module per compilation
unit and immediately calls `setTargetArch(TargetArch::HVM64)`. Producers that
build an executable then call `setFileType(FileType::Executable)` and
`setEntryPoint(...)`.

## 4. Section Entry (40 bytes)

Each section table entry is exactly `40` bytes.

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 8 | `name_offset` (offset into `.strtab`) |
| `0x08` | 4 | `section_type` |
| `0x0C` | 4 | `flags` |
| `0x10` | 8 | `virtual_size` |
| `0x18` | 8 | `file_offset` (`0` for BSS/no payload) |
| `0x20` | 8 | `alignment` |

## 5. Section Types

These IDs match `enum class SectionType` in `src/hvm/HOModule.h`.

| ID | Name |
|---:|---|
| `0x01` | `SHT_NULL` |
| `0x02` | `SHT_TEXT` |
| `0x03` | `SHT_RODATA` |
| `0x04` | `SHT_DATA` |
| `0x05` | `SHT_BSS` |
| `0x06` | `SHT_SYMTAB` |
| `0x07` | `SHT_STRTAB` |
| `0x08` | `SHT_RELOC` |
| `0x09` | `SHT_EXPORT` |
| `0x0A` | `SHT_IMPORT` |
| `0x0B` | `SHT_FUNCMETA` |
| `0x0C` | `SHT_TYPES` |
| `0x0D` | `SHT_NOTE` |
| `0x0E` | `SHT_TLS` |
| `0x0F` | `SHT_DEBUG_LINE` |
| `0x10` | `SHT_DEBUG_INFO` |
| `0x11` | `SHT_DEBUG_ABBREV` |
| `0x12` | `SHT_DEBUG_STR` |
| `0x13` | `SHT_DEBUG_FRAME` |
| `0x14` | `SHT_DEBUG_LOC` |
| `0x15` | `SHT_DEBUG_RANGES` |
| `0x16` | `SHT_DEBUG_MACINFO` |
| `0x17` | `SHT_GROUP` |

## 6. Section Flags

Bit masks from `SectionFlags`:

- `0x8000` `TLS`
- `0x4000` `ALLOC`
- `0x2000` `WRITE`
- `0x1000` `EXECUTE`
- `0x0800` `MERGE`
- `0x0400` `STRINGS`
- `0x0200` `EXCLUDE`
- `0x0100` `COMPRESSED`

## 7. Metadata Table Entry Layouts

### 7.1 Symbol (`32` bytes)

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 4 | name offset (`.strtab`) |
| `0x04` | 1 | binding |
| `0x05` | 1 | type |
| `0x06` | 1 | visibility |
| `0x07` | 1 | reserved |
| `0x08` | 8 | value |
| `0x10` | 8 | size |
| `0x18` | 4 | section index (signed) |
| `0x1C` | 4 | symbol index |

### 7.2 Relocation (`16` bytes)

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 8 | offset |
| `0x08` | 4 | symbol index |
| `0x0C` | 2 | relocation type |
| `0x0E` | 2 | addend (signed) |

### 7.3 Export (`24` bytes)

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 4 | name offset |
| `0x04` | 4 | symbol index |
| `0x08` | 8 | address |
| `0x10` | 8 | size |

### 7.4 Import (`32` bytes)

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 4 | name offset |
| `0x04` | 4 | library name offset |
| `0x08` | 4 | import type |
| `0x0C` | 4 | version |
| `0x10` | 8 | flags |
| `0x18` | 8 | resolved address |

### 7.5 Function Metadata (`48` bytes)

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 4 | name offset |
| `0x04` | 4 | symbol index |
| `0x08` | 8 | entry RVA |
| `0x10` | 4 | code size |
| `0x14` | 4 | local size |
| `0x18` | 4 | param count |
| `0x1C` | 4 | param types offset |
| `0x20` | 4 | return type offset |
| `0x24` | 4 | flags |
| `0x28` | 4 | source line |
| `0x2C` | 4 | debug offset |

### 7.6 Note Section (`.note`) — HVM Feature Flags

The serializer always emits a `.note` section (`SHT_NOTE`, flags `ALLOC`, alignment `8`). When `requiredFeatures_` is non-zero it carries a single ELF-style NOTE record:

| Offset | Size | Field |
|---|---:|---|
| `0x00` | 4 | `n_namesz` — `4` |
| `0x04` | 4 | `n_descsz` — `8` |
| `0x08` | 4 | `n_type` — `1` (HVM feature flags) |
| `0x0C` | 4 | name `"HVM\0"` (4-byte aligned) |
| `0x10` | 8 | `requiredFeatures` (`uint64_t`, LE) — same bit layout as §3.1.1 |

Total payload is 24 bytes; `virtual_size` = 24. When no features are required the section is still emitted but empty (`virtual_size` 0, no payload).

The parser restores `requiredFeatures_` only if the note has `n_namesz >= 4`, `n_descsz >= 8`, `n_type == 1`, and name `"HVM"`; the descriptor is read from the 4-byte-aligned offset following the name.

### 7.7 Symbol, Visibility, and Import Constants

`.symtab` entries use these byte values (matching the `Symbol` constants in `HOModule.h`):

| Field | Value | Constant | Meaning |
|---|---|---:|---|---|
| binding | 0 | `STB_LOCAL` | local symbol |
| binding | 1 | `STB_GLOBAL` | global symbol |
| binding | 2 | `STB_WEAK` | weak symbol |
| type | 0 | `STT_NOTYPE` | no type |
| type | 1 | `STT_FUNC` | function |
| type | 2 | `STT_OBJECT` | data object |
| type | 3 | `STT_TYPE` | Hoo type record |
| type | 4 | `STT_TLS` | thread-local storage |
| visibility | 0 | `STV_DEFAULT` | default visibility |
| visibility | 1 | `STV_INTERNAL` | internal visibility |
| visibility | 2 | `STV_HIDDEN` | hidden visibility |
| visibility | 3 | `STV_PROTECTED` | protected visibility |

`.import` entries use `import_type`:

| Value | Constant | Meaning |
|---:|---|---|
| 1 | `IT_HOOC` | import resolved from a Hoo module |
| 2 | `IT_NATIVE` | import resolved from a native shared library |
| 3 | `IT_RUNTIME` | import resolved from the HVM runtime |
| 4 | `IT_INTRINSIC` | intrinsic imported into JIT-compiled code |

## 8. String Table Rules (`.strtab`)

- First byte is `NUL` (`'\0'`) so offset `0` is valid empty string.
- Names in section table and metadata entries are offsets into `.strtab`.
- If a section name lookup fails, parser may fall back to built-in defaults for known section types.

## 9. Serialization Behavior (Current Implementation)

- Serializer auto-generates metadata sections from in-memory vectors:
  - `.symtab`, `.reloc`, `.export`, `.import`, `.funcmeta`, `.strtab`
- User-supplied payload bytes for those metadata sections are rejected.
- Non-BSS sections with payload are laid out at aligned offsets and copied into file.

### 9.1 Section Emit Order

Sections are serialized in this order:

1. User sections, in insertion order.
2. Auto-generated tables, each emitted **only when its vector is non-empty**: `.symtab`, `.reloc`, `.export`, `.import`, `.funcmeta` (all with alignment `8`).
3. `.note` — always emitted (empty when no required features).
4. `.strtab` — always emitted, last (alignment `1`, flags `ALLOC | STRINGS`).

`.strtab` concatenates section names, symbol names, export names, import names (symbol and library), and function-metadata names. Its own name is appended to the pool as well. Offset 0 is always the NUL byte, so offset 0 is the empty string.

### 9.2 Layout Rules

- Section table entries are fixed 40-byte records; `section_count` is an 8-byte field.
- Payloads start immediately after the section table and are aligned to each section's `alignment` (minimum 1).
- BSS sections and sections with an empty payload are written with `file_offset = 0` and no bytes.
- Metadata tables use fixed record sizes (symbol 32, relocation 16, export 24, import 32, funcmeta 48); the header counts at `0x30`-`0x3C` are updated to match.

### 9.3 Primitive Encoding (Serializer Internals)

- All integers are written little-endian with fixed widths: `u16` at 2 bytes, `u32` at 4 bytes, `u64` at 8 bytes; signed `i16`/`i32` are two's-complement in the same widths.
- `serialize()` rejects any non-little-endian module up front (`endianness_ != Endianness::Little`).
- Every size computation is overflow-checked: `willAddOverflow()` / `willMulOverflow()` guard the table and payload sizing, and `alignUpChecked()` implements `round_up(value, alignment)` for the placement pass (an `alignment` of `0` or `1` passes through unchanged).
- The header buffer is `resize`d to 64 zero bytes before any field is written, so unused bytes are deterministic.
- Payload placement keeps a single running `data_offset`. Each non-BSS section with a non-empty payload is placed at `align_up(data_offset, max(1, alignment))`, the file is padded to that offset, the payload is appended, and `data_offset` advances to the end of the payload.

## 10. Parsing and Validation Guarantees

`HOModule` parser rejects:

- invalid magic
- non-little-endian files
- truncated header/section table
- overflowed table sizes
- out-of-range section payload spans
- malformed metadata section sizes (not divisible by entry size)
- duplicate metadata sections of the same type (`.symtab`, `.reloc`, `.export`, `.import`, `.funcmeta`)

The parser also:

- falls back to a default name for unnamed sections: `.text`, `.rodata`, `.data`, `.bss`, `.symtab`, `.strtab`, `.reloc`, `.export`, `.import`, `.funcmeta`, `.note`
- normalizes the string pool so offset 0 is a NUL byte
- recovers `requiredFeatures_` from the `.note` record (see §7.6)
- keeps `flags_` as the raw header value (feature mirror + attribute bits)

Parser hardening:

- Every `read*` helper bounds-checks `offset + size <= data.size()` before any `memcpy`, so no read can overrun the input buffer.
- `parseSectionTable()` rejects `section_count` values whose table span would overflow `size_t` or exceed the file (checked multiply and add).
- Payload loads verify `file_offset <= size` and `virtual_size <= size - file_offset` before copying.
- Metadata decoders require the section payload length to be an exact multiple of the fixed record size.
- `.symtab`, `.reloc`, `.export`, `.import`, and `.funcmeta` sections are looked up by type, and a second section of the same type is rejected by both `parse()` and `deserialize()`.

## 11. Relationship to HVM ISA

- `.text` carries encoded HVM instructions.
- HVM 1.6 `.text` may contain base32 and escape32 instructions from `docs/hvm/hvm_instruction_set.csv`.
- Optional v1.6 extensions must be reflected in the header `flags` field.
- Supported language/runtime surface is defined by:
  - `docs/hvm/hvm-spec.md`
  - `docs/hvm/hvm_instruction_set.csv`
  - `docs/hvm/hvm_register_set.csv`
- This file-format spec intentionally does not re-list opcode families to avoid drift.

### 11.1 `.text` Payload Encoding

`.text` is an opaque byte stream produced by `HOModule::encodeInstructions()`
and consumed by `HOModule::decodeInstructions()`:

- **base32** — the default 32-bit little-endian instruction word, packed by
  instruction format (R / I / B / J / RI) with 5-bit register fields and a
  `func` / immediate field. `HVMInstruction::encode32()` produces the word.
- **escape32** — instructions whose first byte is `0xFE`
  (`kExtendedOpcodeEscape`) escape to an extended 32-bit encoding for opcodes
  outside the base set.
- **HVM-C compressed (16-bit)** — when the first byte is not the escape byte and
  its high nibble is `0xF` (`(firstByte & 0xF0) == 0xF0`), the instruction is a
  2-byte compressed form: 4-bit opcode, 4-bit immediate, and two 4-bit
  registers. Such instructions may appear in `.text` only when the `HVM_C`
  feature bit is advertised.
- **ULEB128 varints** — some operands use `encodeULEB128` / `decodeULEB128`
  (max 5 bytes).

`decodeInstructions()` walks the stream with `HVMInstruction::decode(bytes,
bytesUsed)`, advancing by `bytesUsed` per instruction and stopping on the first
undecodable byte (guarding against an infinite loop). `instructionsToAssembly()`
and `parseAssembly()` provide a textual round-trip used by tools and tests.

### 11.2 Producing and Consuming `.ho` Files

Producers:

- `HVMCodeGenerator` creates one `HOModule` per compilation unit, calls
  `setTargetArch(TargetArch::HVM64)`, appends instruction bytes to an
  incremental `.compcode` buffer (`SHT_TEXT`, `ALLOC | EXECUTE`), and on
  finalize appends: a `.text` section with the encoded stream, `.data` /
  `.rodata` payloads, a `.types` section (`SHT_TYPES`, `ALLOC`) holding
  NUL-terminated type-descriptor strings, and `.symtab` / `.funcmeta` entries.
  It calls `addRequiredFeatures()` for any feature the unit uses (for example
  `HVM_NZ`, `HVM_ARC`, `HVM_ICACHE`, `HVM_MEM`).
- `HOModule::serialize()` then lays out the file exactly as described in §2-§9.

Consumers:

- `HooArchiveCompiler` / `HooArchiveLoader` package each serialized module as
  `modules/<name>.ho` inside a `.ha` archive and re-parse it with
  `HOModule::parse()` (see `docs/hvm/ha-archive-format.md`).
- `HVMJIT::parseAndLoadModuleFromPath()` / `validateModule()` re-parse a `.ho`
  and validate: magic and version exactly `1.6`, little-endian, `pointer_size`
  `8`, a non-empty `.text` present with `ALLOC | EXECUTE`, `.text`
  `virtual_size` not smaller than its payload, every `STT_FUNC` symbol offset
  within `.text`, and optional `.data` / `.rodata` sections.

## 12. Comparison With Other Formats

HO borrows its section/symbol/relocation vocabulary from ELF and its
VM-targeted, ISA-neutral container philosophy from the JVM `.class` file. As a
container it also parallels the .NET assembly; for the JAR / `.ha` archive
comparison, see `docs/hvm/ha-archive-format.md`. The subsections below map each
format onto HO.

### 12.1 At A Glance

| Concept | HO (this spec) | ELF | COFF `.obj` / Mach-O `.o` | Java `.class` |
|---|---|---|---|---|
| Magic | `0x484F4F43` (`"HOOC"`; LE bytes `43 4F 4F 48`) | `0x7F` `"ELF"` | COFF: machine field in header; Mach-O: `0xfeedfacf` (LE) | `0xCAFEBABE` |
| Byte order | little-endian only | `EI_DATA`: `1` LE / `2` BE | COFF: LE; Mach-O: host byte order | big-endian only |
| Header | fixed 64 bytes | 52 B (32-bit) / 64 B (64-bit) | COFF 20 B; Mach-O `mach_header_64` 32 B | 10 B fixed + variable-length constant pool |
| Target | ISA-neutral HVM64 (`target_arch`) | native CPU (`e_machine`) | native CPU (machine field) | ISA-neutral JVM |
| Code | `.text` section | `.text` section + program segments | `.text` section | `Code` attribute per method |
| Names / strings | single `.strtab` pool | `.strtab`, `.shstrtab`, `.dynstr` | COFF string table (+`.strtab`-like); Mach-O string tables | typed `constant_pool` (UTF-8, refs, numerics) |
| Symbols | `.symtab`, 32 B fixed records | `.symtab` / `.dynsym`, 24/16 B | COFF 18 B; Mach-O `nlist_64` 16 B | none — refs via constant pool |
| Relocations | `.reloc`, 16 B (with addend) | `.rela` / `.rel`, 24/16 B | COFF 10 B; Mach-O relocs in section headers | none — JVM resolves lazily |
| Exports / imports | dedicated `.export` / `.import` tables | `.dynamic`, `.dynsym`, hash tables | imports via relocations + binding | access flags + JVM linkage |
| Required features | header `flags[12:0]` + `.note` | ABI/ISA tags, GNU property `.note` | machine + subsystem fields | class-file major/minor version |
| Debug | `.debug_*` (DWARF-named) | DWARF `.debug_*` | COFF CodeView; DWARF | `LineNumberTable`, `SourceDebugExtension` |
| Function signature data | `.funcmeta` (params, return, source line) | symbol table + relocations | symbol table + relocations | method descriptor + `Signature` attribute |
| Link model | static link (object) or runtime load (executable/shared) | ET_REL link, ET_EXEC/ET_DYN load | always a relocatable unit | always runtime class load |

### 12.2 HO vs Java `.class`

Both are ISA-neutral containers aimed at a VM, deferring native code generation
to a JIT. The differences are structural:

- **Header.** `.class` has a fixed 10-byte prefix (`magic`, `minor_version`,
  `major_version`) followed by a *variable-length* constant pool. HO has a fixed
  64-byte header followed by a *fixed-record* section table. `.class` is a tree
  of variable-length structures; HO is a flat table.
- **Constant pool.** `.class` uses a rich, typed pool (`CONSTANT_Utf8`,
  `CONSTANT_Methodref`, `CONSTANT_Fieldref`, numeric constants, ...) addressed by
  1-based indices with a `tag` byte per entry. HO's `.strtab` is a plain
  NUL-terminated name pool; the semantics live in `.symtab`, `.funcmeta`,
  `.export`, and `.import` records that reference it.
- **Linking.** `.class` has no symbol table, no relocations, and no import/export
  tables. The JVM resolves symbolic references lazily during class loading and
  verification. HO modules carry explicit symbol, relocation, export, and import
  tables, so they can be statically linked (like an object file) or loaded by the
  HVM runtime directly.
- **Methods and attributes.** `.class` encodes each method with a `Code`
  attribute (bytecode, exception table, `LineNumberTable`, `StackMapTable`).
  HO uses a fixed 48-byte `.funcmeta` record per function (entry RVA, code size,
  local size, param/return type offsets, flags, source line, debug offset) plus
  `.debug_*` sections.
- **Types.** `.class` encodes field/method descriptors as UTF-8 strings (e.g.
  `(Ljava/lang/String;)V`). HO uses type offsets in `.funcmeta`, `STT_TYPE`
  symbols, and the `SHT_TYPES` section.
- **Byte order.** `.class` is big-endian; HO is little-endian.
- **Versioning.** Both encode major/minor in the header; HO is currently `1.6`.

### 12.3 HO vs ELF

ELF is HO's closest structural relative — HO deliberately reuses its
section/symbol/relocation vocabulary.

- **Section model.** ELF 64-bit section headers are 64 bytes (`sh_name`,
  `sh_type`, `sh_flags`, `sh_addr`, `sh_offset`, `sh_size`, `sh_link`,
  `sh_info`, `sh_addralign`, `sh_entsize`). HO's 40-byte entry keeps name, type,
  flags, `virtual_size`, `file_offset`, `alignment`, and drops `sh_addr`,
  `sh_link`/`sh_info` cross-references, and `sh_entsize` — record sizes are fixed
  and implicit per type instead.
- **Program headers.** ELF `ET_EXEC`/`ET_DYN` files add program headers for the
  OS loader and dynamic linking. HO has no program headers; loading is driven by
  the HVM loader/runtime, not the OS kernel.
- **Symbols.** `Elf64_Sym` is 24 bytes (name, packed `st_info` bind/type,
  `st_other` visibility, `st_shndx`, `st_value`, `st_size`). HO's symbol is 32
  bytes with explicit binding/type/visibility bytes plus a `symbol_index` field.
  ELF reserves symbol index 0 as a null symbol; HO does not.
- **Relocations.** ELF has `Elf64_Rel` (16 B, implicit addend in-place) and
  `Elf64_Rela` (24 B, explicit addend). HO has a single 16-byte form with a
  2-byte addend.
- **String tables.** ELF keeps `.strtab`, `.shstrtab`, and `.dynstr` separate.
  HO uses one `.strtab` for section names and all metadata names.
- **Notes.** ELF `.note` records (`n_namesz`/`n_descsz`/`n_type`/name/desc) map
  directly onto HO's `.note` feature record (§7.6). ELF ABI-tag and GNU property
  notes fill the same "required features" role.
- **Dynamic linking.** ELF uses `.dynamic`, `.dynsym`, `.gnu.hash`/`.hash`,
  `.rela.dyn`, `.rela.plt` with symbol versioning. HO's `.import`/`.export`
  tables plus `.reloc` cover the same ground more simply: an import record
  carries its type (`IT_HOOC`/`IT_NATIVE`/`IT_RUNTIME`/`IT_INTRINSIC`) and
  version directly.
- **Debug.** Both use DWARF-style sections; HO reserves the `.debug_line`,
  `.debug_info`, `.debug_abbrev`, `.debug_str`, `.debug_frame`, `.debug_loc`,
  `.debug_ranges`, and `.debug_macinfo` section types.
- **Architecture.** ELF relocations are CPU/ABI-specific; HO targets the
  ISA-neutral HVM64, so relocation types are VM-level.

### 12.4 HO vs Object Files (ELF `.o`, COFF `.obj`, Mach-O `.o`)

An object file is a relocatable unit that still needs linking, which is exactly
what HO's `ObjectFile` file kind models.

- **Relocation model.** All three combine a symbol table with relocation records
  that a linker patches into code/data slots. HO `.reloc` (offset, symbol index,
  type, addend) maps to ELF `.rela` (`r_offset`, `r_info`, `r_addend`), COFF
  relocations (`VirtualAddress`, `SymbolTableIndex`, `Type`), and Mach-O
  relocations (address, symbol index, packed type/meta). COFF differs by storing
  relocations in the section header (`PointerToRelocations`,
  `NumberOfRelocations`); ELF, Mach-O, and HO use dedicated relocation
  sections/entries.
- **Section split.** ELF/COFF/Mach-O divide code into `.text`, read-only data
  into `.rodata`/`.rdata`, mutable data into `.data`, and zeroed storage into
  `.bss`. HO uses the same `SHT_TEXT`/`SHT_RODATA`/`SHT_DATA`/`SHT_BSS` split,
  and BSS sections carry no payload bytes.
- **Common symbols.** COFF and ELF support common/`COMMON` symbols for
  tentative definitions; HO has no common-symbol model.
- **Debug.** COFF uses CodeView, ELF uses DWARF, Mach-O uses DWARF (plus dSYM
  for executables). HO reserves DWARF-named debug sections.
- **ISA/ABI.** COFF `.obj` is per-machine (`IMAGE_FILE_MACHINE_AMD64`/`ARM64`),
  ELF `.o` is per-machine+ABI (`e_machine`, `e_flags`), Mach-O `.o` is
  per-CPU/CPU-subtype. HO is ISA-neutral: `target_arch` is an annotation
  (`HVM64`/`Any`), so a single `.ho` can serve any HVM64 implementation.
- **Exports.** In ELF/COFF an exported symbol is just a global binding; in HO the
  `.export` table additionally records an address and size for fast symbol
  resolution, closer to a PE `.edata` or Mach-O export trie in purpose.

### 12.5 HO vs .NET Assemblies

A .NET assembly is a PE/COFF file extended with CLR metadata — the closest
"managed object file" analogue to HO among VM-targeted formats.

| Concept | .NET assembly | HO |
|---|---|---|
| Outer container | PE/COFF (DOS header, COFF header, optional header, section headers) | fixed 64-byte header + section table |
| Managed code | IL bodies in `.text`, addressed by RVA from `MethodDef` rows | HVM bytecode in `.text`, addressed by symbol / `.funcmeta.entry_rva` |
| Metadata | CLI header (`IMAGE_COR20_HEADER`) + metadata root with streams `#~`, `#Strings`, `#US`, `#Blob`, `#GUID` | section table with `.symtab`, `.funcmeta`, `.types`, `.strtab`, `.note` |
| Names | `#Strings` heap (NUL-terminated UTF-8) | `.strtab` pool (NUL-terminated) |
| Type signatures | `#Blob` signatures (length-prefixed, compressed) | `.types` descriptor strings + `STT_TYPE` symbols |
| References | metadata tokens (4-byte table id + row index) resolved by the JIT | symbol indices / string offsets resolved by loader or JIT |
| Entry point | `EntryPointToken` in the CLI header | header `entry_point` RVA or manifest entry point |
| Versioning | runtime version in CLI header; assembly version attributes | header `version_major` / `version_minor` (`1.6`) |
| Relocations | none for managed code (JIT resolves tokens); PE `.reloc` only for native/mixed | explicit `.reloc` table for static linking |
| Features / safety | CLI header flags (ILOnly, 32BitRequired, StrongNameSigned) | `flags` field + `.note` required features |

Key differences:

- .NET metadata lives in **compressed, self-describing tables** (`#~` rows whose
  widths depend on heap sizes), so a reader must size each table from the
  metadata header before reading. HO uses **fixed-record** tables whose entry
  sizes are constant and implicit per section type.
- .NET method bodies are referenced by RVA from metadata rows; HO mirrors this
  with `.funcmeta.entry_rva` plus explicit `.symtab` symbols.
- .NET embeds everything — code, metadata, resources, native stubs — in one PE
  file; HO keeps the equivalent data in dedicated sections but stays a single
  flat file and relies on the `.ha` archive for multi-module distribution.
- Strong-name / Authenticode signing have no HO counterpart yet; integrity is
  handled at the archive level via SHA-256.
