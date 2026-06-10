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
- `String.fromCstr(const char*)`: Creates a string from a null-terminated host string.
- `String.fromBytes(const char*, int64_t)`: Creates a string from a specified byte buffer.
- `str.repeat(count)`: Creates a string by repeating the character in `str` (e.g. `"*".repeat(5)`).
- `String.new()`: Returns an empty string.

## 3. Manipulation
Since strings are immutable, all manipulation functions allocate and return a *new* `String` handle with `refcount=1`.
- `a.concat(b)`: NULL-safe — NULL inputs are treated as empty strings.
- `s.substring(start, length)`
- `s.toUpper()` / `s.toLower()` (ASCII only)
- `s.trim()`: Removes leading/trailing whitespace.
- `s.replace(old, new)`
- `s.split(delim)`: Returns an `Array` of `String`s.
- `parts.join()`: Joins an `Array` of strings.
- `s.toCharacters()`: Returns an `Array` of `Character` objects.

## 4. Query & Inspection
- `s.length()`: Returns the length in **bytes**.
- `s.byteAt(index)`: Retrieves the raw byte (0-255) at the index.
- `s.indexOf(needle)`
- `s.contains(substring)`
- `s.startsWith(prefix)` / `s.endsWith(suffix)`

## 5. Character Support (`Character`)
The `Character` type represents a single Unicode scalar value. Character operations are accessed via class-based method-call syntax (`Character.fromCodepoint(cp)`, `Character.length(ch)`) resolved by `classToPrefix()` mapping `Character` → `character_` in the codegen.

```cpp
struct CharacterImpl {
    int64_t length;     // UTF-8 length (1-4 bytes)
    char data[5];       // UTF-8 bytes, null-terminated
};
```

### 5.1 Factory Methods (Static)
- `Character.fromCodepoint(cp)`: Creates a character from a Unicode codepoint (int64).
- `Character.fromUtf8(data, len)`: Creates a character from a raw UTF-8 byte sequence.

### 5.2 Query Methods (Static)
- `Character.codepoint(ch)`: Retrieves the Unicode codepoint of character handle `ch`.
- `Character.length(ch)`: Returns the byte length (1-4) of character handle `ch`.

### 5.3 JIT Symbol Convention
All Character functions follow the runtime module convention with mangled names in the form `_F_M_hoo_E_character_<method>_v_p*`, registered in `buildRuntimeSymbols()`.

## 6. Comparison
- `a.compare(b)`: Lexicographic C-style comparison (-1, 0, 1). NULL-safe — NULL string sorts before non-NULL.
- `a.equals(b)`
- `a.equalsIgnoreCase(b)`

## 7. Formatting & Conversion
- `String.fromInt64(n)` / `String.fromDouble(d)`
- `String.fromAny(val, type_id)`: Generic intrinsic for converting any primitive or object to a string.
- `String.fromObject(obj)`: Converts a managed object to a string.
- `s.toInt64()` / `s.toDouble()`
- `String.format(fmt, ...)`: `printf`-style formatting into a managed string.
