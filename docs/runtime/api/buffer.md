# Buffer API Reference

## Module Name

Part of the `hoo.buffer` module.

## Import Statement

```hoo
import hoo.buffer;
```

## Module Description

The `Buffer` class provides a managed, mutable byte array for binary data operations. Buffers are reference-counted runtime objects with dynamic capacity growth, supporting arbitrary binary content that is not limited to valid text encodings. The buffer tracks both a current data length and an internal allocated capacity.

## Class: Buffer

### Declaration

```hoo
class Buffer
```

### Public Fields

None.

### Public Instance Functions

#### Constructor: `Buffer`

Creates a new empty buffer with the specified initial capacity. The buffer grows automatically as data is written.

**Syntax:**

```hoo
Buffer(initial_capacity: int64) :Buffer
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `initial_capacity` | `int64` | Initial capacity in bytes. |

**Returns:**

`Buffer` — A new empty buffer with length `0`.

**Errors:**

Returns a null handle if memory allocation fails.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    println("length: " + buf.length());
    return 0;
}
```

---

#### `write`

Appends string data to the end of the buffer. The buffer may reallocate as it grows.

**Syntax:**

```hoo
write(data: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `string` | The string whose contents are appended to the buffer. |

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. If called on a null buffer handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    buf.write(" World");
    println(buf.to_string());
    return 0;
}
```

---

#### `write_byte`

Appends a single byte to the end of the buffer.

**Syntax:**

```hoo
write_byte(byte: int64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `byte` | `int64` | The byte value to append. Only the low 8 bits are used; valid range is `0`–`255`. |

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. Values outside `0`–`255` are truncated to the low 8 bits.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write_byte(72);  // 'H'
    buf.write_byte(105); // 'i'
    println(buf.to_string());
    return 0;
}
```

---

#### `clear`

Resets the buffer position, discarding all stored data. The underlying allocated capacity is preserved for reuse.

**Syntax:**

```hoo
clear() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. If called on a null buffer handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("data");
    buf.clear();
    println(buf.length()); // 0
    return 0;
}
```

---

#### `length`

Returns the number of bytes currently stored in the buffer.

**Syntax:**

```hoo
length() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — The current data length in bytes.

**Errors:**

Returns `0` for a null buffer handle.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("abc");
    return buf.length(); // 3
}
```

---

#### `capacity`

Returns the current allocated capacity of the buffer (may be larger than length).

**Syntax:**

```hoo
capacity() :int64
```

**Parameters:** None.

**Returns:**

`int64` — The allocated capacity in bytes.

**Errors:**

Returns `0` for a null buffer handle.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(128);
    println(buf.capacity()); // >= 128
    buf.release();
    return 0;
}
```

---

#### `byteAt`

Returns the byte at the specified index without modifying the buffer.

**Syntax:**

```hoo
byteAt(index: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | The zero-based index of the byte to read. |

**Returns:**

`int64` — The byte value (0–255) at the given index.

**Errors:**

Returns `0` if the index is out of bounds or the buffer handle is null.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("ABC");
    var b = buf.byteAt(1);
    println(b); // 66 ('B')
    buf.release();
    return 0;
}
```

---

#### `setByte`

Sets the byte at the specified index to the given value.

**Syntax:**

```hoo
setByte(index: int64, value: int64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | The zero-based index of the byte to set. |
| `value` | `int64` | The byte value (0–255). Only the low 8 bits are used. |

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. If the index is out of bounds or the handle is null, the operation is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("ABC");
    buf.setByte(1, 88); // 'X'
    println(buf.to_string()); // "AXC"
    buf.release();
    return 0;
}
```

---

#### `copy`

Creates a new independent copy of the buffer with the same contents and capacity.

**Syntax:**

```hoo
copy() :Buffer
```

**Parameters:** None.

**Returns:**

`Buffer` — A new Buffer containing a copy of the data. Returns `null` if allocation fails.

**Errors:**

Returns `null` if the source buffer handle is null or if memory allocation fails.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var dup = buf.copy();
    println(dup.to_string()); // "Hello"
    buf.release();
    dup.release();
    return 0;
}
```

---

#### `appendBuffer`

Appends the contents of another buffer to the end of this buffer.

**Syntax:**

```hoo
appendBuffer(other: Buffer) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `other` | `Buffer` | The buffer whose contents are appended. |

**Returns:**

`void`

**Errors:**

No errors at the Hoo level. If either handle is null, the operation is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var a = Buffer(64);
    var b = Buffer(64);
    a.write("Hello ");
    b.write("World");
    a.appendBuffer(b);
    println(a.to_string()); // "Hello World"
    a.release();
    b.release();
    return 0;
}
```

---

#### `slice`

Returns a new buffer containing a copy of the bytes in the specified range.

**Syntax:**

```hoo
slice(start: int64, end: int64) :Buffer
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `start` | `int64` | The zero-based start index (inclusive). |
| `end` | `int64` | The zero-based end index (exclusive). |

**Returns:**

`Buffer` — A new Buffer containing the specified byte range. Returns `null` on error.

**Errors:**

Returns `null` if the range is invalid or the source handle is null.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello, World!");
    var sub = buf.slice(7, 12);
    println(sub.to_string()); // "World"
    buf.release();
    sub.release();
    return 0;
}
```

---

#### `to_string`

Extracts the buffer contents as a string. The buffer is not cleared; repeated calls return the same data.

**Syntax:**

```hoo
to_string() :string
```

**Parameters:**

None.

**Returns:**

`string` — A string containing the buffer's byte data.

**Errors:**

Returns an empty string for a null or empty buffer.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello");
    var s: string = buf.to_string();
    println(s);
    return 0;
}
```

---

#### `retain`

Increments the buffer's reference count by one. Use this to extend the lifetime of a buffer when the original handle is released.

**Syntax:**

```hoo
retain() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. If called on a null buffer handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("shared");
    buf.retain();
    // Both `buf` and any retained reference must be released.
    buf.release();
    buf.release();
    return 0;
}
```

---

#### `release`

Decrements the buffer's reference count by one. When the reference count reaches zero the buffer is deallocated.

**Syntax:**

```hoo
release() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. Calling `release` on an already-freed or null buffer handle is a no-op.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("temp");
    buf.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of the buffer.

**Syntax:**

```hoo
refcount() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — The current reference count.

**Errors:**

Returns `0` for a null buffer handle.

**Complete Example:**

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.retain();
    var rc = buf.refcount(); // 2
    buf.release();
    buf.release();
    return 0;
}
```

## Usage Example

```hoo
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("Hello, ");
    buf.write_byte(87);  // 'W'
    buf.write_byte(111); // 'o'
    buf.write_byte(114); // 'r'
    buf.write_byte(108); // 'l'
    buf.write_byte(100); // 'd'
    buf.write_byte(33);  // '!'

    println(buf.to_string());  // Hello, World!
    println("length: " + buf.length());

    buf.clear();
    println("after clear: " + buf.length()); // 0

    return 0;
}
```
