# Strings (`HooString`)

HVM strings are immutable, UTF-8 encoded, and automatically managed via ARC.

## 1. Internal Implementation
The string is backed by `HooStringImpl`, which is allocated contiguously with the ARC header.

```cpp
struct HooStringImpl {
    int64_t length;     // Number of bytes (not characters)
    int64_t capacity;   // Allocated capacity (usually length + 1)
    char data[1];       // Variable-length payload, always null-terminated
};
```
Because it is null-terminated, the internal `data` pointer can be safely passed to C APIs (via `hoo_string_data`).

## 2. Creation
- `hoo_string_from_cstr(const char*)`: Creates a string from a null-terminated host string.
- `hoo_string_from_bytes(const char*, int64_t)`: Creates a string from a specified byte buffer.
- `hoo_string_repeat(char, int64_t)`: Creates a string by repeating a character.
- `hoo_string_new()`: Returns an empty string.

## 3. Manipulation
Since strings are immutable, all manipulation functions allocate and return a *new* `HooString` handle with `refcount=1`.
- `hoo_string_concat(a, b)`
- `hoo_string_substring(str, start, length)`
- `hoo_string_to_upper(str)` / `hoo_string_to_lower(str)` (ASCII only)
- `hoo_string_trim(str)`: Removes leading/trailing whitespace.
- `hoo_string_replace(str, old, new)`
- `hoo_string_split(str, delim)`: Returns a `HooArray` of `HooString`s.

## 4. Query & Inspection
- `hoo_string_length(str)`: Returns the length in **bytes**.
- `hoo_string_byte_at(str, index)`: Retrieves the raw byte (0-255) at the index.
- `hoo_string_index_of(haystack, needle)`
- `hoo_string_contains(haystack, needle)`
- `hoo_string_starts_with(str, prefix)` / `hoo_string_ends_with(str, suffix)`

## 5. Comparison
- `hoo_string_compare(a, b)`: Lexicographic C-style comparison (-1, 0, 1).
- `hoo_string_equals(a, b)`
- `hoo_string_equals_ignore_case(a, b)`

## 6. Formatting & Conversion
- `hoo_string_from_int64(int64_t)` / `hoo_string_from_double(double)`
- `hoo_string_to_int64(str)` / `hoo_string_to_double(str)`
- `hoo_string_format(const char* fmt, ...)`: `printf`-style formatting into a managed string.
