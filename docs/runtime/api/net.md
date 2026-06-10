# Net — URL Parsing and HTTP Client

The `net` module provides URL parsing and HTTP client functionality through three classes: `URL`, `HttpClient`, and `HttpResponse`.

---

## URL

Parses and decomposes URLs.

### Constructor

`URL.new(url: string) :ptr`
Creates a new URL from a string. Returns a URL handle.

### Methods

`URL.getScheme(url: ptr) :string`
Returns the scheme (protocol), e.g. `"https"`.

`URL.getHost(url: ptr) :string`
Returns the host, e.g. `"example.com"`.

`URL.getPort(url: ptr) :int64`
Returns the port number, or -1 if not specified.

`URL.getPath(url: ptr) :string`
Returns the path component, e.g. `"/path"`.

`URL.getQuery(url: ptr) :string`
Returns the query string, or empty string if none.

`URL.getFragment(url: ptr) :string`
Returns the fragment, or empty string if none.

`URL.toString(url: ptr) :string`
Returns the full URL string.

`URL.release(url: ptr)`
Releases the URL handle.

### Example

```hoo
let url = URL.new("https://example.com:8080/path?q=1#section")
println(URL.getScheme(url))    // "https"
println(URL.getHost(url))      // "example.com"
println(URL.getPort(url))      // 8080
println(URL.getPath(url))      // "/path"
println(URL.getQuery(url))     // "q=1"
println(URL.getFragment(url))  // "section"
URL.release(url)
```

---

## HttpClient

Performs HTTP requests.

### Constructor

`HttpClient.new() :HttpClient`
Creates a new HTTP client.

### Methods

`client.setHeader(key: string, value: string) :int64`
Sets a request header. Returns 1 on success.

`client.setTimeout(ms: int64)`
Sets the request timeout in milliseconds.

`client.get(url: string) :HttpResponse`
Performs a GET request.

`client.post(url: string, body: string) :HttpResponse`
Performs a POST request with the given body.

`client.put(url: string, body: string) :HttpResponse`
Performs a PUT request with the given body.

`client.delete(url: string) :HttpResponse`
Performs a DELETE request.

`client.release()`
Releases the HTTP client handle.

### Example

```hoo
let client = HttpClient.new()
client.setTimeout(5000)
client.setHeader("Authorization", "Bearer token123")

let resp = client.get("https://api.example.com/users")
if resp.isSuccess() == 1 {
    println(resp.body())
}
resp.release()
client.release()
```

---

## HttpResponse

Represents an HTTP response.

### Methods

`resp.statusCode() :int64`
Returns the HTTP status code (e.g. 200, 404).

`resp.statusText() :string`
Returns the HTTP status text (e.g. "OK", "Not Found").

`resp.body() :string`
Returns the response body as a string.

`resp.isSuccess() :int64`
Returns 1 if the status code is in the 2xx range, 0 otherwise.

`resp.release()`
Releases the HTTP response handle.

### Example

```hoo
let client = HttpClient.new()
let resp = client.post("https://api.example.com/data", "{\"key\":\"value\"}")

if resp.isSuccess() == 1 {
    println(resp.body())
} else {
    println("Error " + resp.statusCode() + ": " + resp.statusText())
}

resp.release()
client.release()
```
