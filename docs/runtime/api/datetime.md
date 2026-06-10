# DateTime API Reference (`DateTime`)

The `DateTime` singleton class provides date and time operations. All timestamps are represented as milliseconds since the Unix epoch (January 1, 1970 UTC). All methods are static — call them directly on the class.

## 1. Current Time

### `DateTime.now() :int64`

Returns the current system time.

- **Parameters:** None
- **Returns:** `int64` — the current time in milliseconds since the Unix epoch.

```hoo
func :void example() {
    var ts = DateTime.now();
    println(ts.toString());
}
```

---

### `DateTime.nowSeconds() :int64`

Returns the current system time with second precision.

- **Parameters:** None
- **Returns:** `int64` — the current time in seconds since the Unix epoch.

```hoo
func :void example() {
    var ts = DateTime.nowSeconds();
    println(ts.toString());
}
```

## 2. Formatting & Parsing

### `DateTime.format(ts: int64, fmt: string) :string`

Formats a timestamp according to the given format pattern.

- **Parameters:**
  - `ts: int64` — the timestamp in milliseconds.
  - `fmt: string` — the format pattern (e.g. `"%Y-%m-%d"`, `"%H:%M:%S"`).
- **Returns:** `string` — the formatted date-time string.

```hoo
func :void example() {
    var ts = DateTime.now();
    var formatted = DateTime.format(ts, "%Y-%m-%d %H:%M:%S");
    println(formatted);
}
```

---

### `DateTime.parse(str: string, fmt: string) :int64`

Parses a date-time string matching the given format pattern and returns the corresponding timestamp.

- **Parameters:**
  - `str: string` — the date-time string to parse.
  - `fmt: string` — the expected format pattern.
- **Returns:** `int64` — the timestamp in milliseconds, or -1 on failure.

```hoo
func :void example() {
    var ts = DateTime.parse("2024-01-15", "%Y-%m-%d");
    println(ts.toString());
}
```

---

### `DateTime.iso8601(ts: int64) :string`

Formats a timestamp as an ISO 8601 date-time string.

- **Parameters:**
  - `ts: int64` — the timestamp in milliseconds.
- **Returns:** `string` — the ISO 8601 formatted string.

```hoo
func :void example() {
    var ts = DateTime.now();
    var iso = DateTime.iso8601(ts);
    println(iso); // Output: 2024-01-15T10:30:00Z
}
```

---

### `DateTime.fromIso8601(str: string) :int64`

Parses an ISO 8601 date-time string and returns the corresponding timestamp.

- **Parameters:**
  - `str: string` — the ISO 8601 string to parse.
- **Returns:** `int64` — the timestamp in milliseconds, or -1 on failure.

```hoo
func :void example() {
    var ts = DateTime.fromIso8601("2024-01-15T10:30:00Z");
    println(ts.toString());
}
```

## 3. Arithmetic

### `DateTime.addDays(ts: int64, days: int64) :int64`

Adds a number of days to a timestamp.

- **Parameters:**
  - `ts: int64` — the base timestamp in milliseconds.
  - `days: int64` — the number of days to add (negative to subtract).
- **Returns:** `int64` — the resulting timestamp in milliseconds.

```hoo
func :void example() {
    var ts = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var later = DateTime.addDays(ts, 7);
    println(DateTime.iso8601(later)); // Output: 2024-01-08T00:00:00Z
}
```

---

### `DateTime.addHours(ts: int64, hours: int64) :int64`

Adds a number of hours to a timestamp.

- **Parameters:**
  - `ts: int64` — the base timestamp in milliseconds.
  - `hours: int64` — the number of hours to add (negative to subtract).
- **Returns:** `int64` — the resulting timestamp in milliseconds.

```hoo
func :void example() {
    var ts = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var later = DateTime.addHours(ts, 48);
    println(DateTime.iso8601(later)); // Output: 2024-01-03T00:00:00Z
}
```

---

### `DateTime.addMinutes(ts: int64, minutes: int64) :int64`

Adds a number of minutes to a timestamp.

- **Parameters:**
  - `ts: int64` — the base timestamp in milliseconds.
  - `minutes: int64` — the number of minutes to add (negative to subtract).
- **Returns:** `int64` — the resulting timestamp in milliseconds.

```hoo
func :void example() {
    var ts = DateTime.now();
    var later = DateTime.addMinutes(ts, 90);
    println(later.toString());
}
```

---

### `DateTime.addSeconds(ts: int64, seconds: int64) :int64`

Adds a number of seconds to a timestamp.

- **Parameters:**
  - `ts: int64` — the base timestamp in milliseconds.
  - `seconds: int64` — the number of seconds to add (negative to subtract).
- **Returns:** `int64` — the resulting timestamp in milliseconds.

```hoo
func :void example() {
    var ts = DateTime.now();
    var later = DateTime.addSeconds(ts, 300);
    println(later.toString());
}
```

---

### `DateTime.addMilliseconds(ts: int64, ms: int64) :int64`

Adds a number of milliseconds to a timestamp.

- **Parameters:**
  - `ts: int64` — the base timestamp in milliseconds.
  - `ms: int64` — the number of milliseconds to add (negative to subtract).
- **Returns:** `int64` — the resulting timestamp in milliseconds.

```hoo
func :void example() {
    var ts = DateTime.now();
    var later = DateTime.addMilliseconds(ts, 5000);
    println(later.toString());
}
```

## 4. Differences

### `DateTime.diffDays(ts1: int64, ts2: int64) :int64`

Returns the difference between two timestamps in whole days (absolute value).

- **Parameters:**
  - `ts1: int64` — the first timestamp in milliseconds.
  - `ts2: int64` — the second timestamp in milliseconds.
- **Returns:** `int64` — the number of days between the two timestamps.

```hoo
func :void example() {
    var start = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var end = DateTime.parse("2024-01-10", "%Y-%m-%d");
    var diff = DateTime.diffDays(start, end);
    println(diff.toString()); // Output: 9
}
```

---

### `DateTime.diffHours(ts1: int64, ts2: int64) :int64`

Returns the difference between two timestamps in whole hours (absolute value).

- **Parameters:**
  - `ts1: int64` — the first timestamp in milliseconds.
  - `ts2: int64` — the second timestamp in milliseconds.
- **Returns:** `int64` — the number of hours between the two timestamps.

```hoo
func :void example() {
    var start = DateTime.parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    var end = DateTime.parse("2024-01-02 12:00:00", "%Y-%m-%d %H:%M:%S");
    var diff = DateTime.diffHours(start, end);
    println(diff.toString()); // Output: 36
}
```

---

### `DateTime.diffSeconds(ts1: int64, ts2: int64) :double`

Returns the difference between two timestamps in seconds (may include fractional part).

- **Parameters:**
  - `ts1: int64` — the first timestamp in milliseconds.
  - `ts2: int64` — the second timestamp in milliseconds.
- **Returns:** `double` — the number of seconds between the two timestamps.

```hoo
var start = DateTime.parse("2024-01-01 00:00:00", "%Y-%m-%d %H:%M:%S")
var end = DateTime.parse("2024-01-01 00:01:30", "%Y-%m-%d %H:%M:%S")
var diff = DateTime.diffSeconds(start, end)  // 90.0
```

## 5. Comparison

### `DateTime.compare(ts1: int64, ts2: int64) :int64`

Compares two timestamps.

- **Parameters:**
  - `ts1: int64` — the first timestamp in milliseconds.
  - `ts2: int64` — the second timestamp in milliseconds.
- **Returns:** `int64` — `-1` if `ts1 < ts2`, `0` if `ts1 == ts2`, `1` if `ts1 > ts2`.

```hoo
func :void example() {
    var early = DateTime.parse("2024-01-01", "%Y-%m-%d");
    var late = DateTime.parse("2024-06-15", "%Y-%m-%d");
    var cmp = DateTime.compare(early, late);
    println(cmp.toString()); // Output: -1
}
```

## Usage Example

```hoo
func :int64 main() {
    var now = DateTime.now();
    println("Current time: ".concat(DateTime.iso8601(now)));

    var formatted = DateTime.format(now, "%A, %B %d, %Y");
    println("Formatted: ".concat(formatted));

    var tomorrow = DateTime.addDays(now, 1);
    println("Tomorrow: ".concat(DateTime.iso8601(tomorrow)));

    var parsed = DateTime.fromIso8601("2024-12-25T00:00:00Z");
    var daysUntil = DateTime.diffDays(now, parsed);
    println("Days until Christmas: ".concat(daysUntil.toString()));

    return 0;
}
```
