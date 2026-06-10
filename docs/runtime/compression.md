# Compression (`hoo.compression`)

The `hoo.compression` module provides gzip and raw deflate compress/decompress using zlib. Result is a heap-allocated byte buffer.

## 1. Gzip

- `Compression.gzipCompress(data, data_len)` — Gzip compress. Returns compressed buffer (free with `Compression.freeBytes`).
- `Compression.gzipDecompress(data, data_len)` — Gzip decompress.

## 2. Raw Deflate

- `Compression.deflateCompress(data, data_len)` — Raw deflate compress.
- `Compression.deflateDecompress(data, data_len)` — Raw deflate decompress.

## Usage from Hoo Source

All `Compression.*` functions are available on the `Compression` class:

```hoo
func :int64 demo() {
    var data = string_data("Hello, World!");
    var len = string_length("Hello, World!");
    var gzipped = Compression.gzipCompress(data, len);
    var deflated = Compression.deflateCompress(data, len);
    return string_length(gzipped);
}
```

## Memory Management

Output byte buffers must be freed with `Compression.freeBytes(data)`.
