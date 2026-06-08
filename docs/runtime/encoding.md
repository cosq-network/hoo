# Encoding (`hoo.encoding`)

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees.

## 1. Base64

- `Encoding.base64_encode(data, len)` — Encode bytes to Base64 string (free with `Encoding.free_string`).
- `Encoding.base64_decode(encoded)` — Decode Base64 string to bytes (free with `Encoding.free_bytes`). Returns decoded length or -1 on error.

## 2. Hex

- `Encoding.hex_encode(data, len)` — Encode bytes to lowercase hex string.
- `Encoding.hex_decode(hex)` — Decode hex string to bytes. Returns decoded length or -1 on error.

## 3. URL Encoding

- `Encoding.url_encode(str)` — Percent-encode a string (free with `Encoding.free_string`).
- `Encoding.url_decode(encoded)` — Decode percent-encoded string.

## Memory Management

- `Encoding.free_string(str)` — Free allocated string.
- `Encoding.free_bytes(data)` — Free allocated byte buffer.
