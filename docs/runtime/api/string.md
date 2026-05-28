# String API Reference (`hoo.string`)

The `string` module provides robust support for immutable, UTF-8 encoded strings. All string operations are memory-safe and managed via Automatic Reference Counting (ARC).

## 1. Creation

### `string_from_cstr(cstr: ptr) -> string`
Creates a hoo string from a null-terminated host C string.
- **Parameters**: `cstr` - A pointer to a null-terminated UTF-8 sequence.
- **Returns**: A new hoo string.

### `string_new() -> string`
Creates a new, empty hoo string.

### `string_repeat(ch: char, count: int64) -> string`
Creates a string by repeating a single character.
- **Example**: `string_repeat('A', 5)` returns `"AAAAA"`.

## 2. Manipulation

### `string_concat(a: string, b: string) -> string`
Concatenates two strings and returns a new string.
- **Note**: This is NULL-safe. NULL inputs are treated as empty strings.

### `string_substring(str: string, start: int64, length: int64) -> string`
Extracts a substring based on byte indices.
- **Parameters**:
  - `start`: 0-based starting byte index.
  - `length`: Number of bytes to extract.

### `string_to_upper(str: string) -> string`
Returns a new string with all ASCII characters converted to uppercase.

### `string_to_lower(str: string) -> string`
Returns a new string with all ASCII characters converted to lowercase.

### `string_trim(str: string) -> string`
Removes leading and trailing whitespace characters.

### `string_replace(str: string, old: string, new: string) -> string`
Replaces all occurrences of `old` with `new`.

### `string_split(str: string, delim: string) -> array`
Splits the string by the specified delimiter.
- **Returns**: A `HooArray` containing the split substrings.

## 3. Query & Inspection

### `string_length(str: string) -> int64`
Returns the length of the string in **bytes**.

### `string_byte_at(str: string, index: int64) -> int64`
Retrieves the raw byte value (0-255) at the specified index.

### `string_index_of(haystack: string, needle: string) -> int64`
Returns the byte index of the first occurrence of `needle` in `haystack`, or -1 if not found.

### `string_contains(haystack: string, needle: string) -> int64`
Returns 1 if `needle` is present in `haystack`, 0 otherwise.

### `string_starts_with(str: string, prefix: string) -> int64`
Returns 1 if the string starts with the specified prefix.

## 4. Comparison

### `string_compare(a: string, b: string) -> int64`
Performs a lexicographical comparison. Returns -1 if `a < b`, 0 if `a == b`, and 1 if `a > b`.

### `string_equals(a: string, b: string) -> int64`
Returns 1 if the strings are identical, 0 otherwise.

## 5. Formatting & Conversion

### `string_from_int64(val: int64) -> string`
Converts an integer to its string representation.

### `string_from_double(val: double) -> string`
Converts a double-precision float to its string representation.

### `string_to_int64(str: string) -> int64`
Parses the string as an integer.

### `string_to_double(str: string) -> double`
Parses the string as a double-precision float.

---

## Usage Example

```hoo
func :int64 main() {
    var greeting = "Hello, ";
    var name = "Hoo Developer";
    var message = string_concat(greeting, name);
    
    println(message); // Output: Hello, Hoo Developer
    
    var len = string_length(message);
    if (string_contains(message, "Hoo")) {
        println("Found Hoo!");
    }
    
    return 0;
}
```
