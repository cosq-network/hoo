# Regex API Reference

## Module Name

`Regex` — core module (no explicit import required beyond `import hoo;`)

## Import Statement

```hoo
import hoo;
```

## Module Description

The `Regex` class provides compiled regular expression matching with support for pattern compilation, matching, search, replacement, and capture groups. Regular expressions use the ECMAScript grammar. The `Regex` class supports reference counting for shared ownership.

---

## Class: `Regex`

### Declaration

```hoo
class Regex
```

No explicit modifiers; `Regex` is a core runtime type available with only `import hoo;`.

### Public Fields

None — `Regex` instances are opaque handles.

### Public Class (Static) Functions

---

### `Regex.with_flags`

**Description:** Compiles a regular expression pattern with the specified flags and returns a new `Regex` instance.

**Syntax:**
```hoo
Regex.with_flags(pattern: string, flags: string) :Regex
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pattern` | `string` | The regular expression pattern to compile. |
| `flags` | `string` | Flag characters: `i` for case-insensitive, `m` for multiline. |

**Returns:** `Regex` — A new compiled `Regex` instance, or null if compilation fails.

**Errors:** Returns null if the pattern is malformed.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var re = Regex.with_flags("[a-z]+", "i");
    if (re) {
        var result = re.matches("HELLO");
        println(result); // 1
        re.release();
    }
}
```

---

### Public Instance Functions

---

#### Constructor: `Regex`

**Description:** Compiles a regular expression pattern and returns a new `Regex` instance.

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
import hoo;

func :void example() {
    var re = Regex("\\d+");
    if (re) {
        var found = re.matches("order 42");
        println(found); // 1
        re.release();
    }
}
```

---

#### `matches`

**Description:** Checks whether the pattern matches anywhere in the given text.

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
import hoo;

func :void example() {
    var re = Regex("\\d+");
    var found = re.matches("abc123def");
    println(found); // 1
    re.release();
}
```

---

#### `is_match`

**Description:** Checks whether the pattern matches the entire text from start to end.

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
import hoo;

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

**Description:** Finds all non-overlapping matches of the pattern in the given text.

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
import hoo;

func :void example() {
    var re = Regex("\\d+");
    var matches = re.find_all("a1 b22 c333");
    println(matches.length()); // 3
    re.release();
}
```

---

#### `replace`

**Description:** Replaces all non-overlapping matches of the pattern in the text with the replacement string.

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
import hoo;

func :void example() {
    var re = Regex("\\d+");
    var result = re.replace("order 42 item 7", "X");
    println(result); // "order X item X"
    re.release();
}
```

---

#### `capture`

**Description:** Returns the captured groups from the first match of the pattern in the given text. The first element (index 0) is the full match; subsequent elements are the captured groups.

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
import hoo;

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

**Description:** Returns the original pattern string that was used to compile the regex.

**Syntax:**
```hoo
to_string() :string
```

**Parameters:** None.

**Returns:** `string` — The pattern string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var re = Regex("\\d+\\.\\d+");
    println(re.to_string()); // "\d+\.\d+"
    re.release();
}
```

---

#### `retain`

**Description:** Increments the regex's reference count by one. Use this to extend the lifetime of a `Regex` instance when the original handle is released.

**Syntax:**
```hoo
retain() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** No errors. If called on a null handle the operation is a no-op.

**Complete Example:**
```hoo
import hoo;

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

**Description:** Decrements the regex's reference count by one. When the reference count reaches zero, the compiled regex pattern is deallocated.

**Syntax:**
```hoo
release() :void
```

**Parameters:** None.

**Returns:** `void`

**Errors:** No errors. Calling `release` on an already-freed or null handle is a no-op.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var re = Regex("temp");
    re.release();
}
```

---

#### `refcount`

**Description:** Returns the current reference count of the regex instance.

**Syntax:**
```hoo
refcount() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current reference count.

**Errors:** Returns 0 for a null handle.

**Complete Example:**
```hoo
import hoo;

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

**Description:** Frees a string allocated and returned by a regex method. Must be called for every string returned by regex operations to avoid memory leaks.

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
import hoo;

func :void example() {
    var re = Regex("\\w+");
    var result = re.replace("hello world", "hi");
    println(result);
    re.free_string(result);
    re.release();
}
```

---

## Usage Example

```hoo
import hoo;

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
