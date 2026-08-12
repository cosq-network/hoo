# Json API Reference

## Module

`hoo.json`

## Import Statement

```hoo
import hoo.json;
```

## Module Description

The `json` module provides JSON parsing and serialization through free functions. It supports `null`, booleans, integers, floats, strings, objects, and arrays. All functions return `null` on error instead of throwing exceptions.

## Free Functions

---

### `json_parse`

Parses a JSON string into a Hoo value.

**Syntax:**

```hoo
json_parse(text: string): Any
```

**Parameters:**

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `text`    | `string` | The JSON text to parse. |

**Returns:** `Any` — The parsed value (can be `null`, `bool`, `int64`, `f64`, `string`, `Dict<int64, any>`, or `List`), or `null` if the input is malformed.

**Errors:** Returns `null` on parse error. No exceptions are thrown.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var parsed = json_parse("{\"1\":42,\"2\":\"hello\",\"3\":[true, false]}");
    // parsed is Dict<int64, any> with 3 entries
    return 0;
}
```

---

### `json_stringify`

Converts a Hoo value to its JSON string representation.

**Syntax:**

```hoo
json_stringify(value: Any): string
```

**Parameters:**

| Parameter | Type  | Description                          |
|-----------|-------|--------------------------------------|
| `value`   | `Any` | The value to serialize to JSON. |

**Returns:** `string` — The JSON string, or `null` if the value cannot be serialized.

**Errors:** Returns `null` if the value type is unsupported or contains non-finite floats. No exceptions are thrown.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var map = new Dict<int64, any>();
    map[1] = "Alice";
    map[2] = 30;
    var json = json_stringify(map);
    println(json); // {"1":"Alice","2":30}
    return 0;
}
```

---

### `json_pretty`

Converts a Hoo value to a pretty-printed JSON string with indentation.

**Syntax:**

```hoo
json_pretty(value: Any): string
```

**Parameters:**

| Parameter | Type  | Description                          |
|-----------|-------|--------------------------------------|
| `value`   | `Any` | The value to pretty-print to JSON. |

**Returns:** `string` — The formatted JSON string with newlines and 2-space indentation, or `null` on error.

**Errors:** Returns `null` if the value type is unsupported or contains non-finite floats. No exceptions are thrown.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var map = new Dict<int64, any>();
    map[1] = "Alice";
    map[2] = [1, 2, 3]any;
    var pretty = json_pretty(map);
    println(pretty);
    // {
    //   "1": "Alice",
    //   "2": [
    //     1,
    //     2,
    //     3
    //   ]
    // }
    return 0;
}
```

---

### `json_free_string`

Frees a string allocated by a `json` function.

**Syntax:**

```hoo
json_free_string(str: string): void
```

**Parameters:**

| Parameter | Type     | Description                            |
|-----------|----------|----------------------------------------|
| `str`     | `string` | The string returned by a `json` function to free. |

**Returns:** `void`

**Errors:** None.

**Complete Example:**

```hoo
import hoo.json;

func :int64 main() {
    var json = json_stringify(42);
    println(json);
    json_free_string(json);
    return 0;
}
```

## Usage Example

```hoo
import hoo.json;

func :int64 main() {
    var raw = "{\"1\":\"world\",\"2\":[1,2,3]}";
    var parsed = json_parse(raw);

    var str = json_stringify(parsed);
    println(str);
    json_free_string(str);

    var pretty = json_pretty(parsed);
    println(pretty);
    json_free_string(pretty);

    return 0;
}
```
