# Networking (`hoo.net`)

The `hoo.net` module provides URL parsing (scheme, host, port, path, query, fragment) and a real HTTP client (GET, POST, PUT, DELETE) via libcurl with custom headers, timeout, and redirect following.

## 1. URL Parsing

URLs are parsed into an ARC-managed opaque handle (`HooURL`).

- `new URL(urlString)` — Parse URL string. Returns retained handle.
- `url.getScheme()` / `url.getHost()` / `url.getPort()` / `url.getPath()` / `url.getQuery()` / `url.getFragment()` — Component getters.
- `url.toString()` — Reconstruct URL string. Omits default ports (80 for HTTP, 443 for HTTPS).
- `url.release()` — Release the URL handle.

## 2. HTTP Client (`HooHttpClient`)

A libcurl-based real HTTP client with per-instance header and timeout configuration.

- `new HttpClient()` — Create new client.
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
- `res.getBody()` — Response body string.
- `res.isSuccess()` — Returns 1 for 2xx status codes.
- `res.release()` — Release the response handle.

### Mock Fallback

For test URLs containing `"example"` in the domain, the HTTP client falls back to keyword-matched mock responses (e.g., URLs with `"success"` or `"200"` return 200). This allows offline testing without network access.

## Usage from Hoo Source

Constructors use `new URL(...)` / `new HttpClient()`, and object operations use instance methods:

```hoo
func :int64 demo() {
    var url = new URL("https://example.com:8080/path?q=1#frag");
    var scheme = url.getScheme();                      // "https"
    var host = url.getHost();                          // "example.com"
    var port = url.getPort();                          // 8080
    var path = url.getPath();                          // "/path"
    var query = url.getQuery();                        // "q=1"
    var frag = url.getFragment();                      // "frag"
    url.release();
    return port;
}
```

## 3. Memory Management

- Strings returned by URL and HTTP methods are managed runtime strings.
- Handles are released with their instance `release()` methods.
