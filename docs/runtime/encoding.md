# Encoding (`hoo.encoding`)

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees.

## 1. Base64

- `Encoding.base64Encode(data, len)` — Encode bytes to Base64 string (free with `Encoding.freeString`).
- `Encoding.base64Decode(encoded)` — Decode Base64 string to bytes (free with `Encoding.freeBytes`). Returns decoded length or -1 on error.

## 2. Hex

- `Encoding.hexEncode(data, len)` — Encode bytes to lowercase hex string.
- `Encoding.hexDecode(hex)` — Decode hex string to bytes. Returns decoded length or -1 on error.

## 3. URL Encoding

- `Encoding.urlEncode(str)` — Percent-encode a string (free with `Encoding.freeString`).
- `Encoding.urlDecode(encoded)` — Decode percent-encoded string.

## Memory Management

- `Encoding.freeString(str)` — Free allocated string.
- `Encoding.freeBytes(data)` — Free allocated byte buffer.
