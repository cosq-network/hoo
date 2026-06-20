# Compression API Reference

## Module

`hoo.compression`

## Import Statement

```hoo
import hoo.compression;
```

## Module Description

The Compression module provides free functions for compressing and decompressing data using the zlib library. It supports both string and raw byte-array compression via gzip and deflate algorithms.

## Free Functions

---

### `compression_compress`

Compresses a string using zlib compression and returns the compressed data as a string.

**Syntax:**

```hoo
compression_compress(data: string): string
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `data`    | `string` | The string to compress.   |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure causes a runtime error.

---

### `compression_decompress`

Decompresses zlib-compressed data back into the original string.

**Syntax:**

```hoo
compression_decompress(data: string): string
```

**Parameters:**

| Parameter | Type     | Description                 |
|-----------|----------|-----------------------------|
| `data`    | `string` | The compressed string data. |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure causes a runtime error.

---

### `compression_compress_bytes`

Compresses a raw byte array using zlib and returns the compressed byte array.

**Syntax:**

```hoo
compression_compress_bytes(data: array): array
```

**Parameters:**

| Parameter | Type    | Description                   |
|-----------|---------|-------------------------------|
| `data`    | `array` | The raw byte array to compress. |

**Returns:** `array` — The compressed byte array.

**Errors:** Compression failure causes a runtime error.

---

### `compression_decompress_bytes`

Decompresses a compressed byte array back into the original bytes.

**Syntax:**

```hoo
compression_decompress_bytes(data: array): array
```

**Parameters:**

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| `data`    | `array` | The compressed byte array data.   |

**Returns:** `array` — The decompressed original byte array.

**Errors:** Decompression failure causes a runtime error.

## Usage Example

```hoo
import hoo.compression;

func :int64 main() {
    var original = "Hello, Hoo! This is a test string for compression.";

    // Compress and decompress a string
    var compressed = compression_compress(original);
    var decompressed = compression_decompress(compressed);
    println(decompressed);

    // Compress and decompress raw bytes
    var bytes = [72, 101, 108, 108, 111]any;
    var comp = compression_compress_bytes(bytes);
    var decomp = compression_decompress_bytes(comp);

    return 0;
}
```
