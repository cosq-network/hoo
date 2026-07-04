# Async/Await in Hoo

Hoo provides first-class support for asynchronous programming using `async` and `await`. 
This allows you to write non-blocking, concurrent code that performs I/O operations (like file reads or network requests) without freezing the application's thread.

## The `async` keyword
You can declare a function as asynchronous by placing the `async` keyword before `func`.
An async function always returns a `Future<T>`.

```hoo
async func:Future<string> fetchWebpage(url: string) {
    // ...
}
```

## The `await` keyword
The `await` keyword pauses the execution of the current `async` function until the provided `Future` resolves.
While suspended, the underlying thread (managed by `libuv`) is free to perform other tasks.

```hoo
async func process() {
    var data = await(fetchWebpage("https://example.com"));
    print(data);
}
```

## Runtime Details
Hoo uses **libuv** for its cross-platform event loop, providing high-performance, asynchronous I/O across macOS, Linux, and Windows.
Under the hood, Hoo uses LLVM-compiled stackless coroutines to achieve zero-overhead task suspension.
