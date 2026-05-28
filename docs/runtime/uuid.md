# UUID (`hoo.uuid`)

The `hoo.uuid` module provides UUID v4 generation, nil UUID, parse/format, byte access, equality/ordering comparison, and ARC-managed opaque handles.

## 1. Creation

- `hoo_uuid_v4()` — Generate a random UUID v4 (RFC 4122). Returns retained opaque `HooUUID` handle.
- `hoo_uuid_nil()` — Create nil UUID (`00000000-0000-0000-0000-000000000000`).
- `hoo_uuid_from_string(str)` — Parse canonical string `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`. Returns NULL on parse failure.
- `hoo_uuid_from_bytes(bytes)` — Create from 16 raw bytes.

## 2. Conversion

- `hoo_uuid_to_string(uuid)` — Convert to canonical string (free with `hoo_uuid_free_string`).
- `hoo_uuid_to_bytes(uuid, out_16)` — Write 16 raw bytes into buffer. Returns 1 on success.

## 3. Properties & Comparison

- `hoo_uuid_is_nil(uuid)` — Returns 1 if nil.
- `hoo_uuid_equals(a, b)` — Returns 1 if equal.
- `hoo_uuid_compare(a, b)` — Lexicographic comparison. Returns -1, 0, or 1.

## 4. Reference Counting

- `hoo_uuid_retain(uuid)` — Increment reference count.
- `hoo_uuid_release(uuid)` — Decrement; frees when zero.

## Memory Management

Strings allocated by `hoo_uuid_to_string` must be freed with `hoo_uuid_free_string(str)`.
