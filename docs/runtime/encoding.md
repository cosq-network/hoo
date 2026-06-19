# Encoding (`hoo.encoding`)

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees. All functions in this module are snake_case free-functions.

## 1. Base64

- `encoding_base64_encode(data, len)` — Encode bytes to Base64 string.
- `encoding_base64_decode(encoded)` — Decode Base64 string to a string.
- `encoding_base64_encode_buffer(buf)` — Encode a `Buffer` to a Base64 string.
- `encoding_base64_decode_buffer(encoded)` — Decode Base64 string to a `Buffer`.

## 2. Hex

- `encoding_hex_encode(data, len)` — Encode bytes to lowercase hex string.
- `encoding_hex_decode(hex)` — Decode hex string to a string.
- `encoding_hex_encode_buffer(buf)` — Encode a `Buffer` to a lowercase hex string.
- `encoding_hex_decode_buffer(hex)` — Decode hex string to a `Buffer`.

## 3. URL Encoding

- `encoding_url_encode(str)` — Percent-encode a string.
- `encoding_url_decode(encoded)` — Decode percent-encoded string.

## Memory Management

Memory management in Hoo is automated. The returned string and buffer structures are managed by the runtime and garbage collected.
