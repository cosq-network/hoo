# Buffer API Developer Reference

The `Buffer` runtime API provides a managed, mutable byte array for raw binary
data. Buffers are ARC-managed runtime objects with type ID `113` and dynamic
capacity growth.

Hoo supports only one constructor per class and does not support static class
methods. The Buffer API therefore exposes `new Buffer()` as the only constructor
and uses the free function `buffer_fromBytes(...)` for byte-source creation.

Use `Buffer` when a runtime API needs binary-safe bytes instead of a text
`string`. Several modules accept or return `Buffer` values, including `Fs`,
`Encoding`, `Uuid`, `Hashing`, and `Compression`.

## `new Buffer`

### Description

Creates an empty buffer. The runtime reserves an internal default capacity and
grows the buffer as bytes are appended.

### Syntax

```hoo
new Buffer() :Buffer
```

### Parameters

None.

### Return Type

`Buffer`
A new empty buffer with length `0`.

### Errors

Returns a null handle only if allocation fails.

### Complete Example

```hoo
func :int64 main() {
    var buf = new Buffer();
    println("length: " + buf.length());
    println("capacity: " + buf.capacity());
    return buf.length();
}
```

## `buffer_fromBytes`

### Description

Creates a buffer initialized with bytes from a string-like byte source.
`buffer_fromBytes` is a free function, not a static `Buffer` method.

### Syntax

```hoo
buffer_fromBytes(data: string, len: int64) :Buffer
```

### Parameters

`data`
Source bytes.

`len`
Number of bytes to copy from `data`.

### Return Type

`Buffer`
A new buffer containing the copied bytes. Length is `len`.

### Errors

If `data` is nil or `len` is `0`, the native runtime creates an empty buffer.
Negative lengths or oversized lengths return null.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("Hello", 5);
    println(buf.length()); // 5
    return buf.byteAt(0); // 72
}
```

## `length`

### Description

Returns the number of bytes currently stored in the buffer.

### Syntax

```hoo
buf.length() :int64
```

### Parameters

None.

### Return Type

`int64`
The current byte length.

### Errors

Returns `0` for a null buffer handle.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("abc", 3);
    return buf.length();
}
```

## `capacity`

### Description

Returns the number of bytes currently allocated for the buffer payload.

### Syntax

```hoo
buf.capacity() :int64
```

### Parameters

None.

### Return Type

`int64`
The current buffer capacity in bytes.

### Errors

Returns `0` for a null buffer handle.

### Complete Example

```hoo
func :int64 main() {
    var buf = new Buffer();
    return buf.capacity();
}
```

## `copy`

### Description

Creates an independent copy of the buffer with the same byte contents.

### Syntax

```hoo
buf.copy() :Buffer
```

### Parameters

None.

### Return Type

`Buffer`
A new buffer containing the same bytes.

### Errors

Returns null if the source buffer is null or allocation fails.

### Complete Example

```hoo
func :int64 main() {
    var original = buffer_fromBytes("abc", 3);
    var copy = original.copy();

    copy.setByte(0, 120); // 'x'
    return original.byteAt(0); // still 97 ('a')
}
```

## `byteAt`

### Description

Reads one byte by zero-based index.

### Syntax

```hoo
buf.byteAt(index: int64) :int64
```

### Parameters

`index`
Zero-based byte index.

### Return Type

`int64`
The byte value in the range `0..255`, or `-1` when `index` is out of bounds.

### Errors

Does not throw for out-of-bounds indexes.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("ABC", 3);
    return buf.byteAt(1); // 66
}
```

## `setByte`

### Description

Writes one byte at an existing index. The buffer length is not extended.

### Syntax

```hoo
buf.setByte(index: int64, value: int64) :int64
```

### Parameters

`index`
Zero-based byte index.

`value`
Byte value to write. Only the low 8 bits are stored.

### Return Type

`int64`
Returns the previous byte value on success. Returns `-1` when the index is out
of bounds or the buffer is null.

### Errors

Does not throw for out-of-bounds indexes.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("abc", 3);
    buf.setByte(0, 65); // 'A'
    return buf.byteAt(0);
}
```

## `append`

### Description

Appends bytes from a string-like byte source to the end of the buffer. The
buffer may reallocate as it grows.

### Syntax

```hoo
buf.append(data: string, len: int64) :Buffer
```

### Parameters

`data`
Source bytes to append.

`len`
Number of bytes to append.

### Return Type

`Buffer`
The buffer handle after append. Use the returned value because the native buffer
may be reallocated.

### Errors

Returns the original buffer when `data` is nil, `len` is `0`, `len` is negative,
the requested length is too large, or allocation fails. Returns null only when
`buf` is null.

### Complete Example

```hoo
func :int64 main() {
    var buf = new Buffer();
    buf = buf.append("ab", 2);
    buf = buf.append("cd", 2);

    println(buf.length()); // 4
    return buf.byteAt(3); // 100
}
```

## `appendBuffer`

### Description

Appends all bytes from another buffer.

### Syntax

```hoo
buf.appendBuffer(other: Buffer) :Buffer
```

### Parameters

`other`
Buffer whose bytes should be appended.

### Return Type

`Buffer`
The buffer handle after append. Use the returned value because the native buffer
may be reallocated.

### Errors

Returns the original buffer when `other` is null or allocation fails. Returns
null only when `buf` is null.

### Complete Example

```hoo
func :int64 main() {
    var left = buffer_fromBytes("Hello", 5);
    var right = buffer_fromBytes("!", 1);

    left = left.appendBuffer(right);
    return left.length(); // 6
}
```

## `clear`

### Description

Removes all bytes from the buffer while preserving allocated capacity.

### Syntax

```hoo
buf.clear() :int64
```

### Parameters

None.

### Return Type

`int64`
Returns `0` on success and `-1` for a null buffer.

### Errors

Does not throw.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("data", 4);
    buf.clear();
    return buf.length();
}
```

## `slice`

### Description

Creates a new buffer containing bytes from `start` inclusive to `end`
exclusive.

### Syntax

```hoo
buf.slice(start: int64, end: int64) :Buffer
```

### Parameters

`start`
Inclusive start index.

`end`
Exclusive end index.

### Return Type

`Buffer`
A new buffer containing the selected byte range.

### Errors

Returns null when the source buffer is null, `start` is negative, `end` is
smaller than `start`, `end` is greater than the source length, or allocation
fails.

### Complete Example

```hoo
func :int64 main() {
    var buf = buffer_fromBytes("abcdef", 6);
    var mid = buf.slice(2, 5);

    println(mid.length()); // 3
    return mid.byteAt(0); // 99 ('c')
}
```

## Buffer-Aware Runtime APIs

Several modules accept or return `Buffer` handles:

| Module | Function | Signature |
|--------|----------|-----------|
| Fs | `writeBytes` | `Fs.writeBytes(path: string, buf: Buffer) :int64` |
| Fs | `readBytes` | `Fs.readBytes(path: string) :Buffer` |
| Encoding | `base64Encode` | `Encoding.base64Encode(buf: Buffer) :string` |
| Encoding | `base64Decode` | `Encoding.base64Decode(encoded: string) :Buffer` |
| Encoding | `hexEncode` | `Encoding.hexEncode(buf: Buffer) :string` |
| Encoding | `hexDecode` | `Encoding.hexDecode(hex: string) :Buffer` |
| Uuid | `fromBytes` | `Uuid.fromBytes(buf: Buffer) :uuid` |
| Uuid | `toBytes` | `Uuid.toBytes(uuid: uuid) :Buffer` |
| Hashing | `sha256` | `Hashing.sha256(buf: Buffer) :string` |
| Hashing | `sha1` | `Hashing.sha1(buf: Buffer) :string` |
| Hashing | `md5` | `Hashing.md5(buf: Buffer) :string` |
| Hashing | `crc32` | `Hashing.crc32(buf: Buffer) :int64` |
| Hashing | `hmacSha256` | `Hashing.hmacSha256(key: Buffer, data: Buffer) :string` |
| Compression | `gzipCompress` | `Compression.gzipCompress(buf: Buffer) :Buffer` |
| Compression | `gzipDecompress` | `Compression.gzipDecompress(buf: Buffer) :Buffer` |
| Compression | `deflateCompress` | `Compression.deflateCompress(buf: Buffer) :Buffer` |
| Compression | `deflateDecompress` | `Compression.deflateDecompress(buf: Buffer) :Buffer` |

## Native C ABI

The native runtime exposes C functions for host/runtime integration:

```c
HooBuffer hoo_buffer_new(int64_t initial_capacity);
HooBuffer hoo_buffer_from_bytes(const uint8_t* data, int64_t length);
HooBuffer hoo_buffer_copy(HooBuffer buf);
int64_t hoo_buffer_length(HooBuffer buf);
int64_t hoo_buffer_capacity(HooBuffer buf);
const uint8_t* hoo_buffer_data(HooBuffer buf);
int64_t hoo_buffer_byte_at(HooBuffer buf, int64_t index);
int64_t hoo_buffer_set_byte(HooBuffer buf, int64_t index, int64_t byte_val);
HooBuffer hoo_buffer_append(HooBuffer buf, const uint8_t* data, int64_t length);
HooBuffer hoo_buffer_append_buffer(HooBuffer buf, HooBuffer other);
int64_t hoo_buffer_clear(HooBuffer buf);
HooBuffer hoo_buffer_slice(HooBuffer buf, int64_t start, int64_t end);
```

The native `hoo_buffer_new(initial_capacity)` parameter is for runtime and host
code. It is not exposed as a second Hoo constructor.

## Memory Layout

```text
[ARC header: 16 bytes (refcount + type_id=113)] [BufferImpl: length(8) + capacity(8)] [data...]
                                                         ^-- handle points here
```

The handle returned by `new Buffer()` and `buffer_fromBytes(...)` points to the
`BufferImpl` metadata immediately after the 16-byte ARC header. The byte payload
is stored directly after that metadata. This is the same ARC ownership model
used by other runtime-managed values, so `hoo_get_refcount()`,
`hoo_get_type_id()`, `hoo_release()`, and `hoo_retain()` work without offset
correction.
