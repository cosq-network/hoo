# Encoding API Reference

## Module

`hoo.encoding`

## Import Statement

```hoo
import hoo.encoding;
```

## Module Description

The encoding module provides Base64, hex, and URL percent-encoding encode/decode functions. Functions accept strings and return strings. Buffer variants accept and return `Buffer` objects for binary data. All functions throw `RuntimeException` on nil input or encoding/decoding failure.

## Base64 Functions

### `encoding_base64_encode`

Encodes a string to Base64.

**Syntax:**

```hoo
encoding_base64_encode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The string to encode. |

**Returns:** `string` — A Base64-encoded string.

**Errors:** Throws `RuntimeException` if `data` is nil or encoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let encoded = encoding_base64_encode("Hello, World!");
// encoded == "SGVsbG8sIFdvcmxkIQ=="
```

---

### `encoding_base64_decode`

Decodes a Base64-encoded string back to the original string.

**Syntax:**

```hoo
encoding_base64_decode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The Base64-encoded string to decode. |

**Returns:** `string` — The decoded string.

**Errors:** Throws `RuntimeException` if `data` is nil, malformed Base64, or decoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let decoded = encoding_base64_decode("SGVsbG8sIFdvcmxkIQ==");
// decoded == "Hello, World!"
```

---

### `encoding_base64_encode_buffer`

Base64-encodes a buffer's contents.

**Syntax:**

```hoo
encoding_base64_encode_buffer(buf: Buffer) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `buf` | `Buffer` | The buffer to encode. |

**Returns:** `string` — A Base64-encoded string.

**Errors:** Throws `RuntimeException` if `data` is nil or encoding fails.

**Complete Example:**

```hoo
import hoo.buffer;
import hoo.encoding;

let buf = buffer_fromBytes("Hello", 5);
let encoded = encoding_base64_encode_buffer(buf);
// encoded == "SGVsbG8="
```

---

### `encoding_base64_decode_buffer`

Decodes a Base64-encoded string into a `Buffer`.

**Syntax:**

```hoo
encoding_base64_decode_buffer(data: string) :Buffer
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The Base64-encoded string to decode. |

**Returns:** `Buffer` — A buffer containing the decoded bytes.

**Errors:** Throws `RuntimeException` if `data` is nil, malformed Base64, or decoding fails.

**Complete Example:**

```hoo
import hoo.buffer;
import hoo.encoding;

let decoded = encoding_base64_decode_buffer("SGVsbG8=");
```

## Hex Functions

### `encoding_hex_encode`

Hex-encodes a string. Each byte is represented as two hexadecimal characters.

**Syntax:**

```hoo
encoding_hex_encode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The string to encode. |

**Returns:** `string` — A hex-encoded string (lowercase).

**Errors:** Throws `RuntimeException` if `data` is nil or encoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let encoded = encoding_hex_encode("Hello");
// encoded == "48656c6c6f"
```

---

### `encoding_hex_decode`

Decodes a hex-encoded string back to the original string.

**Syntax:**

```hoo
encoding_hex_decode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The hex-encoded string to decode. |

**Returns:** `string` — The decoded string.

**Errors:** Throws `RuntimeException` if `data` is nil, contains non-hex characters, has an odd length, or decoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let decoded = encoding_hex_decode("48656c6c6f");
// decoded == "Hello"
```

## URL Encoding Functions

### `encoding_url_encode`

Percent-encodes a string for safe use in URLs. Reserved characters and non-ASCII bytes are encoded as `%XX` sequences.

**Syntax:**

```hoo
encoding_url_encode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The string to percent-encode. |

**Returns:** `string` — A percent-encoded URL string.

**Errors:** Throws `RuntimeException` if `data` is nil or encoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let encoded = encoding_url_encode("a b=c");
// encoded == "a%20b%3Dc"
```

---

### `encoding_url_decode`

Decodes a percent-encoded URL string back to the original string.

**Syntax:**

```hoo
encoding_url_decode(data: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The percent-encoded string to decode. |

**Returns:** `string` — The decoded string.

**Errors:** Throws `RuntimeException` if `data` is nil, contains malformed percent-encoding, or decoding fails.

**Complete Example:**

```hoo
import hoo.encoding;

let decoded = encoding_url_decode("a%20b%3Dc");
// decoded == "a b=c"
```

## Buffer-Aware Overloads

The base64 and hex functions also accept and return `Buffer` objects directly (requires `import hoo.buffer`):

| Function | Signature | Description |
|----------|-----------|-------------|
| encoding_base64_encode_buffer | `(buf: Buffer):string` | Base64-encodes buffer contents |
| encoding_base64_decode_buffer | `(encoded: string):Buffer` | Decodes Base64 string to buffer |
| encoding_hex_encode_buffer | `(buf: Buffer):string` | Hex-encodes buffer contents |
| encoding_hex_decode_buffer | `(hex: string):Buffer` | Decodes hex string to buffer |

```hoo
import hoo.buffer;
import hoo.encoding;

let buf = buffer_fromBytes("Hello", 5);
let b64 = encoding_base64_encode_buffer(buf);    // "SGVsbG8="
let hex = encoding_hex_encode_buffer(buf);       // "48656c6c6f"

let decoded = encoding_base64_decode_buffer(b64); // Buffer
let fromHex = encoding_hex_decode_buffer(hex);   // Buffer
```

## Usage Example

```hoo
import hoo.buffer;
import hoo.encoding;

func :int64 main() {
    // Base64 round-trip
    var b64 = encoding_base64_encode("Hello, World!");
    var original = encoding_base64_decode(b64);
    println(original == "Hello, World!"); // true

    // Hex round-trip
    var hex = encoding_hex_encode("Hello");
    var fromHex = encoding_hex_decode(hex);
    println(fromHex == "Hello"); // true

    // URL encoding
    var url = encoding_url_encode("a b=c/d?e=f");
    println(url);
    var decoded = encoding_url_decode(url);
    println(decoded == "a b=c/d?e=f"); // true

    // Buffer encoding
    var buf = buffer_fromBytes("Hello", 5);
    var b64buf = encoding_base64_encode_buffer(buf);
    println(b64buf == "SGVsbG8="); // true

    return 0;
}
```
