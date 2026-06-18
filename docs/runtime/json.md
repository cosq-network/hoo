# JSON (`hoo.json`)

The `hoo.json` module provides parsing, stringifying, querying, and construction of JSON values (objects, arrays, strings, numbers, booleans, null) using ARC-managed opaque handles.

## 1. Parsing and Stringifying

- `Json.parse(str)` — Parse a JSON string into a value handle. Returns 0 on parse error.
- `val.stringify()` — Serialize a JSON value to its string representation.

## 2. Type Inspection

- `val.type()` — Returns the type code (see Type Constants below).

## 3. Primitive Value Construction

- `Json.newNull()` — Create a null value.
- `Json.newBool(val)` — Create a boolean value (0 or 1).
- `Json.newInt(val)` — Create an integer value.
- `Json.newFloat(val)` — Create a floating-point value.
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
- `obj.getInt(key)` — Get a field as an integer (also truncates float values).
- `arr.get(index)` — Get element at index from an array.

## 6. HooMap Interop

- `Json.parseToMap(str)` — Parse a flat JSON object into a `map<string, string>`. Returns nil on parse error or if the JSON contains nested objects/arrays. Numbers are stored as their string representation. Bools become `"true"/"false"`. Null becomes `"null"`.
- `Json.serializeMap(map)` — Serialize a `map<string, string>` to a JSON object string. Returns nil if `map` is not a string-valued map. Auto-detects types: `"null"` → null, `"true"/"false"` → bool, parseable numbers → JSON number, otherwise → JSON string (escaped).

## 7. String Transformation

- `Json.minify(str)` — Remove all insignificant whitespace from a JSON string. Preserves whitespace inside string literals. Returns nil on parse error.
- `Json.beautify(str)` — Pretty-print a JSON string with 2-space indentation and newlines. Returns nil on parse error.

## 8. Memory Management

- `val.retain()` / `val.release()` — Reference counting.
- All parsed or constructed values start with a retain count of 1.
- `obj.get()` returns a borrowed reference; call `val.retain()` to keep it alive beyond the parent.
- `parseToMap` returns a HooMap (refcount 1); `serializeMap`, `minify`, `beautify` return a HooString (refcount 1). Caller must release when done.

## Type Constants

| Constant          | Value | Description          |
|-------------------|-------|----------------------|
| `HOO_JSON_NULL`   | 0     | Null value           |
| `HOO_JSON_BOOL`   | 1     | Boolean (true/false) |
| `HOO_JSON_INT`    | 2     | Integer number       |
| `HOO_JSON_STRING` | 3     | String value         |
| `HOO_JSON_ARRAY`  | 4     | Array value          |
| `HOO_JSON_OBJECT` | 5     | Object value         |
| `HOO_JSON_FLOAT`  | 6     | Floating-point number|

The parser produces `HOO_JSON_FLOAT` for numbers containing a decimal point or exponent (e.g. `3.14`, `1e5`), and `HOO_JSON_INT` for plain integers (e.g. `42`).

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

    // Parse to map
    var map = Json.parseToMap("{\"name\":\"Alice\",\"age\":30,\"active\":true}");
    var mapName = map.get("name");                    // "Alice"
    var mapAge = map.get("age");                      // "30"

    // Serialize map to JSON
    var mapJson = Json.serializeMap(map);
    println(mapJson);                                 // {"active":true,"age":30,"name":"Alice"}

    // Minify / Beautify
    var compact = Json.minify("{\n  \"key\": \"value\"\n}");
    var pretty = Json.beautify("{\"key\":\"value\"}");

    // Cleanup
    obj.release();
    user.release();
    map.release();
    return 0;
}
```
