# Compression API Reference (`Compression`)

The `Compression` class provides gzip and deflate compression and decompression utilities. All methods are static — call them directly on the class.

## 1. Gzip

### `Compression.gzipCompress(data: string, len: int64) :string`

Compresses `len` bytes from `data` using gzip compression.

- **Parameters:**
  - `data: string` — the raw byte data to compress.
  - `len: int64` — number of bytes to compress.
- **Returns:** `string` — the gzip-compressed data.

```hoo
var original = "Hello, Hoo!"
var data = original.data()
var len = original.length()
var compressed = Compression.gzipCompress(data, len)
```

---

### `Compression.gzipDecompress(data: string) :string`

Decompresses a gzip-compressed string.

- **Parameters:**
  - `data: string` — the gzip-compressed data.
- **Returns:** `string` — the decompressed original data.

```hoo
var original = "Hello, Hoo!"
var data = original.data()
var len = original.length()
var compressed = Compression.gzipCompress(data, len)
var decompressed = Compression.gzipDecompress(compressed)
```

## 2. Deflate

### `Compression.deflateCompress(data: string, len: int64) :string`

Compresses `len` bytes from `data` using the deflate algorithm.

- **Parameters:**
  - `data: string` — the raw byte data to compress.
  - `len: int64` — number of bytes to compress.
- **Returns:** `string` — the deflate-compressed data.

```hoo
var original = "Hello, Hoo!"
var data = original.data()
var len = original.length()
var compressed = Compression.deflateCompress(data, len)
```

---

### `Compression.deflateDecompress(data: string) :string`

Decompresses a deflate-compressed string.

- **Parameters:**
  - `data: string` — the deflate-compressed data.
- **Returns:** `string` — the decompressed original data.

```hoo
var original = "Hello, Hoo!"
var data = original.data()
var len = original.length()
var compressed = Compression.deflateCompress(data, len)
var decompressed = Compression.deflateDecompress(compressed)
```

## Usage Example

```hoo
var text = "The quick brown fox jumps over the lazy dog. "
var repeated = ""
var i: int64 = 0
while i < 100 {
    repeated = repeated + text
    i = i + 1
}
var data = repeated.data()
var len = repeated.length()

var gzipped = Compression.gzipCompress(data, len)
var deflated = Compression.deflateCompress(data, len)

println("Original: " + len)
println("Gzip: " + gzipped.length())
println("Deflate: " + deflated.length())

var restored = Compression.gzipDecompress(gzipped)
println("Restored: " + restored.length())
```
