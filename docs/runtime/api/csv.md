# CSV API Reference (`Csv`)

The `Csv` singleton class provides CSV parsing, generation, and file I/O operations.

## 1. Parsing

### `Csv.parse(input: string) :array`

Parses a CSV string and returns an array of rows, where each row is an array of field strings.

- **Parameters:**
  - `input: string` — the CSV content to parse.
- **Returns:** `array` — a two-dimensional array (`array` of `array` of `string`).

```hoo
func :void example() {
    var csv = "name,age\nAlice,30\nBob,25";
    var rows = Csv.parse(csv);
    println(rows.length().toString()); // Output: 3 (header + 2 data rows)
}
```

---

### `Csv.parseWithOpts(input: string, delimiter: char, quote: char, escape: char) :array`

Parses a CSV string with custom formatting options.

- **Parameters:**
  - `input: string` — the CSV content to parse.
  - `delimiter: char` — the field delimiter character.
  - `quote: char` — the quote character.
  - `escape: char` — the escape character.
- **Returns:** `array` — a two-dimensional array of field strings.

```hoo
func :void example() {
    var csv = "name|age\nAlice|30\nBob|25";
    var rows = Csv.parseWithOpts(csv, '|', '"', '\\');
    println(rows.length().toString()); // Output: 3
}
```

## 2. File I/O

### `Csv.readFile(path: string) :int64`

Reads a CSV file from disk. Returns 1 on success, 0 on failure.

- **Parameters:**
  - `path: string` — the file path to read.
- **Returns:** `int64` — 1 on success, 0 on failure.

```hoo
if Csv.readFile("data.csv") == 1 {
    println("File read successfully")
}
```

---

### `Csv.writeFile(path: string, data: array) :void`

Writes a two-dimensional array of strings to a CSV file.

- **Parameters:**
  - `path: string` — the file path to write.
  - `data: array` — the data to write (array of array of string).
- **Returns:** `void`

```hoo
func :void example() {
    var data = [["name", "age"], ["Alice", "30"], ["Bob", "25"]];
    Csv.writeFile("output.csv", data);
}
```

## 3. Generation

### `Csv.generate(data: array) :string`

Generates a CSV-formatted string from a two-dimensional array.

- **Parameters:**
  - `data: array` — the data to format (array of array of string).
- **Returns:** `string` — the CSV-formatted string.

```hoo
func :void example() {
    var data = [["a", "b"], ["1", "2"]];
    var csv = Csv.generate(data);
    println(csv);
    // Output:
    // a,b
    // 1,2
}
```

## 4. Utilities

### `Csv.escape(c: int64) :int64`

Checks if a character needs escaping in a CSV field (comma, double-quote, newline).

- **Parameters:**
  - `c: int64` — the ASCII code point to check.
- **Returns:** `int64` — 1 if the character needs escaping, 0 otherwise.

```hoo
var needsEscape = Csv.escape(44)  // comma, returns 1
```

## Usage Example

```hoo
func :int64 main() {
    var csv = "id,value\n1,\"hello, world\"\n2,foo\n3,\"quoted \"\"string\"\"\"";

    var rows = Csv.parse(csv);
    var i: int64 = 1;
    while (i < rows.length()) {
        var row = rows[i];
        println("Row ".concat(i.toString()).concat(": ").concat(row[1]));
        i = i + 1;
    }

    var output = Csv.generate([["x", "y"], ["10", "20"]]);
    Csv.writeFile("result.csv", [["x", "y"], ["10", "20"]]);

    return 0;
}
```
