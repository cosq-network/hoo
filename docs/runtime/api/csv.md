# CSV API Reference (`Csv`)

The `Csv` class provides CSV parsing, generation, and file I/O operations. Create an instance with `Csv.new()` or `Csv.newWithOpts()` and release it with `release()`.

## 1. Constructor / Destructor

### `Csv.new() :Csv`

Creates a new Csv instance with default settings (delimiter=`,`, quote=`"`).

### `Csv.newWithOpts(delimiter: int64, quote: int64) :Csv`

Creates a new Csv instance with a custom delimiter and quote character.

### `csv.release()`

Releases the Csv instance and its resources.

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

var data = [["x", "y"], ["10", "20"]]
csv.writeFile("result.csv", data)
csv.release()
```
