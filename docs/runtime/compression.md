# Compression (`hoo.compression`)

The `hoo.compression` module provides gzip and raw deflate compress/decompress using zlib. Result is a heap-allocated byte buffer.

## 1. Gzip

- `Compression.gzip_compress(data, data_len)` — Gzip compress. Returns compressed buffer (free with `Compression.free_bytes`).
- `Compression.gzip_decompress(data, data_len)` — Gzip decompress.

## 2. Raw Deflate

- `Compression.deflate_compress(data, data_len)` — Raw deflate compress.
- `Compression.deflate_decompress(data, data_len)` — Raw deflate decompress.

## Usage from Hoo Source

All `Compression.*` functions are available on the `Compression` class:

```hoo
func :int64 demo() {
    var data = string_data("Hello, World!");
    var len = string_length("Hello, World!");
    var gzipped = Compression.gzip_compress(data, len);
    var deflated = Compression.deflate_compress(data, len);
    return string_length(gzipped);
}
```

## Memory Management

Output byte buffers must be freed with `Compression.free_bytes(data)`.
