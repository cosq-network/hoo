# Exception API Reference

## Exception Module

The `Exception` class belongs to the core Hoo runtime module.

## Import Statement

```hoo
import hoo;
```

Exception is available without any explicit import in Hoo programs; the `import hoo;` statement ensures the runtime is linked.

## Module Description

The `Exception` class is the base type for all throwable errors in Hoo. Built-in exception types include `RuntimeException`, `NullPointerException`, `IndexOutOfBoundsException`, `DivisionByZeroException`, `InvalidCastException`, and custom exceptions. All exceptions carry a human-readable message and optional chained cause. Exception objects use automatic reference counting (ARC) and should be retained when assigned to new variables.

## Class: Exception

### Declaration

```hoo
class Exception
```

### Public Fields

None.

### Constructor

#### Exception

##### Description

Creates a new `RuntimeException` with the given error message. The returned exception has an initial reference count of 1.

##### Syntax

```hoo
Exception(reason: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| reason | `string` | The error message describing the exception. |

##### Returns

`Exception` — A new exception instance with refcount 1.

##### Errors

Throws `RuntimeException` if allocation fails.

##### Complete Example

```hoo
let err = Exception("Something went wrong");
```

### Public Class (Static) Functions

#### Exception.runtime

##### Description

Creates a new `RuntimeException`.

##### Syntax

```hoo
Exception.runtime(message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new RuntimeException instance.

#### Exception.nullPointer

##### Description

Creates a new `NullPointerException`.

##### Syntax

```hoo
Exception.nullPointer(message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new NullPointerException instance.

#### Exception.indexOutOfBounds

##### Description

Creates a new `IndexOutOfBoundsException`.

##### Syntax

```hoo
Exception.indexOutOfBounds(message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new IndexOutOfBoundsException instance.

#### Exception.divisionByZero

##### Description

Creates a new `DivisionByZeroException`.

##### Syntax

```hoo
Exception.divisionByZero(message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new DivisionByZeroException instance.

#### Exception.invalidCast

##### Description

Creates a new `InvalidCastException`.

##### Syntax

```hoo
Exception.invalidCast(message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new InvalidCastException instance.

#### Exception.custom

##### Description

Creates a custom exception with a user-defined type name.

##### Syntax

```hoo
Exception.custom(typeName: string, message: string):Exception
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| typeName | `string` | The custom exception type name. |
| message | `string` | The error message (may be empty). |

##### Returns

`Exception` — A new custom exception instance.

### Public Instance Functions

#### reason

##### Description

Returns the human-readable error message carried by the exception.

##### Syntax

```hoo
reason() :string
```

##### Parameters

None.

##### Returns

`string` — The exception error message, or an empty string if no message was set.

##### Errors

None (returns empty string if exception is nil).

##### Complete Example

```hoo
let e = Exception("disk full");
let msg = e.reason();
// msg == "disk full"
```

#### typeId

##### Description

Returns the numeric type identifier for the exception.

##### Syntax

```hoo
typeId() :int64
```

##### Parameters

None.

##### Returns

`int64` — The exception type ID (e.g., 0 for RuntimeException, 2 for IndexOutOfBoundsException).

##### Errors

None.

##### Complete Example

```hoo
let e = Exception.indexOutOfBounds("index out of range");
let id = e.typeId();
// id == 2
```

#### typeName

##### Description

Returns the fully qualified type name of the exception.

##### Syntax

```hoo
typeName() :string
```

##### Parameters

None.

##### Returns

`string` — The exception type name (e.g., "RuntimeException", "IndexOutOfBoundsException").

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("error");
let name = e.typeName();
// name == "RuntimeException"
```

#### to_string

##### Description

Returns a string representation of the exception including the type name, message, and stack trace.

##### Syntax

```hoo
to_string() :string
```

##### Parameters

None.

##### Returns

`string` — A formatted string describing the exception and its stack trace.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("test error");
let s = e.to_string();
```

#### stackTrace

##### Description

Returns the full stack trace as a string.

##### Syntax

```hoo
stackTrace() :string
```

##### Parameters

None.

##### Returns

`string` — The exception stack trace including all chained causes.

##### Errors

None.

##### Complete Example

```hoo
try
    let x = arr[100];
catch e: IndexOutOfBoundsException
    let trace = e.stackTrace();
end
```

#### hasCause

##### Description

Checks whether the exception wraps a chained cause.

##### Syntax

```hoo
hasCause() :int64
```

##### Parameters

None.

##### Returns

`int64` — Nonzero (1) if the exception has a cause, zero (0) otherwise.

##### Errors

None.

##### Complete Example

```hoo
if e.hasCause() != 0
    // has a chained cause
end
```

#### cause

##### Description

Returns the wrapped cause exception, or nil if none.

##### Syntax

```hoo
cause() :Exception
```

##### Parameters

None.

##### Returns

`Exception` — The cause exception, or nil if no cause was set.

##### Errors

None.

##### Complete Example

```hoo
let inner = Exception("inner error");
let outer = Exception.withCause("outer error", inner);
let c = outer.cause();
// c == inner
```

#### frameCount

##### Description

Returns the number of stack frames captured in the exception trace.

##### Syntax

```hoo
frameCount() :int64
```

##### Parameters

None.

##### Returns

`int64` — The number of stack frames, or 0 if no trace is available.

##### Errors

None.

##### Complete Example

```hoo
let count = e.frameCount();
// count >= 0
```

#### frame

##### Description

Returns the stack frame at the given index (0-based).

##### Syntax

```hoo
frame(index: int64) :string
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| index | `int64` | The 0-based frame index. |

##### Returns

`string` — The stack frame string, or nil if the index is out of range.

##### Errors

None (returns nil for invalid index).

##### Complete Example

```hoo
let top = e.frame(0);                      // topmost frame
let bottom = e.frame(e.frameCount() - 1);  // bottom frame
```

#### print

##### Description

Prints the exception details (type, message, and stack trace) to stderr.

##### Syntax

```hoo
print() :void
```

##### Parameters

None.

##### Returns

`void` — Nothing.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("fatal error");
e.print();
// stderr: "RuntimeException: fatal error"
```

#### equals

##### Description

Compares two exceptions for equality. Two exceptions are equal if they have the same type ID and message.

##### Syntax

```hoo
equals(other: Exception) :int64
```

##### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| other | `Exception` | The exception to compare against. |

##### Returns

`int64` — 1 if equal, 0 otherwise.

##### Errors

None.

##### Complete Example

```hoo
let a = Exception("error");
let b = Exception("error");
let c = Exception("different");
println(a.equals(b)); // 1
println(a.equals(c)); // 0
```

#### debug

##### Description

Returns a detailed debug representation of the exception, including type, message, refcount, and stack trace.

##### Syntax

```hoo
debug() :string
```

##### Parameters

None.

##### Returns

`string` — A debug string with full exception details.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("test");
let d = e.debug();
```

#### retain

##### Description

Increments the exception's reference count. Used when an exception is assigned to a new variable or passed as a parameter. Every `retain()` must be paired with a `release()`.

##### Syntax

```hoo
retain() :void
```

##### Parameters

None.

##### Returns

`void` — Nothing.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("shared");
e.retain();
// use e in another scope
e.release();
```

#### release

##### Description

Decrements the exception's reference count. When the count reaches zero, the exception is freed. Called automatically when an exception variable goes out of scope.

##### Syntax

```hoo
release() :void
```

##### Parameters

None.

##### Returns

`void` — Nothing.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("temp");
e.release(); // explicit release
```

#### refcount

##### Description

Returns the current reference count of the exception for debugging and testing purposes.

##### Syntax

```hoo
refcount() :int64
```

##### Parameters

None.

##### Returns

`int64` — The current reference count, or 0 if the exception is nil.

##### Errors

None.

##### Complete Example

```hoo
let e = Exception("test");
let rc = e.refcount();
// rc == 1
```

## Usage Example

```hoo
import hoo;

func :int64 main() {
    try
        var arr = [1, 2, 3];
        var x = arr[10];
    catch e: IndexOutOfBoundsException
        var msg = e.reason();
        var name = e.typeName();
        var trace = e.stackTrace();
        var frames = e.frameCount();

        if frames > 0
            var topFrame = e.frame(0);
            println(topFrame);
        end

        if e.hasCause() != 0
            var cause = e.cause();
            println(cause.reason());
        end

        e.print();
    catch e: Exception
        println("Caught: " + e.reason());
    finally
        // cleanup always runs
    end

    return 0;
}
```
