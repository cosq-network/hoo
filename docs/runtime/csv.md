# CSV (`hoo.csv`)

The `hoo.csv` module provides CSV parsing, generation, and file I/O operations with Automatic Reference Counting (ARC). Create an instance with `Csv.new()` and release it with `release()`.

## 1. Constructor / Destructor

- `Csv.new()` — Create with default options (`,`, `"`).
- `Csv.newWithOpts(delimiter, quote)` — Create with custom delimiter and quote.
- `csv.retain()` — Increment reference count.
- `csv.release()` — Decrement reference count; frees when zero.
- `csv.refcount()` — Return current reference count (for debugging).

## 2. Parsing

- `csv.parse(input)` — Parse CSV string into 2D array of strings.
- `csv.parseAsMaps(input)` — Parse CSV string into array of maps, using the first row as string-key headers. Returns `HooArray<HooMap<string,string>>`.

## 3. Generation

- `csv.generate(data)` — Generate CSV string from 2D array.

## 4. File I/O

- `csv.readFile(path)` — Read CSV file into 2D array.
- `csv.readFileAsMaps(path)` — Read CSV file into array of maps (first row = headers).
- `csv.writeFile(path, data)` — Write 2D array to CSV file. Returns 0 on success.

## 5. Utilities

- `csv.escape(c)` — Check if character needs escaping in CSV output.

## 6. Aggregation (DataFrame-like)

Methods that process `parseAsMaps` / `readFileAsMaps` output. All operate on `HooArray<HooMap<string,string>>` data by column name.

- `csv.count(data, column)` — Count non-empty values in a column. Works on any data type.
- `csv.sum(data, column)` — Sum numeric values; throws `InvalidCastException` on non-numeric values.
- `csv.avg(data, column)` — Average numeric values; throws `InvalidCastException` on non-numeric values.
- `csv.min(data, column)` — Minimum value (lexicographic string comparison).
- `csv.max(data, column)` — Maximum value (lexicographic string comparison).

## 7. Transformations

- `csv.select(data, columns)` — Select a subset of columns, returning new array of maps with only those keys.
- `csv.filter(data, column, op, value)` — Filter rows by comparing column values. Equality operators (`==`, `!=`) accept any type. Ordering operators (`>`, `>=`, `<`, `<=`) throw `InvalidCastException` on non-numeric values.
- `csv.sort(data, column, ascending)` — Sort rows by column value (lexicographic string comparison). Pass `1` for ascending, `0` for descending.

## 8. Statistics

- `csv.describe(data, column)` — Compute summary statistics. Returns `Map<string,string>` with keys `count`, `sum`, `avg`, `min`, `max`. Throws `InvalidCastException` on non-numeric values for `sum`/`avg` computation.

## Exception Handling

Aggregation (`sum`, `avg`) and statistics (`describe`) functions and filter ordering operators validate that column values are parseable as numbers. The exception thrown is `InvalidCastException` with a message identifying the non-numeric value and column name. Equality-based filter and lexicographic operations (`min`, `max`, `count`, `select`, `sort`) do not perform type validation.

## Usage from Hoo Source

```hoo
func :int64 demo() {
    var csv = Csv.new()
    var data = [["name", "age"], ["Alice", "30"]]
    var ok = csv.writeFile("/path/to/data.csv", data)
    var rows = csv.readFile("/path/to/data.csv")
    var records = csv.parseAsMaps("name,age\nAlice,30\nBob,25")
    var total = csv.sum(records, "age")       // DataFrame-like
    var filtered = csv.filter(records, "age", ">", "20")
    var stats = csv.describe(records, "age")
    var needs_escape = csv.escape(44)
    csv.release()
    return ok
}
```

## Memory Management

All CSV operations return Hoo strings, arrays, and maps directly with ARC. Instances are reference-counted — retain when sharing, release when done. No manual `free()` calls are needed at the Hoo level.
