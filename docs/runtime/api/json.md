# Json — JSON Parsing and Generation

The `Json` class provides static methods for parsing, manipulating, and generating JSON data.

All JSON values are handles of type `json` created through the API.

## Type Constants

These constants are returned by `Json.type()`:

| Constant          | Value |
|-------------------|-------|
| `HOO_JSON_NULL`   | 0     |
| `HOO_JSON_BOOL`   | 1     |
| `HOO_JSON_INT`    | 2     |
| `HOO_JSON_STRING` | 3     |
| `HOO_JSON_ARRAY`  | 4     |
| `HOO_JSON_OBJECT` | 5     |
| `HOO_JSON_FLOAT`  | 6     |

## Methods

`Json.parse(str: string) :json`
Parses a JSON string and returns a JSON value handle.

`Json.stringify(val: json) :string`
Converts a JSON value to its JSON string representation.

`Json.get(obj: json, key: string) :json`
Gets the value associated with a key from a JSON object.

`Json.getInt(obj: json, key: string) :int64`
Gets the integer value associated with a key from a JSON object. Float values are truncated toward zero.

`Json.getString(obj: json, key: string) :string`
Gets the string value associated with a key from a JSON object.

`Json.set(obj: json, key: string, val: json) :int64`
Sets a value on a JSON object. Returns 1 on success, 0 on failure.

`Json.arrayGet(arr: json, index: int64) :json`
Gets the element at the given index from a JSON array.

`Json.arrayPush(arr: json, val: json) :int64`
Appends a value to a JSON array. Returns 1 on success, 0 on failure.

`Json.arrayLength(arr: json) :int64`
Returns the number of elements in a JSON array.

`Json.type(val: json) :int64`
Returns the type constant of the JSON value (see Type Constants). The parser produces `HOO_JSON_INT` (2) for plain integers (e.g. `42`) and `HOO_JSON_FLOAT` (6) for numbers with a decimal point or exponent (e.g. `3.14`, `1e5`).

`Json.newObject() :json`
Creates a new empty JSON object.

`Json.newArray() :json`
Creates a new empty JSON array.

`Json.newString(s: string) :json`
Creates a new JSON string value.

`Json.newInt(n: int64) :json`
Creates a new JSON integer value.

`Json.newFloat(f: double) :json`
Creates a new JSON floating-point value.

`Json.newBool(b: int64) :json`
Creates a new JSON boolean value (1 = true, 0 = false).

`Json.newNull() :json`
Creates a JSON null value.

`Json.release(val: json)`
Releases a JSON value handle.

## HooMap Interop

`Json.parseToMap(str: string) :map`
Parses a flat JSON object into a `map<string, string>`. Returns nil on parse error or if the input contains nested objects or arrays. String values are unescaped and stored without quotes. Numbers are stored as their string representation. Bools become `"true"`/`"false"`. Null becomes `"null"`.

`Json.serializeMap(map: map) :string`
Serializes a `map<string, string>` to a JSON object string. Returns nil if `map` is not a string-valued map. Auto-detects value types: `"null"` → JSON null, `"true"`/`"false"` → JSON bool, parseable numbers → JSON number, otherwise → JSON string (with proper escaping).

## String Transformation

`Json.minify(str: string) :string`
Removes all insignificant whitespace from a JSON string. Whitespace inside string literals is preserved. Returns nil on parse error.

`Json.beautify(str: string) :string`
Pretty-prints a JSON string with 2-space indentation and newlines after each key-value pair and array element. Returns nil on parse error.

## Example

```hoo
// HooMap interop
let map = Json.parseToMap("{\"name\":\"Alice\",\"age\":30}")
let name = map.get("name")     // "Alice"
let age = map.get("age")       // "30"
let json = Json.serializeMap(map)
println(json)

// Minify / Beautify
let compact = Json.minify("{\n  \"key\": \"value\"\n}")
let pretty = Json.beautify("{\"key\":\"value\"}")
```
