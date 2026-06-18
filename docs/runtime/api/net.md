# Net — URL Parsing and HTTP Client

The `net` module provides URL parsing and HTTP client functionality through three classes: `URL`, `HttpClient`, and `HttpResponse`.

---

## URL

Parses and decomposes URLs.

### Constructor

`new URL(url: string) :ptr`
Creates a new URL from a string. Returns a URL handle.

### Methods

`url.getScheme() :string`
Returns the scheme (protocol), e.g. `"https"`.

`url.getHost() :string`
Returns the host, e.g. `"example.com"`.

`url.getPort() :int64`
Returns the port number, or -1 if not specified.

`url.getPath() :string`
Returns the path component, e.g. `"/path"`.

`url.getQuery() :string`
Returns the query string, or empty string if none.

`url.getFragment() :string`
Returns the fragment, or empty string if none.

`url.toString() :string`
Returns the full URL string.

`url.release()`
Releases the URL handle.

### Example

```hoo
let url = new URL("https://example.com:8080/path?q=1#section")
println(url.getScheme())    // "https"
println(url.getHost())      // "example.com"
println(url.getPort())      // 8080
println(url.getPath())      // "/path"
println(url.getQuery())     // "q=1"
println(url.getFragment())  // "section"
url.release()
```

---

## HttpClient

Performs HTTP requests.

### Constructor

`new HttpClient() :HttpClient`
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
let client = new HttpClient()
client.setTimeout(5000)
client.setHeader("Authorization", "Bearer token123")

let resp = client.get("https://api.example.com/users")
if resp.isSuccess() == 1 {
    println(resp.getBody())
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

`resp.getBody() :string`
Returns the response body as a string.

`resp.isSuccess() :int64`
Returns 1 if the status code is in the 2xx range, 0 otherwise.

`resp.release()`
Releases the HTTP response handle.

### Example

```hoo
let client = new HttpClient()
let resp = client.post("https://api.example.com/data", "{\"key\":\"value\"}")

if resp.isSuccess() == 1 {
    println(resp.getBody())
} else {
    println("Error " + resp.statusCode())
}

resp.release()
client.release()
```
