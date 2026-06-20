# DateTime API Reference

## Module

`hoo`

## Import Statement

```hoo
import hoo;
```

## Module Description

The `DateTime` class is an immutable wrapper around a Unix epoch timestamp (milliseconds) for representing and manipulating dates and times. All arithmetic and mutation operations return a new `DateTime` instance; the original is not modified. Instances are ARC-managed. The companion `DateTimeFields` structure provides decomposed date and time field access.

## Class: DateTime

### Declaration

```hoo
class DateTime
```

### Public Fields

None. DateTime instances wrap a single `timestamp: int64` field (milliseconds since the Unix epoch).

### Public Class (Static) Functions

#### `DateTime`

Creates a new `DateTime` instance from individual date and time components.

**Syntax:**

```hoo
DateTime(year: int64, month: int64, day: int64, hour: int64, minute: int64, second: int64, millisecond: int64) :DateTime
```

**Parameters:**

| Parameter    | Type    | Description            |
|--------------|---------|------------------------|
| `year`       | `int64` | Year (e.g. 2024).     |
| `month`      | `int64` | Month (1–12).          |
| `day`        | `int64` | Day (1–31).            |
| `hour`       | `int64` | Hour (0–23).           |
| `minute`     | `int64` | Minute (0–59).         |
| `second`     | `int64` | Second (0–59).         |
| `millisecond`| `int64` | Millisecond (0–999).   |

**Returns:** `DateTime` — A new `DateTime` instance for the given date and time.

**Errors:** Returns `null` if the field values are out of range.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime(2024, 6, 15, 10, 30, 0, 0);
    println(dt.to_iso8601());
    dt.release();
    return 0;
}
```

---

#### `from_iso8601`

Parses an ISO 8601 date-time string and returns a `DateTime` instance.

**Syntax:**

```hoo
DateTime.from_iso8601(str: string) :DateTime
```

**Parameters:**

| Parameter | Type     | Description                              |
|-----------|----------|------------------------------------------|
| `str`     | `string` | ISO 8601 string (e.g. `"2024-01-15T10:30:00Z"`). |

**Returns:** `DateTime` — A new `DateTime` instance, or `null` on parse failure.

**Errors:** Returns `null` if the string is not valid ISO 8601. No exception is thrown.

**Supported ISO 8601 variants:**
- `2024-01-15T10:30:00Z`
- `2024-01-15T10:30:00.123Z`
- `2024-01-15T10:30:00+05:30`
- `2024-01-15T10:30:00`

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.from_iso8601("2024-12-25T00:00:00Z");
    if (dt) {
        println(dt.to_iso8601());
        dt.release();
    }
    return 0;
}
```

---

#### `now`

Returns a `DateTime` instance representing the current system time (UTC).

**Syntax:**

```hoo
DateTime.now() :DateTime
```

**Parameters:**

None.

**Returns:** `DateTime` — A new `DateTime` instance at the current UTC time.

**Errors:** None (always succeeds).

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    println(dt.to_iso8601());
    dt.release();
    return 0;
}
```

### Public Instance Functions

#### `to_iso8601`

Formats the DateTime as an ISO 8601 string.

**Syntax:**

```hoo
dt.to_iso8601() :string
```

**Parameters:**

None.

**Returns:** `string` — The ISO 8601 formatted string (e.g. `"2024-01-15T10:30:00.000Z"`).

**Errors:** Returns an empty string for a null DateTime handle.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    var iso = dt.to_iso8601();
    println(iso);
    dt.release();
    return 0;
}
```

---

#### `to_string`

Returns a simple string representation of the DateTime.

**Syntax:**

```hoo
dt.to_string() :string
```

**Parameters:**

None.

**Returns:** `string` — A string representation of the DateTime (equivalent to `to_iso8601()`).

**Errors:** Returns an empty string for a null DateTime handle.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    var s = dt.to_string();
    println(s);
    dt.release();
    return 0;
}
```

---

#### Field Accessors

Returns the individual date or time component of the DateTime.

**Syntax:**

```hoo
dt.year() :int64
dt.month() :int64
dt.day() :int64
dt.hour() :int64
dt.minute() :int64
dt.second() :int64
dt.millisecond() :int64
```

**Parameters:**

None.

**Returns:** `int64` — The requested field value. Ranges: year (full 4-digit), month (1–12), day (1–31), hour (0–23), minute (0–59), second (0–59), millisecond (0–999).

**Errors:** Returns `0` for each field if the DateTime handle is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime(2024, 6, 15, 10, 30, 45, 123);
    println(dt.year());  // 2024
    println(dt.month()); // 6
    println(dt.day());   // 15
    println(dt.hour());  // 10
    println(dt.minute());// 30
    println(dt.second());// 45
    println(dt.millisecond()); // 123
    dt.release();
    return 0;
}
```

---

#### `time_since_epoch`

Returns the raw Unix epoch timestamp of the DateTime.

**Syntax:**

```hoo
dt.time_since_epoch() :int64
```

**Parameters:**

None.

**Returns:** `int64` — The timestamp in milliseconds since the Unix epoch.

**Errors:** Returns `0` for a null DateTime handle.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    var ts = dt.time_since_epoch();
    println(ts);
    dt.release();
    return 0;
}
```

---

#### `is_before`

Checks whether this DateTime is earlier than another DateTime.

**Syntax:**

```hoo
dt.is_before(other: DateTime) :int64
```

**Parameters:**

| Parameter | Type       | Description                  |
|-----------|------------|------------------------------|
| `other`   | `DateTime` | The DateTime to compare against. |

**Returns:** `int64` — `1` if this DateTime is earlier than `other`, `0` otherwise.

**Errors:** Returns `0` if either handle is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var a = DateTime.from_iso8601("2024-01-01T00:00:00Z");
    var b = DateTime.from_iso8601("2024-06-15T00:00:00Z");
    println(a.is_before(b)); // 1
    a.release();
    b.release();
    return 0;
}
```

---

#### `is_after`

Checks whether this DateTime is later than another DateTime.

**Syntax:**

```hoo
dt.is_after(other: DateTime) :int64
```

**Parameters:**

| Parameter | Type       | Description                  |
|-----------|------------|------------------------------|
| `other`   | `DateTime` | The DateTime to compare against. |

**Returns:** `int64` — `1` if this DateTime is later than `other`, `0` otherwise.

**Errors:** Returns `0` if either handle is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var a = DateTime.from_iso8601("2024-06-15T00:00:00Z");
    var b = DateTime.from_iso8601("2024-01-01T00:00:00Z");
    println(a.is_after(b)); // 1
    a.release();
    b.release();
    return 0;
}
```

---

#### `difference_in_milliseconds`

Returns the signed difference in milliseconds between two DateTime instances.

**Syntax:**

```hoo
dt.difference_in_milliseconds(other: DateTime) :int64
```

**Parameters:**

| Parameter | Type       | Description                  |
|-----------|------------|------------------------------|
| `other`   | `DateTime` | The DateTime to compare against. |

**Returns:** `int64` — The difference in milliseconds (positive if `dt > other`, negative if `dt < other`, zero if equal).

**Errors:** Returns `0` if either handle is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var a = DateTime.from_iso8601("2024-01-01T00:00:00Z");
    var b = DateTime.from_iso8601("2024-01-01T00:01:00Z");
    var diff = a.difference_in_milliseconds(b);
    println(diff); // -60000
    a.release();
    b.release();
    return 0;
}
```

---

#### `add_milliseconds`

Returns a new DateTime offset by the given number of milliseconds. The original instance is not modified.

**Syntax:**

```hoo
dt.add_milliseconds(ms: int64) :DateTime
```

**Parameters:**

| Parameter | Type    | Description                                       |
|-----------|---------|---------------------------------------------------|
| `ms`      | `int64` | Milliseconds to add (negative values subtract). |

**Returns:** `DateTime` — A new DateTime instance offset by `ms` milliseconds.

**Errors:** Returns `null` if `dt` is null or if allocation fails.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.from_iso8601("2024-01-01T00:00:00Z");
    var later = dt.add_milliseconds(5000);
    println(later.to_iso8601()); // 2024-01-01T00:00:05.000Z
    dt.release();
    later.release();
    return 0;
}
```

---

#### `subtract_milliseconds`

Returns a new DateTime offset backward by the given number of milliseconds. The original instance is not modified.

**Syntax:**

```hoo
dt.subtract_milliseconds(ms: int64) :DateTime
```

**Parameters:**

| Parameter | Type    | Description                                       |
|-----------|---------|---------------------------------------------------|
| `ms`      | `int64` | Milliseconds to subtract (negative values add). |

**Returns:** `DateTime` — A new DateTime instance offset backward by `ms` milliseconds.

**Errors:** Returns `null` if `dt` is null or if allocation fails.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.from_iso8601("2024-01-01T00:00:05Z");
    var earlier = dt.subtract_milliseconds(5000);
    println(earlier.to_iso8601()); // 2024-01-01T00:00:00.000Z
    dt.release();
    earlier.release();
    return 0;
}
```

---

#### `retain`

Increments the reference count of a DateTime instance.

**Syntax:**

```hoo
dt.retain()
```

**Parameters:**

None.

**Returns:** `void`

**Errors:** No-op if `dt` is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    dt.retain();
    dt.release();
    dt.release();
    return 0;
}
```

---

#### `release`

Decrements the reference count of a DateTime instance. When the count reaches zero the instance is freed. After calling `release`, the handle must not be used again.

**Syntax:**

```hoo
dt.release()
```

**Parameters:**

None.

**Returns:** `void`

**Errors:** No-op if `dt` is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    dt.release();
    return 0;
}
```

---

#### `refcount`

Returns the current reference count of a DateTime instance. Intended for debugging and testing.

**Syntax:**

```hoo
dt.refcount() :int64
```

**Parameters:**

None.

**Returns:** `int64` — The current reference count.

**Errors:** Returns `0` if `dt` is null.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    dt.retain();
    println(dt.refcount()); // 2
    dt.release();
    dt.release();
    return 0;
}
```

---

#### `free_string`

Frees a string returned by a DateTime method. Use this to release memory for strings that must be explicitly freed.

**Syntax:**

```hoo
dt.free_string(str: string) :void
```

**Parameters:**

| Parameter | Type     | Description                               |
|-----------|----------|-------------------------------------------|
| `str`     | `string` | The string returned by a DateTime method to free. |

**Returns:** `void`

**Errors:** No-op if `str` is null or empty.

**Complete Example:**

```hoo
import hoo;

func :int64 main() {
    var dt = DateTime.now();
    var s = dt.to_iso8601();
    println(s);
    dt.free_string(s);
    dt.release();
    return 0;
}
```

## Struct: DateTimeFields

`DateTimeFields` is a value-type structure returned by the DateTime decompose operation. It contains the individual date and time fields broken out from the internal timestamp.

### Declaration

```hoo
struct DateTimeFields {
    year: int64,
    month: int64,
    day: int64,
    hour: int64,
    minute: int64,
    second: int64,
    millisecond: int64,
    weekday: int64,
    yearday: int64
}
```

### Fields

| Field         | Type    | Description                        |
|---------------|---------|------------------------------------|
| `year`        | `int64` | Full year (e.g. 2024).             |
| `month`       | `int64` | Month (1–12).                      |
| `day`         | `int64` | Day (1–31).                        |
| `hour`        | `int64` | Hour (0–23).                       |
| `minute`      | `int64` | Minute (0–59).                     |
| `second`      | `int64` | Second (0–59).                     |
| `millisecond` | `int64` | Millisecond (0–999).               |
| `weekday`     | `int64` | Day of week (0=Sunday, 6=Saturday).|
| `yearday`     | `int64` | Day of year (0–365).               |

## Usage Example

```hoo
import hoo;

func :int64 main() {
    // Current time
    var now = DateTime.now();
    println(now.to_iso8601());

    // Parse ISO 8601
    var christmas = DateTime.from_iso8601("2024-12-25T00:00:00Z");
    if (christmas) {
        var diff = now.difference_in_milliseconds(christmas);
        println("ms until Christmas: " + diff.toString());
        christmas.release();
    }

    // Construct from components
    var dt = DateTime(2024, 1, 1, 0, 0, 0, 0);
    var later = dt.add_milliseconds(86400000); // +1 day
    println(later.to_iso8601()); // 2024-01-02T00:00:00.000Z

    // Comparison
    println(now.is_before(later)); // 1

    // Field accessors
    println("Year: " + dt.year().toString());

    // Cleanup
    dt.release();
    later.release();
    now.release();

    return 0;
}
```
