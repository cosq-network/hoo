# CSV API Reference

## Module

`hoo.io`

## Import Statement

```hoo
import hoo.io;
```

## Module Description

The `CSV` class provides static methods for parsing comma-separated values (CSV) text into two-dimensional arrays of strings and serializing arrays back into CSV text. Instance methods provide Automatic Reference Counting (ARC) for memory management of CSV handles.

## Class: CSV

### Declaration

```hoo
class CSV
```

### Public Fields

None.

### Public Class (Static) Functions

#### `parse`

Parses a CSV-formatted string and returns a two-dimensional array of strings. Each top-level element represents a row, and each row contains the field values for that row.

**Syntax:**

```hoo
CSV.parse(text: string) :array
```

**Parameters:**

| Parameter | Type     | Description               |
|-----------|----------|---------------------------|
| `text`    | `string` | The raw CSV text to parse. |

**Returns:** `array` — A two-dimensional array (`array` of `array` of `string`). Returns `null` if the input is malformed.

**Errors:** Returns `null` on malformed CSV input. No exception is thrown.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var rows = CSV.parse("name,age\nAlice,30\nBob,25");
    println(rows.length()); // 3
    return 0;
}
```

---

#### `serialize`

Serializes a two-dimensional array of strings into a CSV-formatted string. Fields containing the delimiter, the quote character, or newlines are automatically quoted; embedded quote characters are doubled.

**Syntax:**

```hoo
CSV.serialize(data: array) :string
```

**Parameters:**

| Parameter | Type    | Description                                                |
|-----------|---------|------------------------------------------------------------|
| `data`    | `array` | A two-dimensional array (`array` of `array` of `string`). |

**Returns:** `string` — The CSV-formatted string. Returns an empty string if `data` is null, empty, or contains only empty rows.

**Errors:** Returns an empty string on invalid input. No exception is thrown.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var data = [["a", "b"], ["1", "2"]];
    var csv = CSV.serialize(data);
    println(csv); // a,b\n1,2\n
    return 0;
}
```

### Public Instance Functions

A `CSV` instance is obtained through the class constructor (`new CSV()`). The following methods manage the instance reference count.

#### `retain`

Increments the reference count of a CSV instance. Returns the same instance for chaining.

**Syntax:**

```hoo
csv.retain() :CSV
```

**Parameters:**

None.

**Returns:** `CSV` — The same CSV instance with an incremented reference count.

**Errors:** Returns `null` if `csv` is null.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var csv = new CSV();
    var csv2 = csv.retain();
    csv.release();
    csv2.release();
    return 0;
}
```

---

#### `release`

Decrements the reference count of a CSV instance. When the count reaches zero the instance is freed. After calling `release`, the handle must not be used again.

**Syntax:**

```hoo
csv.release()
```

**Parameters:**

None.

**Returns:** `void`

**Errors:** No-op if `csv` is null.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var csv = new CSV();
    csv.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of a CSV instance. Intended for debugging and testing.

**Syntax:**

```hoo
csv.refcount() :int64
```

**Parameters:**

None.

**Returns:** `int64` — The current reference count.

**Errors:** Returns `0` if `csv` is null.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var csv = new CSV();
    println(csv.refcount()); // 1
    csv.release();
    return 0;
}
```

## Usage Example

```hoo
import hoo.io;

func :int64 main() {
    // Parse CSV text
    var rows = CSV.parse("name,age\nAlice,30\nBob,25");
    println(rows.length()); // 3

    // Serialize data to CSV
    var output = CSV.serialize([["x", "y"], ["1", "2"]]);
    println(output);

    // Instance lifetime management
    var csv = new CSV();
    csv.release();
    return 0;
}
```
