# JSON (`hoo.json`)

The `hoo.json` module provides parsing, stringifying, querying, and construction of JSON values (objects, arrays, strings, numbers, booleans, null) using ARC-managed opaque handles.

## 1. Parsing and Stringifying

- `hoo_json_parse(str)` — Parse a JSON string into a value handle. Returns 0 on parse error.
- `hoo_json_stringify(val)` — Serialize a JSON value to its string representation.

## 2. Type Inspection

- `hoo_json_type(val)` — Returns the type code: 0 (null), 1 (bool), 2 (int), 3 (double), 4 (string), 5 (object), 6 (array).

## 3. Primitive Value Construction

- `hoo_json_new_null()` — Create a null value.
- `hoo_json_new_bool(val)` — Create a boolean value (0 or 1).
- `hoo_json_new_int(val)` — Create an integer value.
- `hoo_json_new_string(str)` — Create a string value.

## 4. Object/Array Construction

- `hoo_json_new_object()` — Create an empty object.
- `hoo_json_new_array()` — Create an empty array.
- `hoo_json_set(obj, key, val)` — Set a field on an object.
- `hoo_json_array_push(arr, val)` — Append a value to an array.
- `hoo_json_array_length(arr)` — Return the number of elements in an array.

## 5. Value Access

- `hoo_json_get(obj, key)` — Get a field from an object; returns 0 if missing.
- `hoo_json_get_string(obj, key)` — Get a field as a string; returns 0 if missing.
- `hoo_json_get_int(obj, key)` — Get a field as an integer.
- `hoo_json_array_get(arr, index)` — Get element at index from an array.

## Usage from Hoo Source

All `json_` functions are available with the `json_` prefix:

```hoo
func :int64 demo() {
    // Parse
    var obj = json_parse("{\"name\":\"Alice\",\"age\":30,\"scores\":[95,87,92]}");
    var name = json_get_string(obj, "name");           // "Alice"
    var age = json_get_int(obj, "age");                // 30

    // Build
    var user = json_new_object();
    var n = json_new_string("Bob");
    json_set(user, "name", n);
    json_set(user, "age", json_new_int(25));
    var tags = json_new_array();
    json_array_push(tags, json_new_string("admin"));
    json_set(user, "tags", tags);

    // Stringify
    var out = json_stringify(user);
    var len = string_length(out);

    // Cleanup
    json_release(obj);
    json_release(user);
    return len;
}
```

## 6. Memory Management

- `hoo_json_retain(val)` / `hoo_json_release(val)` — Reference counting.
- All parsed or constructed values start with a retain count of 1.
- `hoo_json_get()` returns a borrowed reference; call `hoo_json_retain()` to keep it alive beyond the parent.
