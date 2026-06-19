# DateTime API Reference (`DateTime`)

**Import Requirement:**
```hoo
import hoo.datetime;
```

The `DateTime` class (type ID 119) wraps a Unix epoch timestamp (milliseconds) as
an ARC-managed heap object. Instances carry a single `timestamp: int64` field,
making them directly compatible with the `serializable` modifier.

DateTime instances are **immutable** — all arithmetic methods return a **new**
instance; the original is not modified.

> **Dispatch patterns (Hoo language):**
> - Instance methods (`dt.format(fmt)`) — call a method on a DateTime variable.
> - Module-level free functions (`datetime_now()`) — namespace-prefixed functions.

---

## 1. Factory Functions

### `datetime_now()`

Creates a DateTime instance representing the current system time (UTC).

**Syntax:**
```hoo
var dt = datetime_now();
```

**Parameters:** None

**Returns:** `DateTime` — a new DateTime instance at the current UTC time.

**Errors:** None (always succeeds).

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_now();
    println(dt.iso8601());
}
```

---

### `datetime_nowSeconds()`

Returns the current system time as raw Unix epoch seconds.

**Syntax:**
```hoo
var secs = datetime_nowSeconds();
```

**Parameters:** None

**Returns:** `int64` — the current time in seconds since the Unix epoch.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var secs = datetime_nowSeconds();
    println("Epoch seconds: ".concat(secs.toString()));
}
```

---

### `datetime_nowPrecise()`

Returns the current system time with sub-second precision.

**Syntax:**
```hoo
var precise = datetime_nowPrecise();
```

**Parameters:** None

**Returns:** `double` — the current time in seconds since the Unix epoch
(e.g. `1705312200.456`).

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var precise = datetime_nowPrecise();
    println(precise.toString());
}
```

---

### `datetime_new(timestamp)`

Creates a DateTime instance from a raw Unix epoch timestamp.

**Syntax:**
```hoo
var dt = datetime_new(1704067200000);
```

**Parameters:**
- `timestamp: int64` — milliseconds since the Unix epoch.

**Returns:** `DateTime` — a new DateTime instance wrapping the given timestamp.

**Errors:** None (any `int64` value is accepted).

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var epoch = datetime_new(0);
    println(epoch.iso8601()); // "1970-01-01T00:00:00.000Z"
}
```

---

### `datetime_parse(str, fmt)`

Parses a date-time string according to the given format pattern.

**Syntax:**
```hoo
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
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-06-15 14:30:00", "%Y-%m-%d %H:%M:%S");
    if (dt) {
        println(dt.iso8601());
    } else {
        println("Parse failed");
    }
}
```

---

### `datetime_fromIso8601(str)`

Parses an ISO 8601 date-time string.

**Syntax:**
```hoo
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
import hoo.datetime;

func :void example() {
    var dt = datetime_fromIso8601("2024-12-25T00:00:00Z");
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
import hoo.datetime;

func :void example() {
    var dt = datetime_now();
    var ts = dt.getTimestamp();
    println("Timestamp (ms): ".concat(ts.toString()));
}
```

---

## 3. Formatting

### `dt.format(fmt)` / `datetime_format(dt, fmt)`

Formats a DateTime instance according to the given format pattern.

**Syntax:**
```hoo
var str = dt.format("%Y-%m-%d");
var str = datetime_format(dt, "%Y-%m-%d");
```

**Parameters:**
- `fmt: string` — the format pattern (strftime-style, see specifier table in
  `datetime_parse`).

**Returns:** `string` — the formatted date-time string (ARC-managed).

**Errors:** None (the format string is matched literally or via known specifiers).

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_now();
    var formatted = dt.format("%Y-%m-%d %H:%M:%S");
    println(formatted);
}
```

---

### `dt.iso8601()` / `datetime_iso8601(dt)`

Formats a DateTime instance as an ISO 8601 string.

**Syntax:**
```hoo
var str = dt.iso8601();
var str = datetime_iso8601(dt);
```

**Parameters:** None

**Returns:** `string` — the ISO 8601 formatted string (ARC-managed),
e.g. `"2024-01-15T10:30:00.000Z"`.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_now();
    var iso = dt.iso8601();
    println(iso);
}
```

---

## 4. Arithmetic

All arithmetic methods return a **new** DateTime instance; the original is
not modified (immutable semantics).

### `dt.addDays(n)` / `datetime_addDays(dt, n)`

Adds a number of days.

**Syntax:**
```hoo
var result = dt.addDays(7);
var result = datetime_addDays(dt, 7);
```

**Parameters:**
- `n: int64` — the number of days to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance offset by `n` days.

**Errors:** None (the underlying int64 arithmetic wraps per C++ semantics).

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-01-01", "%Y-%m-%d");
    var later = dt.addDays(7);
    println(later.iso8601()); // "2024-01-08T00:00:00.000Z"
}
```

---

### `dt.addHours(n)` / `datetime_addHours(dt, n)`

Adds a number of hours.

**Syntax:**
```hoo
var result = dt.addHours(48);
var result = datetime_addHours(dt, 48);
```

**Parameters:**
- `n: int64` — the number of hours to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-01-01", "%Y-%m-%d");
    var later = dt.addHours(48);
    println(later.iso8601()); // "2024-01-03T00:00:00.000Z"
}
```

---

### `dt.addMinutes(n)` / `datetime_addMinutes(dt, n)`

Adds a number of minutes.

**Syntax:**
```hoo
var result = dt.addMinutes(90);
var result = datetime_addMinutes(dt, 90);
```

**Parameters:**
- `n: int64` — the number of minutes to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addMinutes(90);
    println(later.iso8601()); // "2024-01-01T01:30:00.000Z"
}
```

---

### `dt.addSeconds(n)` / `datetime_addSeconds(dt, n)`

Adds a number of seconds.

**Syntax:**
```hoo
var result = dt.addSeconds(3600);
var result = datetime_addSeconds(dt, 3600);
```

**Parameters:**
- `n: int64` — the number of seconds to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addSeconds(3600);
    println(later.iso8601()); // "2024-01-01T01:00:00.000Z"
}
```

---

### `dt.addMilliseconds(ms)` / `datetime_addMilliseconds(dt, ms)`

Adds a number of milliseconds.

**Syntax:**
```hoo
var result = dt.addMilliseconds(5000);
var result = datetime_addMilliseconds(dt, 5000);
```

**Parameters:**
- `ms: int64` — the number of milliseconds to add (negative values subtract).

**Returns:** `DateTime` — a new DateTime instance.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var dt = datetime_parse("2024-01-01T00:00:00Z", "%Y-%m-%dT%H:%M:%Sz");
    var later = dt.addMilliseconds(5000);
    println(later.iso8601()); // "2024-01-01T00:00:05.000Z"
}
```

---

## 5. Differences

### `a.diffDays(b)` / `datetime_diffDays(a, b)`

Returns the difference between two DateTime instances in whole days.

**Syntax:**
```hoo
var days = a.diffDays(b);
var days = datetime_diffDays(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — the number of whole days between `a` and `b`. The result
is positive if `a > b`, negative if `a < b`, zero if equal.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var start = datetime_parse("2024-01-01", "%Y-%m-%d");
    var end = datetime_parse("2024-01-10", "%Y-%m-%d");
    var diff = start.diffDays(end);
    println(diff.toString()); // "9"
}
```

---

### `a.diffHours(b)` / `datetime_diffHours(a, b)`

Returns the difference in whole hours.

**Syntax:**
```hoo
var hours = a.diffHours(b);
var hours = datetime_diffHours(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — the number of whole hours between the two instances.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var start = datetime_parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    var end = datetime_parse("2024-01-02 12:00:00", "%Y-%m-%d %H:%M:%S");
    var diff = start.diffHours(end);
    println(diff.toString()); // "36"
}
```

---

### `a.diffSeconds(b)` / `datetime_diffSeconds(a, b)`

Returns the difference in seconds with fractional precision.

**Syntax:**
```hoo
var secs = a.diffSeconds(b);
var secs = datetime_diffSeconds(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `double` — the number of seconds (including fractional) between
the two instances. Positive if `a > b`, negative if `a < b`.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var start = datetime_parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    var end = datetime_parse("2024-01-01 00:01:30", "%Y-%m-%d %H:%M:%S");
    var diff = start.diffSeconds(end);
    println(diff.toString()); // "90.0"
}
```

---

## 6. Comparison

### `a.compare(b)` / `datetime_compare(a, b)`

Compares two DateTime instances.

**Syntax:**
```hoo
var cmp = a.compare(b);
var cmp = datetime_compare(a, b);
```

**Parameters:**
- `b: DateTime` — the other DateTime instance.

**Returns:** `int64` — `-1` if `a < b`, `0` if `a == b`, `1` if `a > b`.

**Errors:** None.

**Example:**
```hoo
import hoo.datetime;

func :void example() {
    var early = datetime_parse("2024-01-01", "%Y-%m-%d");
    var late = datetime_parse("2024-06-15", "%Y-%m-%d");
    var cmp = early.compare(late);
    println(cmp.toString()); // "-1"

    cmp = early.compare(early);
    println(cmp.toString()); // "0"
}
```

---

## 7. Complete Example Program

```hoo
import hoo.datetime;

func :void main() {
    // Current time
    var now = datetime_now();
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
    var christmas = datetime_parse("2024-12-25", "%Y-%m-%d");
    if (christmas) {
        var untilChristmas = now.diffDays(christmas);
        println("Days until Christmas 2024: ".concat(untilChristmas.toString()));
    }

    // Comparison
    var a = datetime_fromIso8601("2024-01-01T00:00:00Z");
    var b = datetime_fromIso8601("2024-06-15T00:00:00Z");
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
| Factory | `datetime_now()` | `DateTime` |
| Factory | `datetime_new(ts)` | `DateTime` |
| Factory | `datetime_parse(s,f)` | `DateTime` |
| Factory | `datetime_fromIso8601(s)` | `DateTime` |
| Raw time | `datetime_nowSeconds()` | `int64` |
| Raw time | `datetime_nowPrecise()` | `double` |
| Accessor | `dt.getTimestamp()` | `int64` |
| Format | `dt.format(f)` / `datetime_format(dt,f)` | `string` |
| Format | `dt.iso8601()` / `datetime_iso8601(dt)` | `string` |
| Arithmetic | `dt.addDays(n)` / `datetime_addDays(dt,n)` | `DateTime` |
| Arithmetic | `dt.addHours(n)` / `datetime_addHours(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMinutes(n)` / `datetime_addMinutes(dt,n)` | `DateTime` |
| Arithmetic | `dt.addSeconds(n)` / `datetime_addSeconds(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMilliseconds(n)` / `datetime_addMilliseconds(dt,n)` | `DateTime` |
| Diff | `a.diffDays(b)` / `datetime_diffDays(a,b)` | `int64` |
| Diff | `a.diffHours(b)` / `datetime_diffHours(a,b)` | `int64` |
| Diff | `a.diffSeconds(b)` / `datetime_diffSeconds(a,b)` | `double` |
| Compare | `a.compare(b)` / `datetime_compare(a,b)` | `int64` |
