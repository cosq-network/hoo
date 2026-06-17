# CSV (`hoo.csv`)

The `hoo.csv` module provides CSV parsing, generation, and file I/O operations. Create an instance with `Csv.new()` and release it with `release()`.

## 1. Constructor / Destructor

- `Csv.new()` — Create with default options (`,`, `"`).
- `Csv.newWithOpts(delimiter, quote)` — Create with custom delimiter and quote.
- `csv.release()` — Release the instance.

## 2. Parsing

- `csv.parse(input)` — Parse CSV string into 2D array of strings.

## 3. Generation

- `csv.generate(data)` — Generate CSV string from 2D array.

## 4. File I/O

- `csv.readFile(path)` — Read CSV file into 2D array.
- `csv.writeFile(path, data)` — Write 2D array to CSV file. Returns 0 on success.

## 5. Utilities

- `csv.escape(c)` — Check if character needs escaping in CSV output.

## Usage from Hoo Source

```hoo
func :int64 demo() {
    var csv = Csv.new()
    var data = [["name", "age"], ["Alice", "30"]]
    var ok = csv.writeFile("/path/to/data.csv", data)
    var rows = csv.readFile("/path/to/data.csv")
    var needs_escape = csv.escape(44)
    csv.release()
    return ok
}
```

## Memory Management

All CSV operations return Hoo strings and arrays directly. No manual memory management is needed at the Hoo level.
