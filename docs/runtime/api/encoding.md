# Encoding

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees.

## Functions

### `encoding_base64_encode(data: string, len: int64) :string`

Base64-encodes `len` bytes from `data`.

```hoo
import hoo.encoding;

let encoded = encoding_base64_encode("Hello, World!", 13)
// encoded == "SGVsbG8sIFdvcmxkIQ=="
```

### `encoding_base64_decode(encoded: string) :string`

Decodes a base64-encoded string.

```hoo
import hoo.encoding;

let decoded = encoding_base64_decode("SGVsbG8sIFdvcmxkIQ==")
// decoded == "Hello, World!"
```

### `encoding_hex_encode(data: string, len: int64) :string`

Hex-encodes `len` bytes from `data`.

```hoo
import hoo.encoding;

let hex = encoding_hex_encode("Hello", 5)
// hex == "48656c6c6f"
```

### `encoding_hex_decode(hex: string) :string`

Decodes a hex-encoded string.

```hoo
import hoo.encoding;

let bytes = encoding_hex_decode("48656c6c6f")
// bytes == "Hello"
```

### `encoding_url_encode(str: string) :string`

Percent-encodes a string for safe use in URLs.

```hoo
import hoo.encoding;

let encoded = encoding_url_encode("a b=c")
// encoded == "a%20b%3Dc"
```

### `encoding_url_decode(encoded: string) :string`

Decodes a percent-encoded URL string.

```hoo
import hoo.encoding;

let decoded = encoding_url_decode("a%20b%3Dc")
// decoded == "a b=c"
```

### Buffer-Aware Overloads

The base64 and hex functions have buffer overloads (accepting `Buffer` directly and returning `Buffer` or `string` appropriately):

#### `encoding_base64_encode_buffer(buf: Buffer) :string`

Base64-encodes the bytes in the buffer.

```hoo
import hoo.encoding;
import hoo.buffer;

let buf = buffer_fromBytes("Hello", 5)
let b64 = encoding_base64_encode_buffer(buf)   // "SGVsbG8="
```

#### `encoding_base64_decode_buffer(encoded: string) :Buffer`

Decodes a base64-encoded string and returns a `Buffer`.

```hoo
import hoo.encoding;

let buf = encoding_base64_decode_buffer("SGVsbG8=")   // Buffer
```

#### `encoding_hex_encode_buffer(buf: Buffer) :string`

Hex-encodes the bytes in the buffer.

```hoo
import hoo.encoding;
import hoo.buffer;

let buf = buffer_fromBytes("Hello", 5)
let hex = encoding_hex_encode_buffer(buf)      // "48656c6c6f"
```

#### `encoding_hex_decode_buffer(hex: string) :Buffer`

Decodes a hex-encoded string and returns a `Buffer`.

```hoo
import hoo.encoding;

let buf = encoding_hex_decode_buffer("48656c6c6f")   // Buffer
```
