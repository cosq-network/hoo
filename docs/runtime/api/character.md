# Character API Reference

## Module Name

Part of the `hoo.character` module.

## Import Statement

```hoo
import hoo.character;
```

## Module Description

The `Character` class provides an object-oriented representation of a single Unicode scalar value. Each `Character` instance wraps a Unicode codepoint and provides access to its data and reference-counted lifetime management. The module also provides free functions for character classification and conversion that operate on the Hoo `char` type.

---

## Class: `Character`

### Declaration

```hoo
class Character
```

### Public Fields

None.

### Public Instance Functions

#### Constructor: `Character`

Creates a `Character` instance wrapping the given Unicode codepoint.

**Syntax:**
```hoo
Character(codepoint: int64) :Character
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `codepoint` | `int64` | The Unicode codepoint value. |

**Returns:** `Character` — A new `Character` instance.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41); // 'A'
    println(ch.print());
    return 0;
}
```

---

#### `codepoint`

Returns the Unicode codepoint value of the character.

**Syntax:**
```hoo
codepoint() :int64
```

**Parameters:** None.

**Returns:** `int64` — The Unicode codepoint value.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    var cp = ch.codepoint();
    println(cp); // 65
    return 0;
}
```

---

#### `length`

Returns the byte length of the character's UTF-8 encoding.

**Syntax:**
```hoo
length() :int64
```

**Parameters:** None.

**Returns:** `int64` — The number of bytes in the UTF-8 encoding (1–4).

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    println(ch.length()); // 1
    return 0;
}
```

---

#### `data`

Returns the string representation of the character.

**Syntax:**
```hoo
data() :string
```

**Parameters:** None.

**Returns:** `string` — The character as a string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    println(ch.data()); // "A"
    return 0;
}
```

---

#### `print`

Prints the character to stdout.

**Syntax:**
```hoo
print() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    ch.print(); // A
    return 0;
}
```

---

#### `retain`

Increments the character's reference count by one.

**Syntax:**
```hoo
retain() :Character
```

**Parameters:** None.

**Returns:** `Character` — The same character instance.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    ch.retain();
    ch.release();
    ch.release();
    return 0;
}
```

---

#### `release`

Decrements the character's reference count by one. When the reference count reaches zero the character is deallocated.

**Syntax:**
```hoo
release() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    ch.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of the character.

**Syntax:**
```hoo
refcount() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current reference count.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = Character(0x41);
    println(ch.refcount()); // 1
    return 0;
}
```

---

## Free Functions

### `character_is_alpha`

Checks whether a character is an alphabetic letter (`a`–`z` or `A`–`Z`).

**Syntax:**
```hoo
character_is_alpha(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is alphabetic, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_alpha('A'));  // 1
    println(character_is_alpha('3'));  // 0
    return 0;
}
```

---

### `character_is_digit`

Checks whether a character is a decimal digit (`0`–`9`).

**Syntax:**
```hoo
character_is_digit(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is a digit, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_digit('5'));  // 1
    println(character_is_digit('A'));  // 0
    return 0;
}
```

---

### `character_is_alnum`

Checks whether a character is alphanumeric (a letter `a`–`z`, `A`–`Z` or a digit `0`–`9`).

**Syntax:**
```hoo
character_is_alnum(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is alphanumeric, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_alnum('Z'));  // 1
    println(character_is_alnum('.'));  // 0
    return 0;
}
```

---

### `character_is_lower`

Checks whether a character is a lowercase letter (`a`–`z`).

**Syntax:**
```hoo
character_is_lower(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is a lowercase letter, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_lower('a'));  // 1
    println(character_is_lower('A'));  // 0
    return 0;
}
```

---

### `character_is_upper`

Checks whether a character is an uppercase letter (`A`–`Z`).

**Syntax:**
```hoo
character_is_upper(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is an uppercase letter, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_upper('A'));  // 1
    println(character_is_upper('a'));  // 0
    return 0;
}
```

---

### `character_is_space`

Checks whether a character is whitespace (space, tab, newline, carriage return, form feed, or vertical tab).

**Syntax:**
```hoo
character_is_space(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:** `int64` — `1` if the character is whitespace, `0` otherwise.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    println(character_is_space(' '));  // 1
    println(character_is_space('\t')); // 1
    println(character_is_space('A'));  // 0
    return 0;
}
```

---

### `character_to_upper`

Converts a lowercase character to its uppercase equivalent. If the character is not a lowercase letter, it is returned unchanged.

**Syntax:**
```hoo
character_to_upper(c: char) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to convert. |

**Returns:** `char` — The uppercase equivalent of `c`, or `c` unchanged if it has no uppercase mapping.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var upper: char = character_to_upper('h');
    println(upper); // H
    return 0;
}
```

---

### `character_to_lower`

Converts an uppercase character to its lowercase equivalent. If the character is not an uppercase letter, it is returned unchanged.

**Syntax:**
```hoo
character_to_lower(c: char) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to convert. |

**Returns:** `char` — The lowercase equivalent of `c`, or `c` unchanged if it has no lowercase mapping.

**Errors:** No errors.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var lower: char = character_to_lower('W');
    println(lower); // w
    return 0;
}
```

---

### `character_digit_to_int64`

Converts a digit character (`'0'`–`'9'`) to its corresponding integer value (`0`–`9`).

**Syntax:**
```hoo
character_digit_to_int64(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The digit character to convert. |

**Returns:** `int64` — The integer value of the digit (`0`–`9`), or `-1` if `c` is not a digit character.

**Errors:** No errors. Non-digit characters return `-1` rather than raising an error.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var val = character_digit_to_int64('7');
    println(val); // 7

    val = character_digit_to_int64('A');
    println(val); // -1
    return 0;
}
```

---

### `character_int64_to_digit`

Converts an integer value (`0`–`9`) to its corresponding digit character (`'0'`–`'9'`).

**Syntax:**
```hoo
character_int64_to_digit(val: int64) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `val` | `int64` | The integer value to convert. |

**Returns:** `char` — The digit character (`'0'`–`'9'`), or the null character `'\0'` if `val` is outside the range `0`–`9`.

**Errors:** No errors. Values outside `0`–`9` return `'\0'` rather than raising an error.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var c: char = character_int64_to_digit(3);
    println(c); // 3

    c = character_int64_to_digit(42);
    println(c); // (prints nothing — null character)
    return 0;
}
```

---

### `character_from_utf8`

Creates a `Character` from the first Unicode scalar value in a UTF-8 string.

**Syntax:**
```hoo
character_from_utf8(str: string) :Character
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `str` | `string` | A UTF-8 encoded string. |

**Returns:** `Character` — A new `Character` for the first codepoint in the string, or a null handle if the string is empty or invalid.

**Errors:** Returns a null handle for empty or invalid UTF-8 input.

**Complete Example:**
```hoo
import hoo.character;

func :int64 main() {
    var ch = character_from_utf8("A");
    println(ch.codepoint()); // 65
    return 0;
}
```

## Usage Example

```hoo
import hoo.character;

func :int64 main() {
    // Character instance
    var ch = Character(0x41);  // 'A'
    println(ch.data());        // "A"
    println(ch.codepoint());   // 65
    ch.print();                // A

    // Classification free functions
    println(character_is_alpha('x'));   // 1
    println(character_is_digit('9'));   // 1
    println(character_is_alnum('.'));   // 0
    println(character_is_lower('A'));   // 0
    println(character_is_upper('Z'));   // 1
    println(character_is_space(' '));   // 1

    // Conversion free functions
    println(character_to_upper('q'));   // Q
    println(character_to_lower('X'));   // x

    // Digit conversion free functions
    var n = character_digit_to_int64('5');
    var d = character_int64_to_digit(n);
    println(n);                         // 5
    println(d);                         // 5

    // Create from UTF-8
    var from_str = character_from_utf8("Hello");
    println(from_str.codepoint());      // 72 ('H')

    return 0;
}
```
