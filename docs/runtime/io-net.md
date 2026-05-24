# I/O & Networking

The runtime provides basic console I/O and scaffolding for a high-level networking and HTTP client API.

## 1. Console I/O (`hoo_io.h`)
Standard I/O functions natively extract the raw UTF-8 string data via `hoo_string_data`.
- `void hoo_print(void* str)`: Flushes the string to `stdout` without a newline. Handles `null` safely.
- `void hoo_println(void* str)`: Flushes to `stdout` with an appended newline.
- `void* hoo_readline(void)`: Blocks and reads `stdin` until EOF or newline. Returns a new `HooString` handle.
- `int64_t hoo_readchar(void)`: Blocks for a single character input. Returns the ASCII/UTF-8 byte value or `-1` on EOF.

## 2. Networking & URL (`hoo_net.h`)
The network module implements parsers and mock clients, setting the stage for future socket integration.

### URL Parsing
URLs are parsed into an ARC-managed `HooURLImpl` containing Scheme, Host, Port, Path, Query, and Fragment.
- `hoo_net_url_new(const char* urlString)`
- `hoo_net_url_get_host(url)` / `hoo_net_url_get_path(url)` etc.
- `hoo_net_url_to_string(url)`: Rebuilds the URL string.

### HTTP Client (Mock/Scaffolding)
Currently implemented as a mock interface for testing. It resolves responses based on keyword detection in the URL (e.g., `success`, `error`, `404`).
- **Client**: `hoo_net_http_client_new()`, `hoo_net_http_client_set_header()`, `hoo_net_http_client_set_timeout()`.
- **Verbs**: `hoo_net_http_client_get()`, `post()`, `put()`, `delete()`.
- **Response**: Managed by `HooHttpResponseImpl` containing the status code, text, headers, and body.
  - `hoo_net_http_response_get_status_code(res)`
  - `hoo_net_http_response_get_body(res)`
  - `hoo_net_http_response_is_success(res)` (Checks for 2xx codes).
