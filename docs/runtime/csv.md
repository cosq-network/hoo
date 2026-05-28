# CSV (`hoo.csv`)

The `hoo.csv` module provides parse and generate comma-separated values with quoting, escape handling, custom delimiters, and file I/O.

## 1. Parsing

- `hoo_csv_parse(csv, &out_rows, &out_cols)` — Parse CSV string into 2D array `[row][col]` of strings. Free with `hoo_csv_free_table`.
- `hoo_csv_parse_with_opts(csv, delimiter, quote_char, &out_rows, &out_cols)` — Parse with custom delimiter and quote character.

## 2. Generation

- `hoo_csv_generate(headers, data, rows, cols)` — Generate CSV string from headers and 2D data array. Free with `hoo_csv_free_string`.
- `hoo_csv_generate_with_opts(headers, data, rows, cols, delimiter, quote_char)` — Generate with custom options.

## 3. File I/O

- `hoo_csv_read_file(path, &out_rows, &out_cols)` — Read CSV file into 2D array.
- `hoo_csv_write_file(path, headers, data, rows, cols)` — Write headers and data to CSV file. Returns 0 on success.

## 4. Utilities

- `hoo_csv_escape(c)` — Check if character needs escaping in CSV output.

## Memory Management

- `hoo_csv_free_table(table, rows, cols)` — Free 2D table.
- `hoo_csv_free_string(str)` — Free allocated string.
