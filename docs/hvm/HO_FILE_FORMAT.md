# HVM Object File Format Specification (HO)

**Version:** 1.0
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

### 2.3 Object Layout

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

---

## 4. Header Specification

### 4.1 File Header (64 bytes)

```
Offset  Size  Field                Description
────────────────────────────────────────────────────────────────
0x00    4     magic                Magic number: 0x484F4F43 ("HOOC")
0x04    2     version_major        Format version major (1)
0x06    2     version_minor        Format version minor (0)
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
01 00           ; Version: 1.0
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

### 5.3 Section Flags

```
Bit 31-16: Reserved
Bit 15   : Sections flags this entry as a TLS segment
Bit 14   : Alloc flag (occupies memory when loaded)
Bit 13   : Write flag (writable)
Bit 12   : Execute flag (executable)
Bit 11   : Merge flag (can be merged with same name)
Bit 10   : Strings flag (contains null-terminated strings)
Bits 9-0 : Reserved
```

---

## 6. Section Types

### 6.1 `.text` Section

Contains raw HVM bytecode instructions.

**Requirements:**
- Must be aligned to 16 bytes in memory
- Entry point must be within this section
- Instructions are 4 bytes each, unaligned jumps are allowed

**Example Layout:**
```
Offset  Content
0x0000  14 00 00 00 00 00 00 00  ; LUI r0, 0x0
0x0008  14 00 00 00 01 00 00 00  ; LUI r0, 0x1
0x0010  34 00 10 00 ...          ; ADDI r0, r0, 16
        ...                      ; more instructions
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

---

## 7. Symbol Table

### 7.1 Symbol Entry (32 bytes)

```
Offset  Size  Field              Description
────────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab
0x04    1     binding            STB_LOCAL(0), STB_GLOBAL(1), STB_WEAK(2)
0x05    1     type               STT_NOTYPE(0), STT_FUNC(1), STT_OBJECT(2), STT_TYPE(3)
0x06    2     reserved           Reserved for future use
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

### 7.3 Symbol Types

| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `STT_NOTYPE` | Type not specified |
| `0x01` | `STT_FUNC` | Function or procedure |
| `0x02` | `STT_OBJECT` | Variable, array, or object |
| `0x03` | `STT_TYPE` | Type descriptor |

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
────────────────────────────────────────────────────────────────
0x00    4     name_offset        Offset into .strtab (function/variable name)
0x04    4     library_offset     Offset into .strtab (library name)
0x08    4     hints              Hints for loader (e.g., hash)
0x0C    4     reserved           Reserved
0x10    8     flags              Import flags
```

### 9.3 Import Library Naming

Imports can be from:
- **Standard library**: `hooc:std` (runtime, stdlib)
- **User libraries**: `hooc:module_name`
- **Native interop**: `native:c_function_name` (FFI)

### 9.4 Example Import Table

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
func main() -> int64 {
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
    func add(a: int64, b: int64) -> int64 {
        return a + b;
    }

    func mul(a: int64, b: int64) -> int64 {
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
func foo() -> int64 {
    return 42;
}

export foo;
```

**File 2 (`b.hoo`):**
```hooc
import foo from "local";

func bar() -> int64 {
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

func main() -> int64 {
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

### 16.3 JIT Compilation Notes

When JIT-compiling HVM bytecode:

1. **Instruction Alignment**: Text section is 16-byte aligned
2. **Jump Patching**: Internal jumps patched after native code emission
3. **GOT Access**: External symbols accessed via Global Offset Table
4. **Stack Spill**: Registers spilled to stack as needed
5. **Calling Convention**: Follow HVMCC (see Section 11.3)

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-04-16 | Initial specification |

---

*This specification defines the HO file format for the Hooc Virtual Machine. For questions or clarification, refer to HVM_SPEC.md and the main project documentation.*
