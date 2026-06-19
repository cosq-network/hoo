# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`).
This documentation is designed for hoo developers to understand and utilize the
built-in capabilities of the language.

The hoo runtime provides modules that bridge the high-level language with the
host system, offering efficient implementations of common data structures,
mathematical operations, network communication, and system interactions.

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

## [Math](math.md) — Singleton `Math` + Class `Random`

**Pattern:** Singleton class + instance class

### Math — Constants

`Math.getPi() :double`, `Math.getE() :double`, `Math.getTau() :double`,
`Math.getInf() :double`, `Math.getNegInf() :double`, `Math.getNan() :double`

### Math — Basic Functions

`Math.abs(x)`, `Math.sign(x)`, `Math.min(a, b)`, `Math.max(a, b)`,
`Math.clamp(val, min, max) :double`

### Math — Power & Roots

`Math.pow(base, exp) :double`, `Math.sqrt(x) :double`, `Math.cbrt(x) :double`,
`Math.hypot(x, y) :double`

### Math — Trigonometric

`Math.sin(x)`, `Math.cos(x)`, `Math.tan(x)`, `Math.asin(x)`, `Math.acos(x)`,
`Math.atan(x)`, `Math.atan2(y, x)`, `Math.sinh(x)`, `Math.cosh(x)`,
`Math.tanh(x)` — all take/return `:double`

### Math — Exponential & Logarithmic

`Math.exp(x)`, `Math.exp2(x)`, `Math.expm1(x)`, `Math.log(x)`, `Math.log10(x)`,
`Math.log2(x)`, `Math.log1p(x)` — all take/return `:double`

### Math — Rounding

`Math.floor(x)`, `Math.ceil(x)`, `Math.round(x)`, `Math.trunc(x)`,
`Math.fract(x)` — all take/return `:double`

### Math — Number Utilities

`Math.isEven(n: int64) :int64`, `Math.isOdd(n: int64) :int64`,
`Math.isPrime(n: int64) :int64`, `Math.gcd(a: int64, b: int64) :int64`,
`Math.lcm(a: int64, b: int64) :int64`,
`Math.factorial(n: int64) :int64`, `Math.fibonacci(n: int64) :int64`

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

**Pattern:** Instance class + singleton static method

| Kind | API | Signature |
|------|-----|-----------|
| Constructor | `new Character` | `new Character(codepoint: int64) :ptr` |
| Static | `Character.fromUtf8` | `Character.fromUtf8(string: string) :ptr` |
| Instance | `ch.codepoint` | `codepoint() :int64` |
| Instance | `ch.length` | `length() :int64` |
| Instance | `ch.data` | `data() :string` |
| Instance | `ch.print` | `print()` |
| Instance | `ch.release` | `release()` |

---

## [Regex](regex.md) — Singleton `Regex`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Regex.compile` | `Regex.compile(pattern: string) :ptr` |
| `Regex.match` | `Regex.match(re: ptr, subject: string) :int64` |
| `Regex.search` | `Regex.search(re: ptr, subject: string) :int64` |
| `Regex.replace` | `Regex.replace(re: ptr, subject: string, replacement: string) :string` |
| `Regex.split` | `Regex.split(re: ptr, subject: string) :array` |
| `Regex.release` | `Regex.release(re: ptr)` |
| `Regex.match` (convenience) | `Regex.match(pattern: string, subject: string) :int64` |
| `Regex.find` (convenience) | `Regex.find(pattern: string, subject: string) :int64` |
| `Regex.replace` (convenience) | `Regex.replace(pattern: string, subject: string, replacement: string) :string` |
| `Regex.split` (convenience) | `Regex.split(pattern: string, subject: string) :array` |

---

## [DateTime](datetime.md) — Singleton `DateTime`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `DateTime.now` | `DateTime.now() :int64` |
| `DateTime.nowSeconds` | `DateTime.nowSeconds() :int64` |
| `DateTime.format` | `DateTime.format(ts: int64, fmt: string) :string` |
| `DateTime.parse` | `DateTime.parse(str: string, fmt: string) :int64` |
| `DateTime.iso8601` | `DateTime.iso8601(ts: int64) :string` |
| `DateTime.fromIso8601` | `DateTime.fromIso8601(str: string) :int64` |
| `DateTime.addDays` | `DateTime.addDays(ts: int64, days: int64) :int64` |
| `DateTime.addHours` | `DateTime.addHours(ts: int64, hours: int64) :int64` |
| `DateTime.addMinutes` | `DateTime.addMinutes(ts: int64, minutes: int64) :int64` |
| `DateTime.addSeconds` | `DateTime.addSeconds(ts: int64, seconds: int64) :int64` |
| `DateTime.addMilliseconds` | `DateTime.addMilliseconds(ts: int64, ms: int64) :int64` |
| `DateTime.diffDays` | `DateTime.diffDays(ts1: int64, ts2: int64) :int64` |
| `DateTime.diffHours` | `DateTime.diffHours(ts1: int64, ts2: int64) :int64` |
| `DateTime.diffSeconds` | `DateTime.diffSeconds(ts1: int64, ts2: int64) :double` |
| `DateTime.compare` | `DateTime.compare(ts1: int64, ts2: int64) :int64` |

---

## [Uuid](uuid.md) — Singleton `Uuid`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Uuid.v4` | `Uuid.v4() :ptr` |
| `Uuid.nil` | `Uuid.nil() :ptr` |
| `Uuid.fromString` | `Uuid.fromString(str: string) :ptr` |
| `Uuid.toString` | `Uuid.toString(uuid: ptr) :string` |
| `Uuid.isNil` | `Uuid.isNil(uuid: ptr) :int64` |
| `Uuid.equals` | `Uuid.equals(a: ptr, b: ptr) :int64` |
| `Uuid.compare` | `Uuid.compare(a: ptr, b: ptr) :int64` |
| `Uuid.release` | `Uuid.release(uuid: ptr)` |
| `Uuid.fromBytes` | `Uuid.fromBytes(buf: buffer) :ptr` |
| `Uuid.toBytes` | `Uuid.toBytes(uuid: ptr) :buffer` |

---

## [Fs](fs.md) — Free Functions

**Pattern:** Namespace-prefixed free functions

| API | Signature |
|-----|-----------|
| `fs_exists` | `fs_exists(path: string) :int64` |
| `fs_isFile` | `fs_isFile(path: string) :int64` |
| `fs_isDir` | `fs_isDir(path: string) :int64` |
| `fs_size` | `fs_size(path: string) :int64` |
| `fs_lastModified` | `fs_lastModified(path: string) :int64` |
| `fs_readText` | `fs_readText(path: string) :string` |
| `fs_writeText` | `fs_writeText(path: string, content: string) :int64` |
| `fs_appendText` | `fs_appendText(path: string, content: string) :int64` |
| `fs_readBytes` | `fs_readBytes(path: string) :buffer` |
| `fs_writeBytes` | `fs_writeBytes(path: string, buf: buffer) :int64` |
| `fs_delete` | `fs_delete(path: string) :int64` |
| `fs_rename` | `fs_rename(oldPath: string, newPath: string) :int64` |
| `fs_copy` | `fs_copy(src: string, dst: string) :int64` |
| `fs_mkdir` | `fs_mkdir(path: string) :int64` |
| `fs_mkdirs` | `fs_mkdirs(path: string) :int64` |
| `fs_rmdir` | `fs_rmdir(path: string) :int64` |
| `fs_listDir` | `fs_listDir(path: string) :array` |
| `fs_tempDir` | `fs_tempDir() :string` |
| `fs_createTempFile` | `fs_createTempFile(prefix: string) :string` |

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
| `Path.isAbsolute` | `Path.isAbsolute(p: string) :int64` |
| `Path.normalize` | `Path.normalize(p: string) :string` |

---

## [Process](process.md) — Singleton `Process`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Process.selfPid` | `Process.selfPid() :int64` |
| `Process.capture` | `Process.capture(command: string) :string` |
| `Process.kill` | `Process.kill(pid: int64, signal: int64) :int64` |
| `Process.spawn` | `Process.spawn(command: string, argv: array) :int64` |
| `Process.wait` | `Process.wait(pid: int64) :int64` |

---

## [System](system.md) — Singleton `System`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `System.getEnv` | `System.getEnv(name: string) :string` |
| `System.setEnv` | `System.setEnv(name: string, value: string) :int64` |
| `System.unsetEnv` | `System.unsetEnv(name: string) :int64` |
| `System.hostname` | `System.hostname() :string` |
| `System.osName` | `System.osName() :string` |
| `System.osVersion` | `System.osVersion() :string` |
| `System.cpuCount` | `System.cpuCount() :int64` |
| `System.processId` | `System.processId() :int64` |
| `System.uptimeMs` | `System.uptimeMs() :int64` |
| `System.exit` | `System.exit(code: int64)` |
| `System.exec` | `System.exec(command: string) :string` |
| `System.execStatus` | `System.execStatus(command: string) :int64` |
| `System.userHome` | `System.userHome() :string` |
| `System.userName` | `System.userName() :string` |
| `System.currentDir` | `System.currentDir() :string` |
| `System.setCurrentDir` | `System.setCurrentDir(path: string) :int64` |
| `System.totalMemory` | `System.totalMemory() :int64` |
| `System.freeMemory` | `System.freeMemory() :int64` |

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

## [Encoding](encoding.md) — Singleton `Encoding`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Encoding.base64Encode` | `Encoding.base64Encode(data: string, len: int64) :string` |
| `Encoding.base64Decode` | `Encoding.base64Decode(encoded: string) :string` |
| `Encoding.hexEncode` | `Encoding.hexEncode(data: string, len: int64) :string` |
| `Encoding.hexDecode` | `Encoding.hexDecode(hex: string) :string` |
| `Encoding.urlEncode` | `Encoding.urlEncode(str: string) :string` |
| `Encoding.urlDecode` | `Encoding.urlDecode(encoded: string) :string` |

Buffer overloads: `Encoding.base64Encode(buf: Buffer) :string`,
`Encoding.base64Decode(encoded: string) :Buffer`,
`Encoding.hexEncode(buf: Buffer) :string`,
`Encoding.hexDecode(hex: string) :Buffer`

---

## [Hashing](hashing.md) — Singleton `Hashing`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Hashing.sha256` | `Hashing.sha256(data: string, len: int64) :string` |
| `Hashing.sha256File` | `Hashing.sha256File(path: string) :string` |
| `Hashing.sha1` | `Hashing.sha1(data: string, len: int64) :string` |
| `Hashing.md5` | `Hashing.md5(data: string, len: int64) :string` |
| `Hashing.crc32` | `Hashing.crc32(data: string, len: int64) :int64` |
| `Hashing.hmacSha256` | `Hashing.hmacSha256(key: string, keyLen: int64, data: string, dataLen: int64) :string` |

Buffer overloads: `Hashing.sha256(buf: Buffer)`, `Hashing.sha1(buf: Buffer)`,
`Hashing.md5(buf: Buffer)`, `Hashing.crc32(buf: Buffer)`,
`Hashing.hmacSha256(key: Buffer, data: Buffer)` — return types same as string versions.

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
| Free function | `csv_fromOpts` | `csv_fromOpts(delimiter: int64, quote: int64) :Csv` |
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

## [Thread](thread.md) — Singleton `Thread`

**Pattern:** Singleton class

| API | Signature |
|-----|-----------|
| `Thread.self` | `Thread.self() :int64` |
| `Thread.mutexCreate` | `Thread.mutexCreate() :ptr` |
| `Thread.mutexLock` | `Thread.mutexLock(mutex: ptr) :int64` |
| `Thread.mutexUnlock` | `Thread.mutexUnlock(mutex: ptr) :int64` |
| `Thread.mutexDestroy` | `Thread.mutexDestroy(mutex: ptr) :int64` |

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
