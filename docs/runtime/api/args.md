# Args — Command-Line Argument Parsing

The `Args` class provides static methods for parsing and inspecting command-line arguments.

## Methods

`Args.parse() :ptr`
Parses the program's command-line arguments and returns a result handle.

`Args.get(result: ptr, key: string) :string`
Returns the value of a named argument (e.g. `--output=file.txt` returns `"file.txt"`).

`Args.has(result: ptr, key: string) :int64`
Returns 1 if the named argument exists, 0 otherwise.

`Args.count(result: ptr) :int64`
Returns the number of positional arguments.

`Args.positional(result: ptr, index: int64) :string`
Returns the positional argument at the given 0-based index.

`Args.free(result: ptr)`
Frees the argument result handle.

## Example

```hoo
let parsed = Args.parse()

if Args.has(parsed, "--output") == 1 {
    let out = Args.get(parsed, "--output")
    println("Output: " + out)
}

for i in 0..Args.count(parsed) {
    println("Arg " + i + ": " + Args.positional(parsed, i))
}

Args.free(parsed)
```
