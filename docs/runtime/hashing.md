# Hashing (`hoo.hashing`)

The `hoo.hashing` module provides SHA-256, SHA-1, MD5, CRC-32, and HMAC-SHA256 using Apple CommonCrypto (macOS) or equivalent, with table-based CRC-32. All hash functions return hex-encoded lowercase string output.

## 1. Hash Functions

- `hoo_hashing_sha256(data, len)` — SHA-256 hash. Returns hex string.
- `hoo_hashing_sha256_file(path)` — SHA-256 hash of a file. Returns hex string.
- `hoo_hashing_sha1(data, len)` — SHA-1 hash. Returns hex string.
- `hoo_hashing_md5(data, len)` — MD5 hash. Returns hex string.
- `hoo_hashing_crc32(data, len)` — CRC-32 checksum. Returns `uint64_t` (not hex-encoded).
- `hoo_hashing_hmac_sha256(key, key_len, data, data_len)` — HMAC-SHA256. Returns hex string.

## Memory Management

All hex string results must be freed with `hoo_hashing_free_string(str)`.
