# Buffer API Reference

The `Buffer` class provides a managed, mutable byte array for working with raw binary data. Buffers are ARC-managed and support dynamic resizing.

## Constructor

### `new Buffer(capacity: int64) :buffer`

Creates a new empty Buffer with the given initial capacity.

```hoo
let buf = new Buffer(64)
```

### `Buffer.fromBytes(data: string, len: int64) :buffer`

Creates a new Buffer initialized with `len` bytes from `data`.

```hoo
let buf = Buffer.fromBytes("Hello", 5)
```

## Instance Methods

### `buf.length() :int64`

Returns the number of bytes currently stored in the buffer.

### `buf.capacity() :int64`

Returns the current allocated capacity of the buffer.

### `buf.copy() :buffer`

Creates an independent copy of the buffer with the same contents.

### `buf.byteAt(index: int64) :int64`

Returns the byte at the given zero-based index, or -1 if the index is out of bounds.

### `buf.setByte(index: int64, val: int64) :int64`

Sets the byte at the given index to `val`. Returns 1 on success, 0 if the index is out of bounds.

### `buf.append(data: string, len: int64) :buffer`

Appends `len` bytes from `data` to the end of the buffer. Returns the buffer (may be a reallocated handle).

### `buf.appendBuffer(other: buffer) :buffer`

Appends all bytes from `other` to the end of this buffer. Returns the buffer.

### `buf.clear() :int64`

Removes all bytes from the buffer (sets length to 0). Capacity is preserved. Returns 1.

### `buf.slice(start: int64, end: int64) :buffer`

Returns a new Buffer containing bytes from index `start` (inclusive) to `end` (exclusive). Returns null if the range is invalid.

## Buffer-Aware Overloads in Other Modules

Several modules accept Buffer handles instead of raw (data, len) pairs:

| Module | Function | Signature |
|--------|----------|-----------|
| Fs | `writeBytes` | `Fs.writeBytes(path: string, buf: buffer) :int64` |
| Fs | `readBytes` | `Fs.readBytes(path: string) :buffer` |
| Encoding | `base64Encode` | `Encoding.base64Encode(buf: buffer) :string` |
| Encoding | `base64Decode` | `Encoding.base64Decode(encoded: string) :buffer` |
| Encoding | `hexEncode` | `Encoding.hexEncode(buf: buffer) :string` |
| Encoding | `hexDecode` | `Encoding.hexDecode(hex: string) :buffer` |
| Uuid | `fromBytes` | `Uuid.fromBytes(buf: buffer) :uuid` |
| Uuid | `toBytes` | `Uuid.toBytes(uuid: uuid) :buffer` |
| Hashing | `sha256` | `Hashing.sha256(buf: buffer) :string` |
| Hashing | `sha1` | `Hashing.sha1(buf: buffer) :string` |
| Hashing | `md5` | `Hashing.md5(buf: buffer) :string` |
| Hashing | `crc32` | `Hashing.crc32(buf: buffer) :int64` |
| Hashing | `hmacSha256` | `Hashing.hmacSha256(key: buffer, data: buffer) :string` |
| Compression | `gzipCompress` | `Compression.gzipCompress(buf: buffer) :buffer` |
| Compression | `gzipDecompress` | `Compression.gzipDecompress(buf: buffer) :buffer` |
| Compression | `deflateCompress` | `Compression.deflateCompress(buf: buffer) :buffer` |
| Compression | `deflateDecompress` | `Compression.deflateDecompress(buf: buffer) :buffer` |

## Memory Layout

```
[ARC header: 16 bytes (refcount + type_id=113)] [BufferImpl: length(8) + capacity(8) + data...]
                                                         ^-- handle points here (right after ARC header)
```

The handle returned by `new Buffer(...)` and `Buffer.fromBytes()` points to the `BufferImpl` struct, which is located immediately after the 16-byte ARC header. This is the same layout used by `HooString`, ensuring `hoo_get_refcount()`, `hoo_get_type_id()`, `hoo_release()`, and `hoo_retain()` work transparently without offset correction.
