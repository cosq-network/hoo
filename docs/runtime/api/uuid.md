# Uuid — Universally Unique Identifiers

The `uuid` module provides a `Uuid` class and free functions for generating and manipulating UUIDs without exposing raw pointer handles.

## Uuid Class

### Constructor

`new Uuid(source: string) :Uuid`
Creates a Uuid. If `source` is empty, generates a random UUID version 4. If `source` is `"nil"`, creates a nil UUID. Otherwise, parses the UUID from its canonical string representation.

### Methods

`uuid.toString() :string`
Converts the UUID to its canonical string representation.

`uuid.isNil() :int64`
Returns 1 if the UUID is the nil UUID, 0 otherwise.

`uuid.equals(other: Uuid) :int64`
Returns 1 if two UUIDs are equal, 0 otherwise.

`uuid.compare(other: Uuid) :int64`
Lexicographically compares two UUIDs. Returns -1 if `this` < `other`, 0 if they are equal, 1 if `this` > `other`.

`uuid.toBytes() :buffer`
Extracts the raw 16 bytes of the UUID into a new Buffer.

`uuid.release()`
Releases the UUID handle.

## Free Functions

`uuid_v4() :string`
Returns a new random UUID version 4 as a string.

`uuid_nil() :string`
Returns the nil UUID string (`00000000-0000-0000-0000-000000000000`).

`uuid_is_nil(str: string) :int64`
Returns 1 if the UUID string is nil, 0 otherwise.

`uuid_from_bytes(buf: buffer) :Uuid`
Creates a Uuid instance from a 16-byte Buffer. Returns NULL if the buffer is not exactly 16 bytes.

`uuid_to_bytes(str: string) :buffer`
Converts a UUID string to a 16-byte Buffer.

`uuid_equals(a: string, b: string) :int64`
Returns 1 if the two UUID strings are equal, 0 otherwise.

`uuid_compare(a: string, b: string) :int64`
Compares two UUID strings. Returns -1, 0, or 1.

`uuid_to_string(id: Uuid) :string`
Returns the canonical string representation of the `Uuid` object.

## Example

```hoo
import hoo.uuid;

-- Object-oriented style
let id = new Uuid("")
let str = id.toString()
println(str)  // e.g. "550e8400-e29b-41d4-a716-446655440000"

let nil = new Uuid("nil")
let isNil = nil.isNil()  // 1

id.release()
nil.release()

-- Free functions style
let simpleV4 = uuid_v4()
println("V4 String: " + simpleV4)
```
