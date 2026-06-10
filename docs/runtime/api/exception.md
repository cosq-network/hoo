# Exception

All Hoo programs can throw and catch exceptions. Built-in exception types include:

- `RuntimeException`
- `NullPointerException`
- `IndexOutOfBoundsException`
- `DivisionByZeroException`
- `InvalidCastException`
- Custom exceptions (subtypes of `RuntimeException`)

## Try / Catch / Finally

```hoo
try
    let x = 10 / 0
catch e: DivisionByZeroException
    // division by zero
catch e: Exception
    // any other exception
finally
    // always runs
end
```

## Instance Methods

Each caught exception `e` supports:

### `e.message() :string`

Returns the human-readable error message.

```hoo
try
    let x = arr[100]
catch e: IndexOutOfBoundsException
    let msg = e.message()  // e.g. "index 100 out of bounds"
end
```

### `e.typeName() :string`

Returns the fully qualified type name of the exception.

```hoo
let t = e.typeName()  // e.g. "IndexOutOfBoundsException"
```

### `e.typeId() :int64`

Returns the numeric type identifier.

```hoo
let id = e.typeId()  // e.g. 3
```

### `e.stackTrace() :string`

Returns the full stack trace as a string.

```hoo
let trace = e.stackTrace()
```

### `e.hasCause() :int64`

Returns nonzero if this exception wraps a cause.

```hoo
if e.hasCause() != 0
    // has a chained cause
end
```

### `e.cause() :Exception`

Returns the wrapped cause exception.

```hoo
let cause = e.cause()
```

### `e.frameCount() :int64`

Returns the number of stack frames in the trace.

```hoo
let count = e.frameCount()  // e.g. 12
```

### `e.frame(index: int64) :string`

Returns the stack frame at the given index (0-based).

```hoo
let top = e.frame(0)  // topmost frame
let bottom = e.frame(e.frameCount() - 1)  // bottom frame
```

## Full Example

```hoo
try
    let arr = [1, 2, 3]
    let x = arr[10]
catch e: IndexOutOfBoundsException
    let msg = e.message()
    let trace = e.stackTrace()
    let frames = e.frameCount()
    if frames > 0
        let topFrame = e.frame(0)
    end
finally
    // cleanup
end
```
