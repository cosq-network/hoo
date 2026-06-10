# JSON (`hoo.json`)

The `hoo.json` module provides parsing, stringifying, querying, and construction of JSON values (objects, arrays, strings, numbers, booleans, null) using ARC-managed opaque handles.

## 1. Parsing and Stringifying

- `Json.parse(str)` — Parse a JSON string into a value handle. Returns 0 on parse error.
- `val.stringify()` — Serialize a JSON value to its string representation.

## 2. Type Inspection

- `val.type()` — Returns the type code: 0 (null), 1 (bool), 2 (int), 3 (double), 4 (string), 5 (object), 6 (array).

## 3. Primitive Value Construction

- `Json.newNull()` — Create a null value.
- `Json.newBool(val)` — Create a boolean value (0 or 1).
- `Json.newInt(val)` — Create an integer value.
- `Json.newString(str)` — Create a string value.

## 4. Object/Array Construction

- `Json.newObject()` — Create an empty object.
- `Json.newArray()` — Create an empty array.
- `obj.set(key, val)` — Set a field on an object.
- `arr.push(val)` — Append a value to an array.
- `arr.length()` — Return the number of elements in an array.

## 5. Value Access

- `obj.get(key)` — Get a field from an object; returns 0 if missing.
- `obj.getString(key)` — Get a field as a string; returns 0 if missing.
- `obj.getInt(key)` — Get a field as an integer.
- `arr.get(index)` — Get element at index from an array.

## Usage from Hoo Source

All `Json.*` and value methods are available:

```hoo
func :int64 demo() {
    // Parse
    var obj = Json.parse("{\"name\":\"Alice\",\"age\":30,\"scores\":[95,87,92]}");
    var name = obj.getString("name");                 // "Alice"
    var age = obj.getInt("age");                      // 30

    // Build
    var user = Json.newObject();
    var n = Json.newString("Bob");
    user.set("name", n);
    user.set("age", Json.newInt(25));
    var tags = Json.newArray();
    tags.push(Json.newString("admin"));
    user.set("tags", tags);

    // Stringify
    var out = user.stringify();
    var len = string_length(out);

    // Cleanup
    obj.release();
    user.release();
    return len;
}
```

## 6. Memory Management

- `val.retain()` / `val.release()` — Reference counting.
- All parsed or constructed values start with a retain count of 1.
- `obj.get()` returns a borrowed reference; call `val.retain()` to keep it alive beyond the parent.
