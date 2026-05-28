# Encoding (`hoo.encoding`)

The `hoo.encoding` module provides Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees.

## 1. Base64

- `hoo_encoding_base64_encode(data, len)` — Encode bytes to Base64 string (free with `hoo_encoding_free_string`).
- `hoo_encoding_base64_decode(encoded, &out_data)` — Decode Base64 string to bytes (free with `hoo_encoding_free_bytes`). Returns decoded length or -1 on error.

## 2. Hex

- `hoo_encoding_hex_encode(data, len)` — Encode bytes to lowercase hex string.
- `hoo_encoding_hex_decode(hex, &out_data)` — Decode hex string to bytes. Returns decoded length or -1 on error.

## 3. URL Encoding

- `hoo_encoding_url_encode(str)` — Percent-encode a string (free with `hoo_encoding_free_string`).
- `hoo_encoding_url_decode(encoded)` — Decode percent-encoded string.

## Memory Management

- `hoo_encoding_free_string(str)` — Free allocated string.
- `hoo_encoding_free_bytes(data)` — Free allocated byte buffer.
