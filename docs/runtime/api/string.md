# String API Reference (`String`)

The `String` type provides robust support for immutable, UTF-8 encoded strings. All string operations are memory-safe and managed via Automatic Reference Counting (ARC).

## 1. Creation

### `String.from_cstr(cstr: ptr) -> string`
Creates a hoo string from a null-terminated host C string.
- **Parameters**: `cstr` - A pointer to a null-terminated UTF-8 sequence.
- **Returns**: A new hoo string.

### `String.new() -> string`
Creates a new, empty hoo string. You can also use `""` directly.

### `String.repeat(ch: char, count: int64) -> string`
Creates a string by repeating a single character.
- **Example**: `String.repeat('A', 5)` returns `"AAAAA"`.

## 2. Manipulation

### `s.concat(other: string) -> string`
Concatenates two strings and returns a new string.
- **Note**: This is NULL-safe. NULL inputs are treated as empty strings.

### `s.substring(start: int64, length: int64) -> string`
Extracts a substring based on byte indices.
- **Parameters**:
  - `start`: 0-based starting byte index.
  - `length`: Number of bytes to extract.

### `s.to_upper() -> string`
Returns a new string with all ASCII characters converted to uppercase.

### `s.to_lower() -> string`
Returns a new string with all ASCII characters converted to lowercase.

### `s.trim() -> string`
Removes leading and trailing whitespace characters.

### `s.replace(old: string, new: string) -> string`
Replaces all occurrences of `old` with `new`.

### `s.split(delim: string) -> array`
Splits the string by the specified delimiter.
- **Returns**: A `HooArray` containing the split substrings.

## 3. Query & Inspection

### `s.length() -> int64`
Returns the length of the string in **bytes**.

### `s.byte_at(index: int64) -> int64`
Retrieves the raw byte value (0-255) at the specified index.

### `s.index_of(needle: string) -> int64`
Returns the byte index of the first occurrence of `needle`, or -1 if not found.

### `s.contains(needle: string) -> int64`
Returns 1 if `needle` is present, 0 otherwise.

### `s.starts_with(prefix: string) -> int64`
Returns 1 if the string starts with the specified prefix.

## 4. Comparison

### `s.compare(other: string) -> int64`
Performs a lexicographical comparison. Returns -1 if `s < other`, 0 if `s == other`, and 1 if `s > other`.

### `s.equals(other: string) -> int64`
Returns 1 if the strings are identical, 0 otherwise.

## 5. Formatting & Conversion

### `String.from_int64(val: int64) -> string`
Converts an integer to its string representation.

### `String.from_double(val: double) -> string`
Converts a double-precision float to its string representation.

### `s.to_int64() -> int64`
Parses the string as an integer.

### `s.to_double() -> double`
Parses the string as a double-precision float.

---

## Usage Example

```hoo
func :int64 main() {
    var greeting = "Hello, ";
    var name = "Hoo Developer";
    var message = greeting.concat(name);
    
    println(message); // Output: Hello, Hoo Developer
    
    var len = message.length();
    if (message.contains("Hoo")) {
        println("Found Hoo!");
    }
    
    return 0;
}
```
