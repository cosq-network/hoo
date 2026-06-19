# DateTime API Reference (`DateTime`)

The `DateTime` class (type ID 119) wraps a Unix epoch timestamp (milliseconds) as
an ARC-managed heap object. Instances carry a single `timestamp: int64` field,
making them directly compatible with the `serializable` modifier.

DateTime instances are **immutable** — all arithmetic methods return a **new**
instance; the original is not modified.

> **Dispatch patterns (Hoo language):**
> - Instance methods (`dt.format(fmt)`) — call a method on a DateTime variable.
> - Class-qualified built-in dispatch (`DateTime.now()`) — compiler-magic syntax
>   for built-in types; takes DateTime handles where applicable.
> - Module-level free functions (`datetime_now()`) — namespace-prefixed functions
>   with the same underlying JIT bridges.

---

## 1. Factory Functions

### `DateTime.now()` / `datetime_now()`

Creates a DateTime instance representing the current system time (UTC).
(`DateTime.now()` uses built-in class-qualified dispatch; `datetime_now()` is the free function equivalent.)

**Syntax:**
```hoo
var dt = DateTime.now();
var dt = datetime_now();
```

**Parameters:** None

**Returns:** `DateTime` — a new DateTime instance at the current UTC time.

**Errors:** None (always succeeds).

**Example:**
```hoo
func :void example() {
    var dt = DateTime.now();
    println(dt.iso8601());
}
```

---

### `DateTime.nowSeconds()` / `datetime_nowSeconds()`

Returns the current system time as raw Unix epoch seconds.

**Syntax:**
```hoo
var secs = DateTime.nowSeconds();
var secs = datetime_nowSeconds();
```

**Parameters:** None

**Returns:** `int64` — the current time in seconds since the Unix epoch.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var secs = DateTime.nowSeconds();
    println("Epoch seconds: ".concat(secs.toString()));
}
```

---

### `DateTime.nowPrecise()` / `datetime_nowPrecise()`

Returns the current system time with sub-second precision.

**Syntax:**
```hoo
var precise = DateTime.nowPrecise();
var precise = datetime_nowPrecise();
```

**Parameters:** None

**Returns:** `double` — the current time in seconds since the Unix epoch
(e.g. `1705312200.456`).

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var precise = DateTime.nowPrecise();
    println(precise.toString());
}
```

---

### `DateTime.new(timestamp)` / `datetime_new(timestamp)`

Creates a DateTime instance from a raw Unix epoch timestamp.

**Syntax:**
```hoo
var dt = DateTime.new(1704067200000);
var dt = datetime_new(1704067200000);
```

**Parameters:**
- `timestamp: int64` — milliseconds since the Unix epoch.

**Returns:** `DateTime` — a new DateTime instance wrapping the given timestamp.

**Errors:** None (any `int64` value is accepted).

**Example:**
```hoo
func :void example() {
    var epoch = DateTime.new(0);
    println(epoch.iso8601()); // "1970-01-01T00:00:00.000Z"
}
```

---

### `DateTime.parse(str, fmt)` / `datetime_parse(str, fmt)`

Parses a date-time string according to the given format pattern.

**Syntax:**
```hoo
var dt = DateTime.parse("2024-01-15", "%Y-%m-%d");
var dt = datetime_parse("2024-01-15", "%Y-%m-%d");
```

**Parameters:**
- `str: string` — the date-time string to parse.
- `fmt: string` — the expected format pattern (strftime-style specifiers).

**Returns:** `DateTime` — a new DateTime instance on success, or `null` if the
string cannot be parsed.

**Errors:** Returns `null` on parse failure (no exception is thrown).

**Supported format specifiers:**

| Specifier | Meaning | Example |
|-----------|---------|---------|
| `%Y` | 4-digit year | `2024` |
| `%m` | 2-digit month (01–12) | `01` |
| `%d` | 2-digit day (01–31) | `15` |
| `%H` | 2-digit hour (00–23) | `10` |
| `%M` | 2-digit minute (00–59) | `30` |
| `%S` | 2-digit second (00–59) | `45` |
| `%f` | Milliseconds (1–6 digits) | `123` |
| `%w` | Weekday number (0=Sun, 6=Sat) | `1` |
| `%j` | Year day number (001–366) | `015` |

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-06-15 14:30:00", "%Y-%m-%d %H:%M:%S");
    if (dt) {
        println(dt.iso8601());
    } else {
        println("Parse failed");
    }
}
```

---

### `DateTime.fromIso8601(str)` / `datetime_fromIso8601(str)`

Parses an ISO 8601 date-time string.

**Syntax:**
```hoo
var dt = DateTime.fromIso8601("2024-01-15T10:30:00Z");
var dt = datetime_fromIso8601("2024-01-15T10:30:00Z");
```

**Parameters:**
- `str: string` — the ISO 8601 string (e.g. `"2024-01-15T10:30:00Z"`).

**Returns:** `DateTime` — a new DateTime instance on success, or `null` on
parse failure.

**Errors:** Returns `null` on parse failure (no exception is thrown).

**Supported ISO 8601 variants:**
- `2024-01-15T10:30:00Z`
- `2024-01-15T10:30:00.123Z`
- `2024-01-15T10:30:00+05:30`
- `2024-01-15T10:30:00`

**Example:**
```hoo
func :void example() {
    var dt = DateTime.fromIso8601("2024-12-25T00:00:00Z");
    if (dt) {
        println("Christmas: ".concat(dt.iso8601()));
    } else {
        println("Invalid ISO 8601");
    }
}
```

---

## 2. Accessors

### `dt.getTimestamp()`

Extracts the raw Unix epoch timestamp from a DateTime instance.

**Syntax:**
```hoo
var ts = dt.getTimestamp();
```

**Parameters:** None

**Returns:** `int64` — the timestamp in milliseconds since the Unix epoch.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.now();
    var ts = dt.getTimestamp();
    println("Timestamp (ms): ".concat(ts.toString()));
}
```

---

## 3. Formatting

### `dt.format(fmt)` / `DateTime.format(dt, fmt)` / `datetime_format(dt, fmt)`

Formats a DateTime instance according to the given format pattern.

**Syntax:**
```hoo
var str = dt.format("%Y-%m-%d");
var str = DateTime.format(dt, "%Y-%m-%d");
var str = datetime_format(dt, "%Y-%m-%d");
```

**Parameters:**
- `fmt: string` — the format pattern (strftime-style, see specifier table in
  `DateTime.parse`).

**Returns:** `string` — the formatted date-time string (ARC-managed).

**Errors:** None (the format string is matched literally or via known specifiers).

**Example:**
```hoo
func :void example() {
    var dt = DateTime.now();
    var formatted = dt.format("%Y-%m-%d %H:%M:%S");
    println(formatted);
}
```

---

### `dt.iso8601()` / `DateTime.iso8601(dt)` / `datetime_iso8601(dt)`

Formats a DateTime instance as an ISO 8601 string.

**Syntax:**
```hoo
var str = dt.iso8601();
var str = DateTime.iso8601(dt);
var str = datetime_iso8601(dt);
```

**Parameters:** None

**Returns:** `string` — the ISO 8601 formatted string (ARC-managed),
e.g. `"2024-01-15T10:30:00.000Z"`.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.now();
    var iso = dt.iso8601();
    println(iso);
}
```

---

## 4. Arithmetic

All arithmetic methods return a **new** DateTime instance; the original is
not modified (immutable semantics).

### `dt.addDays(n)` / `DateTime.addDays(dt, n)` / `datetime_addDays(dt, n)`

Adds a number of days.

**Syntax:**
```hoo
var result = dt.addDays(7);
var result = DateTime.addDays(dt, 7);
var result = datetime_addDays(dt, 7);
```

**Parameters:**
- `n: int64` — the number of days to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance offset by `n` days.

**Errors:** None (the underlying int64 arithmetic wraps per C++ semantics).

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var later = dt.addDays(7);
    println(later.iso8601()); // "2024-01-08T00:00:00.000Z"
}
```

---

### `dt.addHours(n)` / `DateTime.addHours(dt, n)` / `datetime_addHours(dt, n)`

Adds a number of hours.

**Syntax:**
```hoo
var result = dt.addHours(48);
```

**Parameters:**
- `n: int64` — the number of hours to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var later = dt.addHours(48);
    println(later.iso8601()); // "2024-01-03T00:00:00.000Z"
}
```

---

### `dt.addMinutes(n)` / `DateTime.addMinutes(dt, n)` / `datetime_addMinutes(dt, n)`

Adds a number of minutes.

**Syntax:**
```hoo
var result = dt.addMinutes(90);
```

**Parameters:**
- `n: int64` — the number of minutes to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addMinutes(90);
    println(later.iso8601()); // "2024-01-01T01:30:00.000Z"
}
```

---

### `dt.addSeconds(n)` / `DateTime.addSeconds(dt, n)` / `datetime_addSeconds(dt, n)`

Adds a number of seconds.

**Syntax:**
```hoo
var result = dt.addSeconds(3600);
```

**Parameters:**
- `n: int64` — the number of seconds to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addSeconds(3600);
    println(later.iso8601()); // "2024-01-01T01:00:00.000Z"
}
```

---

### `dt.addMilliseconds(ms)` / `DateTime.addMilliseconds(dt, ms)` / `datetime_addMilliseconds(dt, ms)`

Adds a number of milliseconds.

**Syntax:**
```hoo
var result = dt.addMilliseconds(5000);
```

**Parameters:**
- `ms: int64` — the number of milliseconds to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var dt = DateTime.parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addMilliseconds(5000);
    println(later.iso8601()); // "2024-01-01T00:00:05.000Z"
}
```

---

## 5. Differences

### `a.diffDays(b)` / `DateTime.diffDays(a, b)` / `datetime_diffDays(a, b)`

Returns the difference between two DateTime instances in whole days.

**Syntax:**
```hoo
var days = a.diffDays(b);
var days = DateTime.diffDays(a, b);
var days = datetime_diffDays(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — the number of whole days between `a` and `b`. The result
is positive if `a > b`, negative if `a < b`, zero if equal.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var start = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var end = DateTime.parse("2024-01-10", "%Y-%m-%d");
    var diff = start.diffDays(end);
    println(diff.toString()); // "9"
}
```

---

### `a.diffHours(b)` / `DateTime.diffHours(a, b)` / `datetime_diffHours(a, b)`

Returns the difference in whole hours.

**Syntax:**
```hoo
var hours = a.diffHours(b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — the number of whole hours between the two instances.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var start = DateTime.parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    var end = DateTime.parse("2024-01-02 12:00:00", "%Y-%m-%d %H:%M:%S");
    var diff = start.diffHours(end);
    println(diff.toString()); // "36"
}
```

---

### `a.diffSeconds(b)` / `DateTime.diffSeconds(a, b)` / `datetime_diffSeconds(a, b)`

Returns the difference in seconds with fractional precision.

**Syntax:**
```hoo
var secs = a.diffSeconds(b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `double` — the number of seconds (including fractional) between
the two instances. Positive if `a > b`, negative if `a < b`.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var start = DateTime.parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    var end = DateTime.parse("2024-01-01 00:01:30", "%Y-%m-%d %H:%M:%S");
    var diff = start.diffSeconds(end);
    println(diff.toString()); // "90.0"
}
```

---

## 6. Comparison

### `a.compare(b)` / `DateTime.compare(a, b)` / `datetime_compare(a, b)`

Compares two DateTime instances.

**Syntax:**
```hoo
var cmp = a.compare(b);
var cmp = DateTime.compare(a, b);
var cmp = datetime_compare(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — `-1` if `a < b`, `0` if `a == b`, `1` if `a > b`.

**Errors:** None.

**Example:**
```hoo
func :void example() {
    var early = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var late = DateTime.parse("2024-06-15", "%Y-%m-%d");
    var cmp = early.compare(late);
    println(cmp.toString()); // "-1"

    cmp = early.compare(early);
    println(cmp.toString()); // "0"
}
```

---

## 7. Complete Example Program

```hoo
func :void main() {
    // Current time
    var now = DateTime.now();
    println("Now: ".concat(now.iso8601()));

    // Format with custom pattern
    var formatted = now.format("%Y-%m-%d (%A)");
    println("Formatted: ".concat(formatted));

    // Arithmetic
    var tomorrow = now.addDays(1);
    println("Tomorrow: ".concat(tomorrow.iso8601()));

    var nextWeek = now.addDays(7);
    var weekDiff = now.diffDays(nextWeek);
    println("Days between: ".concat(weekDiff.toString())); // "7"

    // Parse from string
    var christmas = DateTime.parse("2024-12-25", "%Y-%m-%d");
    if (christmas) {
        var untilChristmas = now.diffDays(christmas);
        println("Days until Christmas 2024: ".concat(untilChristmas.toString()));
    }

    // Comparison
    var a = DateTime.fromIso8601("2024-01-01T00:00:00Z");
    var b = DateTime.fromIso8601("2024-06-15T00:00:00Z");
    var cmp = a.compare(b);
    if (cmp < 0) {
        println("January comes before June");
    }

    // Free function syntax (equivalent)
    var alsoNow = datetime_now();
    println("Also now (free func): ".concat(alsoNow.iso8601()));

    var fromTs = datetime_new(1704067200000);
    println("From timestamp: ".concat(fromTs.iso8601()));
}
```

## API Summary

| Kind | API | Return Type |
|------|-----|-------------|
| Factory | `DateTime.now()` / `datetime_now()` | `DateTime` |
| Factory | `DateTime.new(ts)` / `datetime_new(ts)` | `DateTime` |
| Factory | `DateTime.parse(s,f)` / `datetime_parse(s,f)` | `DateTime` |
| Factory | `DateTime.fromIso8601(s)` / `datetime_fromIso8601(s)` | `DateTime` |
| Raw time | `DateTime.nowSeconds()` / `datetime_nowSeconds()` | `int64` |
| Raw time | `DateTime.nowPrecise()` / `datetime_nowPrecise()` | `double` |
| Accessor | `dt.getTimestamp()` | `int64` |
| Format | `dt.format(f)` / `DateTime.format(dt,f)` / `datetime_format(dt,f)` | `string` |
| Format | `dt.iso8601()` / `DateTime.iso8601(dt)` / `datetime_iso8601(dt)` | `string` |
| Arithmetic | `dt.addDays(n)` / `DateTime.addDays(dt,n)` / `datetime_addDays(dt,n)` | `DateTime` |
| Arithmetic | `dt.addHours(n)` / `DateTime.addHours(dt,n)` / `datetime_addHours(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMinutes(n)` / `DateTime.addMinutes(dt,n)` / `datetime_addMinutes(dt,n)` | `DateTime` |
| Arithmetic | `dt.addSeconds(n)` / `DateTime.addSeconds(dt,n)` / `datetime_addSeconds(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMilliseconds(n)` / `DateTime.addMilliseconds(dt,n)` / `datetime_addMilliseconds(dt,n)` | `DateTime` |
| Diff | `a.diffDays(b)` / `DateTime.diffDays(a,b)` / `datetime_diffDays(a,b)` | `int64` |
| Diff | `a.diffHours(b)` / `DateTime.diffHours(a,b)` / `datetime_diffHours(a,b)` | `int64` |
| Diff | `a.diffSeconds(b)` / `DateTime.diffSeconds(a,b)` / `datetime_diffSeconds(a,b)` | `double` |
| Compare | `a.compare(b)` / `DateTime.compare(a,b)` / `datetime_compare(a,b)` | `int64` |
