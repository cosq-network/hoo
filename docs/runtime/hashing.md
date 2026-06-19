# Hashing (`hoo.hashing`)

The `hoo.hashing` module provides free functions for SHA-256, SHA-1, MD5, CRC-32, and HMAC-SHA256 operations. All hash functions return hex-encoded lowercase string output (except CRC-32, which returns a number).

## 1. Hash Functions

- `hashing_sha256(data, len)` — SHA-256 hash. Returns hex string.
- `hashing_sha256_file(path)` — SHA-256 hash of a file. Returns hex string.
- `hashing_sha1(data, len)` — SHA-1 hash. Returns hex string.
- `hashing_md5(data, len)` — MD5 hash. Returns hex string.
- `hashing_crc32(data, len)` — CRC-32 checksum. Returns `int64_t` (not hex-encoded).
- `hashing_hmac_sha256(key, key_len, data, data_len)` — HMAC-SHA256. Returns hex string.

For buffer operations, the corresponding `_buffer` versions are available:
- `hashing_sha256_buffer(buf)`
- `hashing_sha1_buffer(buf)`
- `hashing_md5_buffer(buf)`
- `hashing_crc32_buffer(buf)`
- `hashing_hmac_sha256_buffer(key_buf, data_buf)`

## Usage from Hoo Source

Import the module to call the free functions:

```hoo
import hoo.hashing;

func :int64 demo() {
    var data = "hello";
    var bytes = data.data();
    var len = data.length();
    var sha256 = hashing_sha256(bytes, len);            // 64-char hex string
    var sha1 = hashing_sha1(bytes, len);                // 40-char hex string
    var md5 = hashing_md5(bytes, len);                  // 32-char hex string
    var crc = hashing_crc32(bytes, len);                // int64 checksum
    var file = hashing_sha256_file("/path/to/file");    // 64-char hex string
    return sha256.length();
}
```
