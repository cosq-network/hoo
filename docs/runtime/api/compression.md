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

**Returns:** `string` — An empty string.

---

### `gzipCompress`

Compresses data using the gzip algorithm.

**Syntax:**

```hoo
compression.gzipCompress(data: string, length: int64): string
```

**Parameters:**

| Parameter | Type     | Description                      |
|-----------|----------|----------------------------------|
| `data`    | `string` | The data to compress.            |
| `length`  | `int64`  | The number of bytes to compress. |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure returns an empty string.

---

### `gzipDecompress`

Decompresses gzip-compressed data.

**Syntax:**

```hoo
compression.gzipDecompress(data: string, length: int64): string
```

**Parameters:**

| Parameter | Type     | Description                        |
|-----------|----------|------------------------------------|
| `data`    | `string` | The gzip-compressed data.          |
| `length`  | `int64`  | The number of bytes to decompress. |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure returns an empty string.

---

### `deflateCompress`

Compresses data using the deflate algorithm (raw zlib format without gzip header).

**Syntax:**

```hoo
compression.deflateCompress(data: string, length: int64): string
```

**Parameters:**

| Parameter | Type     | Description                      |
|-----------|----------|----------------------------------|
| `data`    | `string` | The data to compress.            |
| `length`  | `int64`  | The number of bytes to compress. |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure returns an empty string.

---

### `deflateDecompress`

Decompresses deflate-compressed data (raw zlib format without gzip header).

**Syntax:**

```hoo
compression.deflateDecompress(data: string, length: int64): string
```

**Parameters:**

| Parameter | Type     | Description                          |
|-----------|----------|--------------------------------------|
| `data`    | `string` | The deflate-compressed data.         |
| `length`  | `int64`  | The number of bytes to decompress.   |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure returns an empty string.

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
