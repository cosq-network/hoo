# Date & Time (`hoo.datetime`)

The `hoo.datetime` module provides current time, decompose/compose fields, ISO 8601 formatting, and duration arithmetic via `<chrono>`.

All timestamps are `int64_t` representing **milliseconds since Unix epoch** (1970-01-01 UTC). Functions returning `char*` allocate strings that the caller must free with `hoo_datetime_free_string`.

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

- `hoo_datetime_now()` — Current time as Unix epoch milliseconds.
- `hoo_datetime_now_seconds()` — Current time as Unix epoch seconds.
- `hoo_datetime_now_precise()` — Current time as `double` seconds since epoch.

## 3. Decompose / Compose

- `hoo_datetime_decompose(epoch_ms)` — Break timestamp into `HooDateTimeFields` (local time).
- `hoo_datetime_compose(fields)` — Rebuild timestamp from fields (local time).
- `hoo_datetime_compose_utc(fields)` — Rebuild timestamp from fields (UTC).

## 4. Formatting & Parsing

- `hoo_datetime_format(epoch_ms, format)` — strftime-style format. Supported specifiers: `%Y`, `%m`, `%d`, `%H`, `%M`, `%S`, `%f` (milliseconds), `%w` (weekday), `%j` (yearday).
- `hoo_datetime_parse(str, format)` — Parse string with strftime-style format, returns milliseconds or -1.
- `hoo_datetime_iso8601(epoch_ms)` — Format as ISO 8601 (`"2024-01-15T10:30:00Z"`).
- `hoo_datetime_from_iso8601(str)` — Parse ISO 8601 string, returns milliseconds or -1.

## 5. Duration Helpers

- `hoo_datetime_add_days(epoch_ms, days)` — Add days (can be negative).
- `hoo_datetime_add_hours(epoch_ms, hours)`
- `hoo_datetime_add_minutes(epoch_ms, minutes)`
- `hoo_datetime_add_seconds(epoch_ms, seconds)`
- `hoo_datetime_add_milliseconds(epoch_ms, ms)`
- `hoo_datetime_diff_days(from, to)` — Difference in days.
- `hoo_datetime_diff_hours(from, to)` — Difference in hours.
- `hoo_datetime_diff_seconds(from, to)` — Difference as `double` (fractional seconds).

## 6. Comparison

- `hoo_datetime_compare(a, b)` — Returns -1, 0, or 1.

## Memory Management

Allocated strings must be freed with `hoo_datetime_free_string(str)`.
