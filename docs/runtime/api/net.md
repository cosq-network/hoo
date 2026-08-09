# Net API Reference

## Module

`hoo.net`

## Import Statement

```hoo
import hoo.net;
```

## Module Description

The `net` module provides URL parsing and HTTP client functionality through three classes: `Url`, `HttpClient`, and `HttpResponse`. All string values returned by `Url` and `HttpResponse` methods must be freed with the corresponding `free_string` method.

## Native TCP Socket ABI

`HooSocket` is an ARC-managed TCP handle. `HooByteSlice` arguments are borrowed
read-only views; socket operations never retain or free their backing memory.
`hoo_net_socket_receive` returns a new owned `HooBuffer`. The current API
supports DNS-aware connect, IPv4 bind/listen/accept, send, receive, error
inspection, close, retain, release, configurable operation timeouts, TLS client
connect with optional peer verification, and TLS server configuration from PEM
certificate/key files. Calls block at the C boundary while libuv performs the
underlying non-blocking stream operations.

```c
HooSocket socket = hoo_net_socket_new();
hoo_net_socket_connect(socket, "127.0.0.1", 8080);
const uint8_t data[] = {'h', 'i'};
hoo_net_socket_send(socket, hoo_byte_slice_from_bytes(data, 2));
HooBuffer response = hoo_net_socket_receive(socket, 4096);
hoo_buffer_release(response);
hoo_net_socket_release(socket);
```

## Class: Url

### Declaration

```hoo
class Url
```

Represents a parsed URL with access to its components.

### Constructor

```hoo
Url(url: string): Url
```

Creates a new `Url` from a string.

**Parameters:**

| Parameter | Type     | Description         |
|-----------|----------|---------------------|
| `url`     | `string` | The URL string to parse. |

**Returns:** `Url` — A new `Url` instance with reference count 1.

**Errors:** Returns `null` if the URL string is malformed.

### Public Instance Functions

#### `to_string`

Returns the full URL as a string.

**Syntax:**

```hoo
url.to_string(): string
```

**Returns:** `string` — The URL string (must be freed with `free_string`).

**Complete Example:**

```hoo
import hoo.net;

func :int64 main() {
    var url = new Url("https://example.com/path");
    var s = url.to_string();
    println(s);
    url.free_string(s);
    url.release();
    return 0;
}
```

#### `retain`

Increments the reference count of the `Url`.

**Syntax:**

```hoo
url.retain(): Url
```

**Returns:** `Url` — The same `Url` instance.

#### `release`

Decrements the reference count of the `Url`. The object is freed when the count reaches zero.

**Syntax:**

```hoo
url.release(): void
```

**Returns:** `void`

#### `free_string`

Frees a string allocated by a `Url` method.

**Syntax:**

```hoo
url.free_string(str: string): void
```

**Parameters:**

| Parameter | Type     | Description                              |
|-----------|----------|------------------------------------------|
| `str`     | `string` | The string returned by a `Url` method to free. |

**Returns:** `void`

## Class: HttpClient

### Declaration

```hoo
class HttpClient
```

Performs HTTP requests.

### Constructor

```hoo
HttpClient(): HttpClient
```

Creates a new `HttpClient`.

**Returns:** `HttpClient` — A new `HttpClient` instance with reference count 1.

### Public Instance Functions

#### `get`

Performs a GET request.

**Syntax:**

```hoo
client.get(url: Url): HttpResponse
```

**Parameters:**

| Parameter | Type | Description                   |
|-----------|------|-------------------------------|
| `url`     | `Url` | The target URL to request. |

**Returns:** `HttpResponse` — The HTTP response (must be released by caller).

**Errors:** Returns `null` on connection or request failure.

**Complete Example:**

```hoo
import hoo.net;

func :int64 main() {
    var url = new Url("https://api.example.com/data");
    var client = new HttpClient();
    var resp = client.get(url);
    if (resp != null) {
        println(resp.status_code());
        var body = resp.body();
        println(body);
        resp.free_string(body);
        resp.release();
    }
    client.release();
    url.release();
    return 0;
}
```

#### `post`

Performs a POST request with a body.

**Syntax:**

```hoo
client.post(url: Url, body: string): HttpResponse
```

**Parameters:**

| Parameter | Type     | Description                   |
|-----------|----------|-------------------------------|
| `url`     | `Url`    | The target URL to request. |
| `body`    | `string` | The request body. |

**Returns:** `HttpResponse` — The HTTP response (must be released by caller).

**Errors:** Returns `null` on connection or request failure.

**Complete Example:**

```hoo
import hoo.net;

func :int64 main() {
    var url = new Url("https://api.example.com/data");
    var client = new HttpClient();
    var resp = client.post(url, "{\"key\":\"value\"}");
    if (resp != null) {
        println(resp.status_code());
        resp.release();
    }
    client.release();
    url.release();
    return 0;
}
```

#### `retain`

Increments the reference count of the `HttpClient`.

**Syntax:**

```hoo
client.retain(): HttpClient
```

**Returns:** `HttpClient` — The same `HttpClient` instance.

#### `release`

Decrements the reference count of the `HttpClient`. The object is freed when the count reaches zero.

**Syntax:**

```hoo
client.release(): void
```

**Returns:** `void`

## Class: HttpResponse

### Declaration

```hoo
class HttpResponse
```

Represents an HTTP response with status code, status text, and body.

### Public Instance Functions

#### `status_code`

Returns the HTTP status code.

**Syntax:**

```hoo
resp.status_code(): int64
```

**Returns:** `int64` — The HTTP status code (e.g., `200`, `404`).

**Complete Example:**

```hoo
import hoo.net;

func :int64 main() {
    var client = new HttpClient();
    var url = new Url("https://api.example.com/data");
    var resp = client.get(url);
    if (resp != null) {
        var code = resp.status_code();
        println(code);
        resp.release();
    }
    url.release();
    client.release();
    return 0;
}
```

#### `status_text`

Returns the HTTP status text.

**Syntax:**

```hoo
resp.status_text(): string
```

**Returns:** `string` — The status text (e.g., `"OK"`, `"Not Found"`). Must be freed with `free_string`.

#### `body`

Returns the response body as a string.

**Syntax:**

```hoo
resp.body(): string
```

**Returns:** `string` — The response body (must be freed with `free_string`).

#### `retain`

Increments the reference count of the `HttpResponse`.

**Syntax:**

```hoo
resp.retain(): HttpResponse
```

**Returns:** `HttpResponse` — The same `HttpResponse` instance.

#### `release`

Decrements the reference count of the `HttpResponse`. The object is freed when the count reaches zero.

**Syntax:**

```hoo
resp.release(): void
```

**Returns:** `void`

#### `free_string`

Frees a string allocated by an `HttpResponse` method.

**Syntax:**

```hoo
resp.free_string(str: string): void
```

**Parameters:**

| Parameter | Type     | Description                                      |
|-----------|----------|--------------------------------------------------|
| `str`     | `string` | The string returned by an `HttpResponse` method to free. |

**Returns:** `void`

## Usage Example

```hoo
import hoo.net;

func :int64 main() {
    var url = new Url("https://jsonplaceholder.typicode.com/posts/1");
    var client = new HttpClient();
    var resp = client.get(url);

    if (resp != null) {
        var code = resp.status_code();
        var text = resp.status_text();
        var body = resp.body();

        println("Status: " + code + " " + text);
        println("Body: " + body);

        resp.free_string(text);
        resp.free_string(body);
        resp.release();
    }

    client.release();
    url.release();
    return 0;
}
```
