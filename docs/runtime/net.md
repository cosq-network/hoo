# Networking (`hoo.net`)

The `hoo.net` module provides URL parsing (scheme, host, port, path, query, fragment) and a real HTTP client (GET, POST, PUT, DELETE) via libcurl with custom headers, timeout, and redirect following.

## 1. URL Parsing

URLs are parsed into an ARC-managed opaque handle (`HooURL`).

- `hoo_net_url_new(urlString)` — Parse URL string. Returns retained handle.
- `hoo_net_url_get_scheme(url)` / `get_host(url)` / `get_port(url)` / `get_path(url)` / `get_query(url)` / `get_fragment(url)` — Component getters (return allocated strings, free with `hoo_net_free_string`).
- `hoo_net_url_to_string(url)` — Reconstruct URL string. Omits default ports (80 for HTTP, 443 for HTTPS).
- `hoo_net_url_retain(url)` / `hoo_net_url_release(url)` — Reference counting.

## 2. HTTP Client (`HooHttpClient`)

A libcurl-based real HTTP client with per-instance header and timeout configuration.

- `hoo_net_http_client_new()` — Create new client.
- `hoo_net_http_client_set_header(client, key, value)` — Set a custom request header.
- `hoo_net_http_client_set_timeout(client, ms)` — Set request timeout in milliseconds.

### HTTP Verbs

- `hoo_net_http_client_get(client, url)` — HTTP GET request.
- `hoo_net_http_client_post(client, url, body)` — HTTP POST request.
- `hoo_net_http_client_put(client, url, body)` — HTTP PUT request.
- `hoo_net_http_client_delete(client, url)` — HTTP DELETE request.

### Response (`HooHttpResponse`)

Each verb call returns a retained `HooHttpResponse` handle:

- `hoo_net_http_response_get_status_code(res)` — HTTP status code.
- `hoo_net_http_response_get_status_text(res)` — Status text (e.g., "OK").
- `hoo_net_http_response_get_body(res)` — Response body string.
- `hoo_net_http_response_get_header(res, name)` — Get response header value.
- `hoo_net_http_response_is_success(res)` — Returns 1 for 2xx status codes.
- `hoo_net_http_response_retain(res)` / `hoo_net_http_response_release(res)` — Reference counting.

### Mock Fallback

For test URLs containing `"example"` in the domain, the HTTP client falls back to keyword-matched mock responses (e.g., URLs with `"success"` or `"200"` return 200). This allows offline testing without network access.

## Usage from Hoo Source

All `net_` functions are available with the `net_` prefix:

```hoo
func :int64 demo() {
    var url = net_url_new("https://example.com:8080/path?q=1#frag");
    var scheme = net_url_get_scheme(url);             // "https"
    var host = net_url_get_host(url);                  // "example.com"
    var port = net_url_get_port(url);                  // 8080
    var path = net_url_get_path(url);                  // "/path"
    var query = net_url_get_query(url);                // "q=1"
    var frag = net_url_get_fragment(url);              // "frag"
    net_url_release(url);
    return port;
}
```

## 3. Memory Management

- `hoo_net_free_string(str)` — Free allocated string.
- All handles use ARC via `retain`/`release` pairs.
