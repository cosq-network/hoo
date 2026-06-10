# Uuid — Universally Unique Identifiers

The `Uuid` class provides static methods for generating and manipulating UUIDs.

## Methods

`Uuid.v4() :ptr`
Generates a random UUID version 4 and returns a UUID handle.

`Uuid.nil() :ptr`
Creates a nil UUID (`00000000-0000-0000-0000-000000000000`).

`Uuid.fromString(str: string) :ptr`
Parses a UUID from its canonical string representation. Returns NULL on parse failure.

`Uuid.toString(uuid: ptr) :string`
Converts a UUID to its canonical string representation.

`Uuid.isNil(uuid: ptr) :int64`
Returns 1 if the UUID is the nil UUID, 0 otherwise.

`Uuid.equals(a: ptr, b: ptr) :int64`
Returns 1 if two UUIDs are equal, 0 otherwise.

`Uuid.compare(a: ptr, b: ptr) :int64`
Lexicographically compares two UUIDs. Returns -1 if a < b, 0 if a == b, 1 if a > b.

`Uuid.release(uuid: ptr)`
Releases a UUID handle.

## Example

```hoo
let id = Uuid.v4()
let str = Uuid.toString(id)
println(str)  // e.g. "550e8400-e29b-41d4-a716-446655440000"

let nil = Uuid.nil()
let isNil = Uuid.isNil(nil)  // 1

Uuid.release(id)
Uuid.release(nil)
```
