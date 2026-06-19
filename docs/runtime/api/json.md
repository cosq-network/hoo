# JSON API Developer Reference

**Import Requirement:**
```hoo
import hoo.json;
```

The JSON runtime API is intentionally small and free-function based. There is
no `Json` class, no instance API, and no opaque JSON document handle. Hoo code
works with JSON through existing collection abstractions:

- `HashMap<integer, T>` for JSON objects. Numeric Hoo keys are serialized as
  JSON object field names.
- `AnyArray` for JSON arrays.
- `string` for raw JSON text and formatted JSON output.

Supported serialized/deserialized JSON values are `null`, booleans, integer
numbers, floating-point numbers, strings, objects, and arrays. JSON objects map
to `HashMap<int64, any>` during deserialization, so object field names must be
valid `int64` text.

All JSON APIs throw `RuntimeException` on invalid input, unsupported values,
wrong root type, invalid object keys, non-finite floating-point values, or
allocation failure.

## `json_serialize_hashmap`

### Description

Serializes a Hoo `HashMap` into a JSON object string.

The map key type must be an integer scalar supported by `HashMap` (`int64`,
`int8`, or `byte`). Keys are written as JSON object field names. Supported map
values are `int64`, `int8`, `byte`, `bool`, `f64`, `string`, nested `HashMap`,
nested `AnyArray`, and `void`/null values carried through `any`.

### Syntax

```hoo
json_serialize_hashmap(map: HashMap<integer, T>) :string
```

### Parameters

`map`
The `HashMap` to serialize. Its keys become JSON object field names.

### Return Type

`string`
A JSON object string. The returned string is a normal Hoo runtime string.

### Errors

Throws `RuntimeException` if `map` is nil, if the map uses unsupported key or
value types, if a nested value is invalid, or if a floating-point value is not
finite.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var user: HashMap<int64, string> = new HashMap<int64, string>();
    user[1] = "Alice";
    user[2] = "admin";

    var json = json_serialize_hashmap(user);
    println(json); // {"1":"Alice","2":"admin"}

    return json.contains("\"1\":\"Alice\"");
}
```

## `json_serialize_anyarray`

### Description

Serializes a Hoo `AnyArray` into a JSON array string.

Supported element types are `int64`, `int8`, `byte`, `bool`, `f64`, `string`,
nested `HashMap`, nested `AnyArray`, and `void`/null values.

### Syntax

```hoo
json_serialize_anyarray(values: AnyArray) :string
```

### Parameters

`values`
The `AnyArray` to serialize.

### Return Type

`string`
A JSON array string.

### Errors

Throws `RuntimeException` if `values` is nil, if an element type is unsupported,
if a nested value is invalid, or if a floating-point value is not finite.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var values = [1, "two", true]any;

    var json = json_serialize_anyarray(values);
    println(json); // [1,"two",true]

    return json.equals("[1,\"two\",true]");
}
```

## `json_deserialize_hashmap`

### Description

Parses a JSON object string into `HashMap<int64, any>`.

The JSON root must be an object. Every object field name must be valid `int64`
text because Hoo `HashMap` keys are integer scalars. Nested JSON objects become
nested `HashMap<int64, any>` values. Nested JSON arrays become `AnyArray`
values.

Deserialized values map as follows:

| JSON value | Hoo value |
|------------|-----------|
| `null` | `any` value with `void` type |
| boolean | `bool` |
| integer number | `int64` |
| decimal/exponent number | `f64` |
| string | `string` |
| object | `HashMap<int64, any>` |
| array | `AnyArray` |

### Syntax

```hoo
json_deserialize_hashmap(json: string) :HashMap<int64, any>
```

### Parameters

`json`
The JSON text to parse. It must contain a JSON object as the root value.

### Return Type

`HashMap<int64, any>`
A new `HashMap` containing the parsed object values.

### Errors

Throws `RuntimeException` if `json` is nil, malformed, not a JSON object, has
object keys that are not valid `int64` keys, contains numbers outside supported
runtime ranges, or cannot allocate the returned values.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var user = json_deserialize_hashmap("{\"1\":42,\"2\":\"Alice\",\"3\":[7,true]}");

    var count = user.count();
    println(count); // 3

    return count;
}
```

## `json_deserialize_anyarray`

### Description

Parses a JSON array string into `AnyArray`.

The JSON root must be an array. Nested JSON objects become
`HashMap<int64, any>` values and nested JSON arrays become nested `AnyArray`
values.

### Syntax

```hoo
json_deserialize_anyarray(json: string) :AnyArray
```

### Parameters

`json`
The JSON text to parse. It must contain a JSON array as the root value.

### Return Type

`AnyArray`
A new `AnyArray` containing the parsed values.

### Errors

Throws `RuntimeException` if `json` is nil, malformed, not a JSON array,
contains an object with non-`int64` field names, contains numbers outside
supported runtime ranges, or cannot allocate the returned values.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var values = json_deserialize_anyarray("[1,\"two\",false,{\"7\":8}]");

    var count = values.length();
    println(count); // 4

    return count;
}
```

## `json_minify`

### Description

Validates a JSON string and returns a compact JSON representation with
insignificant whitespace removed. Whitespace inside string literals is
preserved.

### Syntax

```hoo
json_minify(json: string) :string
```

### Parameters

`json`
The JSON text to validate and minify.

### Return Type

`string`
A minified JSON string.

### Errors

Throws `RuntimeException` if `json` is nil or malformed.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var compact = json_minify("{ \"items\" : [ 1, true, null ], \"name\" : \"A B\" }");

    println(compact); // {"items":[1,true,null],"name":"A B"}

    return compact.contains("\"items\":[1,true,null]");
}
```

## `json_beautify`

### Description

Validates a JSON string and returns a pretty-printed JSON representation using
2-space indentation.

### Syntax

```hoo
json_beautify(json: string) :string
```

### Parameters

`json`
The JSON text to validate and pretty-print.

### Return Type

`string`
A formatted JSON string with newlines and 2-space indentation.

### Errors

Throws `RuntimeException` if `json` is nil or malformed.

### Complete Example

```hoo
import hoo.json;

func :int64 main() {
    var pretty = json_beautify("{\"name\":\"Alice\",\"roles\":[\"admin\",\"user\"]}");

    println(pretty);

    return pretty.contains("\n  \"name\": \"Alice\"");
}
```
