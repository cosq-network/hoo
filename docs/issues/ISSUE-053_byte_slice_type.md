# ISSUE-053: Missing First-Class `byte[]` / Slice Type Causes Runtime API Duplication

## 1. Overview
Many runtime APIs come in pairs — one accepting raw `(data, len)` pointer parameters and another accepting `Buffer` handles. This duplication exists because the language lacks a first-class `byte[]` or span type that can represent a borrowed view of bytes without ownership transfer.

## 2. Technical Analysis
- **Encoding** (`hoo_encoding.h`):
  - `base64_encode(data, len)` / `base64_encode_buffer(Buffer)`
  - `hex_encode(data, len)` / `hex_encode_buffer(Buffer)`
- **Hashing** (`hoo_hashing.h`):
  - `sha256(data, len)` / `sha256_buffer(Buffer)`
  - `md5(data, len)` / `md5_buffer(Buffer)`
  - `crc32(data, len)` / `crc32_buffer(Buffer)`
- **Compression** (`hoo_compression.h`):
  - `gzipCompress(data, len)` / `gzipCompress(Buffer)`
  - `deflateCompress(data, len)` / `deflateCompress(Buffer)`
- **Filesystem** (`hoo_fs.h`):
  - `read_bytes(path, out)` / `read_bytes_buffer(path)`

ISSUE-041 (function overloading) explicitly notes: "raw byte overloads only after the language has a stable byte-slice type."

## 3. Impact
- API surface is unnecessarily large and confusing.
- Raw pointer/length pairs cannot be used safely from Hoo source code.
- Every new byte-oriented API needs at least two variants.

## 4. Suggested Fix
1. Define a `byte[]` or `Span` type in the grammar and AST.
2. Add runtime representation: `{ ptr: int64, len: int64 }` (borrowed, not owned).
3. Provide implicit conversions from `Buffer` to `byte[]`.
4. Unify paired APIs into single overloads accepting `byte[]`.

## 5. Status
- **Date**: 2026-08-09
- **Status**: **IMPLEMENTED**
- **Priority**: **MEDIUM**

## 6. Resolution
The runtime defines `HooByteSlice { const uint8_t* data; int64_t length; }`
as a borrowed, read-only C ABI view. It has explicit constructors from raw
bytes and `Buffer`, validity checks, and no ownership or ARC side effects.
The `slice<byte>` source type is now accepted as a borrowed pointer ABI type,
and encoding, hashing, compression, and socket runtime entry points accept
slice handles. Existing raw and `Buffer` APIs remain compatible aliases while
callers migrate.

The backing `Buffer` must outlive the slice handle. Releasing a slice never
releases or copies its backing storage; APIs that produce results return new
owned strings or `Buffer` objects.
