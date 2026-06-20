# Character API Reference

## Module Name

Part of the `hoo.character` module.

## Import Statement

```hoo
import hoo.character;
```

## Module Description

The `Character` class provides a collection of static utility functions for classifying and converting single characters. These functions operate on the Hoo `char` type and cover common character category checks (alphabetic, digit, whitespace, etc.) and case conversions. All functions are stateless and do not require a `Character` instance.

## Class: Character

### Declaration

```hoo
class Character
```

### Public Fields

None.

### Public Class (Static) Functions

#### `Character.is_alpha`

Checks whether a character is an alphabetic letter (`a`–`z` or `A`–`Z`).

**Syntax:**

```hoo
Character.is_alpha(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is alphabetic, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_alpha('A'));  // 1
    println(Character.is_alpha('3'));  // 0
    return 0;
}
```

---

#### `Character.is_digit`

Checks whether a character is a decimal digit (`0`–`9`).

**Syntax:**

```hoo
Character.is_digit(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is a digit, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_digit('5'));  // 1
    println(Character.is_digit('A'));  // 0
    return 0;
}
```

---

#### `Character.is_alnum`

Checks whether a character is alphanumeric (a letter `a`–`z`, `A`–`Z` or a digit `0`–`9`).

**Syntax:**

```hoo
Character.is_alnum(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is alphanumeric, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_alnum('Z'));  // 1
    println(Character.is_alnum('.'));  // 0
    return 0;
}
```

---

#### `Character.is_lower`

Checks whether a character is a lowercase letter (`a`–`z`).

**Syntax:**

```hoo
Character.is_lower(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is a lowercase letter, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_lower('a'));  // 1
    println(Character.is_lower('A'));  // 0
    return 0;
}
```

---

#### `Character.is_upper`

Checks whether a character is an uppercase letter (`A`–`Z`).

**Syntax:**

```hoo
Character.is_upper(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is an uppercase letter, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_upper('A'));  // 1
    println(Character.is_upper('a'));  // 0
    return 0;
}
```

---

#### `Character.is_space`

Checks whether a character is whitespace (space, tab, newline, carriage return, form feed, or vertical tab).

**Syntax:**

```hoo
Character.is_space(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to test. |

**Returns:**

`int64` — `1` if the character is whitespace, `0` otherwise.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    println(Character.is_space(' '));  // 1
    println(Character.is_space('\t')); // 1
    println(Character.is_space('A'));  // 0
    return 0;
}
```

---

#### `Character.to_upper`

Converts a lowercase character to its uppercase equivalent. If the character is not a lowercase letter, it is returned unchanged.

**Syntax:**

```hoo
Character.to_upper(c: char) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to convert. |

**Returns:**

`char` — The uppercase equivalent of `c`, or `c` unchanged if it has no uppercase mapping.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    var upper: char = Character.to_upper('h');
    println(upper); // H
    return 0;
}
```

---

#### `Character.to_lower`

Converts an uppercase character to its lowercase equivalent. If the character is not an uppercase letter, it is returned unchanged.

**Syntax:**

```hoo
Character.to_lower(c: char) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The character to convert. |

**Returns:**

`char` — The lowercase equivalent of `c`, or `c` unchanged if it has no lowercase mapping.

**Errors:**

No errors.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    var lower: char = Character.to_lower('W');
    println(lower); // w
    return 0;
}
```

---

#### `Character.digit_to_int64`

Converts a digit character (`'0'`–`'9'`) to its corresponding integer value (`0`–`9`).

**Syntax:**

```hoo
Character.digit_to_int64(c: char) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `c` | `char` | The digit character to convert. |

**Returns:**

`int64` — The integer value of the digit (`0`–`9`), or `-1` if `c` is not a digit character.

**Errors:**

No errors. Non-digit characters return `-1` rather than raising an error.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    var val = Character.digit_to_int64('7');
    println(val); // 7

    val = Character.digit_to_int64('A');
    println(val); // -1
    return 0;
}
```

---

#### `Character.int64_to_digit`

Converts an integer value (`0`–`9`) to its corresponding digit character (`'0'`–`'9'`).

**Syntax:**

```hoo
Character.int64_to_digit(val: int64) :char
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `val` | `int64` | The integer value to convert. |

**Returns:**

`char` — The digit character (`'0'`–`'9'`), or the null character `'\0'` if `val` is outside the range `0`–`9`.

**Errors:**

No errors. Values outside `0`–`9` return `'\0'` rather than raising an error.

**Complete Example:**

```hoo
import hoo.character;

func :int64 main() {
    var c: char = Character.int64_to_digit(3);
    println(c); // 3

    c = Character.int64_to_digit(42);
    println(c); // (prints nothing — null character)
    return 0;
}
```

## Usage Example

```hoo
import hoo.character;

func :int64 main() {
    // Classification
    println(Character.is_alpha('x'));   // 1
    println(Character.is_digit('9'));   // 1
    println(Character.is_alnum('.'));   // 0
    println(Character.is_lower('A'));   // 0
    println(Character.is_upper('Z'));   // 1
    println(Character.is_space(' '));   // 1

    // Conversion
    println(Character.to_upper('q'));   // Q
    println(Character.to_lower('X'));   // x

    // Digit conversion
    var n = Character.digit_to_int64('5');
    var d = Character.int64_to_digit(n);
    println(n);                         // 5
    println(d);                         // 5

    return 0;
}
```
