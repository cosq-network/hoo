# CSV API Reference (`Csv`)

The `Csv` class provides CSV parsing, generation, and file I/O operations with Automatic Reference Counting (ARC). Create an instance with `Csv.new()` or `Csv.newWithOpts()` and release it with `release()`.

## 1. Constructor / Destructor & ARC

### `Csv.new() :Csv`

Creates a new Csv instance with default settings (delimiter=`,`, quote=`"`).

### `Csv.newWithOpts(delimiter: int64, quote: int64) :Csv`

Creates a new Csv instance with a custom delimiter and quote character. Both parameters are ASCII code points.

### `csv.retain() :Csv`

Increments the reference count. Returns the same instance for chaining.

### `csv.release()`

Decrements the reference count. The instance is freed when the count reaches zero.

### `csv.refcount() :int64`

Returns the current reference count (for debugging).

```hoo
var csv = Csv.new()
var csv2 = Csv.newWithOpts(59, 39)  // delimiter=';', quote='\''
csv.release()
csv2.release()
```

## 2. Parsing

### `csv.parse(input: string) :array`

Parses a CSV string and returns an array of rows, where each row is an array of field strings.

- **Returns:** `array` — a two-dimensional array (`array` of `array` of `string`).

```hoo
var csv = Csv.new()
var input = "name,age\nAlice,30\nBob,25"
var rows = csv.parse(input)
println(rows.length().toString())  // Output: 3
csv.release()
```

### `csv.parseAsMaps(input: string) :array`

Parses a CSV string and returns an array of maps, using the first row as string-key headers. Each subsequent row is a `Map<string, string>` mapping header names to field values.

- **Returns:** `array` — an array of `Map<string, string>` objects.

```hoo
var csv = Csv.new()
var input = "name,age\nAlice,30\nBob,25"
var records = csv.parseAsMaps(input)
println(records.length().toString())  // Output: 2
var first = records[0]
println(first["name"])  // Output: Alice
println(first["age"])   // Output: 30
csv.release()
```

## 3. Generation

### `csv.generate(data: array) :string`

Generates a CSV-formatted string from a two-dimensional array.

- **Returns:** `string` — the CSV-formatted string.

```hoo
var csv = Csv.new()
var data = [["a", "b"], ["1", "2"]]
var output = csv.generate(data)
println(output)
csv.release()
```

## 4. File I/O

### `csv.readFile(path: string) :array`

Reads a CSV file from disk and returns the parsed data as a two-dimensional array.

- **Returns:** `array` — parsed data, or `0` on failure.

```hoo
var csv = Csv.new()
var rows = csv.readFile("data.csv")
if rows != 0 {
    println(rows.length().toString())
}
csv.release()
```

### `csv.readFileAsMaps(path: string) :array`

Reads a CSV file from disk and returns the parsed data as an array of maps, using the first row as string-key headers.

- **Returns:** `array` — an array of `Map<string, string>` objects, or `0` on failure.

```hoo
var csv = Csv.new()
var records = csv.readFileAsMaps("data.csv")
if records != 0 {
    println(records.length().toString())
}
csv.release()
```

### `csv.writeFile(path: string, data: array) :int64`

Writes a two-dimensional array of strings to a CSV file.

- **Returns:** `int64` — 0 on success, 1 on failure.

```hoo
var csv = Csv.new()
var data = [["name", "age"], ["Alice", "30"], ["Bob", "25"]]
var ok = csv.writeFile("output.csv", data)
csv.release()
```

## 5. Utilities

### `csv.escape(c: int64) :int64`

Checks if a character (ASCII code point) needs escaping in a CSV field.

- **Returns:** `int64` — 1 if the character needs escaping (comma, double-quote, newline), 0 otherwise.

```hoo
var csv = Csv.new()
var r = csv.escape(44)  // comma, returns 1
csv.release()
```

## 6. Aggregation (DataFrame-like)

These methods operate on `HooArray<HooMap<string,string>>` data (from `parseAsMaps` / `readFileAsMaps`) and process values by column name.

### `csv.count(data: array, column: string) :int64`

Counts non-empty values in a column. Accepts any data type.

- **Returns:** `int64` — number of non-empty values.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("val\n10\n\n30")
var n = csv.count(records, "val")  // 2
csv.release()
```

### `csv.sum(data: array, column: string) :int64`

Sums numeric values in a column.

- **Returns:** `int64` — integer sum.
- **Throws:** `InvalidCastException` if a non-numeric value is encountered.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("val\n10\n20\n30")
var total = csv.sum(records, "val")  // 60
csv.release()
```

### `csv.avg(data: array, column: string) :string`

Averages numeric values in a column. Returns the average as a formatted string (trailing zeros stripped).

- **Returns:** `string` — formatted average, or `0` if column is empty.
- **Throws:** `InvalidCastException` if a non-numeric value is encountered.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("val\n10\n20\n30")
var avg = csv.avg(records, "val")
println(avg)  // "20"
csv.release()
```

### `csv.min(data: array, column: string) :string`

Returns the lexicographically smallest string value in a column. Empty values are skipped.

- **Returns:** `string` — minimum value, or `0` if column is empty.

### `csv.max(data: array, column: string) :string`

Returns the lexicographically largest string value in a column. Empty values are skipped.

- **Returns:** `string` — maximum value, or `0` if column is empty.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("name\nCharlie\nAlice\nBob")
var first = csv.min(records, "name")  // "Alice"
var last = csv.max(records, "name")   // "Charlie"
csv.release()
```

## 7. Transformations

### `csv.select(data: array, columns: array) :array`

Selects a subset of columns from each row, returning a new array of maps containing only the specified keys.

- **Returns:** `array` — new array of `Map<string, string>` objects, or `0` on invalid input.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("a,b,c\n1,2,3\n4,5,6")
var cols = [string:"a", string:"c"]
var subset = csv.select(records, cols)
// subset[0] = {"a": "1", "c": "3"}
csv.release()
```

### `csv.filter(data: array, column: string, op: string, value: string) :array`

Filters rows by comparing column values using the given operator.

- **Operators:** `"=="`, `"!="`, `">"`, `">="`, `"<"`, `"<="`
- **Returns:** `array` — filtered array of maps, or `0` on invalid input.
- **Throws:** `InvalidCastException` if an ordering operator is used and a non-numeric value is found (in either the column or the comparison value). Equality operators never throw.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("name,age\nAlice,30\nBob,25\nCharlie,30")
var adults = csv.filter(records, "age", ">=", "30")
// adults contains Alice and Charlie
var named = csv.filter(records, "name", "==", "Bob")
// named contains Bob
csv.release()
```

### `csv.sort(data: array, column: string, ascending: int64) :array`

Sorts rows by column value using lexicographic string comparison.

- **`ascending`:** pass `1` for ascending order, `0` for descending.
- **Returns:** `array` — sorted array of maps, or `0` on invalid input.

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("name\nCharlie\nAlice\nBob")
var sorted = csv.sort(records, "name", 1)
// sorted order: Alice, Bob, Charlie
csv.release()
```

## 8. Statistics

### `csv.describe(data: array, column: string) :map`

Computes summary statistics for a column. Returns a `Map<string, string>` with the following keys:

| Key     | Description                                |
|---------|--------------------------------------------|
| `count` | Number of non-empty values                 |
| `sum`   | Sum of numeric values                      |
| `avg`   | Average of numeric values (formatted)      |
| `min`   | Lexicographic minimum value                |
| `max`   | Lexicographic maximum value                |

- **Returns:** `map` — statistics map, or a map with `"count": "0"` if column is empty.
- **Throws:** `InvalidCastException` if a non-numeric value is encountered (for `sum`/`avg` computation).

```hoo
var csv = Csv.new()
var records = csv.parseAsMaps("val\n10\n20\n30\n40")
var stats = csv.describe(records, "val")
println(stats["count"])  // "4"
println(stats["sum"])    // "100"
println(stats["avg"])    // "25"
println(stats["min"])    // "10"
println(stats["max"])    // "40"
csv.release()
```

## Type Validation & Exceptions

`sum`, `avg`, and `describe` expect all non-empty column values to be parseable as numbers. Filter ordering operators (`>`, `>=`, `<`, `<=`) also require numeric values. When a non-numeric value is encountered, an `InvalidCastException` is thrown with a descriptive message identifying the offending value and column name.

Functions that work with any data type without validation:
- `count`, `min`, `max` — string-based operations
- `select` — pass-through column copy
- `sort` — lexicographic comparison
- `filter` with `==` / `!=` — equality comparison

## Usage Example

```hoo
var csv = Csv.new()
var input = "id,value\n1,\"hello, world\"\n2,foo\n3,\"quoted \"\"string\"\"\""
var rows = csv.parse(input)

var i: int64 = 1
while i < rows.length() {
    var row = rows[i]
    println("Row ".concat(i.toString()).concat(": ").concat(row[1]))
    i = i + 1
}

// Parse as maps for header-based access
var records = csv.parseAsMaps(input)
var r = records[0]
println(r["value"])  // Output: hello, world

var data = [["x", "y"], ["10", "20"]]
csv.writeFile("result.csv", data)
csv.release()
```
