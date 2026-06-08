# JSON (`hoo.json`)

The `hoo.json` module provides parsing, stringifying, querying, and construction of JSON values (objects, arrays, strings, numbers, booleans, null) using ARC-managed opaque handles.

## 1. Parsing and Stringifying

- `Json.parse(str)` — Parse a JSON string into a value handle. Returns 0 on parse error.
- `val.stringify()` — Serialize a JSON value to its string representation.

## 2. Type Inspection

- `val.type()` — Returns the type code: 0 (null), 1 (bool), 2 (int), 3 (double), 4 (string), 5 (object), 6 (array).

## 3. Primitive Value Construction

- `Json.new_null()` — Create a null value.
- `Json.new_bool(val)` — Create a boolean value (0 or 1).
- `Json.new_int(val)` — Create an integer value.
- `Json.new_string(str)` — Create a string value.

## 4. Object/Array Construction

- `Json.new_object()` — Create an empty object.
- `Json.new_array()` — Create an empty array.
- `obj.set(key, val)` — Set a field on an object.
- `arr.push(val)` — Append a value to an array.
- `arr.length()` — Return the number of elements in an array.

## 5. Value Access

- `obj.get(key)` — Get a field from an object; returns 0 if missing.
- `obj.get_string(key)` — Get a field as a string; returns 0 if missing.
- `obj.get_int(key)` — Get a field as an integer.
- `arr.get(index)` — Get element at index from an array.

## Usage from Hoo Source

All `Json.*` and value methods are available:

```hoo
func :int64 demo() {
    // Parse
    var obj = Json.parse("{\"name\":\"Alice\",\"age\":30,\"scores\":[95,87,92]}");
    var name = obj.get_string("name");                 // "Alice"
    var age = obj.get_int("age");                      // 30

    // Build
    var user = Json.new_object();
    var n = Json.new_string("Bob");
    user.set("name", n);
    user.set("age", Json.new_int(25));
    var tags = Json.new_array();
    tags.push(Json.new_string("admin"));
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
