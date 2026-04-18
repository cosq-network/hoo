# HVM Object File Format Specification (HO)

**Version:** 1.2
**File Extension:** `.ho`
**Endianness:** Little-endian

---

## Table of Contents

1. [Overview](#1-overview)
2. [Design Principles](#2-design-principles)
3. [File Layout](#3-file-layout)
4. [Header Specification](#4-header-specification)
5. [Section Table](#5-section-table)
6. [Section Types](#6-section-types)
7. [Symbol Table](#7-symbol-table)
8. [Relocation Table](#8-relocation-table)
9. [Import/Export Tables](#9-importexport-tables)
10. [String Pool](#10-string-pool)
11. [Function Metadata](#11-function-metadata)
12. [Type Information](#12-type-information)
13. [File Type Variants](#13-file-type-variants)
14. [Loading and Linking](#14-loading-and-linking)
15. [Usage Examples](#15-usage-examples)
16. [Appendix: OpCodes Reference](#16-appendix-opcodes-reference)

---

## 1. Overview

The `.ho` (Hooc Object) format is a binary file format for the Hooc Virtual Machine (HVM) instruction set architecture. It supports three file types:

| Type | Flag | Extension | Purpose |
|------|------|-----------|---------|
| **Executable** | `0x01` | `.ho` | Standalone program with entry point |
| **Shared Object** | `0x02` | `.ho` | Dynamically linkable library |
| **Object File** | `0x03` | `.ho` | Statically linkable relocatable |

### Design Goals

- **Simple**: Minimal complexity for fast JIT loading
- **Portable**: Defined semantics independent of host CPU
- **Extensible**: Versioned header with optional sections
- **Linkable**: Full support for static and dynamic linking

---

## 2. Design Principles

### 2.1 Memory Model

```
HVM Virtual Address Space
┌─────────────────────────────────────────────────────────────┐
│  0x0000_0000_0000_0000  │  Reserved (null pointer trap)     │
├─────────────────────────────────────────────────────────────┤
│  0x0000_0000_0001_0000  │  Text Section (code)              │
│                         │  - HVM instructions              │
│                         │  - Grows upward                 │
├─────────────────────────────────────────────────────────────┤
│                         │  Read-Only Data (rodata)         │
│                         │  - Constants                     │
│                         │  - String literals               │
├─────────────────────────────────────────────────────────────┤
│                         │  Read-Write Data (data)          │
│                         │  - Global variables              │
│                         │  - Tables (vtable, itable)       │
├─────────────────────────────────────────────────────────────┤
│                         │  BSS (uninitialized)             │
│                         │  - Zero-initialized globals      │
├─────────────────────────────────────────────────────────────┤
│                         │  Heap (runtime allocation)       │
├─────────────────────────────────────────────────────────────┤
│                         │  Call Stack                      │
│                         │  - Grows downward               │
├─────────────────────────────────────────────────────────────┤
│  0xFFFF_FFFF_FFFF_FFFF  │  Reserved (guard page)           │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Data Representation

| HVM Type | Size | Description |
|----------|------|-------------|
| `i8` | 1 | Signed 8-bit integer |
| `i16` | 2 | Signed 16-bit integer |
| `i32` | 4 | Signed 32-bit integer |
| `i64` | 8 | Signed 64-bit integer |
| `f32` | 4 | 32-bit IEEE 754 float |
| `f64` | 8 | 64-bit IEEE 754 double |
| `v128` | 16 | 128-bit vector (SIMD) |
| `ref` | 8 | Reference (pointer) |

### 2.3 Data Encoding

#### 2.3.1 Little-Endian Base Types

All multi-byte integers are stored in little-endian byte order:
```
Value 0x0123456789ABCDEF stored as: EF CD AB 89 67 45 23 01
```

#### 2.3.2 LEB128 Encoding (Variable-Length Integer)

LEB128 (Little Endian Base 128) is used in debug sections to reduce size.

**Unsigned LEB128 (ULEB128):**
```
Value bits:  [xxxxxxx]
Encoded:     [1xxxxxxx]                    (7 bits per byte)

Value bits:  [xxxxxxx][yyyyyyy]
Encoded:     [1xxxxxxx][0yyyyyyy]        (14 bits in 2 bytes)

Value bits:  [xxxxxxx][yyyyyyy][zzzzzzz]
Encoded:     [1xxxxxxx][1yyyyyyy][0zzzzzzz] (21 bits in 3 bytes)
```

**Encoding Algorithm:**
```
Encode ULEB128(value):
    result = []
    while true:
        byte = value & 0x7F
        value >>= 7
        if value != 0:
            byte |= 0x80  // Set continuation bit
        result.append(byte)
        if value == 0:
            break
    return result
```

**Examples:**
| Value | ULEB128 Bytes |
|-------|---------------|
| 0 | 0x00 |
| 1 | 0x01 |
| 127 | 0x7F |
| 128 | 0x80 0x01 |
| 300 | 0xAC 0x02 |
| 16384 | 0x80 0x80 0x01 |

**Signed LEB128 (SLEB128):**
Same as ULEB128 but sign-extended to handle negative numbers:
```
Value bits:  [Sxxxxxxx] (S = sign bit)
Encoded:     [1Sxxxxxxx] (continue if S differs from higher bits)
```

**Encoding Algorithm:**
```
Encode SLEB128(value):
    result = []
    while true:
        byte = value & 0x7F
        value >>= 7
        // Sign-extend the value
        if (value == 0 and (byte & 0x40) == 0) or
           (value == -1 and (byte & 0x40) != 0):
            // Done - no more bytes needed
            result.append(byte)
            break
        else:
            byte |= 0x80  // Set continuation bit
            result.append(byte)
    return result
```

**Examples:**
| Value | SLEB128 Bytes |
|-------|---------------|
| 0 | 0x00 |
| 1 | 0x01 |
| -1 | 0x7F |
| 127 | 0x7F |
| -128 | 0x80 0x7F |
| 300 | 0xAC 0x02 |
| -300 | 0x74 0xFB 0xFF |

### 2.4 Object Layout

#### HVM Objects (Runtime)

```
┌──────────────────────────────────────────┐
│  Object Header (16 bytes)                │
├──────────────────────────────────────────┤
│  vtable pointer (8 bytes)                │
│  - Points to virtual method table        │
├──────────────────────────────────────────┤
│  refcount (8 bytes)                      │
│  - Atomic reference counter              │
└──────────────────────────────────────────┘
│  Instance Fields (variable size)        │
│  - Aligned to 8 bytes                    │
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│  vtable (per-class, read-only)          │
├──────────────────────────────────────────┤
│  class_id (4 bytes)                     │
│  size (4 bytes)                         │
│  destructor (8 bytes, optional)         │
│  ...                                    │
│  methods[] (8 bytes each)               │
│  - Function pointers                     │
└──────────────────────────────────────────┘
```

#### HVM Arrays

```
┌──────────────────────────────────────────┐
│  Array Header (24 bytes)                 │
├──────────────────────────────────────────┤
│  vtable pointer (8 bytes)               │
│  refcount (8 bytes)                      │
│  length (4 bytes)                        │
│  element_size (4 bytes)                  │
├──────────────────────────────────────────┤
│  Elements[] (variable size)             │
│  - Contiguous memory                     │
│  - Aligned to element alignment         │
└──────────────────────────────────────────┘
```

#### HVM Strings

```
┌──────────────────────────────────────────┐
│  String Header (24 bytes)                │
├──────────────────────────────────────────┤
│  vtable pointer (8 bytes)               │
│  refcount (8 bytes)                      │
│  length (4 bytes)                        │
│  hash (4 bytes, cached)                  │
├──────────────────────────────────────────┤
│  UTF-8 bytes[] (null-terminated)        │
│  - Variable length                       │
└──────────────────────────────────────────┘
```

---

## 3. File Layout

### 3.1 Overall Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                         HO FILE                                 │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    FILE HEADER (64 bytes)                  ││
│  └─────────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────────┐│
│  │               SECTION TABLE (12 bytes × N)                 ││
│  │  [Entry 0] [Entry 1] ... [Entry N-1]                        ││
│  └─────────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                     SECTION DATA                           ││
│  │  Sections appear in the order they are defined in table    ││
│  │  Each section is aligned to 8 bytes                        ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Section Alignment Rules

| Section Type | Alignment | Notes |
|--------------|-----------|-------|
| `.text` | 16 | Code must be 16-byte aligned for JIT |
| `.rodata` | 8 | Read-only constants |
| `.data` | 8 | Writable globals |
| `.bss` | 8 | Zero-initialized (no data in file) |
| `.symtab` | 8 | Symbol table |
| `.reloc` | 8 | Relocation entries |
| `.export` | 8 | Exported symbols |
| `.import` | 8 | Imported symbols |
| `.strtab` | 1 | String pool (no alignment) |
| `.funcmeta` | 8 | Function metadata |
| `.types` | 8 | Type information |
| `.debug_line` | 1 | Line number information |
| `.debug_info` | 8 | Debug information entries |
| `.debug_abbrev` | 1 | Debug abbreviation tables |
| `.debug_str` | 1 | Debug string pool |
| `.debug_frame` | 8 | Call frame information |
| `.debug_loc` | 1 | Location lists |
| `.debug_ranges` | 8 | Address ranges |
| `.tls` | 8 | Thread-local storage |
| `.group` | 4 | COMDAT group section |

---

## 4. Header Specification

### 4.1 File Header (64 bytes)

```
Offset  Size  Field                Description
───────────────────────────────────────────────────────────────────────
0x00    4     magic                Magic number: 0x484F4F43 ("HOOC")
0x04    2     version_major        Format version major (1)
0x06    2     version_minor        Format version minor (2)
0x08    1     file_type            0x01=executable, 0x02=shared, 0x03=object
0x09    1     target_arch          0x00=x86-64, 0x01=arm64, 0xFF=any
0x0A    1     endianness           0x01=little, 0x02=big (only little for v1.0)
0x0B    1     pointer_size         0x08 = 64-bit pointers
0x0C    4     flags                Feature flags (see Flags)
0x10    8     entry_point          RVA of entry point (0 for non-executable)
0x18    8     base_address         Preferred load address
0x20    8     section_count        Number of sections
0x28    8     symtab_offset        Offset to symbol table (0 if none)
0x30    4     symtab_entry_count   Number of symbol entries
0x34    4     reloc_count          Number of relocations
0x38    4     export_count         Number of exported symbols
0x3C    4     import_count         Number of imported symbols
```

### 4.2 Flags Field

```
Bits 31-16: Reserved
Bit  15   : Has debug info (0=no, 1=yes)
Bit  14   : Has type info (0=no, 1=yes)
Bit  13   : Stripped (0=full, 1=stripped)
Bit  12   : Position independent (0=absolute, 1=PIE)
Bits 11-8 : Optimization level (0=none, 1-3=O levels)
Bits  7-0 : Reserved
```

### 4.3 Example Header (Hex Dump)

```
48 4F 4F 43     ; Magic: "HOOC"
01 02           ; Version: 1.2
01              ; File type: Executable
00              ; Target: x86-64
01              ; Endianness: Little
08              ; Pointer size: 8 bytes
00 00 00 00     ; Flags: None
00 00 00 00 00 10 00 00  ; Entry point RVA: 0x1000
00 00 00 00 20 00 00 00  ; Base address: 0x2000
06 00 00 00 00 00 00 00  ; Section count: 6
40 00 00 00 00 00 00 00  ; Symtab offset: 0x40
0A 00 00 00           ; Symbol count: 10
05 00 00 00           ; Relocation count: 5
08 00 00 00           ; Export count: 8
03 00 00 00           ; Import count: 3
```

---

## 5. Section Table

### 5.1 Section Entry (12 bytes each)

```
Offset  Size  Field           Description
────────────────────────────────────────────
0x00    8     name_offset     Offset into .strtab for section name
0x08    4     section_type    Section type (see Section Types)
0x0C    4     flags           Section flags
0x10    8     virtual_size    Size in memory when loaded
0x18    8     file_offset     Offset to section data in file
0x20    8     alignment       Alignment requirement (power of 2)
```

### 5.2 Section Types

| Type ID | Name | Description |
|---------|------|-------------|
| `0x01` | `SHT_NULL` | Unused section |
| `0x02` | `SHT_TEXT` | Executable code |
| `0x03` | `SHT_RODATA` | Read-only data |
| `0x04` | `SHT_DATA` | Writable data |
| `0x05` | `SHT_BSS` | Uninitialized data (no file content) |
| `0x06` | `SHT_SYMTAB` | Symbol table |
| `0x07` | `SHT_STRTAB` | String table |
| `0x08` | `SHT_RELOC` | Relocation entries |
| `0x09` | `SHT_EXPORT` | Exported symbols |
| `0x0A` | `SHT_IMPORT` | Imported symbols |
| `0x0B` | `SHT_FUNCMETA` | Function metadata |
| `0x0C` | `SHT_TYPES` | Type information |
| `0x0D` | `SHT_NOTE` | Annotations/metadata |
| `0x0E` | `SHT_TLS` | Thread-local storage template |
| `0x0F` | `SHT_DEBUG_LINE` | Line number information |
| `0x0F` | `SHT_DEBUG_INFO` | Debug information (DWARF) |
| `0x10` | `SHT_DEBUG_ABBREV` | Debug abbreviations |
| `0x11` | `SHT_DEBUG_STR` | Debug string table |
| `0x12` | `SHT_DEBUG_FRAME` | Call frame information |
| `0x13` | `SHT_DEBUG_LOC` | Location lists |
| `0x14` | `SHT_DEBUG_RANGES` | Address ranges |
| `0x15` | `SHT_DEBUG_MACINFO` | Macro information |
| `0x16` | `SHT_GROUP` | COMDAT group section |

### 5.3 Section Flags

```
Bit 31-16: Reserved
Bit 15   : Section is TLS segment (processed with .tls template)
Bit 14   : Alloc flag (occupies memory when loaded)
Bit 13   : Write flag (writable)
Bit 12   : Execute flag (executable)
Bit 11   : Merge flag (can be merged with same name)
Bit 10   : Strings flag (contains null-terminated strings)
Bit  9   : Exclude from linking (used with COMDAT)
Bit  8   : Contains compressed data
Bits 7-0 : Reserved
```

**TLS Section Processing:**
When loading a file with TLS sections:
1. Copy `.tls` template to per-thread storage
2. Initialize each TLS variable from its offset in template
3. Each thread has independent copy via TLSGET/TLSSET

---

## 6. Section Types

### 6.1 `.text` Section

Contains raw HVM bytecode instructions.

**Instruction Encoding:**

HVM uses a **32-bit fixed-width instruction format** with an **escape prefix** for extended opcodes:

```
┌─────────────────────────────────────────────────────────────┐
│ Standard Instruction (32 bits)                               │
├─────────────────────────────────────────────────────────────┤
│ [opcode:7] [rd:5] [rs1:5] [rs2:5] [func/imm:10]         │
│                                                             │
│ Opcode range: 0x00 - 0x7F                                  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Extended Instruction (64 bits)                               │
├─────────────────────────────────────────────────────────────┤
│ Byte 0: 0x10 (EXT_PREFIX)                                 │
│ Bytes 1-3: Extended opcode + operands                       │
│                                                             │
│ Extended opcode range: 0x100 - 0x1FF                        │
└─────────────────────────────────────────────────────────────┘
```

**Extended Opcode Encoding:**
```
┌─────────────────────────────────────────────────────────────┐
│ EXT_PREFIX (0x10) | [ext_opcode:8] | [rd:5] [rs1:5]       │
│                   | [rs2:5] [func:10]                      │
├─────────────────────────────────────────────────────────────┤
│ ext_opcode = (full_opcode - 0x100)                         │
│ Full opcode 0x100 → ext_opcode = 0x00                       │
│ Full opcode 0x10A → ext_opcode = 0x0A                       │
└─────────────────────────────────────────────────────────────┘
```

**Instruction Categories by Opcode:**

| Opcode Range | Category | Width |
|--------------|----------|-------|
| 0x00-0x0F | Control/NOP/Data movement | 32-bit |
| 0x10 | Integer ALU | 32-bit (func field) |
| 0x11-0x14 | Immediate ops | 32-bit |
| 0x20-0x27 | Bitwise | 32-bit (func field) |
| 0x30-0x35 | Floating-point | 32-bit (func field) |
| 0x40-0x42 | Comparison | 32-bit (func field) |
| 0x50-0x57 | Branch | 32-bit |
| 0x60-0x63 | Jump | 32-bit |
| 0x70-0x7F | Memory/Stack | 32-bit |
| 0x80-0x83 | Stack frame | 32-bit |
| 0x84-0xA7 | String ops | 32-bit |
| 0xA8-0xBB | Object/Call | 32-bit |
| 0xC0-0xCE | Thread/Sync | 32-bit |
| 0xCF-0xD3 | Spinlock/Barrier | 32-bit |
| 0xE0-0xE8 | Atomics/TLS | 32-bit |
| 0xF0-0xF7 | Conversion | 32-bit |
| **0x100-0x10A** | **Vector/SIMD** | **64-bit (escape)** |
| **0x110-0x117** | **Exceptions** | **64-bit (escape)** |
| **0x118-0x11F** | **Interrupts** | **64-bit (escape)** |
| **0x120-0x12D** | **FFI** | **64-bit (escape)** |
| **0x130-0x139** | **System/Debug** | **64-bit (escape)** |

**Requirements:**
- `.text` section must be aligned to 16 bytes in memory
- Entry point must be within this section
- Jumps to extended instructions must account for 64-bit width
- Internal jumps within 32-bit instructions: `target_pc = current_pc + sign_extend(imm * 4)`

**Example Layout:**
```
Offset  Content
0x0000  10 00 05 00 01             ; ADDI r0, r0, 5 (32-bit)
0x0005  00 00 00 00 00 00 00 00   ; NOP + padding (32-bit)
0x000D  10 00 00 00 00 00 00 00   ; NOP + padding (32-bit)
0x0015  10 00 0B 00 02 04 00 00   ; ADD r0, r0, r2 (32-bit)
0x001D  10 00 10 05 00 01 02 00   ; VFMA (64-bit extended)
         ...
```

### 6.2 `.rodata` Section

Contains read-only constant data.

**Contents:**
- Numerical constants
- String literals
- Jump tables
- Format strings

**Alignment:** 8 bytes

### 6.3 `.data` Section

Contains initialized writable data.

**Contents:**
- Global variable initializers
- Virtual method tables (vtable)
- Interface method tables (itable)
- Runtime type information (RTTI)

**Alignment:** 8 bytes

### 6.4 `.bss` Section

Declares zero-initialized storage.

**Note:** No data stored in file; file contains only the section header declaring the size.

```
Section header declares:
  virtual_size = 256   ; 256 bytes of zero-initialized storage
  file_offset = 0      ; No data in file
```

### 6.5 `.strtab` Section

Contains all strings referenced by the file.

**Format:** Null-terminated UTF-8 strings, concatenated.

```
Offset  Content
0x0000  'main\0internal_func\0MyClass\0...'
```

**String references use offsets into this section.**

### 6.6 `.note` Section

Contains platform-specific annotations and metadata.

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     namesz             Length of name field (including null)
0x04    4     descsz             Length of descriptor field
0x08    4     type               Note type (vendor-specific or standard)
0x0C    n     name               Name string (null-terminated, padded to 4 bytes)
0x0C+n  m     desc               Descriptor data
```

**Standard Note Types:**
| Type | Name | Description |
|------|------|-------------|
| 0x01 | `NT_HOOC_BUILD` | Build information |
| 0x02 | `NT_HOOC_DEPEND` | Dependencies |
| 0x03 | `NT_HOOC_GOLD` | Gold linker information |
| 0x04 | `NT_HOOC_PROPERTY` | File properties |

**Example: Build Info Note**
```
namesz = 5 ("hooc\0")
descsz = 16
type   = 0x01 (NT_HOOC_BUILD)
name   = "hooc\0\0"          (padded to 4 bytes)
desc   = [version_major, version_minor, build_date, optimization_level]
```

### 6.7 `.tls` Section

Contains thread-local storage template data.

**Purpose:** Defines the initial values for TLS variables.

**Note:** TLS uses the "local exec" model where each thread gets its own copy:
- `.tls` section provides template for initialization
- Each thread allocates TLS block and copies template
- TLS access via `TLSGET`/`TLSSET` instructions

**Section Structure:**
```
┌─────────────────────────────────────────────────────────────┐
│ TLS Section Header (16 bytes)                                │
├─────────────────────────────────────────────────────────────┤
│ 8 bytes: template_length    Total bytes in TLS template    │
│ 8 bytes: alignment          Alignment requirement (power of 2)│
├─────────────────────────────────────────────────────────────┤
│ TLS Template Data (template_length bytes)                    │
│ - Initial values for TLS variables                           │
│ - Each variable's initial value                             │
└─────────────────────────────────────────────────────────────┘
```

**TLS Variable Layout:**
```
┌─────────────────────────────────────────────────────────────┐
│ For each TLS variable:                                       │
│   alignment bytes: padding (if needed)                     │
│   size bytes: initial value                                 │
└─────────────────────────────────────────────────────────────┘
```

### 6.8 `.group` Section (COMDAT)

COMDAT (Common Data) sections allow the linker to deduplicate identical sections.

**Group Entry Structure:**
```
┌─────────────────────────────────────────────────────────────┐
│ 4 bytes: flags              GRP_COMDAT (always 0x01)       │
├─────────────────────────────────────────────────────────────┤
│ 4 bytes: name_offset        Symbol name for group          │
├─────────────────────────────────────────────────────────────┤
│ For each section in group:                                  │
│   4 bytes: section_index    Index to grouped section       │
└─────────────────────────────────────────────────────────────┘
```

**Group Flags:**
| Value | Name | Description |
|-------|------|-------------|
| `0x01` | `GRP_COMDAT` | This is a COMDAT group |

**Usage:** Sections with identical content (e.g., template instantiations, inline functions) can be placed in the same group. The linker keeps only one copy.

---

## 7. Symbol Table

### 7.1 Symbol Entry (32 bytes)

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab
0x04    1     binding            STB_LOCAL(0), STB_GLOBAL(1), STB_WEAK(2)
0x05    1     type               STT_NOTYPE(0), STT_FUNC(1), STT_OBJECT(2), STT_TYPE(3)
0x06    1     visibility         STV_DEFAULT(0), STV_INTERNAL(1), STV_HIDDEN(2), STV_PROTECTED(3)
0x07    1     reserved           Reserved for future use
0x08    8     value              Symbol value (RVA for code/data, constant for types)
0x10    8     size               Size of the object (0 for functions)
0x18    4     section_index      Section index this symbol belongs to
0x1C    4     symbol_index       Unique symbol identifier
```

### 7.2 Symbol Binding

| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `STB_LOCAL` | Not visible outside the file |
| `0x01` | `STB_GLOBAL` | Visible to all object files |
| `0x02` | `STB_WEAK` | Global but can be overridden |

### 7.3 Symbol Visibility

| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `STV_DEFAULT` | Visible to external linkage |
| `0x01` | `STV_INTERNAL` | Internal to the component |
| `0x02` | `STV_HIDDEN` | Hidden symbol (not in dynamic symbol table) |
| `0x03` | `STV_PROTECTED` | Protected symbol (no interposition) |

### 7.4 Symbol Types

| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `STT_NOTYPE` | Type not specified |
| `0x01` | `STT_FUNC` | Function or procedure |
| `0x02` | `STT_OBJECT` | Variable, array, or object |
| `0x03` | `STT_TYPE` | Type descriptor |
| `0x04` | `STT_TLS` | Thread-local storage variable |

### 7.4 Example Symbol Table

```
Entry 0:
  name_offset = 0x00      -> "main"
  binding = STB_GLOBAL
  type = STT_FUNC
  value = 0x1000           (RVA in .text)
  size = 128              (function size in bytes)
  section = 0             (.text section)

Entry 1:
  name_offset = 0x10       -> "global_counter"
  binding = STB_GLOBAL
  type = STT_OBJECT
  value = 0x2000           (RVA in .data)
  size = 8                (i64 = 8 bytes)
  section = 1             (.data section)
```

---

## 8. Relocation Table

### 8.1 Relocation Entry (16 bytes)

```
Offset  Size  Field              Description
────────────────────────────────────────────────────────────────
0x00    8     offset             Offset within target section
0x08    4     symbol_index       Symbol to relocate against
0x0C    2     relocation_type    Type of relocation (see below)
0x0E    2     addend             Constant addend to add to symbol value
```

### 8.2 Relocation Types

| Type | Name | Description | Calculation |
|------|------|-------------|-------------|
| `0x01` | `R_HVM_64` | 64-bit absolute | `S + A` |
| `0x02` | `R_HVM_32` | 32-bit absolute | `(S + A) & 0xFFFFFFFF` |
| `0x03` | `R_HVM_16` | 16-bit absolute | `(S + A) & 0xFFFF` |
| `0x04` | `R_HVM_8` | 8-bit absolute | `(S + A) & 0xFF` |
| `0x05` | `R_HVM_PC32` | 32-bit PC-relative | `(S + A - P) & 0xFFFFFFFF` |
| `0x06` | `R_HVM_PC16` | 16-bit PC-relative | `(S + A - P) & 0xFFFF` |
| `0x07` | `R_HVM_PC8` | 8-bit PC-relative | `(S + A - P) & 0xFF` |
| `0x08` | `R_HVM_CALL` | Function call | `(S + A - P)` (word-aligned) |
| `0x09` | `R_HVM_GOTREL` | GOT-relative | `G + A` |
| `0x0A` | `R_HVM_GOTPREL` | GOT-relative PC | `G + A - P` |

Where:
- `S` = base address of the symbol
- `A` = addend stored in the relocation
- `P` = address of the place being relocated
- `G` = address of the GOT entry

### 8.3 Relocation Example

For a call instruction to an external function:

```
In .text section at offset 0x0100:
  CALL instruction with placeholder = 0xFFFFFFFF

Relocation entry:
  offset = 0x0100
  symbol_index = 5          ; Points to "puts" in import table
  relocation_type = R_HVM_CALL
  addend = 0

After relocation:
  CALL instruction with displacement = (puts_rva - (0x0100 + 4))
```

---

## 9. Import/Export Tables

### 9.1 Export Entry (24 bytes)

```
Offset  Size  Field              Description
────────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab
0x04    4     symbol_index       Index into symbol table
0x08    8     address            RVA of the exported symbol
0x10    8     size               Size of the exported item
```

### 9.2 Import Entry (32 bytes)

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab (function/variable name)
0x04    4     library_offset     Offset into .strtab (library name)
0x08    4     import_type        Type of import (see below)
0x0C    4     version            Library version (major << 16 | minor)
0x10    8     flags              Import flags (see below)
0x18    8     resolved_address   Set by loader (0 if unresolved)
```

### 9.3 Import Types

| Value | Name | Description |
|-------|------|-------------|
| `0x01` | `IT_HOOC` | Import from another `.ho` module |
| `0x02` | `IT_NATIVE` | Import from native library (FFI) |
| `0x03` | `IT_RUNTIME` | Import from Hooc runtime |
| `0x04` | `IT_INTRINSIC` | Compiler intrinsic |

### 9.4 Import Flags

```
Bit  31   : Lazy binding (resolve on first call)
Bit  30   : Weak import (unresolved is not error)
Bit  29   : No strip (keep symbol in linked output)
Bit  28   : Constructor (call on load)
Bit  27   : Destructor (call on unload)
Bits 26-24: Reserved
Bits 23-16: TLS model (see below)
Bits 15-0 : Reserved
```

**TLS Models:**
| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `TLS_NONE` | Not TLS |
| `0x01` | `TLS_LOCAL` | Local exec model |
| `0x02` | `TLS_INITIAL` | Initial exec model |
| `0x03` | `TLS_LOCAL_DYNAMIC` | Local dynamic model |
| `0x04` | `TLS_GLOBAL_DYNAMIC` | Global dynamic model |

### 9.5 Import Library Naming

Imports can be from:
- **Standard library**: `hooc:std` (runtime, stdlib)
- **User libraries**: `hooc:module_name`
- **Native interop**: `native:c_function_name` (FFI)

### 9.6 Import Resolution Protocol

```
┌─────────────────────────────────────────────────────────────┐
│ Import Resolution Process                                    │
├─────────────────────────────────────────────────────────────┤
│ 1. PARSE IMPORTS                                           │
│    - Read .import section entries                           │
│    - Categorize by import_type                             │
├─────────────────────────────────────────────────────────────┤
│ 2. RESOLVE HOOC IMPORTS                                    │
│    - For IT_HOOC:                                          │
│      a. Search loaded modules for export                    │
│      b. If not found, load required .ho file               │
│      c. If still not found, error (unless weak)            │
├─────────────────────────────────────────────────────────────┤
│ 3. RESOLVE NATIVE IMPORTS                                  │
│    - For IT_NATIVE:                                        │
│      a. Parse library name (e.g., "native:libc")          │
│      b. dlopen() / LoadLibrary()                          │
│      c. dlsym() / GetProcAddress()                       │
│      d. If still not found, error (unless weak)            │
├─────────────────────────────────────────────────────────────┤
│ 4. RESOLVE RUNTIME IMPORTS                                 │
│    - For IT_RUNTIME:                                       │
│      a. Look up in runtime function table                  │
│      b. Pre-registered at VM init                          │
├─────────────────────────────────────────────────────────────┤
│ 5. UPDATE GOT (if needed)                                  │
│    - For lazy binding: create PLT stub                    │
│    - For immediate: write resolved address to GOT         │
├─────────────────────────────────────────────────────────────┤
│ 6. APPLY RELOCATIONS                                      │
│    - Process .reloc entries using resolved addresses       │
└─────────────────────────────────────────────────────────────┘
```

### 9.7 Example Import Table

```
Import 0:
  name_offset = 0x20           -> "print"
  library_offset = 0x40         -> "hooc:std"
  flags = 0x00

Import 1:
  name_offset = 0x60           -> "malloc"
  library_offset = 0x70         -> "native:libc"
  flags = 0x01                 ; NEEDS_GOT_ENTRY
```

---

## 10. String Pool

### 10.1 Structure

The `.strtab` section contains all strings in the file:

```
┌─────────────────────────────────────────────────────────────┐
│ String Pool (.strtab)                                       │
├─────────────────────────────────────────────────────────────┤
│ Offset 0x00: 'main\0'                                       │
│ Offset 0x05: '_hooc_internal_foo\0'                         │
│ Offset 0x1C: 'MyClass\0'                                    │
│ Offset 0x25: 'print\0'                                      │
│ Offset 0x2B: 'hooc:std\0'                                   │
│ ...                                                         │
└─────────────────────────────────────────────────────────────┘
```

### 10.2 String References

All string references are 32-bit offsets into the string pool:
- Symbols reference by `name_offset`
- Import/export tables reference library and symbol names
- Debug info references file/function names

---

## 11. Function Metadata

### 11.1 Function Metadata Entry (48 bytes)

```
Offset  Size  Field              Description
────────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab
0x04    4     symbol_index       Index into symbol table
0x08    8     entry_rva          RVA of function entry point
0x10    4     code_size          Size of function code in bytes
0x14    4     local_size         Stack frame size (in bytes)
0x18    4     param_count        Number of parameters
0x1C    4     param_types_offset Offset into type table
0x20    4     return_type_offset Offset into type table
0x24    4     flags              Function flags
0x28    4     source_line        First source line number
0x2C    4     debug_offset       Debug info offset (0 if none)
```

### 11.2 Function Flags

```
Bit 31-16: Reserved
Bit 15   : Has debug info
Bit 14   : Is leaf function (doesn't call other functions)
Bit 13   : Uses varargs
Bit 12   : Is variadic
Bit 11   : Has EH unwinding info
Bit 10   : Is intrinsic
Bits 9-0 : Reserved
```

### 11.3 Calling Convention

HVM uses the **HVMCC** calling convention:

```
Parameter Passing:
  r0 - r7: First 8 integer/pointer parameters
  f0 - f7: First 8 floating-point parameters
  Stack:   Remaining parameters (right-to-left push)

Return Values:
  r0:      Integer/pointer return
  f0:      Floating-point return
  r0:r1:   128-bit return (if needed)

Caller-Saved (volatile):
  r0 - r7, r10 - r17
  f0 - f7
  ra (return address)
  sp (stack pointer)

Callee-Saved (preserved):
  r18 - r31
  sp (stack pointer)
  gp (global pointer, if used)
```

---

## 12. Type Information

### 12.1 Type Descriptor Entry (variable size)

```
Offset  Size  Field              Description
────────────────────────────────────────────────────────────────
0x00    1     type_kind          Type kind (see below)
0x01    3     reserved           Reserved
0x04    4     name_offset        Offset into .strtab
0x08    4     size               Size in bytes
0x0C    4     alignment          Alignment requirement
0x10    4     field_count        Number of fields (for structs)
0x14    4     type_data_offset   Offset to additional type data
```

### 12.2 Type Kinds

| Value | Name | Description |
|-------|------|-------------|
| `0x01` | `TYPE_PRIMITIVE` | Built-in types (i64, f64, etc.) |
| `0x02` | `TYPE_POINTER` | Pointer/reference type |
| `0x03` | `TYPE_ARRAY` | Fixed-size array |
| `0x04` | `TYPE_DYNAMIC_ARRAY` | Dynamic/slice type |
| `0x05` | `TYPE_STRUCT` | Struct/class type |
| `0x06` | `TYPE_UNION` | Union type |
| `0x07` | `TYPE_ENUM` | Enumeration type |
| `0x08` | `TYPE_FUNCTION` | Function type |
| `0x09` | `TYPE_OPTIONAL` | Optional/nullable type |
| `0x0A` | `TYPE_INTERFACE` | Interface type |
| `0x0B` | `TYPE_TYPARAM` | Type parameter (generics) |

### 12.3 Type Example: Struct

```
Struct Type: Point { x: f64, y: f64 }

Type Descriptor:
  type_kind = TYPE_STRUCT
  name_offset = 0x100           -> "Point"
  size = 16                     ; Two f64 = 16 bytes
  alignment = 8
  field_count = 2
  type_data_offset = 0x200     -> Points to field descriptors

Field Descriptors (at offset 0x200):
  Field 0:
    name_offset = 0x110         -> "x"
    type_index = TYPE_F64 (primitive)
    offset = 0                  ; Offset within struct

  Field 1:
    name_offset = 0x112         -> "y"
    type_index = TYPE_F64 (primitive)
    offset = 8
```

---

## 13. File Type Variants

### 13.1 Executable (file_type = 0x01)

**Requirements:**
- Must have `entry_point` set in header
- Entry point must be in a `.text` section
- Can have imports (e.g., `hooc:std` for runtime)
- Cannot be used as a library

**Example:**
```
hooc run app.ho
```

### 13.2 Shared Object (file_type = 0x02)

**Requirements:**
- Cannot have `entry_point` (entry_point = 0)
- Must have at least one export
- Can have imports
- Loaded by dynamic linker or `dlopen()`

**Example:**
```
hooc link --shared mylib.ho -o libmy.ho

; Later:
hooc run --require libmy.ho main.ho
```

**Dynamic Linking:**
```
Import table specifies required libraries:
  "hooc:std"         -> Runtime library (always available)
  "hooc:mylib"       -> Another .ho shared object
  "native:libc"      -> Native C library (via FFI)
```

### 13.3 Object File (file_type = 0x03)

**Requirements:**
- Cannot have `entry_point`
- May have both exports and imports
- All imports must be resolved by static linker
- Contains relocations for linker to apply

**Example:**
```
; Compile to object file
hooc compile -c module_a.hoo -o module_a.o.ho

; Static link
hooc link module_a.o.ho module_b.o.ho -o final.ho
```

**Static Linking Process:**
```
1. Read all object files
2. Resolve imports against exports
3. Apply relocations
4. Merge sections
5. Produce executable or shared object
```

---

## 14. Loading and Linking

### 14.1 JIT Loader Process

```
┌─────────────────────────────────────────────────────────────┐
│                    HO FILE LOADER                           │
├─────────────────────────────────────────────────────────────┤
│ 1. READ HEADER                                              │
│    - Verify magic number                                    │
│    - Check version compatibility                            │
│    - Validate file type                                     │
├─────────────────────────────────────────────────────────────┤
│ 2. MAP SECTIONS                                             │
│    - Allocate virtual memory for each section               │
│    - Copy data from file to memory                          │
│    - Apply section permissions (rwx)                        │
├─────────────────────────────────────────────────────────────┤
│ 3. RESOLVE IMPORTS                                          │
│    - For hooc:std: load standard runtime                   │
│    - For hooc:* modules: load shared .ho files             │
│    - For native:* symbols: resolve via FFI                  │
│    - Populate GOT with resolved addresses                   │
├─────────────────────────────────────────────────────────────┤
│ 4. APPLY RELOCATIONS                                        │
│    - Process each relocation entry                         │
│    - Calculate final addresses                              │
│    - Write relocated values to memory                       │
├─────────────────────────────────────────────────────────────┤
│ 5. Jit-Compile CODE                                         │
│    - For each .text section:                                │
│      - Parse HVM bytecode                                    │
│      - Emit native machine code                             │
│      - Patch internal jumps                                 │
│    - Create executable memory pages                          │
├─────────────────────────────────────────────────────────────┤
│ 6. INVOKE ENTRY POINT                                       │
│    - Call the entry point function                          │
│    - Pass command-line arguments                            │
└─────────────────────────────────────────────────────────────┘
```

### 14.2 Memory Layout During Load

```
Virtual Address Space (after loading)

Base Address: 0x0000_0000_0020_0000 (example)

0x0000_0000_0020_0000: .text section (code)
                      - HVM bytecode
                      - JIT-compiled to native code
                      - Permissions: RX

0x0000_0000_0040_0000: .rodata section (constants)
                      - String literals
                      - Jump tables
                      - Permissions: R

0x0000_0000_0040_1000: .data section (globals)
                      - Global variables
                      - vtables
                      - Permissions: RW

0x0000_0000_0040_2000: .bss section (zero-init)
                      - Uninitialized globals
                      - Permissions: RW

0x0000_0000_0040_3000: Heap (runtime)
                      - Object allocations
                      - Arrays, strings
                      - Permissions: RW

0x0000_0000_7FFF_0000: Stack
                      - Function call frames
                      - Grows downward
                      - Permissions: RW
```

### 14.3 Symbol Resolution Order

```
For static linking:
  1. Local symbols (STB_LOCAL) - within same file
  2. Exported symbols from other object files being linked
  3. Undefined symbols -> link error

For dynamic linking:
  1. Local symbols
  2. Exported symbols from loaded shared objects
  3. Symbol interposition (LD_PRELOAD style)
  4. Undefined symbols -> runtime error
```

---

## 15. Usage Examples

### 15.1 Creating an Executable

**Source Code (`main.hoo`):**
```hooc
func:int64 main() {
    print("Hello, World!");
    return 0;
}
```

**Compile to Executable:**
```bash
hooc compile main.hoo -o main.ho
```

**Generated File Structure:**
```
main.ho
├── Header
│   ├── magic = "HOOC"
│   ├── version = 1.0
│   ├── file_type = EXECUTABLE
│   ├── entry_point = 0x1000
│   └── ...
├── Section Table
│   ├── .text (index 0)
│   ├── .rodata (index 1)
│   └── .strtab (index 2)
├── .text section
│   └── bytecode for main()
├── .rodata section
│   └── "Hello, World!\0"
└── .strtab section
    └── "main\0"...
```

**Run:**
```bash
hooc run main.ho
# Output: Hello, World!
```

### 15.2 Creating a Shared Library

**Source Code (`math.hoo`):**
```hooc
class Math {
    func:int64 add(a: int64, b: int64) {
        return a + b;
    }

    func:int64 mul(a: int64, b: int64) {
        return a * b;
    }
}

export Math;
```

**Compile to Shared Library:**
```bash
hooc compile --shared math.hoo -o libmath.ho
```

**Generated File Structure:**
```
libmath.ho
├── Header
│   ├── file_type = SHARED_OBJECT
│   ├── entry_point = 0 (no entry)
│   └── export_count = 1
├── Section Table
│   ├── .text
│   ├── .rodata
│   ├── .data (vtable)
│   ├── .strtab
│   ├── .export
│   └── .import
├── .export section
│   └── Entry: "Math" -> vtable_rva
└── ...
```

### 15.3 Static Linking

**File 1 (`a.hoo`):**
```hooc
func:int64 foo() {
    return 42;
}

export foo;
```

**File 2 (`b.hoo`):**
```hooc
import foo from "local";

func:int64 bar() {
    return foo() + 1;
}

export bar;
```

**Compile and Link:**
```bash
hooc compile -c a.hoo -o a.o.ho
hooc compile -c b.hoo -o b.o.ho
hooc link a.o.ho b.o.ho -o program.ho
```

**Result:**
```
program.ho
├── Header
│   ├── file_type = EXECUTABLE
│   ├── entry_point = bar_rva
│   └── import_count = 0 (foo resolved)
├── Merged .text section
│   └── bytecode for foo() + bar()
└── ...
```

### 15.4 Dynamic Linking

**Main Program (`main.hoo`):**
```hooc
import Math from "math";

func:int64 main() {
    let m = new Math();
    return m.add(10, 20);
}

export main;
```

**Compile:**
```bash
hooc compile main.hoo -o main.ho
```

**At runtime:**
```
hooc run main.ho --require libmath.ho
```

**Runtime loading:**
```
1. Load main.ho
2. See import: "Math" from "math"
3. Load libmath.ho
4. Resolve Math symbol
5. Apply relocations
6. Execute
```

---

## 16. Appendix: OpCodes Reference

### 16.1 Full Instruction Set

For the complete HVM instruction set, see `hvm_instruction_set.csv` and `HVM_SPEC.md`.

### 16.2 Quick Reference

| Category | Instructions |
|----------|---------------|
| **Arithmetic** | ADD, ADDI, SUB, SUBI, MUL, MULH, DIV, REM |
| **Logical** | AND, OR, XOR, NOT, SHL, SHR, SRA |
| **Comparison** | EQ, NE, LT, LE, GT, GE, CMP |
| **Control Flow** | JMP, JZ, JNZ, CALL, RET, SYSCALL |
| **Memory** | LD, ST, LDM, STM, LEA |
| **Stack** | PUSH, POP, PUSHALL, POPALL |
| **Function** | ENTER, LEAVE, CALL, RET |
| **Type Conv** | SEXT, ZEXT, TRUNC, FEXT, FTRUNC, BITCAST |
| **Vectors** | VADD, VMUL, VCMP, VSHUFFLE, ... |
| **Debug** | BREAKPOINT, SINGLESTEP, GETREGS, SETREGS |

### 16.3 JIT Compilation Notes

When JIT-compiling HVM bytecode:

1. **Instruction Alignment**: Text section is 16-byte aligned
2. **Jump Patching**: Internal jumps patched after native code emission
3. **GOT Access**: External symbols accessed via Global Offset Table
4. **Stack Spill**: Registers spilled to stack as needed
5. **Calling Convention**: Follow HVMCC (see Section 11.3)

---

## 17. Debug Information

### 17.1 Overview

Debug information enables source-level debugging with tools like GDB and LLDB. HVM uses a **DWARF-inspired format** that is simplified but compatible with standard DWARF parsers.

**Debug Sections:**
| Section | Purpose |
|---------|---------|
| `.debug_line` | Line number program → address mapping |
| `.debug_info` | Debugging Information Entries (DIEs) |
| `.debug_abbrev` | Abbreviation tables (reduces repetition) |
| `.debug_str` | Strings referenced by debug_info |
| `.debug_frame` | Call frame unwinding information |
| `.debug_loc` | Variable location descriptions |
| `.debug_ranges` | Address ranges for compilation units |
| `.debug_macinfo` | Macro definitions and inclusions |

### 17.2 Debug Line Section

#### 17.2.1 Line Number Header (20 bytes)

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     unit_length       Length of debug_line data (excluding this field)
0x04    2     version           DWARF version (4 or 5)
0x06    8     header_length     Length of header after this field
0x0E    1     min_insn_length   Minimum instruction length
0x0F    1     max_ops_per_insn  Maximum operations per instruction (always 1 for HVM)
0x10    1     default_is_stmt   Initial value of is_stmt
0x11    1     line_base         Line base for special opcodes
0x12    1     line_range        Line range for special opcodes
0x13    1     opcode_base       First special opcode (typically 13)
```

#### 17.2.2 Line Number Program

The line number program is a sequence of opcodes operating on a state machine:

**Standard Opcodes:**
| Opcode | Args | Description |
|--------|------|-------------|
| 0x00 | - | Extended opcode (see below) |
| 0x01 | - | DW_LNS_copy |
| 0x02 | 1 | DW_LNS_advance_pc |
| 0x03 | 1 | DW_LNS_advance_line |
| 0x04 | - | DW_LNS_fixed_advance_pc |
| 0x05 | - | DW_LNS_set_file |
| 0x06 | - | DW_LNS_set_column |
| 0x07 | - | DW_LNS_negate_stmt |
| 0x08 | - | DW_LNS_set_basic_block |
| 0x09 | - | DW_LNS_const_add_pc |
| 0x0A | - | DW_LNS_set_prologue_end |
| 0x0B | - | DW_LNS_set_epilogue_begin |
| 0x0C | 1 | DW_LNS_set_isa |

**Extended Opcodes (opcode 0x00):**
| Sub-opcode | Args | Description |
|------------|------|-------------|
| 0x01 | 1 | DW_LNE_end_sequence |
| 0x02 | 4 | DW_LNE_set_address |
| 0x03 | 2+ | DW_LNE_define_file |

#### 17.2.3 Line Number Entry Format

For fast lookup, debug_line can be pre-processed into a lookup table:

```
┌─────────────────────────────────────────────────────────────┐
│ Line Number Lookup Table                                    │
├─────────────────────────────────────────────────────────────┤
│ 4 bytes: entry_count                                      │
├─────────────────────────────────────────────────────────────┤
│ For each entry (16 bytes):                                │
│   8 bytes: address (RVA)                                  │
│   4 bytes: file_index                                     │
│   4 bytes: line_number                                    │
└─────────────────────────────────────────────────────────────┘
```

#### 17.2.4 File Name Entry

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     name_offset       Offset into .debug_str or .strtab
0x04    4     dir_index         Index into include directory table
0x08    8     mod_time          Last modification time (0 for HVM)
0x10    8     file_length       File length (0 for HVM)
```

### 17.3 Debug Abbrev Section

Abbreviations reduce debug_info size by defining reusable attribute specifications.

#### 17.3.1 Abbreviation Entry Format

```
┌─────────────────────────────────────────────────────────────┐
│ abbreviation_code: uleb128                                  │
│ tag: uleb128 (DW_TAG_*)                                     │
│ children: u8 (DW_CHILDREN_no or DW_CHILDREN_yes)            │
├─────────────────────────────────────────────────────────────┤
│ For each attribute:                                         │
│   name: uleb128 (DW_AT_*)                                   │
│   form: uleb128 (DW_FORM_*)                                 │
├─────────────────────────────────────────────────────────────┤
│ Terminator: name=0, form=0                                  │
└─────────────────────────────────────────────────────────────┘
```

#### 17.3.2 Common Attribute Forms

| Form | Encoding | Description |
|------|----------|-------------|
| `DW_FORM_addr` | addr | Address (8 bytes) |
| `DW_FORM_string` | string | Inline string |
| `DW_FORM_strp` | uleb128 | Offset into .debug_str |
| `DW_FORM_data1` | 1 byte | 1-byte constant |
| `DW_FORM_data2` | 2 bytes | 2-byte constant |
| `DW_FORM_data4` | 4 bytes | 4-byte constant |
| `DW_FORM_data8` | 8 bytes | 8-byte constant |
| `DW_FORM_uleb128` | uleb128 | Unsigned LEB128 |
| `DW_FORM_sleb128` | sleb128 | Signed LEB128 |
| `DW_FORM_ref1` | 1 byte | Offset in compilation unit |
| `DW_FORM_ref4` | 4 bytes | Offset in compilation unit |
| `DW_FORM_ref8` | 8 bytes | Offset in compilation unit |
| `DW_FORM_exprloc` | length + expr | Location expression |

### 17.4 Debug Info Section

#### 17.4.1 Compilation Unit Header (24 bytes)

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     unit_length       Length of unit (excluding this field)
0x04    2     version           DWARF version
0x06    4     debug_abbrev_offset Offset into .debug_abbrev
0x0A    1     address_size      Address size (8 for HVM)
0x0B    8     compilation_dir   Offset into .debug_str (or 0)
0x13    4     producer_len      Length of producer string
0x17    n     producer          Producer string (e.g., "hooc 1.0")
```

#### 17.4.2 DIE Structure

```
┌─────────────────────────────────────────────────────────────┐
│ die_code: uleb128 (abbreviation code)                       │
├─────────────────────────────────────────────────────────────┤
│ If die_code != 0:                                           │
│   For each attribute in abbreviation:                       │
│     Encode value according to attribute form                 │
├─────────────────────────────────────────────────────────────┤
│ Children (if abbreviation specifies DW_CHILDREN_yes):       │
│   Nested DIEs...                                            │
│   Terminator: die_code = 0                                  │
└─────────────────────────────────────────────────────────────┘
```

#### 17.4.3 Common DIE Tags

| Tag | Description |
|-----|-------------|
| `DW_TAG_compile_unit` | Compilation unit |
| `DW_TAG_subprogram` | Function/method |
| `DW_TAG_variable` | Variable |
| `DW_TAG_parameter` | Function parameter |
| `DW_TAG_structure_type` | Struct/class |
| `DW_TAG_union_type` | Union |
| `DW_TAG_enumeration_type` | Enum |
| `DW_TAG_array_type` | Array |
| `DW_TAG_base_type` | Primitive type |
| `DW_TAG_pointer_type` | Pointer |
| `DW_TAG_reference_type` | Reference |
| `DW_TAG_typedef` | Type alias |
| `DW_TAG_lexical_block` | Lexical scope |
| `DW_TAG_inlined_subroutine` | Inlined function |
| `DW_TAG_label` | Label |
| `DW_TAG_namespace` | Namespace |

#### 17.4.4 Common Attributes

| Attribute | Form | Description |
|-----------|------|-------------|
| `DW_AT_name` | string/strp | Name |
| `DW_AT_decl_file` | data1/data4 | Source file index |
| `DW_AT_decl_line` | data4 | Source line number |
| `DW_AT_low_pc` | addr | Low address of scope |
| `DW_AT_high_pc` | addr | High address of scope |
| `DW_AT_type` | ref* | Reference to type DIE |
| `DW_AT_external` | flag | Is externally visible |
| `DW_AT_artificial` | flag | Compiler-generated |
| `DW_AT_location` | exprloc/data* | Location description |
| `DW_AT_language` | data2 | Source language ID |
| `DW_AT_producer` | string | Compiler name |

### 17.5 Debug Frame Section

#### 17.5.1 CIE (Common Information Entry) Header

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     length            Length of CIE (excluding this field)
0x04    4     CIE_id            CIE identifier
0x08    1     version           Frame unwind version (3 or 4)
0x09    n     augmentation      Augmentation string (null-terminated)
0x09+n  1     address_size      Address size (8 for HVM)
0x0A+n  1     segment_size      Segment selector size (0 for HVM)
0x0B+n  n     initial_instructions Call frame instructions
```

#### 17.5.2 FDE (Frame Description Entry) Header

```
Offset  Size  Field              Description
───────────────────────────────────────────────────────────────
0x00    4     length            Length of FDE (excluding this field)
0x04    4     CIE_pointer       Offset to associated CIE
0x08    8     initial_location Start address of this frame's code
0x10    8     address_range     Size of address range
0x18    n     instructions      Call frame instructions
```

#### 17.5.3 Call Frame Instructions

| Instruction | Args | Description |
|-------------|------|-------------|
| `DW_CFA_advance_loc` | delta | Advance location by delta |
| `DW_CFA_offset` | register, offset | CFA is at [register + offset] |
| `DW_CFA_restore` | register | Restore register |
| `DW_CFA_set_loc` | address | Set current location |
| `DW_CFA_advance_loc1` | 1 byte | 8-bit delta |
| `DW_CFA_advance_loc2` | 2 bytes | 16-bit delta |
| `DW_CFA_advance_loc4` | 4 bytes | 32-bit delta |
| `DW_CFA_offset_extended` | reg, offset | Extended offset |
| `DW_CFA_restore_extended` | reg | Extended restore |
| `DW_CFA_undefined` | register | Register is undefined |
| `DW_CFA_same_value` | register | Register unchanged |
| `DW_CFA_register` | reg1, reg2 | reg1 saved at reg2 |
| `DW_CFA_remember_state` | - | Push state |
| `DW_CFA_restore_state` | - | Pop state |
| `DW_CFA_def_cfa` | register, offset | Define CFA |
| `DW_CFA_def_cfa_register` | register | Change CFA register |
| `DW_CFA_def_cfa_offset` | offset | Change CFA offset |
| `DW_CFA_nop` | - | No operation |

### 17.6 Debug Location Section

#### 17.6.1 Location List Entry

```
┌─────────────────────────────────────────────────────────────┐
│ begin_address: 8 bytes (low PC)                             │
│ end_address: 8 bytes (high PC)                              │
│ location_description: variable                              │
│   - Either location expression bytecode                      │
│   - Or offset into .debug_loc for linked entries            │
└─────────────────────────────────────────────────────────────┘
```

#### 17.6.2 Location Expression Opcodes

| Opcode | Args | Description |
|--------|------|-------------|
| `DW_OP_addr` | addr | Push address |
| `DW_OP_fbreg` | offset | Push frame base + offset |
| `DW_OP_bregx` | reg, offset | Register + offset |
| `DW_OP_plus` | - | Add top two values |
| `DW_OP_minus` | - | Subtract |
| `DW_OP_mul` | - | Multiply |
| `DW_OP_div` | - | Divide |
| `DW_OP_deref` | - | Dereference pointer |
| `DW_OP_dup` | - | Duplicate stack top |
| `DW_OP_drop` | - | Drop stack top |
| `DW_OP_swap` | - | Swap top two values |
| `DW_OP_stack_value` | - | Value is in register |
| `DW_OP_implicit_value` | len, data | Constant value |

### 17.7 Debug Strings Section

Contains null-terminated strings used by debug_info:

```
┌─────────────────────────────────────────────────────────────┐
│ 0x0000: "main\0"                                           │
│ 0x0005: "add\0"                                            │
│ 0x0009: "Point\0"                                          │
│ 0x0010: "/path/to/source/main.hoo\0"                      │
│ ...                                                         │
└─────────────────────────────────────────────────────────────┘
```

### 17.8 Debug Ranges Section

Defines address ranges for scopes and inline instances:

```
┌─────────────────────────────────────────────────────────────┐
│ Unit Header:                                                │
│   4 bytes: unit_length                                     │
│   4 bytes: debug_info_offset (offset into .debug_info)     │
├─────────────────────────────────────────────────────────────┤
│ For each range:                                             │
│   8 bytes: start_address                                   │
│   8 bytes: end_address                                     │
├─────────────────────────────────────────────────────────────┤
│ Terminator: 0x0000000000000000, 0x0000000000000000         │
└─────────────────────────────────────────────────────────────┘
```

### 17.9 Example: Debug Info for Simple Function

**Source:**
```hooc
func:int64 add(a: int64, b: int64) {
    return a + b;
}
```

**Generated Debug Info:**
```yaml
# Compilation Unit DIE
DW_TAG_compile_unit:
  DW_AT_name: "add.hoo"
  DW_AT_language: DW_LANG_Hooc
  DW_AT_producer: "hooc 1.0"
  DW_AT_comp_dir: "/project/src"

# Function DIE
DW_TAG_subprogram:
  DW_AT_name: "add"
  DW_AT_decl_file: 1
  DW_AT_decl_line: 1
  DW_AT_low_pc: 0x1000
  DW_AT_high_pc: 0x1020
  DW_AT_frame_base: DW_OP_bregx(r31, 0)

  # Parameter 'a'
  DW_TAG_formal_parameter:
    DW_AT_name: "a"
    DW_AT_decl_file: 1
    DW_AT_decl_line: 1
    DW_AT_type: ref to int64
    DW_AT_location: DW_OP_fbreg(-16)

  # Parameter 'b'
  DW_TAG_formal_parameter:
    DW_AT_name: "b"
    DW_AT_decl_file: 1
    DW_AT_decl_line: 1
    DW_AT_type: ref to int64
    DW_AT_location: DW_OP_fbreg(-24)
```

### 17.10 Integration with HVM Instructions

For proper debugger support, add these instructions:

| Instruction | Opcode | Description |
|-------------|--------|-------------|
| `BREAKPOINT` | 0x135 | Source-level breakpoint (uses debug_line) |
| `SINGLESTEP` | 0x136 | Single-step one instruction |
| `GETREGS` | 0x137 | Read all registers to memory |
| `SETREGS` | 0x138 | Write registers from memory |
| `GETFPOFF` | 0x139 | Get current frame pointer offset |

### 17.11 Integration with JIT

When JIT-compiling debug-enabled bytecode:

1. **Preserve debug info**: Copy .debug_* sections to output
2. **Relocate addresses**: Update address references after native code generation
3. **Generate line info**: Map native addresses back to source lines
4. **Unwind info**: Generate .debug_frame for generated code

```
┌─────────────────────────────────────────────────────────────┐
│ JIT Debug Integration                                       │
├─────────────────────────────────────────────────────────────┤
│ 1. COMPILE_WITH_DEBUG(source)                              │
│    - Include debug sections in compilation                   │
│    - Emit DEBUG opcodes at source breakpoints              │
├─────────────────────────────────────────────────────────────┤
│ 2. JIT_COMPILE(bytecode)                                   │
│    - Parse .debug_line for source mapping                   │
│    - Generate native code with debug annotations            │
│    - Update low_pc/high_pc in DIEs to native addresses      │
├─────────────────────────────────────────────────────────────┤
│ 3. EXECUTE()                                               │
│    - Use debug_line to map PC → source line                 │
│    - Use debug_info for variable locations                  │
│    - Use debug_frame for stack unwinding                   │
└─────────────────────────────────────────────────────────────┘
```

### 17.12 HVM DWARF Constants

```c
// DWARF Version
#define DW_VERSION  4

// Source Languages
#define DW_LANG_Hooc       0x8000  // HVM/Hooc (vendor extension)

// Tags
#define DW_TAG_compile_unit        0x11
#define DW_TAG_subprogram          0x2e
#define DW_TAG_variable             0x34
#define DW_TAG_formal_parameter     0x35
#define DW_TAG_structure_type       0x13
#define DW_TAG_base_type            0x24
#define DW_TAG_pointer_type          0x0f
#define DW_TAG_reference_type       0x10
#define DW_TAG_typedef              0x16
#define DW_TAG_lexical_block        0x0b
#define DW_TAG_label                0x0a

// Attributes
#define DW_AT_name                 0x03
#define DW_AT_decl_file            0x3a
#define DW_AT_decl_line            0x3b
#define DW_AT_low_pc               0x11
#define DW_AT_high_pc              0x12
#define DW_AT_type                 0x49
#define DW_AT_location             0x02
#define DW_AT_frame_base           0x40
#define DW_AT_language             0x13
#define DW_AT_producer             0x25
#define DW_AT_external             0x3f
#define DW_AT_artificial           0x34

// Forms
#define DW_FORM_addr               0x01
#define DW_FORM_string             0x02
#define DW_FORM_strp               0x0e
#define DW_FORM_data1              0x0b
#define DW_FORM_data2              0x05
#define DW_FORM_data4              0x06
#define DW_FORM_data8              0x07
#define DW_FORM_uleb128            0x0f
#define DW_FORM_sleb128            0x0d
#define DW_FORM_ref4               0x13
#define DW_FORM_exprloc            0x18

### 17.13 Hooc-Specific Constants

```c
// Hooc File Magic and Version
#define HOOC_MAGIC                 0x484F4F43  // "HOOC"
#define HOOC_VERSION_MAJOR         1
#define HOOC_VERSION_MINOR         1

// File Types
#define HOOC_FT_EXEC               0x01
#define HOOC_FT_SHARED             0x02
#define HOOC_FT_OBJECT             0x03

// Target Architectures
#define HOOC_ARCH_X86_64           0x00
#define HOOC_ARCH_ARM64            0x01
#define HOOC_ARCH_ANY              0xFF

// Import Types
#define HOOC_IT_HOOC               0x01
#define HOOC_IT_NATIVE             0x02
#define HOOC_IT_RUNTIME            0x03
#define HOOC_IT_INTRINSIC          0x04

// TLS Models
#define HOOC_TLS_NONE              0x00
#define HOOC_TLS_LOCAL             0x01
#define HOOC_TLS_INITIAL           0x02

// Hooc DWARF Language ID (vendor extension)
#define DW_LANG_Hooc               0x8000

// Hooc Base Types
#define DW_ATE_hooc_int64          0x01
#define DW_ATE_hooc_int32          0x02
#define DW_ATE_hooc_int16          0x03
#define DW_ATE_hooc_int8           0x04
#define DW_ATE_hooc_uint64         0x05
#define DW_ATE_hooc_uint32         0x06
#define DW_ATE_hooc_uint16         0x07
#define DW_ATE_hooc_uint8          0x08
#define DW_ATE_hooc_bool           0x09
#define DW_ATE_hooc_char           0x0A
#define DW_ATE_hooc_float          0x0B
#define DW_ATE_hooc_double         0x0C
#define DW_ATE_hooc_string         0x0D
```

---

## 18. HVM-Specific Debug Extensions

### 18.1 HVM Location Expression Opcodes

These location expression opcodes extend DWARF for HVM-specific types:

| Opcode | Name | Args | Description |
|--------|------|------|-------------|
| `0xE0` | `DW_OP_hvm_reg` | reg | Push register value (HVM register number) |
| `0xE1` | `DW_OP_hvm_frame` | offset | CFA is at frame pointer + offset |
| `0xE2` | `DW_OP_hvm_slot` | slot | Thread-local storage slot |
| `0xE3` | `DW_OP_hvm_obj` | offset | Object field at [object + offset] |
| `0xE4` | `DW_OP_hvm_array` | offset | Array element at [array + index*element_size + offset] |
| `0xE5` | `DW_OP_hvm_vtable` | - | Virtual method table pointer |
| `0xE6` | `DW_OP_hvm_string` | - | String handle (reference type) |
| `0xE7` | `DW_OP_hvm_closure` | - | Closure/captured variable |

### 18.2 HVM Register Number Mapping

For `DW_OP_hvm_reg`, register numbers map to HVM registers:

| DWARF Reg | HVM Reg | Purpose |
|-----------|---------|---------|
| 0-31 | r0-r31 | General-purpose registers |
| 32-47 | v0-v15 | Vector registers |
| 48 | pc | Program counter |
| 49 | sp | Stack pointer (r31) |
| 50 | fp | Frame pointer (r30) |

### 18.3 Example: HVM Variable Location

**Source:**
```hooc
class Point {
    var x: int64;
    var y: int64;
}

func:double distance(p: Point) {
    var dx = p.x;  // Variable 'dx' in register r9
    var dy = p.y;  // Variable 'dy' in register r10
    return sqrt(dx*dx + dy*dy);
}
```

**Debug Info:**
```
DW_TAG_variable "dx":
    DW_AT_location: DW_OP_hvm_reg(9)
    DW_AT_type: ref to int64

DW_TAG_variable "dy":
    DW_AT_location: DW_OP_hvm_reg(10)
    DW_AT_type: ref to int64

DW_TAG_formal_parameter "p":
    DW_AT_location: DW_OP_hvm_frame(-16)
    DW_AT_type: ref to Point
```

### 18.4 JIT Address Remapping

When JIT compiling with debug info:

```
┌─────────────────────────────────────────────────────────────┐
│ Address Remapping Table                                       │
├─────────────────────────────────────────────────────────────┤
│ 4 bytes: entry_count                                         │
├─────────────────────────────────────────────────────────────┤
│ For each entry:                                              │
│   8 bytes: hvm_address (original bytecode address)           │
│   8 bytes: native_address (compiled native address)         │
│   4 bytes: hvm_length (bytes of bytecode)                   │
└─────────────────────────────────────────────────────────────┘
```

---

## 19. Extended LEB128 Reference

### 19.1 ULEB128 Quick Reference

| Value Range | Bytes Needed |
|-------------|--------------|
| 0 - 0x7F | 1 |
| 0x80 - 0x3FFF | 2 |
| 0x4000 - 0x1FFFFF | 3 |
| 0x200000 - 0xFFFFFFF | 4 |
| 0x10000000 - 0x7FFFFFFFF | 5 |

### 18.2 SLEB128 Quick Reference

| Value Range | Bytes Needed |
|-------------|--------------|
| -64 to 63 | 1 |
| -8192 to 8191 | 2 |
| -1048576 to 1048575 | 3 |
| -134217728 to 134217727 | 4 |
| -17179869184 to 17179869183 | 5 |

### 18.3 Decoder Implementation

```c
// ULEB128 Decoder
uint64_t decode_uleb128(const uint8_t* data, size_t* pos, size_t len) {
    uint64_t result = 0;
    uint64_t shift = 0;
    while (*pos < len) {
        uint64_t byte = data[*pos];
        (*pos)++;
        if ((byte & 0x80) == 0) {
            result |= (byte & 0x7F) << shift;
            break;
        }
        result |= (byte & 0x7F) << shift;
        shift += 7;
    }
    return result;
}

// SLEB128 Decoder
int64_t decode_sleb128(const uint8_t* data, size_t* pos, size_t len) {
    int64_t result = 0;
    int64_t shift = 0;
    uint8_t byte;
    while (*pos < len) {
        byte = data[*pos];
        (*pos)++;
        result |= (int64_t)(byte & 0x7F) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) {
            // Sign-extend if needed
            if (shift < 64 && (byte & 0x40))
                result |= -(1LL << shift);
            break;
        }
    }
    return result;
}
```

---

## 19. Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-04-16 | Initial specification |
| 1.1 | 2026-04-17 | Added debug info sections, extended opcodes, LEB128 encoding, TLS, Note sections, import resolution protocol, COMDAT groups, visibility flags |

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-04-16 | Initial specification |
| 1.1 | 2026-04-17 | Added debug information sections (DWARF-like), new debug instructions |

---

*This specification defines the HO file format for the Hooc Virtual Machine. For questions or clarification, refer to HVM_SPEC.md and the main project documentation.*
