# CSV API Developer Reference (`Csv`)

**Import Requirement:**
```hoo
import hoo.csv;
```

The `Csv` class provides CSV parsing, generation, and file I/O operations with
Automatic Reference Counting (ARC). The class supports only one constructor
(`new Csv()`). To create an instance with custom options, use the free function
`csv_from_opts(...)` instead of a second constructor.

## `new Csv`

### Description

Creates a new `Csv` instance with default settings (delimiter `,`, quote `"`).

### Syntax

```hoo
new Csv() :Csv
```

### Parameters

None.

### Return Type

`Csv`
A new CSV instance with default delimiter and quote character.

### Errors

Returns a null handle only if allocation fails.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var input = "name,age\nAlice,30\nBob,25";
    var rows = csv.parse(input);
    csv.release();
    return rows.length();
}
```

## `csv_from_opts`

### Description

Creates a new `Csv` instance with a custom delimiter and quote character. This
is a free function, not a static method on `Csv`. Both parameters are ASCII code
points.

### Syntax

```hoo
csv_from_opts(delimiter: int64, quote: int64) :Csv
```

### Parameters

`delimiter`
ASCII code point of the field delimiter character (e.g. `59` for `;`).

`quote`
ASCII code point of the quoting character (e.g. `39` for `'`).

### Return Type

`Csv`
A new CSV instance configured with the given delimiter and quote character.

### Errors

Returns a null handle only if allocation fails.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = csv_from_opts(59, 39);
    var input = "a;b;c";
    var rows = csv.parse(input);
    csv.release();
    return rows.length();
}
```

## `retain`

### Description

Increments the reference count of a CSV instance. Returns the same instance for
chaining.

### Syntax

```hoo
csv.retain() :Csv
```

### Parameters

None.

### Return Type

`Csv`
The same CSV instance with an incremented reference count.

### Errors

Returns null if `csv` is null.

### Complete Example

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

## `release`

### Description

Decrements the reference count of a CSV instance. The instance is freed when the
count reaches zero. After calling `release`, the handle must not be used again.

### Syntax

```hoo
csv.release()
```

### Parameters

None.

### Return Type

None.

### Errors

Does nothing (no-op) if `csv` is null.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    csv.release();
    return 0;
}
```

## `refcount`

### Description

Returns the current reference count of a CSV instance. Intended for debugging
and testing.

### Syntax

```hoo
csv.refcount() :int64
```

### Parameters

None.

### Return Type

`int64`
The current reference count.

### Errors

Returns `0` if `csv` is null.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    println(csv.refcount()); // 1
    var csv2 = csv.retain();
    println(csv.refcount()); // 2
    csv.release();
    println(csv.refcount()); // 1
    csv.release();
    return 0;
}
```

---

## `parse`

### Description

Parses a CSV string and returns a two-dimensional array of field strings. Each
top-level element is an array representing one row, and each row contains the
field strings for that row.

### Syntax

```hoo
csv.parse(input: string) :array
```

### Parameters

`input`
The raw CSV text to parse.

### Return Type

`array`
A two-dimensional array (`array` of `array` of `string`). Returns `0` on
failure.

### Errors

Returns `0` if `input` is nil or the CSV is empty. Does not throw on malformed
input.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var input = "name,age\nAlice,30\nBob,25";
    var rows = csv.parse(input);
    println(rows.length()); // 3
    csv.release();
    return rows.length();
}
```

## `parseAsMaps`

### Description

Parses a CSV string and returns an array of maps, using the first row as
string-key headers. Each subsequent row is a `Map<string, string>` mapping
header names to field values.

### Syntax

```hoo
csv.parseAsMaps(input: string) :array
```

### Parameters

`input`
The raw CSV text to parse.

### Return Type

`array`
An array of `Map<string, string>` objects. Returns `0` if the input contains
fewer than two rows (header row only or empty).

### Errors

Returns `0` if `input` is nil or the CSV has fewer than two rows.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var input = "name,age\nAlice,30\nBob,25";
    var records = csv.parseAsMaps(input);
    println(records.length()); // 2
    var first = records[0];
    println(first["name"]); // Alice
    println(first["age"]);  // 30
    csv.release();
    return records.length();
}
```

---

## `generate`

### Description

Generates a CSV-formatted string from a two-dimensional array of strings. Fields
that contain the delimiter, the quote character, or a newline are automatically
quoted. Embedded quote characters are doubled.

### Syntax

```hoo
csv.generate(data: array) :string
```

### Parameters

`data`
A two-dimensional array (`array` of `array` of `string`) containing the data to
serialize.

### Return Type

`string`
The CSV-formatted string. Returns an empty string if `data` is nil, empty, or
all rows are empty.

### Errors

No throw path; returns an empty string on invalid input.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var data = [["a", "b"], ["1", "2"]];
    var output = csv.generate(data);
    println(output); // a,b\n1,2\n
    csv.release();
    return output.length();
}
```

---

## `readFile`

### Description

Reads a CSV file from disk and returns the parsed data as a two-dimensional
array of strings.

### Syntax

```hoo
csv.readFile(path: string) :array
```

### Parameters

`path`
The file system path to the CSV file.

### Return Type

`array`
A two-dimensional array of field strings. Returns `0` if the file cannot be
read, is empty, or is not valid UTF-8 text.

### Errors

Returns `0` if the file does not exist, cannot be read, or contains no data.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var rows = csv.readFile("data.csv");
    if rows != 0 {
        println(rows.length());
    }
    csv.release();
    return rows != 0 ? 0 : 1;
}
```

## `readFileAsMaps`

### Description

Reads a CSV file from disk and returns the parsed data as an array of maps,
using the first row as string-key headers. Each subsequent row is a
`Map<string, string>` mapping header names to field values.

### Syntax

```hoo
csv.readFileAsMaps(path: string) :array
```

### Parameters

`path`
The file system path to the CSV file.

### Return Type

`array`
An array of `Map<string, string>` objects. Returns `0` if the file cannot be
read, is empty, or contains fewer than two rows.

### Errors

Returns `0` if the file does not exist, cannot be read, or contains fewer than
two rows.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.readFileAsMaps("data.csv");
    if records != 0 {
        println(records.length());
    }
    csv.release();
    return records != 0 ? 0 : 1;
}
```

## `writeFile`

### Description

Writes a two-dimensional array of strings to a CSV file.

### Syntax

```hoo
csv.writeFile(path: string, data: array) :int64
```

### Parameters

`path`
The file system path where the CSV file will be written.

`data`
A two-dimensional array (`array` of `array` of `string`) containing the data to
write.

### Return Type

`int64`
Returns `0` on success, `1` on failure.

### Errors

Returns `1` if `path` or `data` is nil, or the file cannot be written.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var data = [["name", "age"], ["Alice", "30"], ["Bob", "25"]];
    var ok = csv.writeFile("output.csv", data);
    csv.release();
    return ok;
}
```

---

## `escape`

### Description

Checks whether a character (specified by its ASCII code point) needs escaping in
a CSV field. A character needs escaping if it is the configured delimiter, the
configured quote character, a newline (`\n`), or a carriage return (`\r`).

### Syntax

```hoo
csv.escape(c: int64) :int64
```

### Parameters

`c`
ASCII code point of the character to check.

### Return Type

`int64`
Returns `1` if the character needs escaping, `0` otherwise.

### Errors

No throw path.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var r1 = csv.escape(44);  // comma
    println(r1); // 1
    var r2 = csv.escape(97);  // 'a'
    println(r2); // 0
    csv.release();
    return r1;
}
```

---

## `count`

### Description

Counts non-empty values in a column. Accepts any data type; no numeric
validation is performed.

### Syntax

```hoo
csv.count(data: array, column: string) :int64
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to count.

### Return Type

`int64`
The number of non-empty values in the column.

### Errors

Returns `0` if `data` or `column` is nil.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("val\n10\n\n30");
    var n = csv.count(records, "val");
    println(n); // 2
    csv.release();
    return n;
}
```

## `sum`

### Description

Sums numeric values in a column. All non-empty values must be parseable as
integers.

### Syntax

```hoo
csv.sum(data: array, column: string) :int64
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to sum.

### Return Type

`int64`
The integer sum of all non-empty numeric values in the column.

### Errors

Throws `InvalidCastException` if a non-numeric value is encountered.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("val\n10\n20\n30");
    var total = csv.sum(records, "val");
    println(total); // 60
    csv.release();
    return total;
}
```

## `avg`

### Description

Averages numeric values in a column. Returns the average as a formatted string
with trailing zeros stripped. All non-empty values must be parseable as numbers.

### Syntax

```hoo
csv.avg(data: array, column: string) :string
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to average.

### Return Type

`string`
The formatted average string (e.g. `"20"`, `"15.5"`). Returns `0` if the column
is empty.

### Errors

Throws `InvalidCastException` if a non-numeric value is encountered.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("val\n10\n20\n30");
    var avg = csv.avg(records, "val");
    println(avg); // "20"
    csv.release();
    return avg.length();
}
```

## `min`

### Description

Returns the lexicographically smallest string value in a column. Empty values
are skipped. No numeric validation is performed.

### Syntax

```hoo
csv.min(data: array, column: string) :string
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to search.

### Return Type

`string`
The lexicographically smallest value. Returns `0` if the column is empty.

### Errors

No throw path.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("name\nCharlie\nAlice\nBob");
    var first = csv.min(records, "name");
    println(first); // "Alice"
    csv.release();
    return first.equals("Alice");
}
```

## `max`

### Description

Returns the lexicographically largest string value in a column. Empty values
are skipped. No numeric validation is performed.

### Syntax

```hoo
csv.max(data: array, column: string) :string
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to search.

### Return Type

`string`
The lexicographically largest value. Returns `0` if the column is empty.

### Errors

No throw path.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("name\nCharlie\nAlice\nBob");
    var last = csv.max(records, "name");
    println(last); // "Charlie"
    csv.release();
    return last.equals("Charlie");
}
```

---

## `select`

### Description

Selects a subset of columns from each row, returning a new array of maps
containing only the specified keys. Missing keys in a source row produce empty
string values in the result.

### Syntax

```hoo
csv.select(data: array, columns: array) :array
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`columns`
An array of strings specifying the column names to include.

### Return Type

`array`
A new array of `Map<string, string>` objects containing only the selected
columns. Returns `0` on invalid input.

### Errors

Returns `0` if `data` or `columns` is nil, or if `columns` is empty.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("a,b,c\n1,2,3\n4,5,6");
    var cols = [string:"a", string:"c"];
    var subset = csv.select(records, cols);
    // subset[0] = {"a": "1", "c": "3"}
    println(subset.length()); // 2
    csv.release();
    return subset.length();
}
```

## `filter`

### Description

Filters rows by comparing column values using the given operator. Equality
operators (`==`, `!=`) accept any type. Ordering operators (`>`, `>=`, `<`,
`<=`) require numeric values in both the column and the comparison value, and
throw `InvalidCastException` on non-numeric data.

### Syntax

```hoo
csv.filter(data: array, column: string, op: string, value: string) :array
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to evaluate.

`op`
The comparison operator. Supported values: `"=="`, `"!="`, `">"`, `">="`, `"<"`,
`"<="`.

`value`
The value to compare against.

### Return Type

`array`
A filtered array of maps containing only matching rows. Returns `0` on invalid
input.

### Errors

Throws `InvalidCastException` if an ordering operator is used and a non-numeric
value is found in either the column or the comparison value. Equality operators
never throw.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("name,age\nAlice,30\nBob,25\nCharlie,30");
    var adults = csv.filter(records, "age", ">=", "30");
    // adults contains Alice and Charlie
    println(adults.length()); // 2
    var named = csv.filter(records, "name", "==", "Bob");
    // named contains Bob
    println(named.length()); // 1
    csv.release();
    return adults.length();
}
```

## `sort`

### Description

Sorts rows by column value using lexicographic string comparison. No numeric
validation is performed.

### Syntax

```hoo
csv.sort(data: array, column: string, ascending: int64) :array
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to sort by.

`ascending`
Pass `1` for ascending order, `0` for descending order.

### Return Type

`array`
A sorted array of maps. Returns `0` on invalid input.

### Errors

Returns `0` if `data` or `column` is nil, or if `data` is empty.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("name\nCharlie\nAlice\nBob");
    var sorted = csv.sort(records, "name", 1);
    // sorted order: Alice, Bob, Charlie
    println(sorted[0]["name"]); // Alice
    csv.release();
    return sorted.length();
}
```

---

## `describe`

### Description

Computes summary statistics for a column. Returns a `Map<string, string>` with
the following keys:

| Key     | Description                                |
|---------|--------------------------------------------|
| `count` | Number of non-empty values                 |
| `sum`   | Sum of numeric values                      |
| `avg`   | Average of numeric values (formatted)      |
| `min`   | Lexicographic minimum value                |
| `max`   | Lexicographic maximum value                |

All non-empty column values must be parseable as numbers for `sum` and `avg`
computation.

### Syntax

```hoo
csv.describe(data: array, column: string) :map
```

### Parameters

`data`
An array of maps (from `parseAsMaps` or `readFileAsMaps`).

`column`
The column name to describe.

### Return Type

`map`
A `Map<string, string>` containing the statistics. Returns a map with
`"count": "0"` if the column is empty.

### Errors

Throws `InvalidCastException` if a non-numeric value is encountered (for
`sum`/`avg` computation). Returns `0` if `data` or `column` is nil.

### Complete Example

```hoo
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var records = csv.parseAsMaps("val\n10\n20\n30\n40");
    var stats = csv.describe(records, "val");
    println(stats["count"]); // "4"
    println(stats["sum"]);   // "100"
    println(stats["avg"]);   // "25"
    println(stats["min"]);   // "10"
    println(stats["max"]);   // "40"
    csv.release();
    return stats.count();
}
```

---

## Type Validation & Exceptions

`sum`, `avg`, and `describe` expect all non-empty column values to be parseable
as numbers. Filter ordering operators (`>`, `>=`, `<`, `<=`) also require
numeric values. When a non-numeric value is encountered, an
`InvalidCastException` is thrown with a descriptive message identifying the
offending value and column name.

Functions that work with any data type without validation:
- `count`, `min`, `max` — string-based operations
- `select` — pass-through column copy
- `sort` — lexicographic comparison
- `filter` with `==` / `!=` — equality comparison
