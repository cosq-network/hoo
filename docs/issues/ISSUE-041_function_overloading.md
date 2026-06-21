# ISSUE-041 Function Overloading Support

## Goal
Enable the Hoo language to allow **function overloading** for both free (top‑level) functions and member functions of classes. Multiple functions may share the same identifier **iff** their parameter type signatures differ. Overloaded functions must be uniquely identified at the binary level through a **mangling scheme** that incorporates the full parameter type list.

---
### Motivation
* Provide a more expressive API surface similar to C++/Java.
* Allow idiomatic generic libraries (e.g., `print(int)`, `print(string)`).
* Reduce boilerplate by avoiding manually‑named variants (`print_i`, `print_s`).

---
## High‑Level Changes
| Area | What needs to change | Rationale |
|------|----------------------|-----------|
| **Grammar** | Add `overload_list` production to allow multiple function declarations with identical name. | Parser must accept repeated identifiers and defer resolution to the semantic phase. |
| **Parsing** | Extend the AST with a `FunctionDecl` node that stores a `vector<Type>` for parameters and a `bool is_overload` flag. | Enables later mangling and overload set construction. |
| **Name Mangling** | New mangling format: `<name>__<param_type_ids>` (e.g., `foo__i64s`). Existing mangling for non‑overloaded functions stays unchanged. | Guarantees a unique symbol per overload and keeps backward compatibility. |
| **Demangling** | Update `hoo_demangle` utility to parse the new pattern and retrieve the base name plus parameter list. | Required for stack‑traces, reflection, and debugging. |
| **Symbol Table** | Change from a *single* entry per name to an **overload set** (`unordered_map<string, vector<FunctionSymbol>>`). | Allows fast lookup of the correct overload based on argument types during resolution. |
| **Type Resolution** | Implement overload resolution algorithm (exact match, then implicit conversion ranking). Emit `AmbiguousOverloadException` when multiple candidates qualify. | Mirrors typical language semantics and provides clear error reporting. |
| **Code Generation** | **Bytecode**: add `CALL_OVERLOADED` carrying a type-signature identifier. **JIT**: emit calls to runtime `hoo_resolve_overload` to obtain the concrete function pointer before invocation. | Keeps bytecode/JIT calls tied to a resolved concrete target. |
| **Runtime API** | Add `hoo_resolve_overload(const char* mangled_name, int64_t* arg_type_ids, size_t argc)` returning a function pointer. Introduce exception types: `AmbiguousOverloadException`, `NoMatchingOverloadException`. | Provides a shared runtime resolution surface for interpreter and JIT paths. |
| **Execution** | During a call site, evaluate argument types, construct the mangled signature, and invoke the resolved address. | Ensures proper dispatch at runtime for dynamic languages (JIT & interpreter). |
| **Testing** | New unit tests covering: simple overloads, member overloads, ambiguous calls, and error cases. |
| **Documentation** | Update `docs/runtime/api/index.md`, `README.md`, and language spec sections about functions. |

---
## Runtime Overload Candidate Audit (2026-06-21)

### Audit Scope
This audit scanned the runtime-facing implementation paths that currently expose overload-like behavior through distinct symbol names:

- `src/runtime/lib/*.h` C ABI declarations.
- `src/hvm/HVMJIT.cpp` runtime symbol registration and alias tables.
- `src/codegen/HVMCodeGenerator.cpp` built-in class, constructor, free-function, and method lowering paths.
- Runtime/JIT tests under `tests/jit` and `tests/runtime` where available.

The implementation does not yet have real overload sets. It emulates overloads with four patterns:

1. **Type suffixes**: `hoo_math_abs_int64`, `hoo_math_abs_double`, `hoo_array_push_int64`, `hoo_array_push_string`.
2. **Arity suffixes**: `hoo_tensor_new1`, `hoo_tensor_new2`, `hoo_tensor_new3`.
3. **Representation suffixes**: `base64_encode` vs `base64_encode_buffer`, `sha256` vs `sha256_buffer`.
4. **Receiver/alias duplication**: JIT symbols such as `HttpClient_setHeader_v_p_p_p` and `HttpClient_setHeader_v_p_p` differ by whether the receiver is explicit in the lowered ABI.

Function overloading can replace many of these source-level names, but the existing C ABI functions should remain as stable implementation targets.

### Highest-Value Overload Families

| Runtime area | Current implementation shape | Proposed source-level overloads | Resolution notes |
|--------------|------------------------------|----------------------------------|------------------|
| Math | `hoo_math_abs_int64`, `_int8`, `_byte`, `_double`, `_f8`; same pattern for `min`, `max`, `sign`. `HVMCodeGenerator.cpp` already has unsuffixed `math_abs`, `math_min`, `math_max`, `math_sign` return inference. | `Math.abs(x)`, `Math.min(a, b)`, `Math.max(a, b)`, `Math.sign(x)` and matching free aliases. | Best first target because overloads are pure, arity is fixed, and dispatch depends only on argument type IDs. |
| Console / IO | `hoo_print(void*)`, `hoo_println(void*)`; string conversion helpers exist in `hoo_string.h`. | `print(string)`, `print(int64)`, `print(double)`, `print(bool)`, `print(Character)`, `print(any)` and the same for `println` / `Console.println`. | Should lower primitive overloads through `String.from*` helpers or dedicated runtime wrappers. This is the motivating example in the issue. |
| Array | Generic `hoo_array_push`; typed variants `hoo_array_push_int64`, `_double`, `_float`, `_bool`, `_char`, `_string`, `_object`, `_array`, `_vector_int64`; typed getters `hoo_array_get_int64`, `_double`, `_float`, `_bool`, `_char`, `_string`, `_object`, `_array`. JIT registers `Array_pushInt64`, `Array_pushString`, etc. | `array.push(value)`, `array.set(index, value)`, `array.get(index)`. | `push`/`set` resolve by value type and declared element type. `get(index)` must not be return-type-only in phase 1; it needs array element type context or an explicit generic form. |
| Map | Generic runtime API plus JIT wrappers for combinations such as `map_set_int64_int64`, `map_set_int64_double`, `map_set_string_string`, `map_get_int8_int64`, and typed contains/remove wrappers. | `map.set(key, value)`, `map.get(key)`, `map.containsKey(key)`, `map.remove(key)`. | Resolver should use declared `Map<K,V>` type. This is a major cleanup opportunity because the current implementation expands key/value combinations into separate symbols. |
| HashMap | `hoo_hashmap_set_fixed_i8`, `hoo_hashmap_set_any_i8`, `hoo_hashmap_get_fixed_i8`, `hoo_hashmap_get_any_i8`, `hoo_hashmap_get_fixed_at_i8`, `hoo_hashmap_get_any_at_i8`, `hoo_hashmap_remove_i8`. | `hashMap.set(key, value)`, `hashMap.get(key)`, `hashMap[index]`, `hashMap.remove(key)`. | Existing codegen already chooses fixed vs any storage paths. Overload resolution should make that choice explicit and type-checked. |
| Tensor | Constructors `hoo_tensor_new`, `hoo_tensor_new1`, `hoo_tensor_new2`, `hoo_tensor_new3`; typed accessors `hoo_tensor_get_int64`, `hoo_tensor_get_double`, `hoo_tensor_get_bits`; generic bit-based setters/pushers. | `Tensor.new(type, d0)`, `Tensor.new(type, d0, d1)`, `Tensor.new(type, d0, d1, d2)`, `tensor.get(index)`, `tensor.set(index, value)`, `tensor.push(value)`. | Constructor overloads are straightforward by arity. `get` needs tensor element-type context; avoid return-type-only dispatch. |
| String | `hoo_string_new`, `hoo_string_from_cstr`, `hoo_string_from_bytes`, `hoo_string_from_object`, `hoo_string_from_any`, `hoo_string_from_int64`, `hoo_string_from_double`, `hoo_string_from_bool`, `hoo_string_repeat`. JIT registers `String_fromCStr_static`, `String_fromInt64_static`, `String_fromDouble_static`, `String_fromAny_static`, etc. | `String.from(value)`, `String.valueOf(value)`, `new String()`, `new String(value)`, `String.repeat(char, count)`. | Good constructor/static-method overload target. `String.from(any)` must be ranked lower than exact primitive and object overloads. |
| Buffer | `hoo_buffer_new(capacity)`, `hoo_buffer_from_bytes`, `hoo_buffer_copy`, `hoo_buffer_append(data, len)`, `hoo_buffer_append_buffer`. Tests currently mark capacity construction and static `fromBytes` as unsupported at the language/JIT layer. | `new Buffer()`, `new Buffer(capacity)`, `Buffer.from(bytes)`, `Buffer.from(Buffer)`, `buffer.append(bytes)`, `buffer.append(Buffer)`. | Requires constructor overload metadata because codegen currently treats several built-in constructors as fixed-arity special cases. |
| Args | `hoo_args_add_string`, `add_int`, `add_flag`, `add_float`, `add_positional`; getters `get_string`, `get_int`, `get_bool`, `get_float`. | `args.add(...)` overloaded by default/value type and option kind; `args.get<T>(name)` or typed `get` only after target typing/generics. | `add` is a good overload family. `get` should not be overloaded by return type alone unless generic call syntax or target-type resolution exists. |
| Regex | `hoo_regex_compile(pattern)` and `hoo_regex_compile_with_flags(pattern, flags)`. | `Regex.compile(pattern)`, `Regex.compile(pattern, flags)`, `new Regex(pattern)`, `new Regex(pattern, flags)`. | Simple arity-based overload; useful early implementation test. |
| DateTime | `hoo_datetime_new(epoch_ms)`, `hoo_datetime_new_now`, `hoo_datetime_new_from_iso8601`, `hoo_datetime_new_parse(str, format)`, plus `parse(str, format)` and many instance/free duplicate helpers. | `new DateTime()`, `new DateTime(epochMs)`, `DateTime.parse(str)`, `DateTime.parse(str, format)`, `DateTime.from(str)`. | Constructors and `parse` are clean overloads. Unit-specific methods such as `diffDays` and `diffHours` should remain separate. |
| Character | `hoo_character_from_codepoint(int64)`, `hoo_character_from_utf8(bytes, length)`. | `new Character(codepoint)`, `Character.from(codepoint)`, `Character.from(string)` or `Character.fromUtf8(string)`. | `from(int64)` vs `from(string)` is a clear overload. Raw bytes plus length should stay an ABI detail unless the language exposes byte slices. |
| UUID | `hoo_uuid_from_string`, `hoo_uuid_from_bytes`, `hoo_uuid_from_bytes_buffer`; `hoo_uuid_to_bytes` and `hoo_uuid_to_bytes_buffer`. | `Uuid.from(string)`, `Uuid.from(Buffer)`, `Uuid.from(bytes)`, `uuid.toBytes()`. | Prefer a single source-level `toBytes()` returning `Buffer`; raw out-param functions should remain runtime implementation details. |
| Exception | `hoo_exception_create(type, message)` and `hoo_exception_create_with_cause(type, message, cause)`. | `Exception.create(type, message)`, `Exception.create(type, message, cause)`. | Straightforward arity-based overload and useful for testing overload resolution in runtime error paths. |

### Free Function and Static API Candidates

#### Math
The runtime already implements the exact shape expected by overloading:

- Unary overloads: `abs(int64)`, `abs(int8)`, `abs(byte)`, `abs(double)`, `abs(f8)`.
- Binary overloads: `min(T, T)` and `max(T, T)` for the same numeric set.
- Unary sign overloads: `sign(int64)`, `sign(int8)`, `sign(byte)`, `sign(double)`, `sign(f8)`.

Current codegen has special return-type inference for unsuffixed `math_abs`, `math_min`, `math_max`, and `math_sign`, so the language surface is already drifting toward overloaded names. The missing part is a real overload-set table and a resolver that maps argument type IDs to the concrete runtime symbol.

#### Console / IO
`print` and `println` are the canonical user-facing overloads. Today the runtime only exposes pointer/string-style `hoo_print` and `hoo_println`, while `hoo_string_from_int64`, `hoo_string_from_double`, `hoo_string_from_bool`, `hoo_string_from_any`, and related helpers provide the conversion backend. The overloading implementation should add either:

- source overloads that lower to explicit conversion plus `hoo_print` / `hoo_println`; or
- small runtime wrappers for each primitive and object type.

The first approach keeps the C ABI smaller and centralizes formatting in `String`.

#### Encoding
`hoo_encoding.h` has parallel raw-byte and `Buffer` variants:

- `base64_encode(data, len)` and `base64_encode_buffer(Buffer)`.
- `base64_decode(text, out)` and `base64_decode_buffer(text)`.
- `hex_encode(data, len)` and `hex_encode_buffer(Buffer)`.
- `hex_decode(text, out)` and `hex_decode_buffer(text)`.
- URL encode/decode are string-only and do not need overloads today.

Good overloads:

- `Encoding.base64Encode(Buffer)`.
- `Encoding.base64Encode(bytes)` if byte slices are first-class.
- `Encoding.hexEncode(Buffer)`.
- `Encoding.hexEncode(bytes)`.

Avoid `base64Decode(string)` overloads that differ only by return representation. Use `base64DecodeBytes`, `base64DecodeBuffer`, or a future `base64Decode<T>(string)` form.

#### Hashing
`hoo_hashing.h` uses representation suffixes:

- `sha256(data, len)`, `sha256_file(path)`, `sha256_buffer(Buffer)`.
- `sha1(data, len)`, `sha1_buffer(Buffer)`.
- `md5(data, len)`, `md5_buffer(Buffer)`.
- `crc32(data, len)`, `crc32_buffer(Buffer)`.
- `hmac_sha256(key bytes, data bytes)`, `hmac_sha256_buffer(Buffer key, Buffer data)`.

Good overloads:

- `Hashing.sha256(Buffer)`, `sha1(Buffer)`, `md5(Buffer)`, `crc32(Buffer)`.
- raw byte overloads only after the language has a stable byte-slice type.
- `Hashing.hmacSha256(Buffer, Buffer)`.

Do not collapse `sha256_file(path)` into `sha256(string)`. A string can mean literal content or a filesystem path, so `sha256File(path)` should stay explicit unless the API introduces a distinct `Path` type.

#### Filesystem and Path
`hoo_fs.h` exposes text and byte-oriented functions:

- `read_text(path)`, `read_bytes(path, out)`, `read_bytes_buffer(path)`.
- `write_text(path, content)`, `write_bytes(path, data, len)`, `write_bytes_buffer(path, Buffer)`.
- `append_text(path, content)`.
- `path_join(a, b)` and `path_join_multi(parts, count)`.

Good overloads:

- `Fs.write(path, string)`.
- `Fs.write(path, Buffer)`.
- `Fs.append(path, string)` now, with `Fs.append(path, Buffer)` later if implemented.
- `Path.join(string, string)`.
- `Path.join(Array<string>)`.

Avoid `Fs.read(path)` overloads that differ only by return type. Keep `readText`, `readBytes`, or use `read<T>(path)` once generics/target typing exists.

#### JSON
`hoo_json.h` exposes:

- `serialize_hashmap(HashMap)`.
- `serialize_anyarray(AnyArray)`.
- `deserialize_hashmap(json)`.
- `deserialize_anyarray(json)`.

Good overloads:

- `Json.serialize(HashMap)`.
- `Json.serialize(AnyArray)`.

Do not overload `Json.deserialize(string)` by return type only. Prefer `Json.deserialize<T>(json)` after generic call syntax exists, or keep explicit `deserializeHashMap` / `deserializeAnyArray`.

#### Process and System
Most process/system functions are semantic variants, not overloads:

- `exec(command)` vs `exec_status(command)` differ in result shape and behavior.
- `capture(command)` vs `capture_status(command, out_stdout, out_exit)` differ by output contract.
- `spawn(command, argv, out_pid)` could eventually become `spawn(command)` and `spawn(command, args)`.

Keep explicit names until the language has result objects or multi-return values.

### OO / Instance Method Candidates

#### Built-in Constructors
`HVMCodeGenerator.cpp` currently validates several built-in constructors manually by class and arity. Examples include fixed handling for `AnyArray`, `Buffer`, `Map`, `Uuid`, and `Character`. This should move to constructor overload metadata so constructor resolution follows the same path as method resolution.

Recommended constructor overloads:

- `new AnyArray()` and `new AnyArray(capacity)`.
- `new Buffer()` and `new Buffer(capacity)`.
- `new Map(keyType, valueType)` and future shorthand constructors if added.
- `new Uuid(string)` and `new Uuid(Buffer)`.
- `new Character(codepoint)` and `new Character(string)`.
- `new Regex(pattern)` and `new Regex(pattern, flags)`.
- `new DateTime()` and `new DateTime(epochMs)`.
- `new Tensor(type, d0)`, `new Tensor(type, d0, d1)`, `new Tensor(type, d0, d1, d2)`.

#### Arrays and AnyArray
For typed arrays, the current source surface leaks implementation names such as `pushInt64`, `pushDouble`, `pushString`, `pushObject`, and typed getters. The source-level API should be:

```hoo
array.push(1)
array.push(1.5)
array.push("value")
array.set(0, "value")
let value = array.get(0)
```

Resolution requirements:

- `push(value)` chooses by value expression type and declared array element type.
- `set(index, value)` chooses by value type and validates assignability.
- `get(index)` uses the declared element type, not return-type-only overloading.
- `AnyArray.push(value)` and `AnyArray.set(index, value)` lower to tagged `type_id + data` runtime functions.

#### Map and HashMap
The current JIT has many explicit `map_set_*_*` and `map_get_*_*` wrappers. This is a strong sign that overloads should be driven by generic container type metadata:

```hoo
map.set(1, "one")
map.set("name", 42)
let x = map.get("name")
hashMap.set(10, value)
```

Resolution requirements:

- Use `Map<K,V>` or `HashMap<K,V>` type arguments as primary dispatch facts.
- Use key type for `get`, `containsKey`, and `remove`.
- Use both key and value type for `set`.
- Preserve separate fixed-value and any-value HashMap implementations, but hide them behind overload selection.

#### Buffer
Runtime functions support both raw bytes and buffer-to-buffer append/copy operations. The source API can be simplified to:

```hoo
let a = new Buffer()
let b = new Buffer(1024)
a.append(b)
a.append(bytes)
```

The current language/JIT tests identify some Buffer constructor/static factory shapes as unsupported. Those should become explicit overload-resolution test cases when implemented.

#### DateTime
DateTime has several constructor/factory shapes and separate instance/free variants for arithmetic. Good overload targets:

- `DateTime.parse(string)`.
- `DateTime.parse(string, format)`.
- `new DateTime()`.
- `new DateTime(epochMs)`.

Keep semantic unit methods separate: `addDays`, `addHours`, `diffDays`, `diffHours`, and `diffSeconds` communicate different operations and should not be collapsed.

#### Args
Argument parser setup is currently split by typed names:

- `addString`, `addInt`, `addFlag`, `addFloat`, `addPositional`.
- `getString`, `getInt`, `getBool`, `getFloat`.

Good overloads:

- `args.add(name, short, long, help, defaultString)`.
- `args.add(name, short, long, help, defaultInt)`.
- `args.add(name, short, long, help, defaultFloat)`.
- `args.addFlag(name, short, long, help)` or `args.add(name, short, long, help, defaultBool)` if flags are modeled as bool options.

Getter overloads need either generic syntax or target-type resolution:

```hoo
let port = args.get<int64>("port")
let verbose = args.get<bool>("verbose")
```

Until then, typed getters should remain available to avoid return-type-only overloading.

#### CSV
CSV exposes constructor and parse variants:

- `new()`, `new_with_opts(delimiter, quote)`, `from_opts(delimiter, quote)`.
- `parse(csv, text)` vs `parse_as_maps(csv, text)`.
- `read_file(csv, path)` vs `read_file_as_maps(csv, path)`.

Good overloads:

- `new Csv()`.
- `new Csv(delimiter, quote)`.
- `Csv.fromOptions(delimiter, quote)`.

Avoid overloading `parse(text)` or `readFile(path)` when the only difference is array-of-arrays vs array-of-maps return shape. Keep `parseAsMaps` / `readFileAsMaps`, or use future generic APIs.

#### Network
The network runtime has URL, HttpClient, and HttpResponse wrappers with several JIT aliases that differ only in receiver handling. Good overloads:

- `client.get(string)` and `client.get(URL)`.
- `client.post(string, string)` and future `client.post(URL, string)` / `client.post(URL, Buffer)`.
- `client.put(string, string)` and future URL/Buffer variants.
- `client.setHeader(key, value)` should have one source method; ABI receiver differences should be hidden.

This area needs a method resolver that treats the receiver type as part of the method lookup key while keeping the implicit receiver out of the source argument list.

#### Compression
Compression APIs have raw-byte and `Buffer` forms:

- `gzipCompress(data, len)` and `gzipCompress(Buffer)`.
- `gzipDecompress(data, len)` and `gzipDecompress(Buffer)`.
- `deflateCompress(data, len)` and `deflateCompress(Buffer)`.
- `deflateDecompress(data, len)` and `deflateDecompress(Buffer)`.

Good overloads should prefer `Buffer` and future byte-slice types. Avoid exposing raw pointer/length pairs at the Hoo source level.

### APIs That Should Not Be Overloaded Yet

| API family | Reason to defer or keep explicit |
|------------|----------------------------------|
| `Json.deserializeHashMap` vs `Json.deserializeAnyArray` | Return-type-only overload without generic target typing. |
| `Fs.readText` vs `Fs.readBytes` | Return-type-only overload and materially different representation. |
| `Csv.parse` vs `Csv.parseAsMaps` | Return type and data model differ; explicit names are clearer. |
| `Encoding.base64Decode` raw bytes vs `base64DecodeBuffer` | Same input type with different output representation. |
| `Hashing.sha256(string)` vs `sha256File(path)` | A string path and string content are semantically ambiguous. |
| `System.exec` vs `execStatus` | Different result contract, not just parameter types. |
| `Process.capture` vs `captureStatus` | Requires result objects or multi-return support before unification is safe. |
| Date/time unit methods such as `diffDays`, `diffHours`, `diffSeconds` | The unit is part of the operation name, not an overload dimension. |

### Implementation Implications From This Audit

1. **Built-in overload metadata is required.** The current codegen uses hard-coded helpers such as math/free-function recognition, hashing/encoding special cases, and built-in constructor arity checks. Add a central table that describes built-in overload sets with source name, receiver type, parameter type list, return type, and target runtime symbol.

2. **Receiver type must participate in method lookup.** Instance overload resolution should use `(receiver_type, method_name, parameter_types)` as the lookup key. The implicit receiver should not be counted as a source argument, even if the lowered ABI symbol includes it.

3. **Return-type-only overloading should be out of phase 1.** Several runtime APIs have the same parameter types but different return representations. Supporting those safely requires generic call syntax or target-type-directed resolution. Phase 1 should reject those as ambiguous and keep explicit API names.

4. **Generic/container type context is necessary.** `Array.get`, `Map.get`, `HashMap.get`, and `Tensor.get` cannot be resolved from runtime argument types alone. The resolver needs declared element/key/value type metadata from semantic analysis.

5. **Ranking rules need to be explicit.** Recommended ranking:
   - exact type match;
   - safe widening conversion, for example `int8`/`byte` to `int64` or `f8` to `double` if those conversions are already language-approved;
   - literal-compatible conversion, for example string literal to `String`;
   - `any` / object fallback last.

6. **Existing ABI names should remain stable.** Overloading should change the Hoo source surface and compiler/JIT resolution, not require renaming existing C runtime functions. Mangled Hoo overload symbols can point at the existing runtime symbols.

7. **Diagnostics should be source-oriented.** Errors should report the overload family and candidate signatures, not internal symbol names such as `map_set_int64_string` or `hoo_array_push_double`.

8. **JIT symbol registration can be simplified gradually.** Keep current aliases for compatibility, then add canonical overloaded registrations. Once call lowering uses overload metadata, many duplicate source aliases can become legacy-only.

### Runtime-Driven Test Plan

Add focused tests as the feature is implemented:

- `Math.abs`, `Math.min`, `Math.max`, and `Math.sign` choose exact primitive overloads.
- `print` / `println` accept string, int64, double, bool, Character, and any/object values.
- `String.from` chooses primitive overloads before `any`.
- `Regex.compile(pattern)` and `Regex.compile(pattern, flags)` resolve by arity.
- `DateTime.parse(text)` and `DateTime.parse(text, format)` resolve by arity.
- `new Buffer()` and `new Buffer(capacity)` resolve through constructor overload metadata.
- `buffer.append(Buffer)` and `buffer.append(bytes)` select different runtime targets.
- `array.push(value)` and `array.set(index, value)` validate declared element type.
- `map.set(key, value)` and `map.get(key)` resolve from generic `Map<K,V>` metadata.
- `HashMap.set` chooses fixed-value vs any-value runtime path correctly.
- `Tensor.new` resolves 1D/2D/3D constructors by arity.
- `Json.serialize(HashMap)` and `Json.serialize(AnyArray)` choose separate overloads.
- Negative tests reject `Json.deserialize(text)` and `Fs.read(path)` when no target type/generic form is provided.
- Negative tests report `NoMatchingOverloadException` for unsupported argument types and `AmbiguousOverloadException` for equal-ranked candidates.

### Source Hotspots Identified During Scan

These are the concrete source locations that should be touched or reviewed during implementation.

| File | Current role | Overloading impact |
|------|--------------|--------------------|
| `src/codegen/HVMCodeGenerator.cpp:94` | `classToPrefix` maps built-in classes to runtime symbol prefixes. | Replace prefix-only construction with overload metadata lookup for built-in static and instance methods. |
| `src/codegen/HVMCodeGenerator.cpp:152` | `builtinConstructorMethodName(className, argCount)` chooses constructor runtime names by class and arity. | Move constructor handling into the same overload resolver used for normal methods. |
| `src/codegen/HVMCodeGenerator.cpp:234` | `isEncodingFreeFunction` recognizes encoding free functions. | Convert encoding raw/buffer variants into overload-set entries where parameter types differ. |
| `src/codegen/HVMCodeGenerator.cpp:250` | `isMathFreeFunction` recognizes unsuffixed math functions. | First built-in free-function overload family to migrate because existing code already expects unsuffixed names. |
| `src/codegen/HVMCodeGenerator.cpp:262` | `isHashingFreeFunction` recognizes hashing variants. | Split safe overloads (`Buffer`) from semantic names (`sha256File`). |
| `src/codegen/HVMCodeGenerator.cpp:462` | Return type inference special-cases encoding functions. | Replace with overload candidate return metadata after resolution. |
| `src/codegen/HVMCodeGenerator.cpp:469` | Return type inference special-cases math functions with argument type IDs. | Reuse this logic as the seed for overload ranking. |
| `src/codegen/HVMCodeGenerator.cpp:1488` | Tensor literal/new lowering emits `Tensor_new1`, `new2`, or `new3`. | Replace arity branch with constructor overload resolution. |
| `src/codegen/HVMCodeGenerator.cpp:1907` | Array literal lowering emits typed `Array_push*` calls. | Use element type and value type to resolve `Array.push(value)`. |
| `src/codegen/HVMCodeGenerator.cpp:2133` | Built-in constructor lowering composes prefix plus constructor method name. | Should become a call to `resolveBuiltinConstructorOverload`. |
| `src/codegen/HVMCodeGenerator.cpp:2525` | Method dispatch explicitly recognizes `push`, `pushInt64`, `pushDouble`, `pushString`, and similar aliases. | Collapse named variants into overloads for `push` and retain old names as compatibility aliases. |
| `src/codegen/HVMCodeGenerator.cpp:2991` | HashMap assignment lowering chooses `set_any_i8` or `set_fixed_i8`. | Turn current storage-mode choice into an overload-resolution result. |
| `src/codegen/HVMCodeGenerator.cpp:3526` | Tensor shape construction emits arity-suffixed constructors. | Same constructor-overload work as the tensor literal path. |
| `src/hvm/HVMJIT.cpp:3573` | String runtime symbols register several `String_from_*` variants. | Register canonical overload metadata for `String.from(...)` while keeping aliases. |
| `src/hvm/HVMJIT.cpp:3650` | Array push runtime symbols register `Array_pushInt64`, `Array_pushString`, `Array_pushBool`, `Array_pushDouble`, etc. | Attach all of these to one `Array.push` overload set. |
| `src/hvm/HVMJIT.cpp:3695` | Tensor constructor runtime symbols register `Tensor_new1`, `Tensor_new2`, `Tensor_new3`, and generic `Tensor_new`. | Attach arity variants to one `Tensor.new` constructor overload set. |
| `src/hvm/HVMJIT.cpp:3725` | Map wrappers register `map_set_*_*` variants. | Attach key/value combinations to `Map.set` overload metadata. |
| `src/hvm/HVMJIT.cpp:3761` | HashMap wrappers register fixed and any value setters. | Attach storage-specific runtime targets to `HashMap.set` based on value type metadata. |
| `src/hvm/HVMJIT.cpp:3782` | JIT registers method-style array aliases. | Mark as legacy aliases after overload lowering is available. |
| `src/hvm/HVMJIT.cpp:3818` | JIT registers method-style map aliases. | Mark as legacy aliases after overload lowering is available. |
| `src/runtime/lib/hoo_math.h:73` | Math type-specific `abs` declarations start here. | Direct implementation targets for `Math.abs` overloads. |
| `src/runtime/lib/hoo_generic_array.h:135` | Typed array `push` declarations start here. | Direct implementation targets for `Array.push` overloads. |
| `src/runtime/lib/hoo_generic_array.h:211` | Typed array `get` declarations start here. | Use only with element-type/target-type context; not return-only dispatch. |
| `src/runtime/lib/hoo_map.h:91` | Generic map set/get/contains/remove C ABI. | Runtime can stay generic while JIT/source wrappers are resolved by `Map<K,V>`. |
| `src/runtime/lib/hoo_hashmap.h:23` | HashMap fixed/any set/get declarations. | Direct implementation targets for `HashMap.set` / `HashMap.get`. |
| `src/runtime/lib/hoo_tensor.h:11` | Tensor constructor declarations. | Direct implementation targets for constructor overloads. |
| `src/runtime/lib/hoo_string.cpp:623` | Primitive string conversion implementations start here. | Direct implementation targets for `String.from` and print lowering. |
| `src/runtime/lib/hoo_buffer.h:14` | Buffer constructor/factory declarations. | Direct implementation targets for `Buffer` constructor and `Buffer.from` overloads. |
| `src/runtime/lib/hoo_args.h:51` | Typed argument option adders start here. | Direct implementation targets for `Args.add` overloads. |
| `src/runtime/lib/hoo_args.h:70` | Typed argument getters start here. | Keep typed or generic; do not overload by return type in phase 1. |
| `src/runtime/lib/hoo_json.h:23` | JSON serialize/deserialize declarations. | `serialize` is safe to overload; `deserialize` requires generic/target typing. |
| `src/runtime/lib/hoo_encoding.h:10` | Encoding raw/buffer declarations. | Safe for encode overloads by input representation; decode needs target typing. |
| `src/runtime/lib/hoo_hashing.h:11` | Hashing raw/file/buffer declarations. | Safe for Buffer overloads; keep file hashing explicit. |
| `src/runtime/lib/hoo_compression.h:12` | Compression raw/buffer declarations. | Safe for Buffer overloads; raw byte overloads wait for byte slices. |

### Proposed Built-in Overload Metadata Shape

The current implementation spreads built-in knowledge across codegen helpers and JIT symbol aliases. Add a central description that both semantic/codegen and JIT registration can consume:

```cpp
enum class OverloadKind {
    FreeFunction,
    StaticMethod,
    InstanceMethod,
    Constructor
};

struct BuiltinOverloadCandidate {
    OverloadKind kind;
    std::string sourceName;      // "abs", "from", "push", "new"
    std::string receiverType;    // empty for free functions, "Array" for methods
    std::vector<int64_t> params; // source-level argument type IDs, no implicit receiver
    int64_t returnType;
    std::string runtimeSymbol;   // existing C ABI or JIT symbol
    int rankBias;                // exact wrappers before any/object fallbacks
    bool legacyAlias;
};
```

Initial built-in overload registry entries should include:

```cpp
// Free/static examples
{FreeFunction, "math_abs", "", {HOO_TYPE_INT64}, HOO_TYPE_INT64,
 "_F_hoo_math_abs_int64_i8_i8", 0, false}

{StaticMethod, "from", "String", {HOO_TYPE_INT64}, HOO_TYPE_STRING,
 "_F_M_hoo_E_String_fromInt64_static_p_i8", 0, false}

{StaticMethod, "from", "String", {HOO_TYPE_ANY}, HOO_TYPE_STRING,
 "_F_M_hoo_E_String_fromAny_static_p_i8_i8", 20, false}

// Instance examples
{InstanceMethod, "push", "Array<int64>", {HOO_TYPE_INT64}, HOO_TYPE_ARRAY,
 "_F_hoo_Array_pushInt64_p_i8", 0, false}

{Constructor, "new", "Buffer", {}, HOO_TYPE_BUFFER,
 "_F_M_hoo_E_Buffer_new_static_p", 0, false}

{Constructor, "new", "Buffer", {HOO_TYPE_INT64}, HOO_TYPE_BUFFER,
 "_F_hoo_buffer_new_p_i8", 0, false}
```

The exact symbol strings must be verified against the final JIT symbol table when implemented. The important design requirement is that source signatures live in one registry and existing ABI symbols remain targets.

### Overload Resolution Algorithm for Phase 1

Phase 1 should use compile-time/static type information wherever available. Runtime lookup should be a fallback for dynamic `any` values, not the normal path for fully typed calls.

1. Build candidate list by `(kind, receiver type if any, source name, arity)`.
2. Reject candidates whose parameter count differs from the source argument count.
3. Score each argument:
   - `0`: exact type match.
   - `1`: approved numeric widening.
   - `2`: literal conversion that preserves value.
   - `3`: nullable/object-compatible conversion.
   - `20`: `any` fallback.
   - reject: unsafe narrowing, incompatible container element type, or missing conversion.
4. Sum candidate scores plus `rankBias`.
5. Choose the single lowest score.
6. If no candidate remains, emit `NoMatchingOverloadException` or a compile-time semantic diagnostic.
7. If more than one candidate has the same lowest score, emit `AmbiguousOverloadException` or a compile-time ambiguity diagnostic.
8. Cache the resolved runtime symbol at the call site for JIT emission.

Recommended phase 1 conversion policy:

- Allow `int8 -> int64`, `byte -> int64`, and `f8 -> double` only if these conversions already exist in normal assignment/call semantics.
- Do not allow `double -> int64`, `int64 -> int8`, or `int64 -> byte` as implicit overload conversions.
- Prefer exact `String`/`Character` overloads over object/any overloads.
- Treat `null` as compatible only with managed/object-like types, not primitive numeric types.
- Treat `any` as a last-resort overload match, and keep an exact typed overload preferred when the static type is known.

### Phase-by-Phase Implementation Plan

#### Phase 0: Metadata and Diagnostics Only

- Add built-in overload metadata structures without changing source behavior.
- Populate metadata for Math, String conversion, Regex compile, DateTime parse, Buffer constructor/append, and Tensor constructors.
- Add a debug/test-only dump that lists overload sets and target symbols.
- Add semantic diagnostics formatting that prints source-level candidates.
- Keep existing named entry points working exactly as they do now.

#### Phase 1: Static Resolution for Built-ins

- Resolve built-in free/static/constructor calls at compile/codegen time.
- Replace `isMathFreeFunction` return inference with metadata-driven resolution.
- Replace `builtinConstructorMethodName` special cases with constructor overload lookup.
- Lower resolved calls to existing `CALL` with concrete runtime symbols.
- Do not add `CALL_OVERLOADED` yet for statically resolved calls.

Recommended first families:

- `Math.abs/min/max/sign`.
- `Regex.compile`.
- `DateTime.parse`.
- `new Buffer()` / `new Buffer(capacity)`.
- `Tensor.new` arity overloads.
- `String.from`.

#### Phase 2: Instance Methods and Containers

- Add receiver-aware method overload lookup.
- Resolve `Array.push`, `Array.set`, `Map.set`, `Map.get`, `HashMap.set`, `HashMap.get`, and `Tensor.get/set/push`.
- Feed declared container element/key/value types into overload selection.
- Keep typed aliases such as `pushInt64` and `map_set_int64_string` as compatibility paths.
- Add negative tests for element-type mismatch and ambiguous `any` values.

#### Phase 3: Runtime Overload Registry for Dynamic Calls

- Add `src/runtime/lib/hoo_overload.h` and implementation.
- Register runtime overload sets for dynamic or reflective calls.
- Introduce `CALL_OVERLOADED` only for call sites that cannot be statically resolved.
- Add JIT inline/cache support so the first runtime resolution can be reused.
- Add runtime exceptions for `AmbiguousOverloadException` and `NoMatchingOverloadException`.

#### Phase 4: API Cleanup and Compatibility Window

- Prefer overloaded source APIs in docs and examples.
- Keep old explicitly typed names as deprecated aliases.
- Add deprecation diagnostics only after all existing tests and examples migrate.
- Keep C ABI symbols stable indefinitely unless a separate ABI-breaking runtime version is planned.

### Per-Module Resolution Matrix

| Module/API | Phase | Source overload | Target symbols | Notes |
|------------|-------|-----------------|----------------|-------|
| Math | 1 | `abs(T)`, `min(T,T)`, `max(T,T)`, `sign(T)` | `hoo_math_*_<type>` | Pure exact/widening dispatch. |
| String | 1 | `String.from(T)`, `new String(T)` | `hoo_string_from_*`, JIT `String_from*` aliases | `any` fallback must rank last. |
| Regex | 1 | `Regex.compile(pattern[, flags])` | `hoo_regex_compile*` | Arity-only baseline test. |
| DateTime | 1 | `new DateTime()`, `new DateTime(epochMs)`, `parse(text[, format])` | `hoo_datetime_new*` | Keep unit-specific methods separate. |
| Buffer | 1 | `new Buffer([capacity])`, `Buffer.from(...)`, `append(...)` | `hoo_buffer_new`, `hoo_buffer_from_bytes`, `hoo_buffer_append*` | `bytes` overload waits for byte-slice type if unavailable. |
| Tensor constructors | 1 | `Tensor.new(type, dims...)` | `hoo_tensor_new1/2/3` | Arity-based. |
| Array | 2 | `push(value)`, `set(index,value)`, `get(index)` | `hoo_array_push_*`, `hoo_array_get_*` | Needs element type. |
| Map | 2 | `set(key,value)`, `get(key)`, `containsKey(key)`, `remove(key)` | `hoo_map_*`, JIT typed wrappers | Needs `Map<K,V>`. |
| HashMap | 2 | `set(key,value)`, `get(key)` | `hoo_hashmap_*_i8` | Needs fixed vs any value decision. |
| Tensor access | 2 | `get(index)`, `set(index,value)`, `push(value)` | `hoo_tensor_get_*`, bit setters | Needs element type. |
| Console/IO | 2 | `print(T)`, `println(T)` | `hoo_print`, `hoo_println`, `hoo_string_from_*` | Lower through String conversions. |
| JSON serialize | 2 | `Json.serialize(HashMap|AnyArray)` | `hoo_json_serialize_*` | Safe by parameter type. |
| JSON deserialize | 3+ | `Json.deserialize<T>(text)` | `hoo_json_deserialize_*` | Requires generic/target type. |
| Fs write | 2 | `Fs.write(path, string|Buffer)` | `hoo_fs_write_text`, `hoo_fs_write_bytes_buffer` | Safe by second parameter type. |
| Fs read | 3+ | `Fs.read<T>(path)` | `hoo_fs_read_text`, `hoo_fs_read_bytes_buffer` | Requires generic/target type. |
| Encoding encode | 2 | `base64Encode(Buffer)`, `hexEncode(Buffer)` | `hoo_encoding_*_buffer` | Raw bytes wait for byte slices. |
| Encoding decode | 3+ | `base64Decode<T>(string)` | `hoo_encoding_*decode*` | Return representation differs. |
| Hashing | 2 | `sha256(Buffer)`, `sha1(Buffer)`, `md5(Buffer)`, `crc32(Buffer)` | `hoo_hashing_*_buffer` | Keep `sha256File` explicit. |
| Args add | 2 | `args.add(...)` | `hoo_args_add_*` | Good source cleanup. |
| Args get | 3+ | `args.get<T>(name)` | `hoo_args_get_*` | Requires generic/target type. |
| CSV constructors | 1 | `new Csv()`, `new Csv(delimiter, quote)` | `hoo_csv_new*` | Arity-based. |
| CSV parse/read maps | 3+ | generic parse/read if desired | `hoo_csv_parse*`, `hoo_csv_read_file*` | Return model differs. |
| Net | 2 | `client.get(string|URL)`, `post(...)`, `put(...)` | HttpClient JIT/runtime symbols | Needs receiver-aware method lookup. |
| Compression | 2 | `gzipCompress(Buffer)`, `deflateCompress(Buffer)` | `hoo_compression_*_buffer` | Raw bytes wait for byte slices. |

---
## Detailed Work Plan
### 1. Grammar & Parser
1. Modify `parser.y` (or equivalent) to allow repeated `function_decl` productions with the same identifier.
2. Introduce rule `function_decl_list : function_decl_list function_decl | function_decl`.
3. Attach semantic actions that build an `OverloadSet` entry in the AST.
4. Update lexer to recognize the same identifier token without conflict.

### 2. AST & Semantic Analysis
* Extend `FunctionDecl` structure:
```c
struct FunctionDecl {
    std::string name;
    std::vector<Type> param_types; // concrete Hoo type IDs
    Type return_type;
    bool is_method; // true for class members
    // ... existing fields
};
```
* In the symbol builder, group declarations with identical `name` into an `OverloadSet`.
* Perform **overload resolution** during type‑checking of call expressions:
  - Gather candidate functions.
  - Rank by exact match, then by implicit conversion rules.
  - If none match → emit `NoMatchingOverloadException`.
  - If multiple best matches → emit `AmbiguousOverloadException`.

### 3. Name Mangling & Demangling
* New format: `<base>__<type1>_<type2>_…` where each `typeN` is the numeric type ID defined in `hoo_runtime.h` (e.g., `i64` for `HOO_TYPE_INT64`).
* Implement helper `std::string mangle_overload(const FunctionDecl&)`.
* Extend `hoo_demangle.cpp` to parse this schema and return both base name and vector of type IDs.

### 4. Symbol Table & Resolution Engine
* Change symbol table entry type from `FunctionSymbol*` to `OverloadSet*`:
```c
struct OverloadSet {
    std::string base_name;
    std::vector<FunctionSymbol*> overloads;
};
```
* Provide API `FunctionSymbol* resolve_overload(const std::string& base, const std::vector<int64_t>& arg_types)`.
* Throw the new runtime exceptions on failure.

### 5. Code Generation
* **Bytecode**: Add opcode `CALL_OVERLOADED` that carries the mangled name as an immediate operand.
* **JIT**: Emit a call to `hoo_resolve_overload` at the call site, then an indirect call to the returned pointer.
* Update the optimizer to treat overloaded calls like normal calls after resolution.

### 6. Runtime API Adjustments
Create header `runtime/lib/hoo_overload.h` with:
```c
/* Resolve an overloaded function at runtime */
void* hoo_resolve_overload(const char* mangled_name,
                           const int64_t* arg_type_ids,
                           size_t argc);

/* Exception registration (optional) */
void hoo_register_overload_exception(int64_t type_id, const char* message);
```
Add corresponding implementations that look up the overload set, perform matching, and return the concrete function pointer.

### 7. Exception Types
Add new type IDs in `hoo_runtime.h`:
```c
#define HOO_TYPE_AMBIGUOUS_OVERLOAD 130
#define HOO_TYPE_NO_MATCHING_OVERLOAD 131
```
Provide constructors `hoo_ambiguous_overload_new(message)` and `hoo_no_matching_overload_new(message)`.

### 8. Testing Strategy
* **Positive Tests** – simple overloads, method overloads, generic overloads.
* **Negative Tests** – ambiguous calls, mismatched argument count, unsupported conversions.
* **Integration Tests** – verify that generated bytecode/JIT correctly resolves at runtime.
* Update `tests/runtime/FunctionOverloadTest.cpp` with coverage of all cases.

### 9. Documentation Updates
* Add a section *Function Overloading* to `docs/runtime/api/index.md`.
* Create a new spec file `docs/specs/function_overloading.md` describing the language rule.
* Update `README.md` with a short feature overview and examples.

---
## Impact Assessment
* **Binary Compatibility** – Existing symbols remain unchanged; new mangled names are added, so old binaries continue to work.
* **Performance** – Overload resolution adds a small overhead at call sites (type introspection + hash lookup). The JIT can cache resolved pointers after the first call to mitigate.
* **Build System** – No changes needed beyond recompiling the runtime and compiler components.
* **Backward Compatibility** – Code that previously relied on manual name mangling will still compile because the old mangling scheme is retained for non‑overloaded functions.

---
## Implementation Timeline (approx.)
| Sprint | Tasks |
|--------|-------|
| 1 | Grammar, parser changes, AST extensions |
| 2 | Symbol table overhaul, overload set construction |
| 3 | Mangling/demangling implementation, runtime API stub |
| 4 | Bytecode & JIT integration, exception types |
| 5 | Unit & integration tests, documentation updates |
| 6 | Performance tuning, final review, merge |

---
*Prepared by Antigravity AI – 2026‑06‑21*

## Status
- **Date**: 2026-06-21
- **Status**: **PROPOSED (UNIMPLEMENTED)**
- **Priority**: High
- **Audit 2026-06-21**: No overload-set resolver, `CALL_OVERLOADED` opcode, runtime `hoo_resolve_overload`, or argument-type-aware call lowering was found. Existing symbol mangling tests do not amount to function overloading support.
- **Runtime scan 2026-06-21**: Added a runtime implementation audit covering free functions, static APIs, built-in constructors, and OO methods where overloads can replace current type-suffixed, arity-suffixed, representation-suffixed, and receiver-alias symbols. The first implementation pass should prioritize Math, Console/IO, String factories, Buffer constructors/append, Regex/DateTime arity overloads, Array/Map/HashMap/Tensor typed methods, and JSON serialization. Return-type-only families such as JSON deserialization, filesystem reads, CSV map parsing, and byte-vs-buffer decoders should remain explicit until generic or target-type-directed resolution exists.
- **Detail pass 2026-06-21**: Added source hotspots with file/line anchors, a proposed built-in overload metadata shape, a phase-1 static overload resolution algorithm, a phased implementation sequence, and a per-module resolution matrix mapping source overloads to existing runtime/JIT target symbols.
