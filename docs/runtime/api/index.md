# hoo Runtime API Reference

Welcome to the official API reference for the hoo Runtime Library (`hoort`). This documentation is designed for hoo developers to understand and utilize the built-in capabilities of the language.

The hoo runtime provides a set of modules that bridge the high-level language with the host system, offering efficient implementations of common data structures, mathematical operations, network communication, and system interactions.

## Core Modules

| Module | Description |
| :--- | :--- |
| **[Strings](string.md)** | Comprehensive string manipulation, UTF-8 support, and formatting. |
| **[Buffer](buffer.md)** | Managed mutable byte array for raw binary data with ARC. |
| **[Collections](collections.md)** | Dynamic arrays and type-safe maps for data storage. |
| **[Math](math.md)** | Mathematical constants, basic functions, and random number generation. |
| **[I/O](io.md)** | Console input and output functions. |
| **[Character](character.md)** | Unicode character operations, codepoint conversion, and inspection. |
| **[Regex](regex.md)** | Regular expression matching, search, replace, and split. |
| **[DateTime](datetime.md)** | Date/time operations, formatting, and ISO 8601 parsing. |
| **[Uuid](uuid.md)** | UUID v4 generation and validation. |

## System & Platform Modules

| Module | Description |
| :--- | :--- |
| **[Fs](fs.md)** | File system operations: read, write, copy, delete, list directories. |
| **[Path](path.md)** | Cross-platform file path manipulation and normalization. |
| **[Process](process.md)** | Process control, environment variables, and system information. |
| **[System](system.md)** | OS platform detection, hardware info, and user environment. |
| **[Args](args.md)** | Command-line argument parsing and inspection. |

## Networking & Web

| Module | Description |
| :--- | :--- |
| **[Networking](net.md)** | URL parsing and a robust HTTP client. |
| **[JSON](json.md)** | JSON parsing, generation, and value manipulation. |
| **[Encoding](encoding.md)** | Base64, hex, and URL encoding/decoding. |
| **[Hashing](hashing.md)** | SHA-256, SHA-1, MD5, CRC32, and HMAC-SHA256 hashing. |
| **[Compression](compression.md)** | Gzip and deflate compression/decompression. |
| **[CSV](csv.md)** | CSV data parsing, generation, and map-based access with headers. |

## Concurrency

| Module | Description |
| :--- | :--- |
| **[Thread](thread.md)** | Thread creation, synchronization, and mutex locks. |

## Error Handling

| Module | Description |
| :--- | :--- |
| **[Exception](exception.md)** | Exception types, try/catch/finally, and stack trace inspection. |

## Usage Convention

In hoo source code, runtime functions are called using class-based method syntax. For example:

- `Math.abs(x)` — static methods use the class name
- `s.length()` — instance methods are called on the variable
- `new Array()` — constructors use the same `new Class(...)` syntax as user-defined classes

Standard I/O functions like `print` and `println` are available globally (without a prefix or class qualifier).

### Memory Management

All complex objects in hoo (Strings, Arrays, Maps, Buffers, etc.) are automatically managed. You generally don't need to manually free objects — the runtime handles deallocation when an object is no longer reachable.

### Buffer-Aware Overloads

Several modules offer overloaded methods that accept a `Buffer` handle instead of raw `(data, len)` pairs: **Fs** (`writeBytes`, `readBytes`), **Encoding** (`base64Encode`, `base64Decode`, `hexEncode`, `hexDecode`), **Uuid** (`fromBytes`, `toBytes`), **Hashing** (all hash functions), and **Compression** (gzip/deflate). See the [Buffer reference](buffer.md) for the full list.
