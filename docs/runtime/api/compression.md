# Compression API Reference (`Compression`)

The `Compression` class provides gzip and deflate compression and decompression utilities. Create an instance with `Compression.new()` and release it with `release()`.

## 1. Constructor / Destructor

### `Compression.new() :Compression`

Creates a new Compression instance.

```hoo
var c = Compression.new()
```

### `c.release()`

Releases the Compression instance and its resources.

```hoo
c.release()
```

## 2. Gzip

### `c.gzipCompress(data, len) :string`

Compresses `len` bytes from `data` using gzip compression.

- **Parameters:**
  - `data` — the raw byte data to compress.
  - `len` — number of bytes to compress.
- **Returns:** `string` — the gzip-compressed data.

```hoo
var c = Compression.new()
var original = "Hello, Hoo!"
var compressed = c.gzipCompress(original.data(), original.length())
```

---

### `c.gzipDecompress(data, len) :string`

Decompresses `len` bytes of gzip-compressed data.

- **Parameters:**
  - `data` — the gzip-compressed data.
  - `len` — number of bytes to decompress.
- **Returns:** `string` — the decompressed original data.

```hoo
var c = Compression.new()
var original = "Hello, Hoo!"
var compressed = c.gzipCompress(original.data(), original.length())
var decompressed = c.gzipDecompress(compressed.data(), compressed.length())
c.release()
```

## 3. Deflate

### `c.deflateCompress(data, len) :string`

Compresses `len` bytes from `data` using the deflate algorithm.

```hoo
var c = Compression.new()
var original = "Hello, Hoo!"
var compressed = c.deflateCompress(original.data(), original.length())
```

---

### `c.deflateDecompress(data, len) :string`

Decompresses `len` bytes of deflate-compressed data.

```hoo
var c = Compression.new()
var original = "Hello, Hoo!"
var compressed = c.deflateCompress(original.data(), original.length())
var decompressed = c.deflateDecompress(compressed.data(), compressed.length())
c.release()
```

### Buffer-Aware Overloads

Each compression method also accepts a `Buffer` handle and returns a `Buffer`:

- `c.gzipCompress(buf: buffer) :buffer` — gzip-compress a buffer.
- `c.gzipDecompress(buf: buffer) :buffer` — gzip-decompress a buffer.
- `c.deflateCompress(buf: buffer) :buffer` — deflate-compress a buffer.
- `c.deflateDecompress(buf: buffer) :buffer` — deflate-decompress a buffer.

```hoo
var c = Compression.new()
var input = Buffer.fromBytes("Hello, Hoo!", 12)
var compressed = c.gzipCompress(input)
var decompressed = c.gzipDecompress(compressed)
c.release()
```

## Usage Example

```hoo
var c = Compression.new()
var text = "The quick brown fox jumps over the lazy dog. "
var repeated = ""
var i: int64 = 0
while i < 100 {
    repeated = repeated + text
    i = i + 1
}

var gzipped = c.gzipCompress(repeated.data(), repeated.length())
var deflated = c.deflateCompress(repeated.data(), repeated.length())

println("Original: " + repeated.length())
println("Gzip: " + gzipped.length())
println("Deflate: " + deflated.length())

var restored = c.gzipDecompress(gzipped.data(), gzipped.length())
println("Restored: " + restored.length())
c.release()
```
