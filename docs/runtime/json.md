# JSON Reference Guide (`Json`)

The `Json` class provides parsing, stringifying, querying, and construction of JSON values (objects, arrays, strings, numbers, booleans, null) using Automatic Reference Counting (ARC) managed opaque handles.

---

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

---

## 1. Parsing and Stringifying

### `Json.parse(str)`
- **Description:** Parses a JSON-formatted string into a structured JSON value handle. If the input string contains invalid or malformed JSON syntax, the function stops and returns a zero/null handle wrapper.
- **Syntax:** `Json.parse(str: string) :ptr`
- **Parameters:** `str` — The raw JSON string data to parse.
- **Return Value:** `ptr` — An ARC-managed opaque handle representing the root JSON structure, or `0` on parse failure.
- **Example:**
```hoo
func :int64 main() {
    var rawJson = "{\"status\":\"ok\"}";
    var handle = Json.parse(rawJson);
    if (handle.type() == 5) {
        println("Successfully parsed JSON Object.");
    }
    handle.release();
    return 0;
}
```

### `val.stringify()`
- **Description:** Serializes a JSON value handle structure back into its equivalent compact string representation. It traces all nested properties, arrays, and primitives starting from the targeted node reference.
- **Syntax:** `val.stringify() :string`
- **Parameters:** None
- **Return Value:** `string` — The serialized JSON string format.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.newObject();
    var jsonStr = obj.stringify();
    println("Serialized: ".concat(jsonStr));
    obj.release();
    return 0;
}
```

---

## 2. Type Inspection

### `val.type()`
- **Description:** Queries and returns the specific JSON data type code associated with the target value handle wrapper. The returned integer maps directly onto one of the standard Type Constants.
- **Syntax:** `val.type() :int64`
- **Parameters:** None
- **Return Value:** `int64` — The type code index from 0 to 6 representing the true inner node type.
- **Example:**
```hoo
func :int64 main() {
    var item = Json.newNull();
    var t = item.type();
    println("Type Code: ".concat(t.toString()));
    item.release();
    return 0;
}
```

---

## 3. Primitive Value Construction

### `Json.newNull()`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized to represent a `null` structural token. This is widely used to signal unassigned or explicitly empty keys inside an object schema.
- **Syntax:** `Json.newNull() :ptr`
- **Parameters:** None
- **Return Value:** `ptr` — A managed JSON handle representing a `null` value.
- **Example:**
```hoo
func :int64 main() {
    var item = Json.newNull();
    println("Null handle created successfully.");
    item.release();
    return 0;
}
```

### `Json.newBool(val)`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized to represent a literal boolean value. It converts standard integer status flags into canonical JSON `true` or `false` elements.
- **Syntax:** `Json.newBool(val: int64) :ptr`
- **Parameters:** `val` — An integer flag where `0` means false and non-zero values translate to true.
- **Return Value:** `ptr` — A managed JSON handle representing a boolean.
- **Example:**
```hoo
func :int64 main() {
    var b = Json.newBool(1);
    println("Boolean handle created successfully.");
    b.release();
    return 0;
}
```

### `Json.newInt(val)`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized to represent a 64-bit integer number. It encapsulates native whole numbers safely inside a managed document node block.
- **Syntax:** `Json.newInt(val: int64) :ptr`
- **Parameters:** `val` — The integer value to encapsulate.
- **Return Value:** `ptr` — A managed JSON handle representing an integer.
- **Example:**
```hoo
func :int64 main() {
    var num = Json.newInt(42);
    println("Integer node created.");
    num.release();
    return 0;
}
```

### `Json.newFloat(val)`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized to represent a floating-point number. It retains double-precision accuracy for real number values inside document blocks.
- **Syntax:** `Json.newFloat(val: double) :ptr`
- **Parameters:** `val` — The double value to encapsulate.
- **Return Value:** `ptr` — A managed JSON handle representing a floating-point number.
- **Example:**
```hoo
func :int64 main() {
    var f = Json.newFloat(3.14159);
    println("Floating-point node created.");
    f.release();
    return 0;
}
```

### `Json.newString(str)`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized to contain a copy of a text string. It preserves UTF-8 format character content cleanly for safe text transport.
- **Syntax:** `Json.newString(str: string) :ptr`
- **Parameters:** `str` — The native Hoo string text data.
- **Return Value:** `ptr` — A managed JSON handle representing a string element.
- **Example:**
```hoo
func :int64 main() {
    var s = Json.newString("Hello, World!");
    println("String node created.");
    s.release();
    return 0;
}
```

---

## 4. Object/Array Construction

### `Json.newObject()`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized as an empty object containing zero key-value elements. It acts as a blank parent collection node suitable for storing complex sub-properties.
- **Syntax:** `Json.newObject() :ptr`
- **Parameters:** None
- **Return Value:** `ptr` — A managed JSON handle representing an empty object.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.newObject();
    println("Object container allocated.");
    obj.release();
    return 0;
}
```

### `Json.newArray()`
- **Description:** Allocates and returns a fresh JSON value handle wrapper initialized as an empty sequential array list with a length of zero. It provides a linear database container designed to accommodate ordered streams of values.
- **Syntax:** `Json.newArray() :ptr`
- **Parameters:** None
- **Return Value:** `ptr` — A managed JSON handle representing an empty array sequence.
- **Example:**
```hoo
func :int64 main() {
    var arr = Json.newArray();
    println("Array sequence allocated.");
    arr.release();
    return 0;
}
```

### `obj.set(key, val)`
- **Description:** Binds a specified key name string to a targeted JSON value node reference inside an active object collection. If the target key name already exists, the old property value reference is cleanly overwritten.
- **Syntax:** `obj.set(key: string, val: ptr) :void`
- **Parameters:**
  - `key` — The property identifier label name.
  - `val` — The managed JSON value node handle to associate with the key.
- **Return Value:** Nothing.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.newObject();
    var val = Json.newString("Hoo");
    obj.set("name", val);
    obj.release();
    return 0;
}
```

### `arr.push(val)`
- **Description:** Appends a targeted JSON value handle wrapper onto the tail end of an active sequential array container. This automatically increases the array structure's tracking length score by one slot.
- **Syntax:** `arr.push(val: ptr) :void`
- **Parameters:** `val` — The managed JSON value handle reference to append.
- **Return Value:** Nothing.
- **Example:**
```hoo
func :int64 main() {
    var arr = Json.newArray();
    var item = Json.newInt(100);
    arr.push(item);
    arr.release();
    return 0;
}
```

### `arr.length()`
- **Description:** Returns the total quantity of data value elements packed inside the targeted sequential array sequence handle. It provides immediate structural sizing records for iteration boundaries.
- **Syntax:** `arr.length() :int64`
- **Parameters:** None
- **Return Value:** `int64` — The current element size length count score of the array.
- **Example:**
```hoo
func :int64 main() {
    var arr = Json.newArray();
    var len = arr.length();
    println("Array Size: ".concat(len.toString()));
    arr.release();
    return 0;
}
```

---

## 5. Value Access

### `obj.get(key)`
- **Description:** Locates and returns a nested property value handle from an object container matching the specified key identifier string. If the requested property key is completely missing from the object schema, a zero pointer fallback is returned instead.
- **Syntax:** `obj.get(key: string) :ptr`
- **Parameters:** `key` — The text label key string name to query.
- **Return Value:** `ptr` — A borrowed reference pointer handle to the sub-property element, or `0` if not discovered.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.parse("{\"id\":123}");
    var node = obj.get("id");
    if (node != 0) {
        println("Property key discovered.");
    }
    obj.release();
    return 0;
}
```

### `obj.getString(key)`
- **Description:** Retrieves the text string data contents packed inside a specific object property field matching the provided key label name. If the designated key does not exist inside the object instance, an empty text string wrapper fallback is safely emitted.
- **Syntax:** `obj.getString(key: string) :string`
- **Parameters:** `key` — The text property label key name to query.
- **Return Value:** `string` — The native Hoo text contents of the field, or an empty string fallback if not present.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.parse("{\"user\":\"admin\"}");
    var username = obj.getString("user");
    println("User: ".concat(username));
    obj.release();
    return 0;
}
```

### `obj.getInt(key)`
- **Description:** Retrieves the whole integer number packed inside a specific object property field matching the provided key label name. If the targeted property contains a floating-point value instead, its decimal segment components are truncated toward zero automatically.
- **Syntax:** `obj.getInt(key: string) :int64`
- **Parameters:** `key` — The text property label key name to query.
- **Return Value:** `int64` — The integer numeric value extracted from the key, or `0` if missing.
- **Example:**
```hoo
func :int64 main() {
    var obj = Json.parse("{\"count\":5}");
    var c = obj.getInt("count");
    println("Count: ".concat(c.toString()));
    obj.release();
    return 0;
}
```

### `arr.get(index)`
- **Description:** Extracts a borrowed value node handle reference stored at a targeted linear position layout offset inside an active sequential array container. The index query bounds must comfortably fall within the array length limits, otherwise a zero handle is returned.
- **Syntax:** `arr.get(index: int64) :ptr`
- **Parameters:** `index` — The zero-based numerical cell array coordinate index location.
- **Return Value:** `ptr` — A borrowed reference pointer handle to the sequence cell node, or `0` if invalid.
- **Example:**
```hoo
func :int64 main() {
    var arr = Json.parse("[10, 20, 30]");
    var item = arr.get(1);
    if (item != 0) {
        println("Extracted valid cell element reference.");
    }
    arr.release();
    return 0;
}
```

---

## 6. HooMap Interop

### `Json.parseToMap(str)`
- **Description:** Parses a flat, non-nested JSON object layout string straight into a native dictionary collection structure. If the targeted JSON payload contains multi-dimensional nested array components or nested objects, the operation breaks and returns an empty map template.
- **Syntax:** `Json.parseToMap(str: string) :map[string, string]`
- **Parameters:** `str` — The flat string text payload containing valid JSON object expressions.
- **Return Value:** `map[string, string]` — A native dictionary populated with key-value text pairings, or an empty dictionary instance on failure.
- **Example:**
```hoo
func :int64 main() {
    var m = Json.parseToMap("{\"name\":\"Alice\",\"age\":\"30\"}");
    var val = m.get("name");
    println("Name from Map: ".concat(val));
    m.release();
    return 0;
}
```

### `Json.serializeMap(map)`
- **Description:** Serializes a native string dictionary collection structure into its equivalent encoded JSON object text payload string. It automatically detects and translates special value formats like valid numbers or true/false strings into their pristine unquoted JSON datatypes.
- **Syntax:** `Json.serializeMap(map: map[string, string]) :string`
- **Parameters:** `map` — The native string dictionary map reference.
- **Return Value:** `string` — The encoded serialized JSON object string document text format.
- **Example:**
```hoo
func :int64 main() {
    var m = new map[string, string]();
    m.set("item", "widget");
    var s = Json.serializeMap(m);
    println("Serialized Map: ".concat(s));
    m.release();
    return 0;
}
```

---

## 7. String Transformation

### `Json.minify(str)`
- **Description:** Strips all insignificant formatting spaces, tabs, and newline separator tokens out of a JSON text document payload string. It carefully leaves all space metrics bounded inside structural text literals completely un-touched.
- **Syntax:** `Json.minify(str: string) :string`
- **Parameters:** `str` — The raw or pretty-printed JSON string input.
- **Return Value:** `string` — A minified and compressed JSON string layout profile, or an empty string on error.
- **Example:**
```hoo
func :int64 main() {
    var compact = Json.minify("{\n  \"key\": \"val\"\n}");
    println("Minified: ".concat(compact));
    return 0;
}
```

### `Json.beautify(str)`
- **Description:** Pretty-prints a compact JSON string structure layout, adding clean indentation grids and newline breaks for optimal human read metrics. It standardizes all indentation profiles to use a standard uniform 2-space column pattern.
- **Syntax:** `Json.beautify(str: string) :string`
- **Parameters:** `str` — A valid unformatted or minified JSON string input.
- **Return Value:** `string` — A stylized pretty-printed JSON document string format, or an empty string on parse error.
- **Example:**
```hoo
func :int64 main() {
    var pretty = Json.beautify("{\"a\":1,\"b\":2}");
    println("Beautified:\n".concat(pretty));
    return 0;
}
```

---

## 8. Memory Management

### `val.retain()`
- **Description:** Explicitly increments the inner Automatic Reference Counting (ARC) transaction life tally metadata tracked by the targeted JSON value node reference wrapper. This shields the resource slot from getting prematurely purged when its parent element is deleted.
- **Syntax:** `val.retain() :ptr`
- **Parameters:** None
- **Return Value:** `ptr` — The same targeted node pointer handle wrapper instance.
- **Example:**
```hoo
func :int64 main() {
    var item = Json.newString("data");
    var parallelRef = item.retain();
    parallelRef.release();
    item.release();
    return 0;
}
```

### `val.release()`
- **Description:** Explicitly decrements the inner Automatic Reference Counting (ARC) transaction life tally metadata tracked by the targeted JSON value node reference wrapper. Once the internal tally records reach absolute zero thresholds, the resource memory is automatically recycled.
- **Syntax:** `val.release() :void`
- **Parameters:** None
- **Return Value:** Nothing.
- **Example:**
```hoo
func :int64 main() {
    var item = Json.newObject();
    item.release();
    return 0;
}
```
