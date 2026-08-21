# Json API Reference

## Module

`hoo.json`

## Import Statement

```hoo
import hoo.json;
```

## Module Description

The `json` module provides JSON parsing and serialization through free functions. It supports `null`, booleans, integers, floating-point numbers, strings, objects, arrays, and Hoo-specific tagged types (Buffer, Tensor). All functions throw a `RuntimeException` on invalid input or unsupported types.

JSON object keys must be valid `int64` integers when deserializing into a `Dict<int64, any>`.

## Supported Value Types

| Hoo Type | JSON | Notes |
|----------|------|-------|
| `int64`, `int8`, `byte` | number | Integer representation |
| `bool` | `true` / `false` | |
| `f64` | number | Always includes decimal point for round-trip safety |
| `string` | string | `null` pointer serializes as `null` |
| `Dict<int64, any>` | object | Keys serialized as decimal strings |
| `List` | array | |
| `Buffer` | tagged object | `{"__hoo_buffer__":true,"data":"<base64>"}` |
| `Tensor` | tagged object | `{"__hoo_tensor__":true,"element_type":N,"dims":[...],"data":[...]}` |
| `void` | `null` | |

## Free Functions

---

### `json_serialize_hashmap`

Serializes a `Dict<int64, any>` to a JSON object string.

**Syntax:**

```hoo
json_serialize_hashmap(map: Dict<int64, any>): string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `map` | `Dict<int64, any>` | The dictionary to serialize. Keys are rendered as decimal string field names. |

**Returns:** `string` — The JSON object string.

**Errors:** Throws `RuntimeException` if the map is `nil` or contains unsupported value types.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var map = new Dict<int64, any>();
    map.set(1, "Alice");
    map.set(2, 30);
    var json = json_serialize_hashmap(map);
    println(json); // {"1":"Alice","2":30}
    return 0;
}
```

---

### `json_serialize_anyarray`

Serializes a `List` to a JSON array string.

**Syntax:**

```hoo
json_serialize_anyarray(array: List): string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `array` | `List` | The list to serialize. |

**Returns:** `string` — The JSON array string.

**Errors:** Throws `RuntimeException` if the list is `nil` or contains unsupported element types.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var arr = new List();
    arr.push(1);
    arr.push("two");
    arr.push(true);
    var json = json_serialize_anyarray(arr);
    println(json); // [1,"two",true]
    return 0;
}
```

---

### `json_deserialize_hashmap`

Deserializes a JSON object string into a `Dict<int64, any>`.

**Syntax:**

```hoo
json_deserialize_hashmap(json: string): Dict<int64, any>
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `json` | `string` | A JSON string containing an object (not an array or scalar). |

**Returns:** `Dict<int64, any>` — The deserialized dictionary.

**Errors:** Throws `RuntimeException` on:
- `nil` input
- Invalid JSON syntax
- Non-object JSON root
- Keys that cannot be parsed as `int64`

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var map = json_deserialize_hashmap("{\"1\":42,\"2\":\"hello\"}");
    // map[1] == 42, map[2] == "hello"
    return 0;
}
```

---

### `json_deserialize_anyarray`

Deserializes a JSON array string into a `List`.

**Syntax:**

```hoo
json_deserialize_anyarray(json: string): List
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `json` | `string` | A JSON string containing an array (not an object or scalar). |

**Returns:** `List` — The deserialized list.

**Errors:** Throws `RuntimeException` on:
- `nil` input
- Invalid JSON syntax
- Non-array JSON root

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var arr = json_deserialize_anyarray("[1,\"two\",null,true]");
    // arr[0] == 1, arr[1] == "two", arr[2] == void, arr[3] == true
    return 0;
}
```

---

### `json_minify`

Removes insignificant whitespace from a JSON string.

**Syntax:**

```hoo
json_minify(json: string): string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `json` | `string` | Any valid JSON string. |

**Returns:** `string` — The minified JSON with all whitespace removed.

**Errors:** Throws `RuntimeException` on `nil` input or invalid JSON.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var pretty = "{ \"1\": 42, \"2\": [1, 2, 3] }";
    var minified = json_minify(pretty);
    println(minified); // {"1":42,"2":[1,2,3]}
    return 0;
}
```

---

### `json_beautify`

Pretty-prints a JSON string with 2-space indentation.

**Syntax:**

```hoo
json_beautify(json: string): string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `json` | `string` | Any valid JSON string. |

**Returns:** `string` — The formatted JSON with newlines and 2-space indentation.

**Errors:** Throws `RuntimeException` on `nil` input or invalid JSON.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var compact = "{\"1\":42,\"2\":[1,2,3]}";
    var pretty = json_beautify(compact);
    println(pretty);
    // {
    //   "1": 42,
    //   "2": [
    //     1,
    //     2,
    //     3
    //   ]
    // }
    return 0;
}
```

## Usage Example

```hoo
import hoo.json;

func :int64 main() {
    // Serialize a Dict
    var map = new Dict<int64, any>();
    map.set(1, "world");
    map.set(2, [1, 2, 3]any);
    var json = json_serialize_hashmap(map);
    println(json);

    // Beautify
    var pretty = json_beautify(json);
    println(pretty);

    // Deserialize back
    var parsed = json_deserialize_hashmap(json);

    // Round-trip through serialize/deserialize
    var arr = json_deserialize_anyarray("[1, 2.5, \"hello\", null, true]");
    var arrJson = json_serialize_anyarray(arr);
    println(json_beautify(arrJson));

    return 0;
}
```

## Error Handling

All functions throw `RuntimeException` on errors. Wrap calls in try/catch:

```hoo
import hoo.json;

func :void safeParse(string input) {
    try {
        var result = json_deserialize_hashmap(input);
        // use result
    } catch (RuntimeException e) {
        println("JSON error: " + e.getMessage());
    }
}
```

## Limits

- Maximum nesting depth: 256 levels (both parsing and serialization)
- Dict keys must be valid `int64` values
- Tensor rank limited to 0-3
- Buffer payloads are base64-encoded
