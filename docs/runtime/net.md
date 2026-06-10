# Networking (`hoo.net`)

The `hoo.net` module provides URL parsing (scheme, host, port, path, query, fragment) and a real HTTP client (GET, POST, PUT, DELETE) via libcurl with custom headers, timeout, and redirect following.

## 1. URL Parsing

URLs are parsed into an ARC-managed opaque handle (`HooURL`).

- `Url.new(urlString)` — Parse URL string. Returns retained handle.
- `url.scheme()` / `url.host()` / `url.port()` / `url.path()` / `url.query()` / `url.fragment()` — Component getters (return allocated strings, free with `Url.freeString`).
- `url.toString()` — Reconstruct URL string. Omits default ports (80 for HTTP, 443 for HTTPS).
- `url.retain()` / `url.release()` — Reference counting.

## 2. HTTP Client (`HooHttpClient`)

A libcurl-based real HTTP client with per-instance header and timeout configuration.

- `HttpClient.new()` — Create new client.
- `client.setHeader(key, value)` — Set a custom request header.
- `client.setTimeout(ms)` — Set request timeout in milliseconds.

### HTTP Verbs

- `client.get(url)` — HTTP GET request.
- `client.post(url, body)` — HTTP POST request.
- `client.put(url, body)` — HTTP PUT request.
- `client.delete(url)` — HTTP DELETE request.

### Response (`HooHttpResponse`)

Each verb call returns a retained `HooHttpResponse` handle:

- `res.statusCode()` — HTTP status code.
- `res.statusText()` — Status text (e.g., "OK").
- `res.body()` — Response body string.
- `res.header(name)` — Get response header value.
- `res.isSuccess()` — Returns 1 for 2xx status codes.
- `res.retain()` / `res.release()` — Reference counting.

### Mock Fallback

For test URLs containing `"example"` in the domain, the HTTP client falls back to keyword-matched mock responses (e.g., URLs with `"success"` or `"200"` return 200). This allows offline testing without network access.

## Usage from Hoo Source

All `Url.*`, `HttpClient.*`, and instance methods are available:

```hoo
func :int64 demo() {
    var url = Url.new("https://example.com:8080/path?q=1#frag");
    var scheme = url.scheme();                         // "https"
    var host = url.host();                             // "example.com"
    var port = url.port();                             // 8080
    var path = url.path();                             // "/path"
    var query = url.query();                           // "q=1"
    var frag = url.fragment();                         // "frag"
    url.release();
    return port;
}
```

## 3. Memory Management

- `Url.freeString(str)` — Free allocated string.
- All handles use ARC via `retain`/`release` pairs.
