# Regex — Regular Expressions

The `regex` module provides a `Regex` class and free functions for matching and manipulating regular expressions.

## Regex Class

### Constructor

`new Regex(pattern: string) :Regex`
Compiles a regular expression pattern and returns a Regex instance.

### Methods

`re.match(subject: string) :int64`
Returns 1 if `subject` fully matches the compiled pattern, 0 otherwise.

`re.search(subject: string) :int64`
Returns 1 if any part of `subject` matches the compiled pattern, 0 otherwise.

`re.find(subject: string) :string`
Returns the first substring match of the pattern in `subject`, or null if no match is found.

`re.group(subject: string, group_index: int64) :string`
Returns the captured group at `group_index` from the first search match in `subject`, or null if no match is found.

`re.replace(subject: string, replacement: string) :string`
Replaces all matches in `subject` with `replacement`.

`re.split(subject: string) :array`
Splits `subject` around matches. Returns an array of strings.

`re.release()`
Releases the compiled regex resources.

## Free Functions

`regex_match(pattern: string, subject: string) :int64`
Compiles `pattern` and returns 1 if `subject` fully matches it, 0 otherwise.

`regex_search(pattern: string, subject: string) :int64`
Compiles `pattern` and returns 1 if any part of `subject` matches it, 0 otherwise.

`regex_replace(pattern: string, subject: string, replacement: string) :string`
Compiles `pattern` and replaces all matches in `subject` with `replacement`.

`regex_split(pattern: string, subject: string) :array`
Compiles `pattern` and splits `subject` around matches. Returns an array of strings.

## Example

```hoo
import hoo.regex;

-- Object-oriented style
let re = new Regex("[a-z]+")

let matchResult = re.match("hello")   // 1
let searchResult = re.search("foo123")  // 1

let replaced = re.replace("hello 123", "X")  // "X 123"
let parts = re.split("a1b2c3")           // ["a", "b", "c"]

re.release()

-- Free functions style
let found = regex_search("\\d+", "order 42")  // 1
println(found)
```
