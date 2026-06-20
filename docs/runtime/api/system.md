# System API Reference

## Module Name

`System` — core module (no explicit import required beyond `import hoo;`)

## Import Statement

```hoo
import hoo;
```

## Module Description

The `System` module provides free functions for interacting with the operating system, including querying system information, reading and modifying environment variables, and accessing high-resolution timing. All functions are available with only `import hoo;`.

---

## Free Functions

---

### `system_info`

**Description:** Returns a string containing information about the operating system.

**Syntax:**
```hoo
system_info() :string
```

**Parameters:** None.

**Returns:** `string` — A descriptive string with system information.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var info = system_info();
    println(info);
}
```

---

### `system_env`

**Description:** Retrieves the value of an environment variable.

**Syntax:**
```hoo
system_env(name: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable. |

**Returns:** `string` — The value of the environment variable, or an empty string if the variable is not set.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var path = system_env("PATH");
    println("PATH: " + path);
}
```

---

### `system_time_nanos`

**Description:** Returns a high-resolution time value in nanoseconds. The epoch is not specified; this function is intended for measuring elapsed time (differences between successive calls).

**Syntax:**
```hoo
system_time_nanos() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current time in nanoseconds.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var start = system_time_nanos();
    // perform some operation
    var elapsed = system_time_nanos() - start;
    println("Elapsed: " + elapsed + " ns");
}
```

---

### `system_env_set`

**Description:** Sets an environment variable to the specified value. If the variable already exists, it is overwritten.

**Syntax:**
```hoo
system_env_set(name: string, value: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable. |
| `value` | `string` | The value to set. |

**Returns:** `int64` — 0 on success, non-zero on failure.

**Errors:** Returns a non-zero value if the operation fails.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var status = system_env_set("MY_VAR", "hello");
    if (status == 0) {
        println("Variable set successfully");
    }
}
```

---

### `system_env_unset`

**Description:** Unsets (removes) an environment variable.

**Syntax:**
```hoo
system_env_unset(name: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable to unset. |

**Returns:** `int64` — 0 on success, non-zero on failure.

**Errors:** Returns a non-zero value if the operation fails.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var status = system_env_unset("MY_VAR");
    if (status == 0) {
        println("Variable unset successfully");
    }
}
```

---

### `system_free_string`

**Description:** Frees a string allocated and returned by a system function. Must be called for every string returned by system functions to avoid memory leaks.

**Syntax:**
```hoo
system_free_string(str: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `str` | `string` | The string to free. |

**Returns:** `void`

**Errors:** No errors. Passing null is a no-op.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var val = system_env("HOME");
    println("Home: " + val);
    system_free_string(val);
}
```

---

## Usage Example

```hoo
import hoo;

func :void example() {
    var path = system_env("PATH");
    if (path.length() > 0) {
        println("PATH is set");
        system_free_string(path);
    }

    var t0 = system_time_nanos();
    var info = system_info();
    var t1 = system_time_nanos();
    println(info);
    println("system_info took: " + (t1 - t0) + " ns");

    system_env_set("MY_APP_MODE", "test");
    var mode = system_env("MY_APP_MODE");
    println("Mode: " + mode);
    system_free_string(mode);
    system_env_unset("MY_APP_MODE");
}
```
