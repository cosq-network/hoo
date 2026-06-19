# Date & Time (`DateTime`)

The `DateTime` class is an instantiable ARC-managed class (type ID 119) that wraps
a Unix epoch timestamp (milliseconds). Each instance is a heap object with a single
`int64 timestamp` field, making it eligible for the `serializable` modifier.

## Object Model

| Property | Value |
|----------|-------|
| Type ID  | 119   |
| Fields   | `timestamp: int64` (PUBLIC, offset 0) |
| Memory   | ARC-managed heap object (16-byte header + 8-byte payload) |
| Serialization | Compatible via `serializable` modifier (all fields are valid serializable types) |

## Construction

| Expression | Returns | Notes |
|---|---|---|
| `DateTime.now()` | `DateTime` | Current system time |
| `DateTime.parse(str, fmt)` | `DateTime` | Parse with strftime-style format |
| `DateTime.from_iso8601(str)` | `DateTime` | Parse ISO 8601 string |
| `DateTime.now_seconds()` | `int64` | Raw seconds (static utility) |
| `DateTime.now_precise()` | `double` | Raw seconds with sub-ms precision |

## Instance Methods

All arithmetic methods return a new DateTime (original is unmodified).

| Method | Returns | Description |
|---|---|---|
| `dt.getTimestamp()` | `int64` | Raw timestamp in ms |
| `dt.format(fmt)` | `string` | strftime-style format |
| `dt.iso8601()` | `string` | ISO 8601 string |
| `dt.addDays(n)` | `DateTime` | Add days (negative subtracts) |
| `dt.addHours(n)` | `DateTime` | Add hours |
| `dt.addMinutes(n)` | `DateTime` | Add minutes |
| `dt.addSeconds(n)` | `DateTime` | Add seconds |
| `dt.addMilliseconds(ms)` | `DateTime` | Add milliseconds |
| `a.diffDays(b)` | `int64` | Difference in days |
| `a.diffHours(b)` | `int64` | Difference in hours |
| `a.diffSeconds(b)` | `double` | Fractional difference in seconds |
| `a.compare(b)` | `int64` | -1, 0, or 1 |

## Format Specifiers

`%Y` `%m` `%d` `%H` `%M` `%S` `%f` (ms) `%w` (weekday 0-6) `%j` (yearday 0-365)

## Example

```hoo
func :int64 demo() {
    var now = DateTime.now();
    var iso = now.iso8601();                         // "2024-01-15T10:30:00.000Z"
    var formatted = now.format("%Y-%m-%d");          // "2024-01-15"
    var later = now.addDays(7);
    var cmp = now.compare(later);                    // -1
    return now.diffDays(later);                      // 7
}
```

## C API

The C implementation layer provides three tiers of functions:

- **Instance API** (`hoo_datetime_new`, `hoo_datetime_instance_format`, etc.) —
  ARC-managed DateTime handle operations. All functions take a `void* dt` handle
  as their first parameter where applicable. These are used by the JIT bridges.

- **Free function API** (`hoo_datetime_now`, `hoo_datetime_format(dt, fmt)`, etc.) —
  Thin wrappers that delegate to the instance API. Name-matched to the Hoo-language
  `datetime_*()` free functions.

- **Time utilities** (`hoo_datetime_now_seconds`, `hoo_datetime_now_precise`) —
  Return raw `int64`/`double` values without creating a DateTime instance.

No raw-int64 timestamp functions are exposed in the public API; all timestamp
arithmetic operates through DateTime handles.
