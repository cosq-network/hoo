# Compression (`hoo.compression`)

The `hoo.compression` module provides gzip and raw deflate compress/decompress using zlib. Create an instance with `Compression.new()` and release it with `release()`.

## 1. Constructor / Destructor

- `Compression.new()` — Create a new Compression instance.
- `c.release()` — Release the instance.

## 2. Gzip

- `c.gzipCompress(data, len)` — Gzip compress. Returns compressed string.
- `c.gzipDecompress(data, len)` — Gzip decompress. Returns original string.

## 3. Raw Deflate

- `c.deflateCompress(data, len)` — Raw deflate compress. Returns compressed string.
- `c.deflateDecompress(data, len)` — Raw deflate decompress. Returns original string.

## Usage from Hoo Source

```hoo
func :int64 demo() {
    var c = Compression.new()
    var original = "Hello, World!"
    var compressed = c.gzipCompress(original.data(), original.length())
    c.release()
    return 1
}
```

## Memory Management

Output byte buffers from compress/decompress are automatically wrapped into Hoo strings by the runtime. No manual freeing is needed.
