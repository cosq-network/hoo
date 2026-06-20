# Compression API Reference

## Compression

The Compression module belongs to the Hoo standard library.

## Import Statement

```hoo
import hoo.compression;
```

## Module Description

The Compression module provides static utility functions for compressing and decompressing data using the zlib library. It supports both string and raw byte-array compression via gzip and deflate algorithms.

## Class: Compression

A static utility class providing compression and decompression operations.

### Declaration

```hoo
class Compression
```

### Public Fields

None.

### Public Class (Static) Functions

#### compress

Compresses a string using zlib compression and returns the compressed data as a string.

```hoo
Compression.compress(data: string): string
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `data`    | `string` | The string to compress.   |

**Returns:** `string` — The compressed data.

**Errors:** Compression failure causes a runtime error.

---

#### decompress

Decompresses zlib-compressed data back into the original string.

```hoo
Compression.decompress(data: string): string
```

**Parameters:**

| Parameter | Type     | Description                 |
|-----------|----------|-----------------------------|
| `data`    | `string` | The compressed string data. |

**Returns:** `string` — The decompressed original data.

**Errors:** Decompression failure causes a runtime error.

---

#### compress_bytes

Compresses a raw byte array using zlib and returns the compressed byte array.

```hoo
Compression.compress_bytes(data: array): array
```

**Parameters:**

| Parameter | Type    | Description                   |
|-----------|---------|-------------------------------|
| `data`    | `array` | The raw byte array to compress. |

**Returns:** `array` — The compressed byte array.

**Errors:** Compression failure causes a runtime error.

---

#### decompress_bytes

Decompresses a compressed byte array back into the original bytes.

```hoo
Compression.decompress_bytes(data: array): array
```

**Parameters:**

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| `data`    | `array` | The compressed byte array data.   |

**Returns:** `array` — The decompressed original byte array.

**Errors:** Decompression failure causes a runtime error.

---

#### free_bytes

Frees memory allocated by a compressed or decompressed byte array operation.

```hoo
Compression.free_bytes(data: array): void
```

**Parameters:**

| Parameter | Type    | Description                               |
|-----------|---------|-------------------------------------------|
| `data`    | `array` | The byte array to free.                   |

**Returns:** `void`

---

## Usage Example

```hoo
import hoo.compression;

func :int64 main() {
    var original = "Hello, Hoo! This is a test string for compression.";

    // Compress and decompress a string
    var compressed = Compression.compress(original);
    var decompressed = Compression.decompress(compressed);
    println(decompressed);

    // Compress and decompress raw bytes
    var bytes = [72, 101, 108, 108, 111]any;
    var comp = Compression.compress_bytes(bytes);
    var decomp = Compression.decompress_bytes(comp);
    Compression.free_bytes(comp);
    Compression.free_bytes(decomp);

    return 0;
}
```
