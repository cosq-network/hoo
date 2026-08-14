# Compression API Reference

## Module

`hoo.compression`

## Import Statement

```hoo
import hoo.compression;
```

## Module Description

The Compression module provides the `Compression` class for compressing and decompressing data using the zlib library. It supports both gzip and deflate algorithms.

Instances are created with `new Compression()` and must be released with `release()` when no longer needed.

## Compression Class

### `new`

Creates a new Compression instance.

**Syntax:**

```hoo
new Compression()
```

**Returns:** `Compression` — A new Compression instance.

---

### `release`

Releases the resources held by the Compression instance.

**Syntax:**

```hoo
compression.release()
```

**Parameters:** None.

**Returns:** `int64` — `0` on success.

---

### `gzipCompress`

Compresses data using the gzip algorithm.

**Syntax:**

```hoo
compression.gzipCompress(data: ptr, length: int64): string
```

**Parameters:**

| Parameter | Type    | Description                                  |
|-----------|---------|----------------------------------------------|
| `data`    | `ptr`   | Raw byte pointer to the data to compress (e.g. `str.data()`). |
| `length`  | `int64` | The number of bytes to compress.             |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure returns an empty string.

---

### `gzipDecompress`

Decompresses gzip-compressed data.

**Syntax:**

```hoo
compression.gzipDecompress(data: ptr, length: int64): string
```

**Parameters:**

| Parameter | Type    | Description                                    |
|-----------|---------|------------------------------------------------|
| `data`    | `ptr`   | Raw byte pointer to the gzip-compressed data (e.g. `buf.data()`). |
| `length`  | `int64` | The number of bytes to decompress.             |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure returns an empty string.

---

### `deflateCompress`

Compresses data using the deflate algorithm (raw zlib format without gzip header).

**Syntax:**

```hoo
compression.deflateCompress(data: ptr, length: int64): string
```

**Parameters:**

| Parameter | Type    | Description                                  |
|-----------|---------|----------------------------------------------|
| `data`    | `ptr`   | Raw byte pointer to the data to compress (e.g. `str.data()`). |
| `length`  | `int64` | The number of bytes to compress.             |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure returns an empty string.

---

### `deflateDecompress`

Decompresses deflate-compressed data (raw zlib format without gzip header).

**Syntax:**

```hoo
compression.deflateDecompress(data: ptr, length: int64): string
```

**Parameters:**

| Parameter | Type    | Description                                      |
|-----------|---------|--------------------------------------------------|
| `data`    | `ptr`   | Raw byte pointer to the deflate-compressed data (e.g. `buf.data()`). |
| `length`  | `int64` | The number of bytes to decompress.               |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure returns an empty string.

---

## Free Functions

### `compression_gzip_compress_slice`

Compresses the bytes of a byte slice using the gzip algorithm, without requiring a `Compression` instance.

**Syntax:**

```hoo
compression_gzip_compress_slice(slice: ByteSlice): Buffer
```

**Parameters:**

| Parameter | Type        | Description                    |
|-----------|-------------|--------------------------------|
| `slice`   | `ByteSlice` | The byte slice to compress.    |

**Returns:** `Buffer` — The compressed data.

**Errors:** Compression failure returns an empty buffer.

---

### `compression_deflate_compress_slice`

Compresses the bytes of a byte slice using the deflate algorithm (raw zlib format without gzip header), without requiring a `Compression` instance.

**Syntax:**

```hoo
compression_deflate_compress_slice(slice: ByteSlice): Buffer
```

**Parameters:**

| Parameter | Type        | Description                  |
|-----------|-------------|------------------------------|
| `slice`   | `ByteSlice` | The byte slice to compress.  |

**Returns:** `Buffer` — The compressed data.

**Errors:** Compression failure returns an empty buffer.

---

## Usage Example

```hoo
import hoo.compression;

func :int64 main() {
    var c = new Compression();
    var original = "Hello, Hoo! This is a test string for compression.";

    var compressed = c.gzipCompress(original.data(), original.length());
    var decompressed = c.gzipDecompress(compressed.data(), compressed.length());
    println(decompressed);

    var deflated = c.deflateCompress(original.data(), original.length());
    var inflated = c.deflateDecompress(deflated.data(), deflated.length());
    println(inflated);

    c.release();
    return 0;
}
```
