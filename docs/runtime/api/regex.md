# Regex API Reference

## Module Name

`hoo.regex`

## Import Statement

```hoo
import hoo.regex;
```

## Module Description

The `Regex` class provides compiled regular expression matching with support for pattern compilation, matching, search, replacement, and capture groups. Regular expressions use the ECMAScript grammar. The `Regex` class supports reference counting for shared ownership.

---

## Class: `Regex`

### Declaration

```hoo
class Regex
```

No explicit modifiers; `Regex` is a core runtime type available with `import hoo.regex;`.

### Constructor

#### `Regex`

Compiles a regular expression pattern and returns a new `Regex` instance.

**Syntax:**

```hoo
Regex(pattern: string) :Regex
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern to compile. |

**Returns:** `Regex` — A new compiled `Regex` instance, or null if compilation fails.

**Errors:** Returns null if the pattern is malformed.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+");
    if (re) {
        var found = re.matches("order 42");
        println(found); // 1
        re.release();
    }
}
```

### Instance Methods

#### `matches`

Checks whether the pattern matches anywhere in the given text.

**Syntax:**

```hoo
matches(text: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The text to search. |

**Returns:** `int64` — 1 if the pattern matches somewhere in `text`, 0 otherwise.

**Errors:** Returns -1 if the regex evaluation fails.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+");
    var found = re.matches("abc123def");
    println(found); // 1
    re.release();
}
```

---

#### `is_match`

Checks whether the pattern matches the entire text from start to end.

**Syntax:**

```hoo
is_match(text: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The text to match against. |

**Returns:** `int64` — 1 if the pattern fully matches `text`, 0 otherwise.

**Errors:** Returns -1 if the regex evaluation fails.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+");
    var full = re.is_match("123");
    println(full); // 1
    var partial = re.is_match("abc123");
    println(partial); // 0
    re.release();
}
```

---

#### `find_all`

Finds all non-overlapping matches of the pattern in the given text.

**Syntax:**

```hoo
find_all(text: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The text to search. |

**Returns:** `array` — An array of strings, each being a matched substring. Returns an empty array if no matches are found.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+");
    var matches = re.find_all("a1 b22 c333");
    println(matches.length()); // 3
    re.release();
}
```

---

#### `replace`

Replaces all non-overlapping matches of the pattern in the text with the replacement string.

**Syntax:**

```hoo
replace(text: string, replacement: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The text to process. |
| `replacement` | `string` | The replacement string. |

**Returns:** `string` — A new string with all matches replaced.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+");
    var result = re.replace("order 42 item 7", "X");
    println(result); // "order X item X"
    re.release();
}
```

---

#### `capture`

Returns the captured groups from the first match of the pattern in the given text. The first element (index 0) is the full match; subsequent elements are the captured groups.

**Syntax:**

```hoo
capture(text: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `text` | `string` | The text to search. |

**Returns:** `array` — An array of strings containing the captured groups. Returns an empty array if no match is found.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("(\\w+)@(\\w+)");
    var groups = re.capture("user@host");
    if (groups.length() > 0) {
        println(groups[0]); // "user@host"
        println(groups[1]); // "user"
        println(groups[2]); // "host"
    }
    re.release();
}
```

---

#### `to_string`

Returns the original pattern string that was used to compile the regex.

**Syntax:**

```hoo
to_string() :string
```

**Parameters:** None.

**Returns:** `string` — The pattern string.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\d+\\.\\d+");
    println(re.to_string()); // "\d+\.\d+"
    re.release();
}
```

---

#### `retain`

Increments the regex's reference count by one.

**Syntax:**

```hoo
retain() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** No errors. If called on a null handle the operation is a no-op.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("test");
    re.retain();
    // Both references must be released
    re.release();
    re.release();
}
```

---

#### `release`

Decrements the regex's reference count by one. When the reference count reaches zero, the compiled regex pattern is deallocated.

**Syntax:**

```hoo
release() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** No errors. Calling `release` on an already-freed or null handle is a no-op.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("temp");
    re.release();
}
```

---

#### `refcount`

Returns the current reference count of the regex instance.

**Syntax:**

```hoo
refcount() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current reference count.

**Errors:** Returns 0 for a null handle.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("count");
    re.retain();
    var rc = re.refcount(); // 2
    re.release();
    re.release();
}
```

---

#### `free_string`

Frees a string allocated and returned by a regex method.

**Syntax:**

```hoo
free_string(str: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `str` | `string` | The string to free. |

**Returns:** `void`

**Errors:** No errors. Passing null is a no-op.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("\\w+");
    var result = re.replace("hello world", "hi");
    println(result);
    re.free_string(result);
    re.release();
}
```

---

## Free Functions

### `regex_match`

Compiles a pattern and checks if it matches the entire text.

**Syntax:**

```hoo
regex_match(pattern: string, text: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern. |
| `text` | `string` | The text to match against. |

**Returns:** `int64` — 1 if the text matches the pattern, 0 otherwise.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var found = regex_match("\\d+", "order 42");
    println(found); // 0
}
```

---

### `regex_search`

Compiles a pattern and searches for a match anywhere in the text.

**Syntax:**

```hoo
regex_search(pattern: string, text: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern. |
| `text` | `string` | The text to search in. |

**Returns:** `int64` — 1 if the pattern is found, 0 otherwise.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var found = regex_search("\\d+", "order 42");
    println(found); // 1
}
```

---

### `regex_replace`

Compiles a pattern and replaces all matches with a replacement string.

**Syntax:**

```hoo
regex_replace(pattern: string, text: string, replacement: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern. |
| `text` | `string` | The text to search in. |
| `replacement` | `string` | The replacement string. |

**Returns:** `string` — The text with all matches replaced.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var result = regex_replace("\\d+", "order 42", "###");
    println(result); // "order ###"
}
```

---

### `regex_split`

Compiles a pattern and splits the text at each match.

**Syntax:**

```hoo
regex_split(pattern: string, text: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern. |
| `text` | `string` | The text to split. |

**Returns:** `array` — An array of string parts.

**Complete Example:**

```hoo
import hoo.regex;

func :void example() {
    var parts = regex_split("\\s+", "a b   c");
    println(parts.length()); // 3
}
```

---

## Usage Example

```hoo
import hoo.regex;

func :void example() {
    var re = Regex("(\\w+)@(\\w+\\.\\w+)");
    if (re) {
        var text = "Contact: alice@example.com, bob@test.org";
        if (re.matches(text)) {
            println("Found email addresses");
            var all = re.find_all(text);
            println("Count: " + all.length());
        }
        var groups = re.capture("alice@example.com");
        if (groups.length() >= 3) {
            println("User: " + groups[1]);
            println("Domain: " + groups[2]);
        }
        var masked = re.replace(text, "***@***");
        println(masked);
        re.release();
    }
}
```
