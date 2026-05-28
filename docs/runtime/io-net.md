# Console I/O (`hoo_io.h`)

The runtime provides basic console I/O functions. Standard I/O functions natively extract the raw UTF-8 string data via `hoo_string_data`.

- `void hoo_print(void* str)`: Flushes the string to `stdout` without a newline. Handles `null` safely.
- `void hoo_println(void* str)`: Flushes to `stdout` with an appended newline.
- `void* hoo_readline(void)`: Blocks and reads `stdin` until EOF or newline. Returns a new `HooString` handle.
- `int64_t hoo_readchar(void)`: Blocks for a single character input. Returns the ASCII/UTF-8 byte value or `-1` on EOF.

> **Note**: The networking and HTTP client APIs (URL parsing, libcurl-based HTTP client) are now documented in a separate [`net.md`](net.md) page.
