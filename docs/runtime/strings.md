# Strings (`String`)

HVM strings are immutable, UTF-8 encoded, and automatically managed via ARC.

## 1. Internal Implementation
The string is backed by `StringImpl`, which is allocated contiguously with the ARC header.

```cpp
struct StringImpl {
    int64_t length;     // Number of bytes (not characters)
    int64_t capacity;   // Allocated capacity (usually length + 1)
    char data[1];       // Variable-length payload, always null-terminated
};
```
Because it is null-terminated, the internal `data` pointer can be safely passed to C APIs (via `s.data()`).

## 2. Creation
- `String.from_cstr(const char*)`: Creates a string from a null-terminated host string.
- `String.from_bytes(const char*, int64_t)`: Creates a string from a specified byte buffer.
- `str.repeat(count)`: Creates a string by repeating the character in `str` (e.g. `"*".repeat(5)`).
- `String.new()`: Returns an empty string.

## 3. Manipulation
Since strings are immutable, all manipulation functions allocate and return a *new* `String` handle with `refcount=1`.
- `a.concat(b)`: NULL-safe — NULL inputs are treated as empty strings.
- `s.substring(start, length)`
- `s.to_upper()` / `s.to_lower()` (ASCII only)
- `s.trim()`: Removes leading/trailing whitespace.
- `s.replace(old, new)`
- `s.split(delim)`: Returns an `Array` of `String`s.
- `parts.join()`: Joins an `Array` of strings.
- `s.to_characters()`: Returns an `Array` of `Character` objects.

## 4. Query & Inspection
- `s.length()`: Returns the length in **bytes**.
- `s.byte_at(index)`: Retrieves the raw byte (0-255) at the index.
- `s.index_of(needle)`
- `s.contains(substring)`
- `s.starts_with(prefix)` / `s.ends_with(suffix)`

## 5. Character Support (`HooCharacter`)
The `Character` type represents a single Unicode scalar value.

```cpp
struct CharacterImpl {
    int64_t length;     // UTF-8 length (1-4 bytes)
    char data[5];       // UTF-8 bytes, null-terminated
};
```

- `Character.from_codepoint(int64_t)`: Creates a character from a Unicode codepoint.
- `Character.from_utf8(const char*, int64_t)`: Creates a character from a raw UTF-8 sequence.
- `ch.codepoint()`: Retrieves the Unicode codepoint.
- `ch.length()`: Returns the byte length (1-4).

## 6. Comparison
- `a.compare(b)`: Lexicographic C-style comparison (-1, 0, 1). NULL-safe — NULL string sorts before non-NULL.
- `a.equals(b)`
- `a.equals_ignore_case(b)`

## 7. Formatting & Conversion
- `String.from_int64(n)` / `String.from_double(d)`
- `String.from_any(val, type_id)`: Generic intrinsic for converting any primitive or object to a string.
- `String.from_object(obj)`: Converts a managed object to a string.
- `s.to_int64()` / `s.to_double()`
- `String.format(fmt, ...)`: `printf`-style formatting into a managed string.
