# Regex — Regular Expressions

The `Regex` class provides static methods for compiling and matching regular expressions.

## Methods

`Regex.compile(pattern: string) :ptr`
Compiles a regular expression pattern and returns a compiled regex handle.

`Regex.match(re: ptr, subject: string) :int64`
Returns 1 if `subject` fully matches the compiled pattern, 0 otherwise.

`Regex.search(re: ptr, subject: string) :int64`
Returns 1 if any part of `subject` matches the compiled pattern, 0 otherwise.

`Regex.replace(re: ptr, subject: string, replacement: string) :string`
Replaces all matches in `subject` with `replacement`.

`Regex.split(re: ptr, subject: string) :array`
Splits `subject` around matches. Returns an array of strings.

`Regex.release(re: ptr)`
Releases the compiled regex handle.

## Convenience Static Methods

`Regex.match(pattern: string, subject: string) :int64`
Compiles `pattern` and checks if `subject` fully matches it. Returns 1 on match, 0 otherwise.

`Regex.find(pattern: string, subject: string) :int64`
Compiles `pattern` and checks if any part of `subject` matches. Returns 1 if a match is found, 0 otherwise.

`Regex.replace(pattern: string, subject: string, replacement: string) :string`
Compiles `pattern` and replaces all matches in `subject` with `replacement`.

`Regex.split(pattern: string, subject: string) :array`
Compiles `pattern` and splits `subject` around matches. Returns an array of strings.

## Example

```hoo
let re = Regex.compile("[a-z]+")

let matchResult = Regex.match(re, "hello")   // 1
let searchResult = Regex.search(re, "foo123")  // 1

let replaced = Regex.replace(re, "hello 123", "X")  // "X 123"
let parts = Regex.split(re, "a1b2c3")           // ["a", "b", "c"]

Regex.release(re)

-- Convenience one-liner
let found = Regex.find("\\d+", "order 42")  // 1
println(found)
```
