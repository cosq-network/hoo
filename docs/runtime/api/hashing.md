# Hashing API Reference

## Module Name

`hoo.hashing`

## Import Statement

```hoo
import hoo.hashing;
```

## Module Description

The `hashing` module provides free functions for computing hash digests and checksums. The module supports CRC-32, MD5, SHA-1, and SHA-256 algorithms, returning results as hex-encoded strings or integer checksum values.

## Functions

### Function: `hashing_crc32`

Computes the CRC-32 checksum of a string.

**Syntax:**

```hoo
hashing_crc32(data: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The input string to hash. |

**Returns:** `int64` — The CRC-32 checksum as an unsigned 32-bit value (mapped to `int64` in Hoo). Returns `0` on error or null input.

**Errors:** Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var crc = hashing_crc32("Hello");
    println(crc); // 907060870
    return 0;
}
```

---

### Function: `hashing_md5`

Computes the MD5 hash of a string and returns the digest as a hex-encoded string.

**Syntax:**

```hoo
hashing_md5(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The input string to hash. |

**Returns:** `string` — The MD5 hex digest (32 hexadecimal characters). Returns `0` on error or null input.

**Errors:** Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var hash = hashing_md5("Hello");
    if hash != 0 {
        println(hash); // 8b1a9953c4611296a827abf8c47804d7
    }
    return 0;
}
```

---

### Function: `hashing_sha1`

Computes the SHA-1 hash of a string and returns the digest as a hex-encoded string.

**Syntax:**

```hoo
hashing_sha1(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The input string to hash. |

**Returns:** `string` — The SHA-1 hex digest (40 hexadecimal characters). Returns `0` on error or null input.

**Errors:** Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var hash = hashing_sha1("Hello");
    if hash != 0 {
        println(hash); // f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0
    }
    return 0;
}
```

---

### Function: `hashing_sha256`

Computes the SHA-256 hash of a string and returns the digest as a hex-encoded string.

**Syntax:**

```hoo
hashing_sha256(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The input string to hash. |

**Returns:** `string` — The SHA-256 hex digest (64 hexadecimal characters). Returns `0` on error or null input.

**Errors:** Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var hash = hashing_sha256("Hello, World!");
    if hash != 0 {
        println(hash); // dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f
    }
    return 0;
}
```

---

### Function: `hashing_sha256_file`

Computes the SHA-256 hash of a file and returns the digest as a hex-encoded string.

**Syntax:**

```hoo
hashing_sha256_file(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to the file to hash. |

**Returns:** `string` — The SHA-256 hex digest (64 hexadecimal characters). Returns `0` on error.

**Errors:** Returns `0` if `path` is nil or the file cannot be read.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var hash = hashing_sha256_file("/tmp/data.txt");
    if hash != 0 {
        println(hash);
    }
    return 0;
}
```

---

### Function: `hashing_hmac_sha256`

Computes the HMAC-SHA256 of a message using a secret key.

**Syntax:**

```hoo
hashing_hmac_sha256(key: string, data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `key` | `string` | The secret key. |
| `data` | `string` | The message data to authenticate. |

**Returns:** `string` — The HMAC-SHA256 hex digest (64 hexadecimal characters). Returns `0` on error.

**Errors:** Returns `0` if either argument is nil.

**Complete Example:**

```hoo
import hoo.hashing;

func :int64 main() {
    var hmac = hashing_hmac_sha256("secret", "message");
    if hmac != 0 {
        println(hmac);
    }
    return 0;
}
```

---

### Function: `hashing_crc32_buffer`

Computes the CRC-32 checksum of a Buffer.

**Syntax:**

```hoo
hashing_crc32_buffer(buf: Buffer) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `buf` | `Buffer` | The buffer to hash. |

**Returns:** `int64` — The CRC-32 checksum. Returns `0` on error.

**Errors:** Returns `0` if `buf` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var crc = hashing_crc32_buffer(buf);
    println(crc);
    buf.release();
    return 0;
}
```

---

### Function: `hashing_md5_buffer`

Computes the MD5 hash of a Buffer and returns the hex digest.

**Syntax:**

```hoo
hashing_md5_buffer(buf: Buffer) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `buf` | `Buffer` | The buffer to hash. |

**Returns:** `string` — The MD5 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `buf` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var hash = hashing_md5_buffer(buf);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_sha1_buffer`

Computes the SHA-1 hash of a Buffer and returns the hex digest.

**Syntax:**

```hoo
hashing_sha1_buffer(buf: Buffer) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `buf` | `Buffer` | The buffer to hash. |

**Returns:** `string` — The SHA-1 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `buf` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var hash = hashing_sha1_buffer(buf);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_sha256_buffer`

Computes the SHA-256 hash of a Buffer and returns the hex digest.

**Syntax:**

```hoo
hashing_sha256_buffer(buf: Buffer) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `buf` | `Buffer` | The buffer to hash. |

**Returns:** `string` — The SHA-256 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `buf` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello, World!");
    var hash = hashing_sha256_buffer(buf);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_hmac_sha256_buffer`

Computes the HMAC-SHA256 of a Buffer using a Buffer key.

**Syntax:**

```hoo
hashing_hmac_sha256_buffer(key: Buffer, data: Buffer) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `key` | `Buffer` | The secret key buffer. |
| `data` | `Buffer` | The message data buffer to authenticate. |

**Returns:** `string` — The HMAC-SHA256 hex digest (64 hexadecimal characters). Returns `0` on error.

**Errors:** Returns `0` if either argument is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var key = Buffer(32);
    key.write("secret");
    var data = Buffer(64);
    data.write("message");
    var hmac = hashing_hmac_sha256_buffer(key, data);
    if hmac != 0 { println(hmac); }
    key.release();
    data.release();
    return 0;
}
```

---

### Function: `hashing_sha256_slice`

Computes the SHA-256 hash of a ByteSlice and returns the hex digest.

**Syntax:**

```hoo
hashing_sha256_slice(slice: ByteSlice) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `slice` | `ByteSlice` | The byte slice to hash. |

**Returns:** `string` — The SHA-256 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `slice` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello, World!");
    var slice = buf.slice(0, 5);
    var hash = hashing_sha256_slice(slice);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_sha1_slice`

Computes the SHA-1 hash of a ByteSlice and returns the hex digest.

**Syntax:**

```hoo
hashing_sha1_slice(slice: ByteSlice) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `slice` | `ByteSlice` | The byte slice to hash. |

**Returns:** `string` — The SHA-1 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `slice` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var slice = buf.slice(0, 5);
    var hash = hashing_sha1_slice(slice);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_md5_slice`

Computes the MD5 hash of a ByteSlice and returns the hex digest.

**Syntax:**

```hoo
hashing_md5_slice(slice: ByteSlice) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `slice` | `ByteSlice` | The byte slice to hash. |

**Returns:** `string` — The MD5 hex digest. Returns `0` on error.

**Errors:** Returns `0` if `slice` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var slice = buf.slice(0, 5);
    var hash = hashing_md5_slice(slice);
    if hash != 0 { println(hash); }
    buf.release();
    return 0;
}
```

---

### Function: `hashing_crc32_slice`

Computes the CRC-32 checksum of a ByteSlice.

**Syntax:**

```hoo
hashing_crc32_slice(slice: ByteSlice) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `slice` | `ByteSlice` | The byte slice to checksum. |

**Returns:** `int64` — The CRC-32 checksum. Returns `0` on error.

**Errors:** Returns `0` if `slice` is null.

**Complete Example:**

```hoo
import hoo.hashing;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var slice = buf.slice(0, 5);
    var crc = hashing_crc32_slice(slice);
    println(crc);
    buf.release();
    return 0;
}
```

---

## Usage Example

```hoo
import hoo.hashing;

func :int64 main() {
    var input = "Hello, Hoo!";

    var crc = hashing_crc32(input);
    println("CRC-32:  " + crc);

    var md5 = hashing_md5(input);
    if md5 != 0 {
        println("MD5:     " + md5);
    }

    var sha1 = hashing_sha1(input);
    if sha1 != 0 {
        println("SHA-1:   " + sha1);
    }

    var sha256 = hashing_sha256(input);
    if sha256 != 0 {
        println("SHA-256: " + sha256);
    }

    return 0;
}
```
