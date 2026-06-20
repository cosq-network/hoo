# String API Reference

## Module Name

`String` — core module (no explicit import required beyond `import hoo;`)

## Import Statement

```hoo
import hoo;
```

## Module Description

The `String` class provides immutable, UTF-8 encoded strings with automatic reference counting (ARC). Strings are a core type in Hoo and are available with only `import hoo;`. All string operations return new strings; the original is never modified. String length is measured in bytes.

---

## Class: `String`

### Declaration

```hoo
class String
```

No explicit modifiers; Strings are a built-in core type. The class has no user-accessible base class.

### Public Fields

None — strings are opaque handles.

### Public Instance Functions

---

### `s.concat`

**Description:** Concatenates two strings and returns a new string. Null inputs are treated as empty strings.

**Syntax:**
```hoo
s.concat(other: string) :string
```

**Parameters:**
- `other: string` — The string to append.

**Returns:** `string` — A new concatenated string.

**Errors:** If `other` is null, it is treated as an empty string.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var greeting = "Hello, ";
    var name = "World";
    var msg = greeting.concat(name);
    println(msg); // "Hello, World"
}
```

---

### `s.substring`

**Description:** Extracts a substring based on byte indices.

**Syntax:**
```hoo
s.substring(start: int64, length: int64) :string
```

**Parameters:**
- `start: int64` — The 0-based starting byte index.
- `length: int64` — The number of bytes to extract.

**Returns:** `string` — The extracted substring.

**Errors:** If `start` is out of bounds, an empty string or null is returned.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "Hello, World";
    var sub = s.substring(7, 5);
    println(sub); // "World"
}
```

---

### `s.toUpper`

**Description:** Returns a new string with all ASCII letters converted to uppercase.

**Syntax:**
```hoo
s.toUpper() :string
```

**Parameters:** None.

**Returns:** `string` — The uppercased string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "hello";
    println(s.toUpper()); // "HELLO"
}
```

---

### `s.toLower`

**Description:** Returns a new string with all ASCII letters converted to lowercase.

**Syntax:**
```hoo
s.toLower() :string
```

**Parameters:** None.

**Returns:** `string` — The lowercased string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "HELLO";
    println(s.toLower()); // "hello"
}
```

---

### `s.trim`

**Description:** Removes leading and trailing whitespace (spaces, tabs, newlines, carriage returns).

**Syntax:**
```hoo
s.trim() :string
```

**Parameters:** None.

**Returns:** `string` — The trimmed string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "  hello  ";
    println(s.trim()); // "hello"
}
```

---

### `s.replace`

**Description:** Replaces all occurrences of `old` with `replacement`.

**Syntax:**
```hoo
s.replace(old: string, replacement: string) :string
```

**Parameters:**
- `old: string` — The substring to find.
- `replacement: string` — The replacement string.

**Returns:** `string` — A new string with all occurrences replaced.

**Errors:** If `old` is empty, the original string is returned unchanged.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "one one one";
    var r = s.replace("one", "two");
    println(r); // "two two two"
}
```

---

### `s.split`

**Description:** Splits the string by the specified delimiter. Empty strings between delimiters are not included in the result.

**Syntax:**
```hoo
s.split(delim: string) :array
```

**Parameters:**
- `delim: string` — The delimiter string.

**Returns:** `array` — An array of strings.

**Errors:** If `delim` is empty, an array containing the original string is returned.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "a,b,c";
    var parts = s.split(",");
    println(parts.length()); // 3
}
```

---

### `s.length`

**Description:** Returns the length of the string in bytes.

**Syntax:**
```hoo
s.length() :int64
```

**Parameters:** None.

**Returns:** `int64` — The byte length. Returns 0 if the string is null.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "Hello";
    println(s.length()); // 5
}
```

---

### `s.byteAt`

**Description:** Retrieves the raw byte value (0-255) at the specified index.

**Syntax:**
```hoo
s.byteAt(index: int64) :int64
```

**Parameters:**
- `index: int64` — The 0-based byte index.

**Returns:** `int64` — The byte value (0-255), or -1 if the index is out of bounds.

**Errors:** Returns -1 for out-of-bounds access; no exception is thrown.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "ABC";
    var b = s.byteAt(1);
    println(b); // 66
}
```

---

### `s.indexOf`

**Description:** Returns the byte index of the first occurrence of `needle`.

**Syntax:**
```hoo
s.indexOf(needle: string) :int64
```

**Parameters:**
- `needle: string` — The substring to find.

**Returns:** `int64` — The byte index of the first match, or -1 if not found.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "Hello, World";
    var i = s.indexOf("World");
    println(i); // 7
}
```

---

### `s.lastIndexOf`

**Description:** Returns the byte index of the last occurrence of `needle`.

**Syntax:**
```hoo
s.lastIndexOf(needle: string) :int64
```

**Parameters:**
- `needle: string` — The substring to find.

**Returns:** `int64` — The byte index of the last match, or -1 if not found.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "one one one";
    var i = s.lastIndexOf("one");
    println(i); // 8
}
```

---

### `s.contains`

**Description:** Checks whether the string contains a substring.

**Syntax:**
```hoo
s.contains(needle: string) :int64
```

**Parameters:**
- `needle: string` — The substring to search for.

**Returns:** `int64` — 1 if found, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "Hello, World";
    if (s.contains("World")) {
        println("Found!");
    }
}
```

---

### `s.startsWith`

**Description:** Checks whether the string starts with the specified prefix.

**Syntax:**
```hoo
s.startsWith(prefix: string) :int64
```

**Parameters:**
- `prefix: string` — The prefix to check.

**Returns:** `int64` — 1 if the string starts with the prefix, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "hello.txt";
    if (s.startsWith("hello")) {
        println("Starts with hello");
    }
}
```

---

### `s.endsWith`

**Description:** Checks whether the string ends with the specified suffix.

**Syntax:**
```hoo
s.endsWith(suffix: string) :int64
```

**Parameters:**
- `suffix: string` — The suffix to check.

**Returns:** `int64` — 1 if the string ends with the suffix, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "hello.txt";
    if (s.endsWith(".txt")) {
        println("Is a text file");
    }
}
```

---

### `s.compare`

**Description:** Performs a lexicographic comparison.

**Syntax:**
```hoo
s.compare(other: string) :int64
```

**Parameters:**
- `other: string` — The string to compare against.

**Returns:** `int64` — -1 if `s < other`, 0 if `s == other`, 1 if `s > other`.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var a = "apple";
    var b = "banana";
    var cmp = a.compare(b);
    println(cmp); // -1
}
```

---

### `s.equals`

**Description:** Checks whether two strings are identical.

**Syntax:**
```hoo
s.equals(other: string) :int64
```

**Parameters:**
- `other: string` — The string to compare.

**Returns:** `int64` — 1 if the strings are identical, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var a = "hello";
    var b = "hello";
    if (a.equals(b)) {
        println("Equal");
    }
}
```

---

### `s.equalsIgnoreCase`

**Description:** Checks whether two strings are equal, ignoring ASCII case differences.

**Syntax:**
```hoo
s.equalsIgnoreCase(other: string) :int64
```

**Parameters:**
- `other: string` — The string to compare.

**Returns:** `int64` — 1 if the strings are equal ignoring case, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    if ("Hello".equalsIgnoreCase("HELLO")) {
        println("Case-insensitive match");
    }
}
```

---

### `s.toInt64`

**Description:** Parses the string as an integer. Leading decimal digits are parsed; non-digit characters are ignored.

**Syntax:**
```hoo
s.toInt64() :int64
```

**Parameters:** None.

**Returns:** `int64` — The parsed integer value, or 0 if no digits are found.

**Errors:** Returns 0 on invalid input or null string.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "42";
    var n = s.toInt64();
    println(n); // 42
}
```

---

### `s.toDouble`

**Description:** Parses the string as a double-precision float.

**Syntax:**
```hoo
s.toDouble() :double
```

**Parameters:** None.

**Returns:** `double` — The parsed double value, or 0.0 if parsing fails.

**Errors:** Returns 0.0 on invalid input or null string.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "3.14";
    var d = s.toDouble();
    println(d); // 3.14
}
```

---

### `s.is_empty`

**Description:** Checks whether the string is empty (length == 0).

**Syntax:**
```hoo
s.is_empty() :int64
```

**Parameters:** None.

**Returns:** `int64` — 1 if the string is empty, 0 otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "";
    if (s.is_empty()) {
        println("Empty string");
    }
}
```

---

### `s.toCharacters`

**Description:** Returns an array of `Character` objects representing each Unicode scalar value in the string.

**Syntax:**
```hoo
s.toCharacters() :array
```

**Parameters:** None.

**Returns:** `array` — An array of `Character` objects.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = "Hi";
    var chars = s.toCharacters();
    println(chars.length()); // 2
}
```

---

## Free Functions

### `string_repeat`

**Description:** Creates a string by repeating a single character `count` times.

**Syntax:**
```hoo
string_repeat(ch: char, count: int64) :string
```

**Parameters:**
- `ch: char` — The character to repeat.
- `count: int64` — The number of repetitions.

**Returns:** `string` — A new string containing the repeated character.

**Errors:** If `count` is 0 or negative, an empty string is returned.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var stars = string_repeat('*', 10);
    println(stars); // "**********"
}
```

---

### `string_from_int64`

**Description:** Converts an integer to its string representation.

**Syntax:**
```hoo
string_from_int64(val: int64) :string
```

**Parameters:**
- `val: int64` — The integer to convert.

**Returns:** `string` — The string representation of the integer.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = string_from_int64(42);
    println(s); // "42"
}
```

---

### `string_from_double`

**Description:** Converts a double-precision float to its string representation.

**Syntax:**
```hoo
string_from_double(val: double) :string
```

**Parameters:**
- `val: double` — The floating-point value to convert.

**Returns:** `string` — The string representation of the value.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var s = string_from_double(3.14);
    println(s); // "3.14"
}
```

---

### `string_join`

**Description:** Joins an array of strings into a single string.

**Syntax:**
```hoo
string_join(parts: array) :string
```

**Parameters:**
- `parts: array` — An array of strings to join.

**Returns:** `string` — The concatenated string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var parts = ["a", "b", "c"];
    var result = string_join(parts);
    println(result); // "abc"
}
```

---

## Usage Example

```hoo
import hoo;

func :int64 main() {
    var greeting = "Hello, ";
    var name = "Hoo Developer";
    var message = greeting.concat(name);

    println(message);

    var len = message.length();
    if (message.contains("Hoo")) {
        println("Found Hoo!");
    }

    var upper = message.toUpper();
    println(upper);

    if (message.endsWith("Developer")) {
        println("Ends with Developer");
    }

    return 0;
}
```
