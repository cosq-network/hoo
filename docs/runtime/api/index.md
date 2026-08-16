# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`).
This documentation is designed for hoo developers to understand and utilize the
built-in capabilities of the language.

The hoo runtime provides modules that bridge the high-level language with the
host system, offering efficient implementations of common data structures,
mathematical operations, network communication, and system interactions.

## Import Requirements

To prevent namespace pollution and avoid symbol ambiguity, hoo enforces compile-time verification of import statements when using standard library APIs:

- **Core types**: Types such as `String`, `Array`, `Map`, `Any`, `Exception`, and `Thread` are in the `hoo` core module — `import hoo;` is sufficient.
- **Submodules**: Types in submodules such as `hoo.io`, `hoo.math`, `hoo.encoding`, `hoo.json`, `hoo.net`, `hoo.buffer`, `hoo.character`, `hoo.collections`, `hoo.compression`, `hoo.path`, `hoo.datetime`, `hoo.uuid`, `hoo.regex`, `hoo.args`, `hoo.process`, `hoo.system`, and `hoo.csv` require their specific import path.

## Usage Patterns

hoo runtime APIs follow one of three patterns:

- **Instance class** — create with `Class(args)`, call methods on the variable
- **Free functions** — namespace-prefixed functions with no class wrapper
- **Global functions** — available without any prefix

---

## [Strings](string.md) — Class `String`

**Import:** `import hoo;`

**Pattern:** Instance class + free functions

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `String` | `String() :string` |
| Free | `string_repeat` | `string_repeat(ch: char, count: int64) :string` |
| Free | `string_from_int64` | `string_from_int64(val: int64) :string` |
| Free | `string_from_double` | `string_from_double(val: double) :string` |
| Free | `string_join` | `string_join(parts: array) :string` |
| Instance | `s.concat` | `s.concat(other: string) :string` |
| Instance | `s.substring` | `s.substring(start: int64, length: int64) :string` |
| Instance | `s.toUpper` | `s.toUpper() :string` |
| Instance | `s.toLower` | `s.toLower() :string` |
| Instance | `s.trim` | `s.trim() :string` |
| Instance | `s.replace` | `s.replace(old: string, replacement: string) :string` |
| Instance | `s.split` | `s.split(delim: string) :array` |
| Instance | `s.length` | `s.length() :int64` |
| Instance | `s.byteAt` | `s.byteAt(index: int64) :int64` |
| Instance | `s.indexOf` | `s.indexOf(needle: string) :int64` |
| Instance | `s.lastIndexOf` | `s.lastIndexOf(needle: string) :int64` |
| Instance | `s.contains` | `s.contains(needle: string) :int64` |
| Instance | `s.startsWith` | `s.startsWith(prefix: string) :int64` |
| Instance | `s.endsWith` | `s.endsWith(suffix: string) :int64` |
| Instance | `s.compare` | `s.compare(other: string) :int64` |
| Instance | `s.equals` | `s.equals(other: string) :int64` |
| Instance | `s.equalsIgnoreCase` | `s.equalsIgnoreCase(other: string) :int64` |
| Instance | `s.toInt64` | `s.toInt64() :int64` |
| Instance | `s.toDouble` | `s.toDouble() :double` |
| Instance | `s.is_empty` | `s.is_empty() :int64` |
| Instance | `s.toCharacters` | `s.toCharacters() :array` |

---

## [Buffer](buffer.md) — Class `Buffer`

**Import:** `import hoo.buffer;`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Buffer` | `Buffer(initial_capacity: int64) :Buffer` |
| Instance | `buf.write` | `buf.write(data: string) :void` |
| Instance | `buf.write_byte` | `buf.write_byte(byte: int64) :void` |
| Instance | `buf.clear` | `buf.clear() :void` |
| Instance | `buf.length` | `buf.length() :int64` |
| Instance | `buf.to_string` | `buf.to_string() :string` |
| Instance | `buf.retain` | `buf.retain() :Buffer` |
| Instance | `buf.release` | `buf.release() :void` |
| Instance | `buf.refcount` | `buf.refcount() :int64` |

---

## [Character](character.md) — Free Functions

**Import:** `import hoo.character;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `character_is_alpha` | `character_is_alpha(c: char) :int64` |
| `character_is_digit` | `character_is_digit(c: char) :int64` |
| `character_is_alnum` | `character_is_alnum(c: char) :int64` |
| `character_is_lower` | `character_is_lower(c: char) :int64` |
| `character_is_upper` | `character_is_upper(c: char) :int64` |
| `character_is_space` | `character_is_space(c: char) :int64` |
| `character_to_upper` | `character_to_upper(c: char) :char` |
| `character_to_lower` | `character_to_lower(c: char) :char` |
| `character_digit_to_int64` | `character_digit_to_int64(c: char) :int64` |
| `character_int64_to_digit` | `character_int64_to_digit(val: int64) :char` |

---

## [I/O](io.md) — Global Free Functions

**Import:** `import hoo.io;` (available globally with `import hoo;`)

**Pattern:** Global functions (no prefix)

| API | Signature |
|-----|-----------|
| `println` | `println(s: string) :void` |
| `print` | `print(s: string) :void` |
| `print_error` | `print_error(s: string) :void` |
| `readln` | `readln() :string` |
| `print_format` | `print_format(fmt: string, ...) :void` |

---

## [Math](math.md) — Free Functions + Class `Random`

**Import:** `import hoo.math;`

**Pattern:** Free functions + instance class

### Math — Basic Functions

`math_abs(x: double) :double`, `math_min(a: double, b: double) :double`,
`math_max(a: double, b: double) :double`, `math_clamp(val: double, min: double, max: double) :double`,
`math_min_int64(a: int64, b: int64) :int64`, `math_max_int64(a: int64, b: int64) :int64`

### Math — Rounding

`math_floor(x: double) :double`, `math_ceil(x: double) :double`, `math_round(x: double) :double`

### Math — Power & Roots

`math_sqrt(x: double) :double`, `math_pow(base: double, exp: double) :double`, `math_cbrt(x: double) :double`

### Math — Exponential & Logarithmic

`math_log(x: double) :double`, `math_log10(x: double) :double`, `math_log2(x: double) :double`,
`math_exp(x: double) :double`, `math_exp2(x: double) :double`

### Math — Trigonometric

`math_sin(x: double) :double`, `math_cos(x: double) :double`, `math_tan(x: double) :double`,
`math_asin(x: double) :double`, `math_acos(x: double) :double`, `math_atan(x: double) :double`,
`math_atan2(y: double, x: double) :double`, `math_sinh(x: double) :double`, `math_cosh(x: double) :double`,
`math_tanh(x: double) :double`, `math_asinh(x: double) :double`, `math_acosh(x: double) :double`,
`math_atanh(x: double) :double`

### Math — Statistics

`math_mean(data: array) :double`, `math_median(data: array) :double`,
`math_variance(data: array) :double`, `math_stddev(data: array) :double`

### Random

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Random` | `Random() :Random` |
| Instance | `rng.next_int64` | `rng.next_int64(max: int64) :int64` |
| Instance | `rng.next_double` | `rng.next_double() :double` |
| Instance | `rng.retain` | `rng.retain() :Random` |
| Instance | `rng.release` | `rng.release() :void` |
| Instance | `rng.refcount` | `rng.refcount() :int64` |

---

## [Collections](collections.md) — Classes `Array`, `Map`, `Dict`, `Any`, `List`, `Tensor`

**Import:** `import hoo;` for Array, Map, Any. `import hoo.collections;` for Dict, List, Tensor.

**Pattern:** Instance classes and instance methods

### Array

| Kind | API | Signature |
|------|-----|-----------|
| Instance | `arr.length` | `arr.length() :int64` |
| Instance | `arr.pushInt64` | `arr.pushInt64(val: int64) :array` |
| Instance | `arr.pushDouble` | `arr.pushDouble(val: double) :array` |
| Instance | `arr.pushString` | `arr.pushString(val: string) :array` |
| Instance | `arr.pushBool` | `arr.pushBool(val: bool) :array` |
| Instance | `arr.getInt64` | `arr.getInt64(index: int64) :int64` |
| Instance | `arr.getDouble` | `arr.getDouble(index: int64) :double` |
| Instance | `arr.getString` | `arr.getString(index: int64) :string` |
| Instance | `arr.getBool` | `arr.getBool(index: int64) :bool` |
| Instance | `arr.setInt64` | `arr.setInt64(index: int64, val: int64) :void` |
| Instance | `arr.setDouble` | `arr.setDouble(index: int64, val: double) :void` |
| Instance | `arr.setString` | `arr.setString(index: int64, val: string) :void` |
| Instance | `arr.sort` | `arr.sort() :array` |
| Instance | `arr.reverse` | `arr.reverse() :array` |
| Instance | `arr.empty` | `arr.empty() :int64` |
| Instance | `arr.clear` | `arr.clear() :void` |
| Instance | `arr.retain` / `arr.release` / `arr.refcount` | Reference counting |

### Map

| Kind | API | Signature |
|------|-----|-----------|
| Free | `map_new` | `map_new() :map` |
| Instance | `m.length` | `m.length() :int64` |
| Instance | `m.has` | `m.has(key) :int64` |
| Instance | `m.remove` | `m.remove(key) :int64` |
| Instance | `m.clear` | `m.clear() :void` |
| Instance | `m.keys` | `m.keys() :array` |
| Instance | `m.values` | `m.values() :array` |

Key-specific operations (int64, string, char, byte): `m.put(key, value)`, `m.get(key)`, `m.has(key)`, `m.remove(key)`

### Dict

| Kind | API | Signature |
|------|-----|-----------|
| Free | `hashmap_new` | `hashmap_new(size: int64) :Dict` |
| Instance | `hm.put` | `hm.put(key, value) :void` |
| Instance | `hm.get` | `hm.get(key)` |
| Instance | `hm.has` | `hm.has(key) :int64` |
| Instance | `hm.remove` | `hm.remove(key) :int64` |
| Instance | `hm.clear` | `hm.clear() :void` |
| Instance | `hm.length` | `hm.length() :int64` |
| Instance | `hm.key_type` | `hm.key_type() :int64` |
| Instance | `hm.value_type` | `hm.value_type() :int64` |
| Instance | `hm.keys` | `hm.keys() :array` |
| Instance | `hm.values` | `hm.values() :array` |
| Instance | `hm.retain` / `hm.release` / `hm.refcount` | Reference counting |

### Any

| Kind | API | Signature |
|------|-----|-----------|
| Instance | `any.is_null` | `any.is_null() :int64` |
| Instance | `any.is_array` | `any.is_array() :int64` |
| Instance | `any.is_string` | `any.is_string() :int64` |
| Instance | `any.is_int64` | `any.is_int64() :int64` |
| Instance | `any.is_double` | `any.is_double() :int64` |
| Instance | `any.is_bool` | `any.is_bool() :int64` |
| Instance | `any.as_int64` | `any.as_int64() :int64` |
| Instance | `any.as_double` | `any.as_double() :double` |
| Instance | `any.as_string` | `any.as_string() :string` |
| Instance | `any.as_bool` | `any.as_bool() :int64` |
| Instance | `any.as_array` | `any.as_array() :array` |

### List

| Kind | API | Signature |
|------|-----|-----------|
| Instance | `arr.length` | `arr.length() :int64` |
| Instance | `arr.push_back` | `arr.push_back(value) :void` |
| Instance | `arr.get` | `arr.get(index: int64)` |
| Instance | `arr.set` | `arr.set(index: int64, value) :void` |

### Tensor

| Kind | API | Signature |
|------|-----|-----------|
| Free | `tensor_new` | `tensor_new(dimensions: int64, ...) :Tensor` |
| Instance | `t.rank` | `t.rank() :int64` |
| Instance | `t.shape` | `t.shape() :array` |
| Instance | `t.num_elements` | `t.num_elements() :int64` |
| Instance | `t.get` | `t.get(indices: array) :double` |
| Instance | `t.set` | `t.set(indices: array, val: double) :void` |

---

## [DateTime](datetime.md) — Class `DateTime`

**Import:** `import hoo.datetime;`

**Pattern:** Instance class + free functions

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `DateTime` | `DateTime(year: int64, month: int64, day: int64, hour: int64, minute: int64, second: int64, millisecond: int64) :DateTime` |
| Free | `datetime_from_iso8601` | `datetime_from_iso8601(str: string) :DateTime` |
| Free | `datetime_now` | `datetime_now() :DateTime` |
| Instance | `dt.to_iso8601` | `dt.to_iso8601() :string` |
| Instance | `dt.to_string` | `dt.to_string() :string` |
| Instance | `dt.year` / `dt.month` / `dt.day` | Field accessors: `() :int64` |
| Instance | `dt.hour` / `dt.minute` / `dt.second` / `dt.millisecond` | Field accessors: `() :int64` |
| Instance | `dt.time_since_epoch` | `dt.time_since_epoch() :int64` |
| Instance | `dt.is_before` | `dt.is_before(other: DateTime) :int64` |
| Instance | `dt.is_after` | `dt.is_after(other: DateTime) :int64` |
| Instance | `dt.difference_in_milliseconds` | `dt.difference_in_milliseconds(other: DateTime) :int64` |
| Instance | `dt.add_milliseconds` | `dt.add_milliseconds(ms: int64) :DateTime` |
| Instance | `dt.subtract_milliseconds` | `dt.subtract_milliseconds(ms: int64) :DateTime` |
| Instance | `dt.retain` / `dt.release` / `dt.refcount` | Reference counting |
| Instance | `dt.free_string` | `dt.free_string(str: string) :void` |

---

## [Regex](regex.md) — Class `Regex`

**Import:** `import hoo.regex;`

**Pattern:** Instance class + free function

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Regex` | `Regex(pattern: string) :Regex` |
| Free | `regex_with_flags` | `regex_with_flags(pattern: string, flags: string) :Regex` |
| Instance | `re.matches` | `re.matches(text: string) :int64` |
| Instance | `re.is_match` | `re.is_match(text: string) :int64` |
| Instance | `re.find_all` | `re.find_all(text: string) :array` |
| Instance | `re.replace` | `re.replace(text: string, replacement: string) :string` |
| Instance | `re.capture` | `re.capture(text: string) :array` |
| Instance | `re.to_string` | `re.to_string() :string` |
| Instance | `re.retain` / `re.release` / `re.refcount` | Reference counting |
| Instance | `re.free_string` | `re.free_string(str: string) :void` |

---

## [Uuid](uuid.md) — Class `Uuid`

**Import:** `import hoo.uuid;`

**Pattern:** Instance class + free functions

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Uuid` | `Uuid(value: string) :Uuid` |
| Free | `uuid_v4` | `uuid_v4() :Uuid` |
| Free | `uuid_nil` | `uuid_nil() :Uuid` |
| Instance | `id.to_string` | `id.to_string() :string` |
| Instance | `id.equals` | `id.equals(other: Uuid) :int64` |
| Instance | `id.is_nil` | `id.is_nil() :int64` |
| Instance | `id.retain` / `id.release` / `id.refcount` | Reference counting |
| Instance | `id.free_string` | `id.free_string(str: string) :void` |

---

## [Fs](fs.md) — Free Functions

**Import:** `import hoo.io;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `fs_read_text` | `fs_read_text(path: string) :string` |
| `fs_read_text` | `fs_read_text(path: string, fallback: string) :string` |
| `fs_write_text` | `fs_write_text(path: string, data: string) :int64` |
| `fs_append_text` | `fs_append_text(path: string, data: string) :int64` |
| `fs_exists` | `fs_exists(path: string) :int64` |
| `fs_is_dir` | `fs_is_dir(path: string) :int64` |
| `fs_is_file` | `fs_is_file(path: string) :int64` |
| `fs_delete` | `fs_delete(path: string) :int64` |
| `fs_remove` | `fs_remove(path: string) :int64` |
| `fs_mkdir` | `fs_mkdir(path: string) :int64` |
| `fs_mkdirs` | `fs_mkdirs(path: string) :int64` |
| `fs_rmdir` | `fs_rmdir(path: string) :int64` |
| `fs_list_dir` | `fs_list_dir(path: string) :array` |
| `fs_current_dir` | `fs_current_dir() :string` |
| `fs_current_exe_dir` | `fs_current_exe_dir() :string` |
| `fs_temp_dir` | `fs_temp_dir() :string` |
| `fs_create_temp_file` | `fs_create_temp_file(prefix: string) :string` |
| `fs_create_temp_dir` | `fs_create_temp_dir() :string` |
| `fs_size` | `fs_size(path: string) :int64` |
| `fs_last_modified` | `fs_last_modified(path: string) :int64` |
| `fs_copy` | `fs_copy(source: string, dest: string) :int64` |
| `fs_move` | `fs_move(source: string, dest: string) :int64` |
| `fs_rename` | `fs_rename(old_path: string, new_path: string) :int64` |
| `fs_read_bytes` | `fs_read_bytes(path: string) :Buffer` |
| `fs_read_bytes` | `fs_read_bytes(path: string, fallback: Buffer) :Buffer` |
| `fs_read_bytes_buffer` | `fs_read_bytes_buffer(path: string) :Buffer` |
| `fs_read_bytes_buffer` | `fs_read_bytes_buffer(path: string, fallback: Buffer) :Buffer` |
| `fs_write_bytes` | `fs_write_bytes(path: string, buf: Buffer) :int64` |
| `fs_write_bytes_buffer` | `fs_write_bytes_buffer(path: string, buf: Buffer) :int64` |

---

## [Path](path.md) — Free Functions

**Import:** `import hoo.path;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `path_separator` | `path_separator() :char` |
| `path_join` | `path_join(path1: string, path2: string) :string` |
| `path_extension` | `path_extension(path: string) :string` |
| `path_stem` | `path_stem(path: string) :string` |
| `path_filename` | `path_filename(path: string) :string` |
| `path_parent` | `path_parent(path: string) :string` |
| `path_absolute` | `path_absolute(path: string) :string` |
| `path_normalize` | `path_normalize(path: string) :string` |
| `path_root` | `path_root(path: string) :string` |
| `path_relative` | `path_relative(path: string, base: string) :string` |
| `path_has_extension` | `path_has_extension(path: string) :int64` |
| `path_split` | `path_split(path: string) :array` |

---

## [Process](process.md) — Free Functions

**Import:** `import hoo.process;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `process_execute` | `process_execute(command: string) :string` |
| `process_capture` | `process_capture(command: string, input: string) :string` |
| `process_capture_status` | `process_capture_status(command: string, input: string) :array` |
| `process_exit` | `process_exit(exit_code: int64) :void` |
| `process_pid` | `process_pid() :int64` |

---

## [System](system.md) — Free Functions

**Import:** `import hoo.system;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `system_info` | `system_info() :string` |
| `system_env` | `system_env(name: string) :string` |
| `system_time_nanos` | `system_time_nanos() :int64` |
| `system_env_set` | `system_env_set(name: string, value: string) :int64` |
| `system_env_unset` | `system_env_unset(name: string) :int64` |
| `system_free_string` | `system_free_string(str: string) :void` |

---

## [Thread](thread.md) — Free Functions + Class `Mutex`

**Import:** `import hoo.thread;`

**Pattern:** Free functions + instance class

### Thread

| API | Signature |
|-----|-----------|
| `thread_spawn` | `thread_spawn(func: any) :int64` |
| `thread_sleep` | `thread_sleep(millis: int64) :void` |

### Mutex

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Mutex` | `Mutex() :Mutex` |
| Instance | `m.lock` | `m.lock() :void` |
| Instance | `m.unlock` | `m.unlock() :void` |
| Instance | `m.retain` / `m.release` / `m.refcount` | Reference counting |

---

## [Exception](exception.md) — Class `Exception`

**Import:** `import hoo;`

**Pattern:** Instance class + free functions

Exception types: `RuntimeException`, `NullPointerException`,
`IndexOutOfBoundsException`, `DivisionByZeroException`, `InvalidCastException`,
and custom subtypes.

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Exception` | `Exception(reason: string) :Exception` |
| Free | `exception_runtime` | `exception_runtime(message: string) :Exception` |
| Free | `exception_nullPointer` | `exception_nullPointer(message: string) :Exception` |
| Free | `exception_indexOutOfBounds` | `exception_indexOutOfBounds(message: string) :Exception` |
| Free | `exception_divisionByZero` | `exception_divisionByZero(message: string) :Exception` |
| Free | `exception_invalidCast` | `exception_invalidCast(message: string) :Exception` |
| Free | `exception_custom` | `exception_custom(typeName: string, message: string) :Exception` |
| Instance | `e.reason` | `e.reason() :string` |
| Instance | `e.typeId` | `e.typeId() :int64` |
| Instance | `e.typeName` | `e.typeName() :string` |
| Instance | `e.to_string` | `e.to_string() :string` |
| Instance | `e.stackTrace` | `e.stackTrace() :string` |
| Instance | `e.hasCause` | `e.hasCause() :int64` |
| Instance | `e.cause` | `e.cause() :Exception` |
| Instance | `e.frameCount` | `e.frameCount() :int64` |
| Instance | `e.frame` | `e.frame(index: int64) :string` |
| Instance | `e.print` | `e.print() :void` |
| Instance | `e.equals` | `e.equals(other: Exception) :int64` |
| Instance | `e.debug` | `e.debug() :string` |
| Instance | `e.retain` / `e.release` / `e.refcount` | Reference counting |

---

## [Args](args.md) — Class and Free Functions

**Import:** `import hoo.args;`

**Pattern:** `Args` instance methods plus free functions for raw positional access

| API | Signature |
|-----|-----------|
| `args_get` | `args_get(index: int64) :string` |
| `args_count` | `args_count() :int64` |
| `parser.has` | `parser.has(key: string) :int64` — 1 when the named argument was explicitly supplied, otherwise 0 |

---

## [Net](net.md) — Classes `Url`, `HttpClient`, `HttpResponse`

**Import:** `import hoo.net;`

**Pattern:** Instance classes

### Url

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Url` | `Url(url: string) :Url` |
| Instance | `url.to_string` | `url.to_string() :string` |
| Instance | `url.retain` / `url.release` | Reference counting |
| Instance | `url.free_string` | `url.free_string(str: string) :void` |

### HttpClient

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `HttpClient` | `HttpClient() :HttpClient` |
| Instance | `client.get` | `client.get(url: Url) :HttpResponse` |
| Instance | `client.post` | `client.post(url: Url, body: string) :HttpResponse` |
| Instance | `client.retain` / `client.release` | Reference counting |

### HttpResponse

| API | Signature |
|-----|-----------|
| `resp.status_code` | `resp.status_code() :int64` |
| `resp.status_text` | `resp.status_text() :string` |
| `resp.body` | `resp.body() :string` |
| `resp.retain` / `resp.release` | Reference counting |
| `resp.free_string` | `resp.free_string(str: string) :void` |

---

## [JSON](json.md) — Free Functions

**Import:** `import hoo.json;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `json_parse` | `json_parse(text: string) :any` |
| `json_stringify` | `json_stringify(value: any) :string` |
| `json_pretty` | `json_pretty(value: any) :string` |
| `json_free_string` | `json_free_string(str: string) :void` |

---

## [Encoding](encoding.md) — Free Functions

**Import:** `import hoo.encoding;`

**Pattern:** Free functions

### Base64

| API | Signature |
|-----|-----------|
| `encoding_base64_encode` | `encoding_base64_encode(data: string) :string` |
| `encoding_base64_decode` | `encoding_base64_decode(data: string) :string` |
| `encoding_base64_encode_buffer` | `encoding_base64_encode_buffer(data: Buffer) :string` |
| `encoding_base64_decode_buffer` | `encoding_base64_decode_buffer(data: string) :Buffer` |

### URL Encoding

| API | Signature |
|-----|-----------|
| `encoding_url_encode` | `encoding_url_encode(data: string) :string` |
| `encoding_url_decode` | `encoding_url_decode(data: string) :string` |

### Hex

| API | Signature |
|-----|-----------|
| `encoding_hex_encode` | `encoding_hex_encode(data: string) :string` |
| `encoding_hex_decode` | `encoding_hex_decode(data: string) :string` |

---

## [Hashing](hashing.md) — Free Functions

**Import:** `import hoo.hashing;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `hashing_crc32` | `hashing_crc32(data: string) :int64` |
| `hashing_md5` | `hashing_md5(data: string) :string` |
| `hashing_sha1` | `hashing_sha1(data: string) :string` |
| `hashing_sha256` | `hashing_sha256(data: string) :string` |

---

## [Compression](compression.md) — Free Functions

**Import:** `import hoo.compression;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `compression_compress` | `compression_compress(data: string) :string` |
| `compression_decompress` | `compression_decompress(data: string) :string` |
| `compression_compress_bytes` | `compression_compress_bytes(data: array) :array` |
| `compression_decompress_bytes` | `compression_decompress_bytes(data: array) :array` |

---

## [Csv](csv.md) — Class `Csv` + free constructor

**Import:** `import hoo.csv;`

**Pattern:** Instance class with free constructor and ARC methods

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Csv` | `new Csv() :Csv` |
| Free | `csv_from_opts` | `csv_from_opts(delimiter: int64, quote: int64) :Csv` |
| Instance | `csv.parse` / `csv.generate` | Parse rows or generate CSV text |
| Instance | `csv.readFile` / `csv.writeFile` | File I/O |
| Instance | `csv.escape` | `csv.escape(character: int64) :int64` |
| Instance | `csv.parseAsMaps` / `csv.readFileAsMaps` | Header-keyed map rows |
| Instance | `csv.count` / `csv.sum` / `csv.avg` / `csv.min` / `csv.max` | Map-row aggregations |
| Instance | `csv.select` / `csv.filter` / `csv.sort` | Map-row transformations |
| Instance | `csv.describe` | Numeric column statistics |
| Instance | `csv.retain` / `csv.release` / `csv.refcount` | Reference counting |
