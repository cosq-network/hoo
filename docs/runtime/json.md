# JSON Runtime Reference

The JSON runtime is a small set of free functions for serialization,
deserialization, and string formatting. It is not class-based and does not
expose JSON document handles.

Removed APIs include `Json.parse`, `Json.stringify`, `Json.get`,
`Json.newObject`, `Json.newArray`, `Json.set`, `Json.release`, and every other
opaque `json`/`ptr`-style operation. Hoo code should use `HashMap` and
`AnyArray` as the data model.

## Data Model

### `HashMap`

`json_serialize_hashmap(map)` serializes a `HashMap` as a JSON object.

Hoo `HashMap` keys are integer scalars (`int64`, `int8`, or `byte`), so JSON
object keys are emitted as decimal strings:

```hoo
var m: HashMap<int64, int64> = new HashMap<int64, int64>();
m[7] = 99;
var json = json_serialize_hashmap(m); // {"7":99}
```

Supported values:

- `int64`, `int8`, `byte`
- `bool`
- `f64`
- `string`
- nested `HashMap`
- nested `AnyArray`

Unsupported values throw a `RuntimeException`.

### `AnyArray`

`json_serialize_anyarray(values)` serializes an `AnyArray` as a JSON array:

```hoo
var values = [1, "two", true]any;
var json = json_serialize_anyarray(values); // [1,"two",true]
```

Supported elements are the same as supported `HashMap` values.

## Deserialization

`json_deserialize_hashmap(json)` parses a JSON object into
`HashMap<int64, any>`. JSON object keys must be valid `int64` values:

```hoo
var map = json_deserialize_hashmap("{\"1\":42,\"2\":[7,true]}");
var count = map.count(); // 2
```

`json_deserialize_anyarray(json)` parses a JSON array into `AnyArray`:

```hoo
var values = json_deserialize_anyarray("[1,\"two\",true]");
var count = values.length(); // 3
```

Deserialized JSON values map to Hoo runtime values as follows:

| JSON value | Hoo value |
|------------|-----------|
| `null` | `any` value with `void` type |
| boolean | `bool` |
| integer number | `int64` |
| decimal/exponent number | `f64` |
| string | `string` |
| object | `HashMap<int64, any>` |
| array | `AnyArray` |

## Functions

### `json_serialize_hashmap(map: HashMap<integer, T>) :string`

Returns a JSON object string.

### `json_serialize_anyarray(values: AnyArray) :string`

Returns a JSON array string.

### `json_deserialize_hashmap(json: string) :HashMap<int64, any>`

Returns a `HashMap<int64, any>` parsed from a JSON object.

### `json_deserialize_anyarray(json: string) :AnyArray`

Returns an `AnyArray` parsed from a JSON array.

### `json_minify(json: string) :string`

Validates and minifies a JSON string.

### `json_beautify(json: string) :string`

Validates and pretty-prints a JSON string with 2-space indentation.

## Error Handling

All JSON functions throw `RuntimeException` on errors. This includes invalid
JSON, nil inputs, wrong root types for deserialization, object keys that are not
valid `int64` keys, unsupported serialization value types, non-finite
floating-point values, and allocation failures.

## C ABI

The native runtime functions are:

```c
HooString hoo_json_serialize_hashmap(HooHashMap map);
HooString hoo_json_serialize_anyarray(HooAnyArray array);
HooHashMap hoo_json_deserialize_hashmap(const char* json);
HooAnyArray hoo_json_deserialize_anyarray(const char* json);
HooString hoo_json_minify(const char* json);
HooString hoo_json_beautify(const char* json);
```

Returned `HooString`, `HooHashMap`, and `HooAnyArray` values have refcount 1 and
must be released by the caller when used directly from C/C++ tests or native
integrations.
