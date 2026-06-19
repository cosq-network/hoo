# Hashing API Reference (`hoo.hashing`)

**Import Requirement:**
```hoo
import hoo.hashing;
```

The `hoo.hashing` module provides free functions for hashing and HMAC operations.

## Functions

### `hashing_sha256(data: string, len: int64) :string`

Returns the SHA-256 hex digest of `len` bytes from `data`.

```hoo
import hoo.hashing;

func :void example() {
    var hash = hashing_sha256("Hello, World!", 13);
    // hash == "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f"
}
```

### `hashing_sha256_file(path: string) :string`

Returns the SHA-256 hex digest of a file's contents.

```hoo
import hoo.hashing;

func :void example() {
    var hash = hashing_sha256_file("/path/to/file.bin");
}
```

### `hashing_sha1(data: string, len: int64) :string`

Returns the SHA-1 hex digest of `len` bytes from `data`.

```hoo
import hoo.hashing;

func :void example() {
    var hash = hashing_sha1("Hello", 5);
    // hash == "f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0"
}
```

### `hashing_md5(data: string, len: int64) :string`

Returns the MD5 hex digest of `len` bytes from `data`.

```hoo
import hoo.hashing;

func :void example() {
    var hash = hashing_md5("Hello", 5);
    // hash == "8b1a9953c4611296a827abf8c47804d7"
}
```

### `hashing_crc32(data: string, len: int64) :int64`

Returns the CRC-32 checksum of `len` bytes from `data`.

```hoo
import hoo.hashing;

func :void example() {
    var crc = hashing_crc32("Hello", 5);
    // crc == 907060870
}
```

### `hashing_hmac_sha256(key: string, keyLen: int64, data: string, dataLen: int64) :string`

Returns the HMAC-SHA256 hex digest of `data` using the specified `key`.

```hoo
import hoo.hashing;

func :void example() {
    var hmac = hashing_hmac_sha256("key", 3, "data", 4);
    // hmac == "104152c5bfdca07bc633eebd46199f0255c2f48d2e584e4b485d239b5c3c14cf"
}
```

### Buffer-Aware Functions

For buffer operations, use the corresponding `_buffer` suffixed functions:

- `hashing_sha256_buffer(buf: buffer) :string`
- `hashing_sha1_buffer(buf: buffer) :string`
- `hashing_md5_buffer(buf: buffer) :string`
- `hashing_crc32_buffer(buf: buffer) :int64`
- `hashing_hmac_sha256_buffer(key: buffer, data: buffer) :string`

```hoo
import hoo.buffer;
import hoo.hashing;

func :void example() {
    var buf = buffer_fromBytes("Hello, World!", 13);
    var hash = hashing_sha256_buffer(buf);
}
```
