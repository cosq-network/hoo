# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`).
This documentation is designed for hoo developers to understand and utilize the
built-in capabilities of the language.

The hoo runtime provides modules that bridge the high-level language with the
host system, offering efficient implementations of common data structures,
mathematical operations, network communication, and system interactions.

## Import Requirements

To prevent namespace pollution and avoid symbol ambiguity, hoo enforces compile-time verification of import statements when using standard library APIs:

- **Core types**: Types such as `String`, `Array`, `Map`, `Any`, `Exception`, `DateTime`, `Uuid`, `Regex`, `Args`, `Process`, and `Thread` are in the `hoo` core module — `import hoo;` is sufficient.
- **Submodules**: Types in submodules such as `hoo.io`, `hoo.math`, `hoo.encoding`, `hoo.json`, `hoo.net`, `hoo.buffer`, `hoo.character`, `hoo.collections`, and `hoo.compression` require their specific import path.

## Usage Patterns

hoo runtime APIs follow one of four patterns:

- **Instance class** — create with `Class(args)`, call methods on the variable
- **Static class** — call methods directly on the class name (e.g. `Math.abs(x)`)
- **Free functions** — namespace-prefixed functions with no class wrapper
- **Global functions** — available without any prefix

---

## [Strings](string.md) — Class `String`

**Import:** `import hoo;`

**Pattern:** Instance class + static class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `String` | `String()` |
| Static | `String.repeat` | `String.repeat(ch: char, count: int64) :string` |
| Static | `String.fromInt64` | `String.fromInt64(val: int64) :string` |
| Static | `String.fromDouble` | `String.fromDouble(val: double) :string` |
| Static | `String.join` | `String.join(parts: array) :string` |
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
| Constructor | `Buffer` | `Buffer(initial_capacity: int64 = 0) :Buffer` |
| Instance | `buf.write` | `buf.write(data: string) :void` |
| Instance | `buf.write_byte` | `buf.write_byte(byte: int64) :void` |
| Instance | `buf.clear` | `buf.clear() :void` |
| Instance | `buf.length` | `buf.length() :int64` |
| Instance | `buf.to_string` | `buf.to_string() :string` |
| Instance | `buf.retain` | `buf.retain() :Buffer` |
| Instance | `buf.release` | `buf.release() :void` |
| Instance | `buf.refcount` | `buf.refcount() :int64` |

---

## [Character](character.md) — Class `Character`

**Import:** `import hoo.character;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Character.is_alpha` | `Character.is_alpha(c: char) :int64` |
| Static | `Character.is_digit` | `Character.is_digit(c: char) :int64` |
| Static | `Character.is_alnum` | `Character.is_alnum(c: char) :int64` |
| Static | `Character.is_lower` | `Character.is_lower(c: char) :int64` |
| Static | `Character.is_upper` | `Character.is_upper(c: char) :int64` |
| Static | `Character.is_space` | `Character.is_space(c: char) :int64` |
| Static | `Character.to_upper` | `Character.to_upper(c: char) :char` |
| Static | `Character.to_lower` | `Character.to_lower(c: char) :char` |
| Static | `Character.digit_to_int64` | `Character.digit_to_int64(c: char) :int64` |
| Static | `Character.int64_to_digit` | `Character.int64_to_digit(val: int64) :char` |

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

**Import:** `import hoo;`

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

## [Collections](collections.md) — Classes `Array`, `Map`, `HashMap`, `Any`, `AnyArray`, `Tensor`

**Import:** `import hoo;` for Array, Map, Any. `import hoo.collections;` for HashMap, AnyArray, Tensor.

**Pattern:** Instance classes

### Array

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Array.new` | `Array.new(capacity: int64 = 0) :array` |
| Static | `Array.new_int64` | `Array.new_int64(capacity: int64 = 0, default_value: int64 = 0) :array` |
| Static | `Array.new_double` | `Array.new_double(capacity: int64 = 0, default_value: double = 0.0) :array` |
| Static | `Array.new_string` | `Array.new_string(capacity: int64 = 0) :array` |
| Instance | `arr.length` | `arr.length() :int64` |
| Instance | `arr.push_int64` | `arr.push_int64(val: int64) :array` |
| Instance | `arr.push_double` | `arr.push_double(val: double) :array` |
| Instance | `arr.push_string` | `arr.push_string(val: string) :array` |
| Instance | `arr.get_int64` | `arr.get_int64(index: int64) :int64` |
| Instance | `arr.get_double` | `arr.get_double(index: int64) :double` |
| Instance | `arr.get_string` | `arr.get_string(index: int64) :string` |
| Instance | `arr.set_int64` | `arr.set_int64(index: int64, val: int64) :void` |
| Instance | `arr.set_double` | `arr.set_double(index: int64, val: double) :void` |
| Instance | `arr.set_string` | `arr.set_string(index: int64, val: string) :void` |
| Instance | `arr.clear` | `arr.clear() :void` |
| Instance | `arr.retain` / `arr.release` / `arr.refcount` | Reference counting |

### Map

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Map.new` | `Map.new() :map` |
| Instance | `m.length` | `m.length() :int64` |
| Instance | `m.has` | `m.has(key) :int64` |
| Instance | `m.remove` | `m.remove(key) :int64` |
| Instance | `m.clear` | `m.clear() :void` |
| Instance | `m.keys` | `m.keys() :array` |
| Instance | `m.values` | `m.values() :array` |

Key-specific operations (int64, string, char, byte): `m.put(key, value)`, `m.get(key)`, `m.has(key)`, `m.remove(key)`

### HashMap

| Kind | API | Signature |
|------|-----|-----------|
| Static | `HashMap.new` | `HashMap.new(size: int64 = 16) :HashMap` |
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

### AnyArray

| Kind | API | Signature |
|------|-----|-----------|
| Instance | `arr.length` | `arr.length() :int64` |
| Instance | `arr.push_back` | `arr.push_back(value) :void` |
| Instance | `arr.get` | `arr.get(index: int64)` |
| Instance | `arr.set` | `arr.set(index: int64, value) :void` |

### Tensor

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Tensor.new` | `Tensor.new(dimensions: int64, ...) :Tensor` |
| Instance | `t.rank` | `t.rank() :int64` |
| Instance | `t.shape` | `t.shape() :array` |
| Instance | `t.num_elements` | `t.num_elements() :int64` |
| Instance | `t.get` | `t.get(indices: array) :double` |
| Instance | `t.set` | `t.set(indices: array, val: double) :void` |

---

## [DateTime](datetime.md) — Class `DateTime`

**Import:** `import hoo;`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `DateTime` | `DateTime(year: int64, month: int64, day: int64, hour: int64, minute: int64, second: int64, millisecond: int64) :DateTime` |
| Static | `DateTime.from_iso8601` | `DateTime.from_iso8601(str: string) :DateTime` |
| Static | `DateTime.now` | `DateTime.now() :DateTime` |
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

**Import:** `import hoo;`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Regex` | `Regex(pattern: string) :Regex` |
| Static | `Regex.with_flags` | `Regex.with_flags(pattern: string, flags: string) :Regex` |
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

**Import:** `import hoo;`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Uuid` | `Uuid(value: string) :Uuid` |
| Static | `Uuid.v4` | `Uuid.v4() :Uuid` |
| Static | `Uuid.nil` | `Uuid.nil() :Uuid` |
| Instance | `id.to_string` | `id.to_string() :string` |
| Instance | `id.equals` | `id.equals(other: Uuid) :int64` |
| Instance | `id.is_nil` | `id.is_nil() :int64` |
| Instance | `id.retain` / `id.release` / `id.refcount` | Reference counting |
| Instance | `id.free_string` | `id.free_string(str: string) :void` |

---

## [Fs](fs.md) — Class `Fs`

**Import:** `import hoo.io;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Fs.read` | `Fs.read(path: string) :string` |
| Static | `Fs.write` | `Fs.write(path: string, data: string) :void` |
| Static | `Fs.append` | `Fs.append(path: string, data: string) :void` |
| Static | `Fs.exists` | `Fs.exists(path: string) :int64` |
| Static | `Fs.is_dir` | `Fs.is_dir(path: string) :int64` |
| Static | `Fs.is_file` | `Fs.is_file(path: string) :int64` |
| Static | `Fs.delete` | `Fs.delete(path: string) :int64` |
| Static | `Fs.mkdir` | `Fs.mkdir(path: string) :int64` |
| Static | `Fs.mkdirs` | `Fs.mkdirs(path: string) :int64` |
| Static | `Fs.rmdir` | `Fs.rmdir(path: string) :int64` |
| Static | `Fs.list` | `Fs.list(path: string) :array` |
| Static | `Fs.cwd` | `Fs.cwd() :string` |
| Static | `Fs.size` | `Fs.size(path: string) :int64` |
| Static | `Fs.copy` | `Fs.copy(source: string, dest: string) :int64` |
| Static | `Fs.move` | `Fs.move(source: string, dest: string) :int64` |
| Static | `Fs.read_bytes` | `Fs.read_bytes(path: string) :array` |
| Static | `Fs.read_bytes_buffer` | `Fs.read_bytes_buffer(path: string, buffer: Buffer, offset: int64, length: int64) :int64` |

---

## [Path](path.md) — Class `Path`

**Import:** `import hoo.io;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Path.separator` | `Path.separator() :char` |
| Static | `Path.join` | `Path.join(path1: string, path2: string) :string` |
| Static | `Path.extension` | `Path.extension(path: string) :string` |
| Static | `Path.stem` | `Path.stem(path: string) :string` |
| Static | `Path.filename` | `Path.filename(path: string) :string` |
| Static | `Path.parent` | `Path.parent(path: string) :string` |
| Static | `Path.absolute` | `Path.absolute(path: string) :string` |
| Static | `Path.normalize` | `Path.normalize(path: string) :string` |
| Static | `Path.root` | `Path.root(path: string) :string` |
| Static | `Path.relative` | `Path.relative(path: string, base: string) :string` |
| Static | `Path.has_extension` | `Path.has_extension(path: string) :int64` |
| Static | `Path.split` | `Path.split(path: string) :array` |

---

## [Process](process.md) — Class `Process`

**Import:** `import hoo;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Process.execute` | `Process.execute(command: string) :string` |
| Static | `Process.capture` | `Process.capture(command: string, input: string) :string` |
| Static | `Process.capture_status` | `Process.capture_status(command: string, input: string) :array` |
| Static | `Process.exit` | `Process.exit(exit_code: int64) :void` |
| Static | `Process.pid` | `Process.pid() :int64` |

---

## [System](system.md) — Free Functions

**Import:** `import hoo;`

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

## [Thread](thread.md) — Class `Thread` + Class `Mutex`

**Import:** `import hoo;`

**Pattern:** Static class + instance class

### Thread

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Thread.spawn` | `Thread.spawn(func: any) :int64` |
| Static | `Thread.sleep` | `Thread.sleep(millis: int64) :void` |

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

**Pattern:** Instance class

Exception types: `RuntimeException`, `NullPointerException`,
`IndexOutOfBoundsException`, `DivisionByZeroException`, `InvalidCastException`,
and custom subtypes.

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `Exception` | `Exception(reason: string) :Exception` |
| Static | `Exception.runtime` | `Exception.runtime(message: string) :Exception` |
| Static | `Exception.nullPointer` | `Exception.nullPointer(message: string) :Exception` |
| Static | `Exception.indexOutOfBounds` | `Exception.indexOutOfBounds(message: string) :Exception` |
| Static | `Exception.divisionByZero` | `Exception.divisionByZero(message: string) :Exception` |
| Static | `Exception.invalidCast` | `Exception.invalidCast(message: string) :Exception` |
| Static | `Exception.custom` | `Exception.custom(typeName: string, message: string) :Exception` |
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

## [Args](args.md) — Class `Args`

**Import:** `import hoo;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Args.get` | `Args.get(index: int64) :string` |
| Static | `Args.count` | `Args.count() :int64` |

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

## [JSON](json.md) — Class `Json`

**Import:** `import hoo.json;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Json.parse` | `Json.parse(text: string) :Any` |
| Static | `Json.stringify` | `Json.stringify(value: Any) :string` |
| Static | `Json.pretty` | `Json.pretty(value: Any) :string` |
| Static | `Json.free_string` | `Json.free_string(str: string) :void` |

---

## [Encoding](encoding.md) — Free Functions

**Import:** `import hoo.encoding;`

**Pattern:** Free functions

### Base64

| API | Signature |
|-----|-----------|
| `base64_encode` | `base64_encode(data: string) :string` |
| `base64_decode` | `base64_decode(data: string) :string` |
| `base64_encode_bytes` | `base64_encode_bytes(data: array) :string` |
| `base64_decode_bytes` | `base64_decode_bytes(data: string) :array` |

### URL Encoding

| API | Signature |
|-----|-----------|
| `uri_encode` | `uri_encode(data: string) :string` |
| `uri_decode` | `uri_decode(data: string) :string` |

### Hex

| API | Signature |
|-----|-----------|
| `hex_encode` | `hex_encode(data: string) :string` |
| `hex_decode` | `hex_decode(data: string) :string` |

---

## [Hashing](hashing.md) — Free Functions

**Import:** `import hoo;`

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `hashing_crc32` | `hashing_crc32(data: string) :int64` |
| `hashing_md5` | `hashing_md5(data: string) :string` |
| `hashing_sha1` | `hashing_sha1(data: string) :string` |
| `hashing_sha256` | `hashing_sha256(data: string) :string` |

---

## [Compression](compression.md) — Class `Compression`

**Import:** `import hoo.compression;`

**Pattern:** Static class

| Kind | API | Signature |
|------|-----|-----------|
| Static | `Compression.compress` | `Compression.compress(data: string) :string` |
| Static | `Compression.decompress` | `Compression.decompress(data: string) :string` |
| Static | `Compression.compress_bytes` | `Compression.compress_bytes(data: array) :array` |
| Static | `Compression.decompress_bytes` | `Compression.decompress_bytes(data: array) :array` |

---

## [CSV](csv.md) — Class `CSV`

**Import:** `import hoo.io;`

**Pattern:** Static class (with instance ARC methods)

| Kind | API | Signature |
|------|-----|-----------|
| Static | `CSV.parse` | `CSV.parse(text: string) :array` |
| Static | `CSV.serialize` | `CSV.serialize(data: array) :string` |
| Instance | `csv.retain` / `csv.release` / `csv.refcount` | Reference counting |
