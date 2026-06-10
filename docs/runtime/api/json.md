# Json — JSON Parsing and Generation

The `Json` class provides static methods for parsing, manipulating, and generating JSON data.

All JSON values are handles of type `json` created through the API.

## Type Constants

These constants are returned by `Json.type()`:

| Constant         | Value |
|------------------|-------|
| `HOO_JSON_NULL`  | 0     |
| `HOO_JSON_BOOL`  | 1     |
| `HOO_JSON_INT`   | 2     |
| `HOO_JSON_STRING`| 3     |
| `HOO_JSON_ARRAY` | 4     |
| `HOO_JSON_OBJECT`| 5     |

## Methods

`Json.parse(str: string) :json`
Parses a JSON string and returns a JSON value handle.

`Json.stringify(val: json) :string`
Converts a JSON value to its JSON string representation.

`Json.get(obj: json, key: string) :json`
Gets the value associated with a key from a JSON object.

`Json.getInt(obj: json, key: string) :int64`
Gets the integer value associated with a key from a JSON object.

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
Returns the type constant of the JSON value (see Type Constants).

`Json.newObject() :json`
Creates a new empty JSON object.

`Json.newArray() :json`
Creates a new empty JSON array.

`Json.newString(s: string) :json`
Creates a new JSON string value.

`Json.newInt(n: int64) :json`
Creates a new JSON integer value.

`Json.newBool(b: int64) :json`
Creates a new JSON boolean value (1 = true, 0 = false).

`Json.newNull() :json`
Creates a JSON null value.

`Json.release(val: json)`
Releases a JSON value handle.

## Example

```hoo
let obj = Json.newObject()
Json.set(obj, "name", Json.newString("Alice"))
Json.set(obj, "age", Json.newInt(30))
Json.set(obj, "active", Json.newBool(1))

let jsonStr = Json.stringify(obj)
println(jsonStr)

let parsed = Json.parse(jsonStr)
let name = Json.getString(parsed, "name")
let age = Json.getInt(parsed, "age")
println(name + " is " + age + " years old")

Json.release(parsed)
Json.release(obj)

let arr = Json.newArray()
Json.arrayPush(arr, Json.newInt(10))
Json.arrayPush(arr, Json.newInt(20))
Json.arrayPush(arr, Json.newInt(30))

let first = Json.arrayGet(arr, 0)
println(Json.type(first))  // 2 (HOO_JSON_INT)
Json.release(arr)
```
