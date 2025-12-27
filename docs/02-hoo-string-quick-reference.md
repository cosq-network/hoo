# HooString - Quick Reference Card

UTF-8 String Library with Automatic Reference Counting (ARC)

## CREATION

```c
HooString str = hoo_string_from_cstr("text");     // From C string
HooString str = hoo_string_new();                 // Empty string
HooString str = hoo_string_from_bytes(ptr, len);  // From bytes
HooString str = hoo_string_repeat('x', 5);        // "xxxxx"
```

## MANIPULATION

```c
hoo_string_concat(a, b)           // "ab"
hoo_string_substring(str, 0, 3)   // First 3 bytes
hoo_string_to_upper(str)          // "HELLO"
hoo_string_to_lower(str)          // "hello"
hoo_string_trim(str)              // Remove whitespace
hoo_string_replace(str, "a", "b") // Replace all "a" with "b"
```

## QUERY

```c
int64_t len      = hoo_string_length(str);       // Byte count
const char* data = hoo_string_data(str);         // Raw UTF-8
int64_t byte     = hoo_string_byte_at(str, idx); // Get byte
int64_t empty    = hoo_string_is_empty(str);     // Is empty?
int64_t pos      = hoo_string_index_of(h, n);    // Find substring
int64_t has      = hoo_string_contains(h, n);    // Contains?
int64_t starts   = hoo_string_starts_with(s, p); // Starts with?
int64_t ends     = hoo_string_ends_with(s, suf); // Ends with?
```

## COMPARISON

```c
int64_t cmp = hoo_string_compare(a, b);          // -1, 0, or 1
int64_t eq  = hoo_string_equals(a, b);           // 1 if equal
int64_t eq  = hoo_string_equals_ignore_case(a, b); // Case-insensitive
```

## REFERENCE COUNTING (ARC)

```c
HooString copy    = hoo_string_retain(str);       // Increment refcount
hoo_string_release(str);                          // Decrement, free if 0
int64_t refcount  = hoo_string_refcount(str);     // Get refcount
```

**NOTE:** All strings start with refcount=1. Call release() when done.

## CONVERSION

```c
// To String
HooString s1 = hoo_string_from_int64(42);        // "42"
HooString s2 = hoo_string_from_double(3.14);     // "3.14"
HooString s3 = hoo_string_from_bool(1);          // "true"

// From String
int64_t i    = hoo_string_to_int64(str);         // Parse int
double d     = hoo_string_to_double(str);        // Parse double
HooString fmt= hoo_string_format(fmt, ...);      // Printf-style
```

## DEBUGGING

```c
hoo_string_print(str);                           // Print to stdout
hoo_string_println(str);                         // Print + newline
HooString info = hoo_string_debug(str);          // Get debug info
```

## COMMON PATTERNS

### String concatenation
```c
HooString result = hoo_string_concat(a, hoo_string_concat(b, c));
```

### Safe assignment with ARC
```c
HooString old = current;
current = new_value;
hoo_string_retain(current);
hoo_string_release(old);
```

### Safe parameter passing
```c
void process(HooString str) {
    // Do something with str
    // Caller is responsible for cleanup
}
```

### Cleanup when done
```c
hoo_string_release(str);  // ALWAYS do this!
```

## RETURN VALUES

Functions returning HooString always return with refcount=1 (if non-NULL)

Functions returning int64_t:
- `-1` : Error or not found
- `0` : False / Default
- `1+` : True / Count / Position

Comparison functions return:
- `-1` : First < Second
- `0` : First == Second
- `1` : First > Second

## MEMORY MANAGEMENT

**RULE 1:** Every hoo_string_*() call that returns HooString means YOU OWN it

**RULE 2:** Call hoo_string_release() when done with a string

**RULE 3:** Call hoo_string_retain() before assigning to another variable

**RULE 4:** NULL strings are valid - operate on them gracefully

### Example
```c
HooString str = hoo_string_from_cstr("hello");  // Refcount=1, you own it
HooString copy = hoo_string_retain(str);        // Refcount=2, you own both
hoo_string_release(str);                        // Refcount=1
hoo_string_release(copy);                       // Refcount=0, freed!
```

## ENCODING

All strings are UTF-8 encoded and null-terminated.

- `hoo_string_length()` returns BYTE count (not character count)
- `hoo_string_data()` returns UTF-8 pointer (safe for C string functions)
- `hoo_string_byte_at()` gets individual bytes
- Multi-byte UTF-8 characters may span multiple bytes

## NULL HANDLING

Most functions handle NULL gracefully:
- `hoo_string_release(NULL)` : No-op (safe)
- `hoo_string_retain(NULL)` : Returns NULL (safe)
- `hoo_string_length(NULL)` : Returns 0 (safe)
- `hoo_string_data(NULL)` : Returns "" (safe)
- `hoo_string_concat(NULL, str)` : Works like empty string
- `hoo_string_equals(a, NULL)` : Works correctly

## INCLUDE IN YOUR CODE

```c
#include "runtime/hoo_string.h"
```

Link with: hoort library (automatically included in hoo compiler)

## PERFORMANCE NOTES

### O(1) operations
- `hoo_string_length()` : Stored in header
- `hoo_string_data()` : Direct pointer access
- `hoo_string_refcount()` : Stored in header
- `hoo_string_retain()` : Atomic increment
- `hoo_string_release()` : Atomic decrement

### O(n) operations
- `hoo_string_concat()` : Allocates and copies both strings
- `hoo_string_substring()` : Allocates and copies substring
- `hoo_string_index_of()` : Searches through string
- Case conversion : Single pass through string

### Tips for performance
- Minimize concatenation (use combine pattern)
- Reuse substrings when possible
- Use retain/release judiciously (compiler will optimize)

## MORE INFORMATION

- See: `runtime/hoo_string.h` - Full API documentation
- See: `docs/04-strings-implementation-guide.md` - Architecture & design decisions
- See: `docs/string-integration-guide.md` - Integration with HoocJIT
- See: `tests/StringBasicsTest.cpp` - Test examples
