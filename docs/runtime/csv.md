# CSV (`hoo.csv`)

The `hoo.csv` module provides parse and generate comma-separated values with quoting, escape handling, custom delimiters, and file I/O.

## 1. Parsing

- `Csv.parse(csv)` — Parse CSV string into 2D array `[row][col]` of strings. Free with `Csv.free_table`.
- `Csv.parse(csv, delimiter, quote_char)` — Parse with custom delimiter and quote character.

## 2. Generation

- `Csv.generate(headers, data, rows, cols)` — Generate CSV string from headers and 2D data array. Free with `Csv.free_string`.
- `Csv.generate(headers, data, rows, cols, delimiter, quote_char)` — Generate with custom options.

## 3. File I/O

- `Csv.read_file(path)` — Read CSV file into 2D array.
- `Csv.write_file(path, headers, data, rows, cols)` — Write headers and data to CSV file. Returns 0 on success.

## 4. Utilities

- `Csv.escape(c)` — Check if character needs escaping in CSV output.

## Usage from Hoo Source

All `Csv.*` functions are available on the `Csv` class:

```hoo
func :int64 demo() {
    var ok = Csv.read_file("/path/to/data.csv");  // 0 = error, 1 = success
    var needs_escape = Csv.escape(44);              // 1 if comma needs quoting
    return ok;
}
```

## Memory Management

- `Csv.free_table(table, rows, cols)` — Free 2D table.
- `Csv.free_string(str)` — Free allocated string.
