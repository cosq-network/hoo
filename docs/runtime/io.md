# Console I/O

The runtime provides basic console I/O functions. Standard I/O functions natively extract the raw UTF-8 string data via `string_data`.

- `print(str)`: Flushes the string to `stdout` without a newline. A null argument prints `"null"`.
- `println(str)`: Flushes to `stdout` with an appended newline. A null argument prints `"null"`.
- `readline()`: Blocks and reads `stdin` until EOF or newline. Returns a new `HooString` handle; returns an empty string on EOF.
- `readchar()`: Blocks for a single character input. Returns the ASCII/UTF-8 byte value or `-1` on EOF.

> **Note**: Networking and HTTP client APIs (URL parsing, libcurl-based HTTP client) are documented in a separate [`net.md`](net.md) page.
