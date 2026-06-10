# Hashing

Singleton class providing hashing and HMAC functions.

## Static Methods

### `Hashing.sha256(data: string, len: int64) :string`

Returns the SHA-256 hex digest of `len` bytes from `data`.

```hoo
let hash = Hashing.sha256("Hello, World!", 13)
// hash == "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f"
```

### `Hashing.sha256File(path: string) :string`

Returns the SHA-256 hex digest of a file's contents.

```hoo
let hash = Hashing.sha256File("/path/to/file.bin")
```

### `Hashing.sha1(data: string, len: int64) :string`

Returns the SHA-1 hex digest of `len` bytes from `data`.

```hoo
let hash = Hashing.sha1("Hello", 5)
// hash == "f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0"
```

### `Hashing.md5(data: string, len: int64) :string`

Returns the MD5 hex digest of `len` bytes from `data`.

```hoo
let hash = Hashing.md5("Hello", 5)
// hash == "8b1a9953c4611296a827abf8c47804d7"
```

### `Hashing.crc32(data: string, len: int64) :int64`

Returns the CRC-32 checksum of `len` bytes from `data`.

```hoo
let crc = Hashing.crc32("Hello", 5)
// crc == 907060870
```

### `Hashing.hmacSha256(key: string, keyLen: int64, data: string, dataLen: int64) :string`

Returns the HMAC-SHA256 hex digest.

```hoo
let hmac = Hashing.hmacSha256("key", 3, "data", 4)
// hmac == "104152c5bfdca07bc633eebd46199f0255c2f48d2e584e4b485d239b5c3c14cf"
```
