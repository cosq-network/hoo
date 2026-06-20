# Hashing API Reference

## Module Name

Part of the `hoo` module.

## Import Statement

```hoo
import hoo;
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

**Returns:**

`int64` — The CRC-32 checksum as an unsigned 32-bit value (mapped to `int64` in Hoo). Returns `0` on error or null input.

**Errors:**

Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo;

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

**Returns:**

`string` — The MD5 hex digest (32 hexadecimal characters). Returns `0` on error or null input.

**Errors:**

Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo;

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

**Returns:**

`string` — The SHA-1 hex digest (40 hexadecimal characters). Returns `0` on error or null input.

**Errors:**

Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo;

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

**Returns:**

`string` — The SHA-256 hex digest (64 hexadecimal characters). Returns `0` on error or null input.

**Errors:**

Returns `0` if `data` is nil.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var hash = hashing_sha256("Hello, World!");
    if hash != 0 {
        println(hash); // dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f
    }
    return 0;
}
```

## Usage Example

```hoo
import hoo;

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
