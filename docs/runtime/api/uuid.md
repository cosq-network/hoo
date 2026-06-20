# UUID API Reference

## Module Name

Part of the `hoo` module.

## Import Statement

```hoo
import hoo.uuid;
```

## Module Description

The `Uuid` class provides parsing and manipulation of universally unique identifiers (UUIDs). The module supports generating random version 4 UUIDs, parsing UUIDs from their canonical string representation, and comparing UUIDs. UUID objects are reference-counted.

## Class: Uuid

### Declaration

```hoo
class Uuid
```

### Public Fields

None.

### Public Instance Functions

#### Constructor: `Uuid`

Parses a UUID from its canonical string representation (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).

**Syntax:**

```hoo
Uuid(value: string) :Uuid
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `value` | `string` | The UUID string to parse. |

**Returns:**

`Uuid` — A new `Uuid` instance parsed from the string.

**Errors:**

Returns a null handle if the string is not a valid UUID.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = Uuid("550e8400-e29b-41d4-a716-446655440000");
    println(id.to_string());
    id.release();
    return 0;
}
```

---

#### `to_string`

Returns the canonical string representation of the UUID.

**Syntax:**

```hoo
to_string() :string
```

**Parameters:**

None.

**Returns:**

`string` — The UUID in `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` format.

**Errors:**

Returns an empty string for a null UUID handle.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    var s = id.to_string();
    println(s);
    id.release();
    return 0;
}
```

---

#### `equals`

Checks whether two UUIDs are equal.

**Syntax:**

```hoo
equals(other: Uuid) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `other` | `Uuid` | The UUID to compare against. |

**Returns:**

`int64` — `1` if the UUIDs are equal, `0` otherwise.

**Errors:**

No errors. Returns `0` if either handle is null.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var a = uuid_nil();
    var b = uuid_nil();
    println(a.equals(b)); // 1
    a.release();
    b.release();
    return 0;
}
```

---

#### `is_nil`

Checks whether this UUID is the nil UUID (`00000000-0000-0000-0000-000000000000`).

**Syntax:**

```hoo
is_nil() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — `1` if this is the nil UUID, `0` otherwise.

**Errors:**

Returns `0` for a null UUID handle.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_nil();
    println(id.is_nil()); // 1
    id.release();
    return 0;
}
```

---

#### `retain`

Increments the UUID's reference count by one.

**Syntax:**

```hoo
retain() :Uuid
```

**Parameters:**

None.

**Returns:**

`Uuid` — The UUID with incremented reference count.

**Errors:**

No errors. If called on a null UUID handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    id.retain();
    id.release();
    id.release();
    return 0;
}
```

---

#### `release`

Decrements the UUID's reference count by one. When the reference count reaches zero the UUID is deallocated.

**Syntax:**

```hoo
release() :void
```

**Parameters:**

None.

**Returns:**

`void`

**Errors:**

No errors. Calling `release` on an already-freed or null UUID handle is a no-op.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    id.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of the UUID.

**Syntax:**

```hoo
refcount() :int64
```

**Parameters:**

None.

**Returns:**

`int64` — The current reference count.

**Errors:**

Returns `0` for a null UUID handle.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    id.retain();
    var rc = id.refcount(); // 2
    id.release();
    id.release();
    return 0;
}
```

---

#### `free_string`

Frees a string allocated by a UUID method. Use this to release memory returned by `to_string` when the string must be explicitly freed.

**Syntax:**

```hoo
free_string(str: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `str` | `string` | The string to free. |

**Returns:**

`void`

**Errors:**

No errors. Passing a null or empty string is a no-op.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    var s = id.to_string();
    println(s);
    id.free_string(s);
    id.release();
    return 0;
}
```

## Free Functions

---

#### `uuid_v4`

Generates a random version 4 UUID.

**Syntax:**

```hoo
uuid_v4() :Uuid
```

**Parameters:**

None.

**Returns:**

`Uuid` — A new `Uuid` instance containing a random version 4 UUID.

**Errors:**

Returns a null handle if UUID generation fails.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    println(id.to_string());
    id.release();
    return 0;
}
```

---

#### `uuid_nil`

Returns the nil UUID (`00000000-0000-0000-0000-000000000000`).

**Syntax:**

```hoo
uuid_nil() :Uuid
```

**Parameters:**

None.

**Returns:**

`Uuid` — A new `Uuid` instance set to the nil UUID.

**Errors:**

Returns a null handle if allocation fails.

**Complete Example:**

```hoo
import hoo.uuid;

func :int64 main() {
    var nil = uuid_nil();
    println(nil.to_string()); // 00000000-0000-0000-0000-000000000000
    nil.release();
    return 0;
}
```

## Usage Example

```hoo
import hoo.uuid;

func :int64 main() {
    var id = uuid_v4();
    var s = id.to_string();
    println("UUID: " + s);
    id.free_string(s);

    var nil = uuid_nil();
    println("nil: " + nil.is_nil()); // 1

    var parsed = Uuid("550e8400-e29b-41d4-a716-446655440000");
    println(parsed.equals(id)); // 0

    id.release();
    nil.release();
    parsed.release();
    return 0;
}
```
