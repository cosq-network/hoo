# Csv API Reference

## Module

`hoo.csv`

## Import

```hoo
import hoo.csv;
```

## Overview

`Csv` is an ARC-managed CSV parser and generator. A default instance uses a
comma delimiter and double quotes. Instances created with `csv_from_opts` use
the supplied single-byte delimiter and quote characters for parsing,
generation, and escaping.

CSV rows are returned as `array` values containing `string` fields. Map-based
methods treat the first row as the header and return arrays of `Map` values
whose keys and values are strings.

Malformed input and invalid handles generally return `0`/`null` rather than
throwing. Numeric aggregation and ordering operations throw an invalid-cast
exception when a non-empty value is not numeric.

## Class: Csv

### Constructor

```hoo
new Csv() :Csv
```

Creates a CSV instance configured with `,` as the delimiter and `"` as the
quote character. The initial reference count is one.

### Parsing and Generation

#### `parse`

```hoo
csv.parse(text: string) :array
```

Parses CSV text into an `array` of rows, where each row is an `array` of
strings. Quoted fields support embedded delimiters, doubled quote characters,
and newlines. Empty trailing fields are preserved. Unquoted surrounding spaces
and tabs are trimmed.

Returns `null` for a null or empty input, unterminated quoted fields, or text
following a closing quote that is not whitespace, a delimiter, or a row
separator.

#### `generate`

```hoo
csv.generate(data: array) :string
```

Serializes an array of string rows. Fields containing the configured delimiter,
quote character, or a newline are quoted; quote characters inside quoted fields
are doubled. Missing fields are emitted as empty fields and rows are separated
by newlines.

Returns an empty string for null, empty, or all-empty input, and `null` for
allocation or invalid-input failures.

#### `readFile`

```hoo
csv.readFile(path: string) :array
```

Reads a file and parses it using the instance delimiter and quote character.
Returns `null` when the path cannot be read or the contents are malformed.

#### `writeFile`

```hoo
csv.writeFile(path: string, data: array) :int64
```

Generates CSV and writes it to `path` using the instance options. Returns `0`
on success and `1` on invalid input or an I/O failure.

#### `escape`

```hoo
csv.escape(character: int64) :int64
```

Returns `1` when the character must be quoted in a generated field because it
is the configured delimiter, the configured quote character, or a newline.
Otherwise returns `0`.

### Map-Based Rows

#### `parseAsMaps`

```hoo
csv.parseAsMaps(text: string) :array
```

Parses CSV text using the first row as column names and returns one `Map` per
remaining row. Map keys and values are strings. Returns `null` for malformed or
empty input.

#### `readFileAsMaps`

```hoo
csv.readFileAsMaps(path: string) :array
```

Reads a CSV file and returns its data as an array of header-keyed maps.

### Aggregations

All aggregation methods operate on the array returned by `parseAsMaps` or
`readFileAsMaps`. Missing columns and empty values are ignored where possible.

| Method | Signature | Result |
|---|---|---|
| `count` | `csv.count(data: array, column: string) :int64` | Number of non-empty values. |
| `sum` | `csv.sum(data: array, column: string) :int64` | Exact signed `int64` sum of non-empty integer values. |
| `avg` | `csv.avg(data: array, column: string) :string` | Numeric average as a string, or `null` when there are no values. |
| `min` | `csv.min(data: array, column: string) :string` | Lexicographic minimum, or `null` when there are no values. |
| `max` | `csv.max(data: array, column: string) :string` | Lexicographic maximum, or `null` when there are no values. |

`sum` rejects values that are not valid signed integers. `avg` and the
numeric-ordering operations reject non-finite or non-numeric values.

### Transformations

#### `select`

```hoo
csv.select(data: array, columns: array) :array
```

Returns new maps containing only the requested string columns. Missing values
are represented as empty strings.

#### `filter`

```hoo
csv.filter(data: array, column: string, operator: string, value: string) :array
```

Returns rows matching `==`, `!=`, `>`, `>=`, `<`, or `<=`. Equality operators
compare strings. Ordering operators compare numeric values and validate the
comparison value and non-empty column values as finite numbers.

#### `sort`

```hoo
csv.sort(data: array, column: string, ascending: int64) :array
```

Returns rows sorted by the selected column. A non-zero `ascending` value sorts
ascending; zero sorts descending. Values are compared as strings.

### Statistics

#### `describe`

```hoo
csv.describe(data: array, column: string) :Map
```

Returns a string-valued map containing `count`, `sum`, `avg`, `min`, and `max`
for non-empty numeric values in the selected column. An empty input returns a
map containing `count` set to `"0"`.

## Reference Counting

#### `retain`

```hoo
csv.retain() :Csv
```

Increments the reference count and returns the same instance. Returns `null`
when called on a null handle.

#### `release`

```hoo
csv.release() :void
```

Decrements the reference count and frees the instance when it reaches zero.
Do not use the handle after its final release. Releasing a null handle is a
no-op.

#### `refcount`

```hoo
csv.refcount() :int64
```

Returns the current reference count, or `0` for a null handle.

## Free Functions

### `csv_from_opts`

```hoo
csv_from_opts(delimiter: int64, quote: int64) :Csv
```

Creates a CSV instance with custom single-byte delimiter and quote characters.
Both values must be between `1` and `255`, and they must differ. Returns
`null` for invalid options.

## Complete Example

```hoo
import hoo;
import hoo.csv;

func :int64 main() {
    var csv = new Csv();
    var rows = csv.parse("name,score\nAlice,10\nBob,20");
    println(rows.length()); // 3

    var records = csv.parseAsMaps("name,score\nAlice,10\nBob,20");
    var total: int64 = csv.sum(records, "score");
    println(string_from_int64(total)); // 30

    var output = csv.generate([["name", "score"], ["Cara", "30"]]);
    println(output);

    csv.release();
    return 0;
}
```
