# Csv API Reference

## Module

`hoo.csv`

## Import Statement

```hoo
import hoo.csv;
```

## Module Description

The `Csv` class provides instance methods for parsing comma-separated values (CSV) text into two-dimensional arrays of strings and generating CSV text from arrays. Instance methods provide Automatic Reference Counting (ARC) for memory management of CSV handles.

## Class: Csv

### Declaration

```hoo
class Csv
```

### Constructor

```hoo
new Csv() :Csv
```

Creates a new CSV instance.

### Public Instance Functions

#### `parse`

Parses a CSV-formatted string and returns a two-dimensional array of strings. Each top-level element represents a row, and each row contains the field values for that row.

**Syntax:**

```hoo
csv.parse(text: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The raw CSV text to parse. |

**Returns:** `array` — A two-dimensional array (`array` of `array` of `string`). Returns `null` if the input is malformed.

**Errors:** Returns `null` on malformed CSV input. No exception is thrown.

**Complete Example:**

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var rows = csv.parse("name,age\nAlice,30\nBob,25");
    println(rows.length()); // 3
    csv.release();
    return 0;
}
```

---

#### `generate`

Serializes a two-dimensional array of strings into a CSV-formatted string. Fields containing the delimiter, the quote character, or newlines are automatically quoted; embedded quote characters are doubled.

**Syntax:**

```hoo
csv.generate(data: array) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `array` | A two-dimensional array (`array` of `array` of `string`). |

**Returns:** `string` — The CSV-formatted string. Returns an empty string if `data` is null, empty, or contains only empty rows.

**Errors:** Returns an empty string on invalid input. No exception is thrown.

**Complete Example:**

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var data = [["a", "b"], ["1", "2"]];
    var result = csv.generate(data);
    println(result); // a,b\n1,2\n
    csv.release();
    return 0;
}
```

---

#### `retain`

Increments the reference count of a CSV instance. Returns the same instance for chaining.

**Syntax:**

```hoo
csv.retain() :Csv
```

**Parameters:**

None.

**Returns:** `Csv` — The same Csv instance with an incremented reference count.

**Errors:** Returns `null` if `csv` is null.

**Complete Example:**

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
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
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
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
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    println(csv.refcount()); // 1
    csv.release();
    return 0;
}
```

## Free Functions

### `csv_from_opts`

Creates a new CSV instance with custom delimiter and quote characters.

**Syntax:**

```hoo
csv_from_opts(delimiter: int64, quote: int64) :Csv
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `delimiter` | `int64` | The delimiter character code. |
| `quote` | `int64` | The quote character code. |

**Returns:** `Csv` — A new Csv instance with the specified options.

**Errors:** Returns `null` if parameters are invalid.

**Complete Example:**

```hoo
import hoo.csv;

func :int64 main() {
    var csv = csv_from_opts(59, 34); // semicolon delimiter, double-quote
    var rows = csv.parse("a;b\n1;2");
    println(rows.length()); // 2
    csv.release();
    return 0;
}
```

## Usage Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();

    // Parse CSV text
    var rows = csv.parse("name,age\nAlice,30\nBob,25");
    println(rows.length()); // 3

    // Serialize data to CSV
    var output = csv.generate([["x", "y"], ["1", "2"]]);
    println(output);

    csv.release();
    return 0;
}
```
