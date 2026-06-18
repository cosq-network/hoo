# Encoding

Singleton class providing base64, hex, and URL encoding/decoding.

## Static Methods

### `Encoding.base64Encode(data: string, len: int64) :string`

Base64-encodes `len` bytes from `data`.

```hoo
let encoded = Encoding.base64Encode("Hello, World!", 13)
// encoded == "SGVsbG8sIFdvcmxkIQ=="
```

### `Encoding.base64Decode(encoded: string) :string`

Decodes a base64-encoded string.

```hoo
let decoded = Encoding.base64Decode("SGVsbG8sIFdvcmxkIQ==")
// decoded == "Hello, World!"
```

### `Encoding.hexEncode(data: string, len: int64) :string`

Hex-encodes `len` bytes from `data`.

```hoo
let hex = Encoding.hexEncode("Hello", 5)
// hex == "48656C6C6F"
```

### `Encoding.hexDecode(hex: string) :string`

Decodes a hex-encoded string.

```hoo
let bytes = Encoding.hexDecode("48656C6C6F")
// bytes == "Hello"
```

### `Encoding.urlEncode(str: string) :string`

Percent-encodes a string for safe use in URLs.

```hoo
let encoded = Encoding.urlEncode("a b=c")
// encoded == "a%20b%3Dc"
```

### `Encoding.urlDecode(encoded: string) :string`

Decodes a percent-encoded URL string.

```hoo
let decoded = Encoding.urlDecode("a%20b%3Dc")
// decoded == "a b=c"
```

### Buffer-Aware Overloads

Base64 and hex functions accept a `Buffer` handle directly (no length parameter needed):

```hoo
let buf = Buffer.fromBytes("Hello", 5)
let b64 = Encoding.base64Encode(buf)   // "SGVsbG8="
let hex = Encoding.hexEncode(buf)      // "48656C6C6F"
```

The decode overloads return a `Buffer` instead of a string:

```hoo
let decoded = Encoding.base64Decode("SGVsbG8=")   // Buffer
let raw = Encoding.hexDecode("48656C6C6F")        // Buffer
```
