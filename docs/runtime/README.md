# hoo Runtime Library (`hoort`) Reference

This directory contains the normative documentation for the hoo Runtime Library (`hoort`). The runtime provides the high-level services and intrinsic functions necessary to execute hoo applications, acting as a bridge between the physical HVM RISC core and the host system.

> **Note on syntax**: The runtime API uses class-based method-call syntax. Factory functions are called on the class name (e.g., `Character.new(cp)`, `Array.new(16)`, `Json.parse(s)`), while instance methods are called on the variable (e.g., `s.length()`, `ch.codepoint()`, `arr.push(val)`). The Hoo system has no static methods — all operations on instances use dot notation. The `DOT NEW` syntax (`Map.new(1)`, `Character.new(65)`) is supported as a constructor convention. This syntax is resolved by the code generator to the appropriate C-ABI function under the hood.

## Core Philosophy: The Opaque Handle Model

Because the HVM v1.4 specification describes a pure physical hardware architecture, it lacks native concepts of "managed objects" or "garbage collection". To bridge this gap, the runtime library implements all complex data structures (Strings, Arrays, Maps, Objects) as **Opaque Handles**. 

*   **JIT / HVM View**: An opaque 64-bit integer (`int64_t`) representing an absolute host memory pointer.
*   **Runtime View**: A fully managed C++ instance preceded by a normative 16-byte Automatic Reference Counting (ARC) header.

## Module Reference

### Core
1. **[Memory Model & ARC](memory-model.md)**
   * Details the 16-byte object header, Reference Counting (ARC), and the Thread-Local Allocation Buffer (TLAB) system.
2. **[Strings & Unicode](strings.md)**
   * `HooString` and `HooCharacter` implementations, immutable UTF-8 buffers, and Unicode scalar support.
3. **[Collections](collections.md)**
   * Hardware-ready low-level arrays (`HooArray`) and type-safe dictionaries (`HooMap`).
4. **[Exceptions](exceptions.md)**
   * `HooException` type IDs, stack unwinding, and shadow stack management.
5. **[Math](math.md)**
   * Mathematical constants, functions, and the random number generator state.
6. **[Console I/O](io.md)**
   * Console input/output (`print`, `readline`, `readchar`).

### System & Platform
7. **[File System](fs.md)**
   * `hoo.fs` — object-oriented class API (`hoo::fs::File`, `hoo::fs::Directory`, `hoo::fs::Path`) wrapping C++17 `<filesystem>`; C-ABI bridge preserved for JIT/FFI compatibility.
8. **[Date & Time](datetime.md)**
   * `hoo.datetime` — current time, decompose/compose fields, ISO 8601 formatting, duration arithmetic via `<chrono>`.
9. **[System Information](system.md)**
   * `hoo.system` — environment variables, OS name, hostname, CPU count, process ID, user info, current/working directory.
10. **[Path Manipulation](path.md)**
    * `hoo.path` — dirname, basename, extension, join, normalize, absolute/relative resolution, split on `std::filesystem`.

### Data & Text Processing
11. **[Encoding](encoding.md)**
    * `hoo.encoding` — Base64, hex, and URL percent-encoding encode/decode with round-trip guarantees.
12. **[Regular Expressions](regex.md)**
    * `hoo.regex` — compile, match, search, find-all, replace, split, and capture groups via C++ `<regex>` with opaque handles and reference counting.
13. **[CSV](csv.md)**
    * `hoo.csv` — parse and generate comma-separated values with quoting, escape handling, custom delimiters, and file I/O.
14. **[UUID](uuid.md)**
    * `hoo.uuid` — UUID v4 generation, nil UUID, parse/format, byte access, equality/ordering comparison, and ARC-managed opaque handles.

### Security & Data Integrity
15. **[Hashing](hashing.md)**
    * `hoo.hashing` — SHA-256, SHA-1, MD5, CRC-32, HMAC-SHA256 using Apple CommonCrypto with table-based CRC-32. Hex-encoded string output.
16. **[Compression](compression.md)**
    * `hoo.compression` — gzip and raw deflate compress/decompress using zlib. Result is heap-allocated byte buffer.

### System & Process Control
17. **[Process](process.md)**
    * `hoo.process` — spawn (fork/exec), wait, kill, self-pid, and command capture via POSIX APIs and `popen`.
18. **[Args](args.md)**
    * `hoo.args` — CLI argument parser for `--key=value`, `--flag`, `-k`, and positional args. Returns a struct result (`HooArgsResult`).

### Data Interchange
19. **[JSON](json.md)**
    * `hoo.json` — parse, stringify, query, and construct JSON values (objects, arrays, strings, numbers, bools, null) with ARC-managed opaque handles.

### Network & Concurrency
20. **[Networking & HTTP](net.md)**
    * `hoo.net` — URL parsing (scheme, host, port, path, query, fragment), HTTP client (GET, POST, PUT, DELETE) via libcurl with custom headers, timeout, and redirect following.
21. **[Threading](thread.md)**
    * `hoo.thread` — thread spawn/join/self via pthreads and Win32 threads, mutex create/lock/unlock/destroy for concurrent synchronization.

### JIT Bridge
22. **[JIT Integration](jit-integration.md)**
    * System call mapping (`SYSCALL` 1-23) with platform-specific behavior, ARC optimization passes, host symbol bridging, and flexible symbol resolution (`buildLookupCandidates`).
23. **[Name Mangling & Demangling](name-mangling.md)**
    * Complete reference for the `_F_` (function) and `_H_` (header) symbol formats, type encoding, module path qualification, class member qualification, and JIT symbol resolution conventions.
24. **[New Module Guide](new-module-guide.md)**
    * Step-by-step walkthrough for wiring a new runtime library function through the codegen, JIT wrapper, and symbol table layers.

## Integration & C-ABI
The library exposes its JIT-facing API via `extern "C"` to guarantee ABI stability with the JIT's LLVM `ExecutionEngine`. Some modules (notably `hoo.fs`) additionally provide a C++ class API (`hoo::fs::File`, `hoo::fs::Directory`, `hoo::fs::Path`) as the primary interface, with `extern "C"` bridge functions delegating to the classes for JIT/FFI compatibility. The `HVMJIT` maps absolute host function pointers into the isolated `hoo` JITDylib so HVM code can resolve `CALL` targets natively. Each module has corresponding JIT wrapper functions in `src/hvm/HVMJIT.cpp` and a mangled symbol entry in `buildRuntimeSymbols()`. The code generator in `src/codegen/HVMCodeGenerator.cpp` resolves class-based method calls to the appropriate runtime module using a `classToPrefix()` mapping — for example, `Math` → `math_`, `String` → `string_`, `Array` → `array_`, `Map` → `map_`, `Json` → `json_`, `Csv` → `csv_`, `DateTime` → `datetime_`, `Hashing` → `hashing_`, `Compression` → `compression_`, `Path` → `path_`, `Process` → `process_`, `Thread` → `thread_`, `System` → `system_`, `Regex` → `regex_`, `Encoding` → `encoding_`, `Uuid` → `uuid_`, `Fs` → `fs_`, `Character` → `character_`, `HttpClient` → `http_client_`, `HttpResponse` → `http_response_`, and `Url` → `url_`. The compiler then redirects the resolved call to the `hoo` module path. Instance method calls on `var` variables are resolved via type-ID inference, which now covers: primitive literals, constructor calls (`Map.new`, `Array.new`, `Character.new`), function return types (from `functionReturnTypes_` map), user-defined class method return types (from `ClassLayout::methodReturnTypes`), array subscript access (from `Local::elementTypeId`), and built-in method return types for `Args`, `Character`, and `Map`. Unresolved cases default to typeId 100 (Object) and dispatch via `methodNameToClass_`.

## Build
All runtime sources live in `src/runtime/lib/` and are compiled into the `hoort` static library target. Test sources in `tests/runtime/` are linked into the `hoo-tests` executable. Legacy C-ABI function pointers are additionally registered in `lookupPlainRuntimeSymbolAddress()` for interpreter and non-JIT code paths.
