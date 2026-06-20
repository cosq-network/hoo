# Encoding API Reference

## Encoding Module

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode functions.

## Import Statement

```hoo
import hoo.encoding;
```

## Module Description

The encoding module offers encode/decode for three formats: Base64, hexadecimal, and URL percent-encoding. Functions accept strings and return strings. Byte-array variants accept and return `array` types for binary data. Buffer-aware overloads accept `Buffer` objects directly. All functions throw `RuntimeException` on nil input or encoding/decoding failure.

## Base64 Functions

### base64_encode

#### Description

Base64-encodes a string.

#### Syntax

```hoo
base64_encode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The string to encode. |

#### Returns

`string` — A Base64-encoded string.

#### Errors

Throws `RuntimeException` if `data` is nil or encoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let encoded = base64_encode("Hello, World!");
// encoded == "SGVsbG8sIFdvcmxkIQ=="
```

### base64_decode

#### Description

Decodes a Base64-encoded string back to the original string.

#### Syntax

```hoo
base64_decode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The Base64-encoded string to decode. |

#### Returns

`string` — The decoded string.

#### Errors

Throws `RuntimeException` if `data` is nil, malformed Base64, or decoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let decoded = base64_decode("SGVsbG8sIFdvcmxkIQ==");
// decoded == "Hello, World!"
```

### base64_encode_bytes

#### Description

Base64-encodes a byte array.

#### Syntax

```hoo
base64_encode_bytes(data: array):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `array` | The byte array to encode. |

#### Returns

`string` — A Base64-encoded string.

#### Errors

Throws `RuntimeException` if `data` is nil or encoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let data = [72, 101, 108, 108, 111]byte;
let encoded = base64_encode_bytes(data);
// encoded == "SGVsbG8="
```

### base64_decode_bytes

#### Description

Decodes a Base64-encoded string into a byte array.

#### Syntax

```hoo
base64_decode_bytes(data: string):array
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The Base64-encoded string to decode. |

#### Returns

`array` — A byte array containing the decoded bytes.

#### Errors

Throws `RuntimeException` if `data` is nil, malformed Base64, or decoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let decoded = base64_decode_bytes("SGVsbG8=");
// decoded == [72, 101, 108, 108, 111]byte
```

## Hex Functions

### hex_encode

#### Description

Hex-encodes a string. Each byte is represented as two hexadecimal characters.

#### Syntax

```hoo
hex_encode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The string to encode. |

#### Returns

`string` — A hex-encoded string (lowercase).

#### Errors

Throws `RuntimeException` if `data` is nil or encoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let encoded = hex_encode("Hello");
// encoded == "48656c6c6f"
```

### hex_decode

#### Description

Decodes a hex-encoded string back to the original string.

#### Syntax

```hoo
hex_decode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The hex-encoded string to decode. |

#### Returns

`string` — The decoded string.

#### Errors

Throws `RuntimeException` if `data` is nil, contains non-hex characters, has an odd length, or decoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let decoded = hex_decode("48656c6c6f");
// decoded == "Hello"
```

## URL Encoding Functions

### uri_encode

#### Description

Percent-encodes a string for safe use in URLs. Reserved characters and non-ASCII bytes are encoded as `%XX` sequences.

#### Syntax

```hoo
uri_encode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The string to percent-encode. |

#### Returns

`string` — A percent-encoded URL string.

#### Errors

Throws `RuntimeException` if `data` is nil or encoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let encoded = uri_encode("a b=c");
// encoded == "a%20b%3Dc"
```

### uri_decode

#### Description

Decodes a percent-encoded URL string back to the original string.

#### Syntax

```hoo
uri_decode(data: string):string
```

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| data | `string` | The percent-encoded string to decode. |

#### Returns

`string` — The decoded string.

#### Errors

Throws `RuntimeException` if `data` is nil, contains malformed percent-encoding, or decoding fails.

#### Complete Example

```hoo
import hoo.encoding;

let decoded = uri_decode("a%20b%3Dc");
// decoded == "a b=c"
```

## Buffer-Aware Overloads

The base64 and hex functions also accept and return `Buffer` objects directly:

| Function | Signature | Description |
|----------|-----------|-------------|
| base64_encode_buffer | `(buf: Buffer):string` | Base64-encodes buffer contents |
| base64_decode_buffer | `(encoded: string):Buffer` | Decodes Base64 string to buffer |
| hex_encode_buffer | `(buf: Buffer):string` | Hex-encodes buffer contents |
| hex_decode_buffer | `(hex: string):Buffer` | Decodes hex string to buffer |

```hoo
import hoo.buffer;
import hoo.encoding;

let buf = buffer_fromBytes("Hello", 5);
let b64 = base64_encode_buffer(buf);    // "SGVsbG8="
let hex = hex_encode_buffer(buf);       // "48656c6c6f"

let decoded = base64_decode_buffer(b64); // Buffer
let fromHex = hex_decode_buffer(hex);   // Buffer
```

## Usage Example

```hoo
import hoo.encoding;

func :int64 main() {
    // Base64 round-trip
    var b64 = base64_encode("Hello, World!");
    var original = base64_decode(b64);
    println(original == "Hello, World!"); // true

    // Hex round-trip
    var hex = hex_encode("Hello");
    var fromHex = hex_decode(hex);
    println(fromHex == "Hello"); // true

    // URL encoding
    var url = uri_encode("a b=c/d?e=f");
    println(url);  // "a%20b%3Dc%2Fd%3Fe%3Df"
    var decoded = uri_decode(url);
    println(decoded == "a b=c/d?e=f"); // true

    // Byte array encoding
    var bytes = [0, 1, 255]byte;
    var b64bytes = base64_encode_bytes(bytes);
    var decodedBytes = base64_decode_bytes(b64bytes);
    println(decodedBytes[2] == 255); // true

    return 0;
}
```
