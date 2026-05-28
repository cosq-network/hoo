# Compression (`hoo.compression`)

The `hoo.compression` module provides gzip and raw deflate compress/decompress using zlib. Result is a heap-allocated byte buffer.

## 1. Gzip

- `hoo_compression_gzip_compress(data, data_len, &out_data, &out_len)` — Gzip compress. Returns 0 on success, non-zero on failure.
- `hoo_compression_gzip_decompress(data, data_len, &out_data, &out_len)` — Gzip decompress.

## 2. Raw Deflate

- `hoo_compression_deflate_compress(data, data_len, &out_data, &out_len)` — Raw deflate compress.
- `hoo_compression_deflate_decompress(data, data_len, &out_data, &out_len)` — Raw deflate decompress.

## Memory Management

Output byte buffers must be freed with `hoo_compression_free_bytes(data)`.
