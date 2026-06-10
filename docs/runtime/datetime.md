# Date & Time (`DateTime`)

The `DateTime` class provides current time, decompose/compose fields, ISO 8601 formatting, and duration arithmetic via `<chrono>`.

All timestamps are `int64_t` representing **milliseconds since Unix epoch** (1970-01-01 UTC). Functions returning `String` are ARC-managed and do not require manual freeing.

## 1. DateTime Fields

```c
typedef struct {
    int64_t year;
    int64_t month;       // 1-12
    int64_t day;         // 1-31
    int64_t hour;        // 0-23
    int64_t minute;      // 0-59
    int64_t second;      // 0-59
    int64_t millisecond; // 0-999
    int64_t weekday;     // 0=Sunday, 1=Monday, ..., 6=Saturday
    int64_t yearday;     // 0-365
} HooDateTimeFields;
```

## 2. Current Time

- `DateTime.now()` — Current time as Unix epoch milliseconds.
- `DateTime.nowSeconds()` — Current time as Unix epoch seconds.
- `DateTime.nowPrecise()` — Current time as `double` seconds since epoch.

## 3. Decompose / Compose

- `DateTime.decompose(epoch_ms)` — Break timestamp into `DateTimeFields` (local time).
- `DateTime.compose(fields)` — Rebuild timestamp from fields (local time).
- `DateTime.composeUtc(fields)` — Rebuild timestamp from fields (UTC).

## 4. Formatting & Parsing

- `ts.format(format)` — strftime-style format on a timestamp. Supported specifiers: `%Y`, `%m`, `%d`, `%H`, `%M`, `%S`, `%f` (milliseconds), `%w` (weekday), `%j` (yearday).
- `DateTime.parse(str, format)` — Parse string with strftime-style format, returns milliseconds or -1.
- `ts.iso8601()` — Format as ISO 8601 (`"2024-01-15T10:30:00Z"`).
- `DateTime.fromIso8601(str)` — Parse ISO 8601 string, returns milliseconds or -1.

## 5. Duration Helpers

- `ts.addDays(days)` — Add days (can be negative).
- `ts.addHours(hours)`
- `ts.addMinutes(minutes)`
- `ts.addSeconds(seconds)`
- `ts.addMilliseconds(ms)`
- `from.diffDays(to)` — Difference in days.
- `from.diffHours(to)` — Difference in hours.
- `from.diffSeconds(to)` — Difference as `double` (fractional seconds).

## 6. Comparison

- `a.compare(b)` — Returns -1, 0, or 1.

## Usage from Hoo Source

All `DateTime` methods are accessed via the class or instance:

```hoo
func :int64 demo() {
    var now = DateTime.now();                        // ms since Unix epoch
    var iso = now.iso8601();                         // "2024-01-15T10:30:00.000Z"
    var formatted = now.format("%Y-%m-%d");
    var parsed = DateTime.fromIso8601("2024-01-15T10:30:00Z");
    var later = now.addDays(7);
    var cmp = now.compare(later);                    // -1, 0, or 1
    return formatted.length();                       // 10 for "2024-01-15"
}
```

## Memory Management

All `DateTime` methods return ARC-managed `String` objects that do not require manual freeing.
