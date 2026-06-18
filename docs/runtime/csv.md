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

## Usage from Hoo Source

```hoo
func :int64 demo() {
    var csv = Csv.new()
    var data = [["name", "age"], ["Alice", "30"]]
    var ok = csv.writeFile("/path/to/data.csv", data)
    var rows = csv.readFile("/path/to/data.csv")
    var records = csv.parseAsMaps("name,age\nAlice,30\nBob,25")
    var needs_escape = csv.escape(44)
    csv.release()
    return ok
}
```

## Memory Management

All CSV operations return Hoo strings, arrays, and maps directly with ARC. Instances are reference-counted — retain when sharing, release when done. No manual `free()` calls are needed at the Hoo level.
