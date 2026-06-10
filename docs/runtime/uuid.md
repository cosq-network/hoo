# UUID (`hoo.uuid`)

The `hoo.uuid` module provides UUID v4 generation, nil UUID, parse/format, byte access, equality/ordering comparison, and ARC-managed opaque handles.

## 1. Creation

- `Uuid.v4()` — Generate a random UUID v4 (RFC 4122). Returns retained opaque `HooUUID` handle.
- `Uuid.nil()` — Create nil UUID (`00000000-0000-0000-0000-000000000000`).
- `Uuid.parse(str)` — Parse canonical string `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`. Returns NULL on parse failure.
- `Uuid.fromBytes(bytes)` — Create from 16 raw bytes.

## 2. Conversion

- `u.format()` — Convert to canonical string (free with `Uuid.freeString`).
- `u.toBytes(out_16)` — Write 16 raw bytes into buffer. Returns 1 on success.

## 3. Properties & Comparison

- `u.isNil()` — Returns 1 if nil.
- `a.equals(b)` — Returns 1 if equal.
- `a.compare(b)` — Lexicographic comparison. Returns -1, 0, or 1.

## 4. Reference Counting

- `u.retain()` — Increment reference count.
- `u.release()` — Decrement; frees when zero.

## Memory Management

Strings allocated by `u.format()` must be freed with `Uuid.freeString(str)`.
