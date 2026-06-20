# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`).
This documentation is designed for hoo developers to understand and utilize the
built-in capabilities of the language.

The hoo runtime provides modules that bridge the high-level language with the
host system, offering efficient implementations of common data structures,
mathematical operations, network communication, and system interactions.

## Import Requirements

To prevent namespace pollution and avoid symbol ambiguity, hoo enforces compile-time verification of import statements when using standard library APIs:

- **Core Module**: Elements belonging to the core namespace (such as `String`, `Array`, `Map`, `Exception`, `Console`) do not require submodule imports. Only `import hoo;` is sufficient (and primitive types are exempt entirely).
- **Submodules**: Elements belonging to standard submodules (such as `Math`, `DateTime`, `Fs`, `Thread`, etc.) must be imported explicitly by their submodule path (e.g., `import hoo.math;`, `import hoo.datetime;`, `import hoo.io;`, `import hoo.thread;`). A generic `import hoo;` is NOT sufficient for submodules.

## Usage Patterns

hoo runtime APIs follow one of four patterns:

- **Instance class** — create with `new ClassName()`, call methods on the variable
- **Singleton class** — call methods directly on the class name (e.g. `Math.abs(x)`)
- **Free functions** — namespace-prefixed functions with no class wrapper
- **Global functions** — available without any prefix

Standard I/O (`print`, `println`, `readline`, `readchar`) are available globally.

---

## [Strings](string.md) — Class `String`

**Pattern:** Instance class + singleton static methods

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new String` | `new String() :string` |
| Static | `String.repeat` | `String.repeat(ch: char, count: int64) :string` |
| Static | `String.fromInt64` | `String.fromInt64(val: int64) :string` |
| Static | `String.fromDouble` | `String.fromDouble(val: double) :string` |
| Instance | `s.concat` | `s.concat(other: string) :string` |
| Instance | `s.substring` | `s.substring(start: int64, length: int64) :string` |
| Instance | `s.toUpper` | `s.toUpper() :string` |
| Instance | `s.toLower` | `s.toLower() :string` |
| Instance | `s.trim` | `s.trim() :string` |
| Instance | `s.replace` | `s.replace(old: string, replacement: string) :string` |
| Instance | `s.split` | `s.split(delim: string) :array` |
| Instance | `s.length` | `s.length() :int64` |
| Instance | `s.byteAt` | `s.byteAt(index: int64) :byte` |
| Instance | `s.indexOf` | `s.indexOf(needle: string) :int64` |
| Instance | `s.contains` | `s.contains(needle: string) :int64` |
| Instance | `s.startsWith` | `s.startsWith(prefix: string) :int64` |
| Instance | `s.compare` | `s.compare(other: string) :int64` |
| Instance | `s.equals` | `s.equals(other: string) :int64` |
| Instance | `s.toInt64` | `s.toInt64() :int64` |
| Instance | `s.toDouble` | `s.toDouble() :double` |

---

## [Buffer](buffer.md) — Class `Buffer`

**Pattern:** Instance class + free function

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Buffer` | `new Buffer() :Buffer` |
| Free function | `buffer_fromBytes` | `buffer_fromBytes(data: string, len: int64) :Buffer` |
| Instance | `buf.length` | `buf.length() :int64` |
| Instance | `buf.capacity` | `buf.capacity() :int64` |
| Instance | `buf.copy` | `buf.copy() :Buffer` |
| Instance | `buf.byteAt` | `buf.byteAt(index: int64) :int64` |
| Instance | `buf.setByte` | `buf.setByte(index: int64, value: int64) :int64` |
| Instance | `buf.append` | `buf.append(data: string, len: int64) :Buffer` |
| Instance | `buf.appendBuffer` | `buf.appendBuffer(other: Buffer) :Buffer` |
| Instance | `buf.clear` | `buf.clear() :int64` |
| Instance | `buf.slice` | `buf.slice(start: int64, end: int64) :Buffer` |

---

## [Collections](collections.md) — Classes `Array`, `AnyArray`, `Map`, `HashMap`

**Pattern:** Instance classes

### Array

| API | Signature |
|-----|-----------|
| `new Array` | `new Array() :array` |
| `arr.length` | `arr.length() :int64` |
| `arr.push` | `arr.push(val: int64)` / `arr.push(val: double)` / `arr.push(val: string)` |
| `arr.getInt64` | `arr.getInt64(index: int64) :int64` |
| `arr.getString` | `arr.getString(index: int64) :string` |
| `arr.pop` | `arr.pop()` |
| `arr.clear` | `arr.clear()` |

### AnyArray (heterogeneous)

| API | Signature |
|-----|-----------|
| `new AnyArray` | `new AnyArray() :AnyArray` / `new AnyArray(capacity: int64) :AnyArray` |
| Literal syntax | `[expr, ...]any :AnyArray` |
| `arr.length` | `arr.length() :int64` |
| `arr.push` | `arr.push(value) :int64` |
| Index get | `arr[index]` |
| Index set | `arr[index] = value` |
| `arr.clear` | `arr.clear()` |

### Map (type-safe)

| API | Signature |
|-----|-----------|
| `new Map` | `new Map(keyType: int64, valueType: int64) :map` |
| `m.length` | `m.length() :int64` |
| `m.empty` | `m.empty() :int64` |
| `m.clear` | `m.clear()` |
| `m.keyType` | `m.keyType() :int64` |
| `m.valueType` | `m.valueType() :int64` |

Int64 key operations: `m.containsInt64(key)`, `m.removeInt64(key)`, `m.setInt64Int64(key, val)`, `m.getInt64Int64(key)`, `m.setInt64Double(key, val)`, `m.getInt64Double(key)`, `m.setInt64String(key, val)`, `m.getInt64String(key)`, `m.setInt64Bool(key, val)`, `m.getInt64Bool(key)`

String key operations: `m.containsString(key)`, `m.removeString(key)`, `m.setStringInt64(key, val)`, `m.getStringInt64(key)`, `m.setStringDouble(key, val)`, `m.getStringDouble(key)`, `m.setStringString(key, val)`, `m.getStringString(key)`, `m.setStringBool(key, val)`, `m.getStringBool(key)`

### HashMap (intrinsic, `K` = `byte`/`int8`/`int64`, `V` = fixed or `any`)

| API | Signature |
|-----|-----------|
| `new HashMap` | `new HashMap<K, V>() :HashMap` |
| Index set | `m[key] = value` |
| Index get | `m[key]` |
| `m.count` | `m.count() :int64` |
| `m.remove` | `m.remove(key) :int64` |
| `m.clear` | `m.clear()` |

---

## [Math](math.md) — `math` free functions + Class `Random`

**Pattern:** Module free functions + instance class

### Math — Constants

`math_get_pi() :double`, `math_get_e() :double`, `math_get_tau() :double`,
`math_get_inf() :double`, `math_get_neg_inf() :double`, `math_get_nan() :double`

### Math — Basic Functions

`math_abs(x)`, `math_sign(x)`, `math_min(a, b)`, `math_max(a, b)`,
`math_clamp(val, min, max) :double`

### Math — Power & Roots

`math_pow(base, exp) :double`, `math_sqrt(x) :double`, `math_cbrt(x) :double`,
`math_hypot(x, y) :double`

### Math — Trigonometric

`math_sin(x)`, `math_cos(x)`, `math_tan(x)`, `math_asin(x)`, `math_acos(x)`,
`math_atan(x)`, `math_atan2(y, x)`, `math_sinh(x)`, `math_cosh(x)`,
`math_tanh(x)` — all take/return `:double`

### Math — Exponential & Logarithmic

`math_exp(x)`, `math_exp2(x)`, `math_expm1(x)`, `math_log(x)`, `math_log10(x)`,
`math_log2(x)`, `math_log1p(x)` — all take/return `:double`

### Math — Rounding

`math_floor(x)`, `math_ceil(x)`, `math_round(x)`, `math_trunc(x)`,
`math_fract(x)` — all take/return `:double`

### Math — Number Utilities

`math_is_even(n: int64) :int64`, `math_is_odd(n: int64) :int64`,
`math_is_prime(n: int64) :int64`, `math_gcd(a: int64, b: int64) :int64`,
`math_lcm(a: int64, b: int64) :int64`,
`math_factorial(n: int64) :int64`, `math_fibonacci(n: int64) :int64`

### Random

| API | Signature |
|-----|-----------|
| `new Random` | `new Random() :Random` / `new Random(seed: int64) :Random` |
| `rng.nextInt` | `rng.nextInt() :int64` |
| `rng.nextIntMax` | `rng.nextIntMax(max: int64) :int64` |
| `rng.nextDouble` | `rng.nextDouble() :double` |
| `rng.nextBool` | `rng.nextBool() :bool` |
| `rng.nextBytes` | `rng.nextBytes(buffer: Buffer, count: int64) :int64` |
| `rng.release` | `rng.release()` |

---

## [I/O](io.md) — Global Free Functions

**Pattern:** Global functions (no prefix)

| API | Signature |
|-----|-----------|
| `print` | `print(str: string)` |
| `println` | `println(str: string)` |
| `readline` | `readline() :string` |
| `readchar` | `readchar() :int64` |

---

## [Character](character.md) — Class `Character`

**Pattern:** Instance class + free functions

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Character` | `new Character(codepoint: int64) :Character` |
| Free Function | `character_from_utf8` | `character_from_utf8(string: string) :Character` |
| Instance | `ch.codepoint` | `codepoint() :int64` |
| Instance | `ch.length` | `length() :int64` |
| Instance | `ch.data` | `data() :string` |
| Instance | `ch.print` | `print()` |
| Instance | `ch.release` | `release()` |

---

## [Regex](regex.md) — regular expressions

**Pattern:** Instance class & free functions

| API | Signature |
|-----|-----------|
| `new Regex` | `new Regex(pattern: string) :Regex` |
| `re.match` | `re.match(subject: string) :int64` |
| `re.search` | `re.search(subject: string) :int64` |
| `re.find` | `re.find(subject: string) :string` |
| `re.group` | `re.group(subject: string, group_index: int64) :string` |
| `re.replace` | `re.replace(subject: string, replacement: string) :string` |
| `re.split` | `re.split(subject: string) :array` |
| `re.release` | `re.release()` |
| `regex_match` | `regex_match(pattern: string, subject: string) :int64` |
| `regex_search` | `regex_search(pattern: string, subject: string) :int64` |
| `regex_replace` | `regex_replace(pattern: string, subject: string, replacement: string) :string` |
| `regex_split` | `regex_split(pattern: string, subject: string) :array` |

---

## [DateTime](datetime.md) — Instance Class `DateTime` + Free Functions

**Pattern:** Instance class + built-in class-qualified dispatch + free functions

| Kind | API | Return Type |
|------|-----|-------------|
| Factory | `DateTime.now()` / `datetime_now()` | `DateTime` |
| Factory | `DateTime.new(ts)` / `datetime_new(ts)` | `DateTime` |
| Factory | `DateTime.parse(s,f)` / `datetime_parse(s,f)` | `DateTime` |
| Factory | `DateTime.from_iso8601(s)` / `datetime_from_iso8601(s)` | `DateTime` |
| Raw time | `DateTime.now_seconds()` / `datetime_now_seconds()` | `int64` |
| Raw time | `DateTime.now_precise()` / `datetime_now_precise()` | `double` |
| Accessor | `dt.getTimestamp()` | `int64` |
| Format | `dt.format(f)` / `DateTime.format(dt,f)` / `datetime_format(dt,f)` | `string` |
| Format | `dt.iso8601()` / `DateTime.iso8601(dt)` / `datetime_iso8601(dt)` | `string` |
| Arithmetic | `dt.addDays(n)` / `DateTime.add_days(dt,n)` / `datetime_add_days(dt,n)` | `DateTime` |
| Arithmetic | `dt.addHours(n)` / `DateTime.add_hours(dt,n)` / `datetime_add_hours(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMinutes(n)` / `DateTime.add_minutes(dt,n)` / `datetime_add_minutes(dt,n)` | `DateTime` |
| Arithmetic | `dt.addSeconds(n)` / `DateTime.add_seconds(dt,n)` / `datetime_add_seconds(dt,n)` | `DateTime` |
| Arithmetic | `dt.addMilliseconds(n)` / `DateTime.add_milliseconds(dt,n)` / `datetime_add_milliseconds(dt,n)` | `DateTime` |
| Diff | `a.diffDays(b)` / `DateTime.diff_days(a,b)` / `datetime_diff_days(a,b)` | `int64` |
| Diff | `a.diffHours(b)` / `DateTime.diff_hours(a,b)` / `datetime_diff_hours(a,b)` | `int64` |
| Diff | `a.diffSeconds(b)` / `DateTime.diff_seconds(a,b)` / `datetime_diff_seconds(a,b)` | `double` |
| Compare | `a.compare(b)` / `DateTime.compare(a,b)` / `datetime_compare(a,b)` | `int64` |

---

## [Uuid](uuid.md) — universally unique identifiers

**Pattern:** Instance class & free functions

| API | Signature |
|-----|-----------|
| `new Uuid` | `new Uuid(source: string) :Uuid` |
| `uuid.toString` | `uuid.toString() :string` |
| `uuid.isNil` | `uuid.isNil() :int64` |
| `uuid.equals` | `uuid.equals(other: Uuid) :int64` |
| `uuid.compare` | `uuid.compare(other: Uuid) :int64` |
| `uuid.toBytes` | `uuid.toBytes() :buffer` |
| `uuid.release` | `uuid.release()` |
| `uuid_v4` | `uuid_v4() :string` |
| `uuid_nil` | `uuid_nil() :string` |
| `uuid_is_nil` | `uuid_is_nil(str: string) :int64` |
| `uuid_from_bytes` | `uuid_from_bytes(buf: buffer) :Uuid` |
| `uuid_to_bytes` | `uuid_to_bytes(str: string) :buffer` |
| `uuid_equals` | `uuid_equals(a: string, b: string) :int64` |
| `uuid_compare` | `uuid_compare(a: string, b: string) :int64` |
| `uuid_to_string` | `uuid_to_string(id: Uuid) :string` |

---

## [Fs](fs.md) — Free Functions

**Pattern:** Namespace-prefixed free functions

| API | Signature |
|-----|-----------|
| `fs_exists` | `fs_exists(path: string) :int64` |
| `fs_is_file` | `fs_is_file(path: string) :int64` |
| `fs_is_dir` | `fs_is_dir(path: string) :int64` |
| `fs_size` | `fs_size(path: string) :int64` |
| `fs_last_modified` | `fs_last_modified(path: string) :int64` |
| `fs_read_text` | `fs_read_text(path: string) :string` |
| `fs_write_text` | `fs_write_text(path: string, content: string) :int64` |
| `fs_append_text` | `fs_append_text(path: string, content: string) :int64` |
| `fs_read_bytes` | `fs_read_bytes(path: string) :buffer` |
| `fs_write_bytes` | `fs_write_bytes(path: string, buf: buffer) :int64` |
| `fs_delete` | `fs_delete(path: string) :int64` |
| `fs_rename` | `fs_rename(oldPath: string, newPath: string) :int64` |
| `fs_copy` | `fs_copy(src: string, dst: string) :int64` |
| `fs_mkdir` | `fs_mkdir(path: string) :int64` |
| `fs_mkdirs` | `fs_mkdirs(path: string) :int64` |
| `fs_rmdir` | `fs_rmdir(path: string) :int64` |
| `fs_list_dir` | `fs_list_dir(path: string) :array` |
| `fs_temp_dir` | `fs_temp_dir() :string` |
| `fs_create_temp_file` | `fs_create_temp_file(prefix: string) :string` |

---

## [Path](path.md) — Singleton `Path`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Path.basename` | `Path.basename(p: string) :string` |
| `Path.dirname` | `Path.dirname(p: string) :string` |
| `Path.extension` | `Path.extension(p: string) :string` |
| `Path.filename` | `Path.filename(p: string) :string` |
| `Path.join` | `Path.join(parts: array) :string` |
| `Path.absolute` | `Path.absolute(p: string) :string` |
| `Path.separator` | `Path.separator() :string` |
| `Path.is_absolute` | `Path.is_absolute(p: string) :int64` |
| `Path.normalize` | `Path.normalize(p: string) :string` |

---

## [Process](process.md) — `process` free functions

**Pattern:** Module free functions

| API | Signature |
|-----|-----------|
| `process_self_pid` | `process_self_pid() :int64` |
| `process_capture` | `process_capture(command: string) :string` |
| `process_kill` | `process_kill(pid: int64, signal: int64) :int64` |
| `process_spawn` | `process_spawn(command: string, argv: array) :int64` |
| `process_wait` | `process_wait(pid: int64) :int64` |

---

## [System](system.md) — `system` free functions

**Pattern:** Module free functions

| API | Signature |
|-----|-----------|
| `system_get_env` | `system_get_env(name: string) :string` |
| `system_set_env` | `system_set_env(name: string, value: string) :int64` |
| `system_unset_env` | `system_unset_env(name: string) :int64` |
| `system_hostname` | `system_hostname() :string` |
| `system_os_name` | `system_os_name() :string` |
| `system_os_version` | `system_os_version() :string` |
| `system_cpu_count` | `system_cpu_count() :int64` |
| `system_process_id` | `system_process_id() :int64` |
| `system_uptime_ms` | `system_uptime_ms() :int64` |
| `system_exit` | `system_exit(code: int64)` |
| `system_exec` | `system_exec(command: string) :string` |
| `system_exec_status` | `system_exec_status(command: string) :int64` |
| `system_user_home` | `system_user_home() :string` |
| `system_user_name` | `system_user_name() :string` |
| `system_current_dir` | `system_current_dir() :string` |
| `system_set_current_dir` | `system_set_current_dir(path: string) :int64` |
| `system_total_memory` | `system_total_memory() :int64` |
| `system_free_memory` | `system_free_memory() :int64` |

---

## [Args](args.md) — Class `Args`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Args` | `new Args() :Args` |
| Instance | `args.count` | `args.count() :int64` |
| Instance | `args.get` | `args.get(index: int64) :string` |
| Instance | `args.has` | `args.has(name: string) :int64` |
| Instance | `args.value` | `args.value(name: string) :string` |
| Instance | `args.programName` | `args.programName() :string` |
| Instance | `args.addString` | `args.addString(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: string)` |
| Instance | `args.addInt` | `args.addInt(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: int64)` |
| Instance | `args.addFlag` | `args.addFlag(name: string, shortOpt: string, longOpt: string, help: string)` |
| Instance | `args.addFloat` | `args.addFloat(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: f64)` |
| Instance | `args.addPositional` | `args.addPositional(name: string, help: string)` |
| Instance | `args.parse` | `args.parse() :int64` |
| Instance | `args.getString` | `args.getString(name: string) :string` |
| Instance | `args.getInt` | `args.getInt(name: string) :int64` |
| Instance | `args.getBool` | `args.getBool(name: string) :int64` |
| Instance | `args.getFloat` | `args.getFloat(name: string) :f64` |
| Instance | `args.helpText` | `args.helpText() :string` |
| Instance | `args.clear` | `args.clear()` |

---

## [Net](net.md) — Classes `URL`, `HttpClient`, `HttpResponse`

**Pattern:** Instance classes

### URL

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new URL` | `new URL(url: string) :ptr` |
| Instance | `url.getScheme` | `url.getScheme() :string` |
| Instance | `url.getHost` | `url.getHost() :string` |
| Instance | `url.getPort` | `url.getPort() :int64` |
| Instance | `url.getPath` | `url.getPath() :string` |
| Instance | `url.getQuery` | `url.getQuery() :string` |
| Instance | `url.getFragment` | `url.getFragment() :string` |
| Instance | `url.toString` | `url.toString() :string` |
| Instance | `url.release` | `url.release()` |

### HttpClient

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new HttpClient` | `new HttpClient() :HttpClient` |
| Instance | `client.setHeader` | `client.setHeader(key: string, value: string) :int64` |
| Instance | `client.setTimeout` | `client.setTimeout(ms: int64)` |
| Instance | `client.get` | `client.get(url: string) :HttpResponse` |
| Instance | `client.post` | `client.post(url: string, body: string) :HttpResponse` |
| Instance | `client.put` | `client.put(url: string, body: string) :HttpResponse` |
| Instance | `client.delete` | `client.delete(url: string) :HttpResponse` |
| Instance | `client.release` | `client.release()` |

### HttpResponse

| API | Signature |
|-----|-----------|
| `resp.statusCode` | `resp.statusCode() :int64` |
| `resp.getBody` | `resp.getBody() :string` |
| `resp.isSuccess` | `resp.isSuccess() :int64` |
| `resp.release` | `resp.release()` |

---

## [JSON](json.md) — Free Functions

**Pattern:** Free functions

| API | Signature |
|-----|-----------|
| `json_serialize_hashmap` | `json_serialize_hashmap(map: HashMap<integer, T>) :string` |
| `json_serialize_anyarray` | `json_serialize_anyarray(values: AnyArray) :string` |
| `json_deserialize_hashmap` | `json_deserialize_hashmap(json: string) :HashMap<int64, any>` |
| `json_deserialize_anyarray` | `json_deserialize_anyarray(json: string) :AnyArray` |
| `json_minify` | `json_minify(json: string) :string` |
| `json_beautify` | `json_beautify(json: string) :string` |

---

## [Encoding](encoding.md) — Module `hoo.encoding`

**Pattern:** Free Functions

| API | Signature |
|-----|-----------|
| `encoding_base64_encode` | `encoding_base64_encode(data: string, len: int64) :string` |
| `encoding_base64_decode` | `encoding_base64_decode(encoded: string) :string` |
| `encoding_hex_encode` | `encoding_hex_encode(data: string, len: int64) :string` |
| `encoding_hex_decode` | `encoding_hex_decode(hex: string) :string` |
| `encoding_url_encode` | `encoding_url_encode(str: string) :string` |
| `encoding_url_decode` | `encoding_url_decode(encoded: string) :string` |

Buffer overloads: `encoding_base64_encode_buffer(buf: Buffer) :string`,
`encoding_base64_decode_buffer(encoded: string) :Buffer`,
`encoding_hex_encode_buffer(buf: Buffer) :string`,
`encoding_hex_decode_buffer(hex: string) :Buffer`

---

## [Hashing](hashing.md) — `hashing` free functions

**Pattern:** Module free functions

| API | Signature |
|-----|-----------|
| `hashing_sha256` | `hashing_sha256(data: string, len: int64) :string` |
| `hashing_sha256_file` | `hashing_sha256_file(path: string) :string` |
| `hashing_sha1` | `hashing_sha1(data: string, len: int64) :string` |
| `hashing_md5` | `hashing_md5(data: string, len: int64) :string` |
| `hashing_crc32` | `hashing_crc32(data: string, len: int64) :int64` |
| `hashing_hmac_sha256` | `hashing_hmac_sha256(key: string, keyLen: int64, data: string, dataLen: int64) :string` |

Buffer-aware functions: `hashing_sha256_buffer(buf: Buffer)`, `hashing_sha1_buffer(buf: Buffer)`,
`hashing_md5_buffer(buf: Buffer)`, `hashing_crc32_buffer(buf: Buffer)`,
`hashing_hmac_sha256_buffer(key: Buffer, data: Buffer)` — return types same as string versions.

---

## [Compression](compression.md) — Class `Compression`

**Pattern:** Instance class

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Compression` | `new Compression() :Compression` |
| Instance | `c.release` | `c.release()` |
| Instance | `c.gzipCompress` | `c.gzipCompress(data, len) :string` |
| Instance | `c.gzipDecompress` | `c.gzipDecompress(data, len) :string` |
| Instance | `c.deflateCompress` | `c.deflateCompress(data, len) :string` |
| Instance | `c.deflateDecompress` | `c.deflateDecompress(data, len) :string` |

Buffer overloads: `c.gzipCompress(buf: Buffer) :Buffer`,
`c.gzipDecompress(buf: Buffer) :Buffer`,
`c.deflateCompress(buf: Buffer) :Buffer`,
`c.deflateDecompress(buf: Buffer) :Buffer`

---

## [CSV](csv.md) — Class `Csv` + Free Function

**Pattern:** Instance class + free function

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Csv` | `new Csv() :Csv` |
| Free function | `csv_from_opts` | `csv_from_opts(delimiter: int64, quote: int64) :Csv` |
| Instance | `csv.retain` | `csv.retain() :Csv` |
| Instance | `csv.release` | `csv.release()` |
| Instance | `csv.refcount` | `csv.refcount() :int64` |
| Instance | `csv.parse` | `csv.parse(input: string) :array` |
| Instance | `csv.parseAsMaps` | `csv.parseAsMaps(input: string) :array` |
| Instance | `csv.generate` | `csv.generate(data: array) :string` |
| Instance | `csv.readFile` | `csv.readFile(path: string) :array` |
| Instance | `csv.readFileAsMaps` | `csv.readFileAsMaps(path: string) :array` |
| Instance | `csv.writeFile` | `csv.writeFile(path: string, data: array) :int64` |
| Instance | `csv.escape` | `csv.escape(c: int64) :int64` |
| Instance | `csv.count` | `csv.count(data: array, column: string) :int64` |
| Instance | `csv.sum` | `csv.sum(data: array, column: string) :int64` |
| Instance | `csv.avg` | `csv.avg(data: array, column: string) :string` |
| Instance | `csv.min` | `csv.min(data: array, column: string) :string` |
| Instance | `csv.max` | `csv.max(data: array, column: string) :string` |
| Instance | `csv.select` | `csv.select(data: array, columns: array) :array` |
| Instance | `csv.filter` | `csv.filter(data: array, column: string, op: string, value: string) :array` |
| Instance | `csv.sort` | `csv.sort(data: array, column: string, ascending: int64) :array` |
| Instance | `csv.describe` | `csv.describe(data: array, column: string) :map` |

---

## [Thread](thread.md) — concurrency and mutexes

**Pattern:** Instance class & free functions

| API | Signature |
|-----|-----------|
| `new Mutex` | `new Mutex() :Mutex` |
| `mutex.lock` | `mutex.lock() :int64` |
| `mutex.unlock` | `mutex.unlock() :int64` |
| `mutex.release` | `mutex.release() :int64` |
| `thread_self` | `thread_self() :int64` |
| `thread_spawn` | `thread_spawn(func: ptr, arg: ptr) :int64` |
| `thread_join` | `thread_join(thread_id: int64) :int64` |

---

## [Exception](exception.md) — Exception Types

**Pattern:** Instance methods on caught exceptions

Exception types: `RuntimeException`, `NullPointerException`,
`IndexOutOfBoundsException`, `DivisionByZeroException`, `InvalidCastException`,
and custom subtypes of `RuntimeException`.

| API | Signature |
|-----|-----------|
| `e.message` | `e.message() :string` |
| `e.typeName` | `e.typeName() :string` |
| `e.typeId` | `e.typeId() :int64` |
| `e.stackTrace` | `e.stackTrace() :string` |
| `e.hasCause` | `e.hasCause() :int64` |
| `e.cause` | `e.cause() :Exception` |
| `e.frameCount` | `e.frameCount() :int64` |
| `e.frame` | `e.frame(index: int64) :string` |
