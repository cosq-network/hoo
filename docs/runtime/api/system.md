# System API Reference

## Module

`hoo.system`

## Import Statement

```hoo
import hoo.system;
```

## Module Description

The `system` module provides free functions for interacting with the operating
system: reading and modifying environment variables, querying system
information (hostname, OS, CPU, memory, uptime), running shell commands, and
querying/change the current user and working directory.

## Call conventions

* String-returning functions return managed Hoo strings.  They are garbage
  collected; never call a manual "free" function.
* `system_get_env` returns `nil` when the variable is not set — an **expected**
  absence, not an error.  The remaining string getters never return `nil`: they
  return `"unknown"` (`system_hostname`, `system_os_name`, `system_os_version`)
  or an empty string (`system_user_home`, `system_user_name`,
  `system_current_dir`).
* Unexpected failures raise a `RuntimeException` (a subtype of `Exception`)
  instead of returning a sentinel value.  For example, setting an
  environment variable, changing the working directory, or launching a shell
  command that cannot be spawned raises rather than returning `-1`/`nil` at the
  Hoo level.  Catch them with `try/catch`:
  ```hoo
  try {
      system_set_current_dir("/nonexistent");
  } catch (e: Exception) {
      // handled
  }
  ```
  An uncaught system failure terminates the program with the standard
  `Unhandled exception trap` diagnostic, exactly like other uncaught Hoo
  exceptions.  (The underlying C functions still expose the old `-1`/`nil`
  sentinels for C embedders; Hoo callers never see them on the failing paths.)
* On Windows, environment names and values, and the home/user names, are
  decoded as UTF-8; values that cannot be encoded or decoded as UTF-8 are
  treated as failures (and raise, where applicable).
* `system_set_env(name, "")` removes the variable, exactly like
  `system_unset_env(name)`.
* `system_exec` and `system_exec_status` capture only **standard output**
  (text).  Standard error is inherited by the parent process, so redirect it
  with `2>&1` when it must be captured.  There is no stdin pipe.
* `system_exit(code)` clamps the code to `0..255`, flushes pending output, and
  terminates immediately without running ARC/drop teardown or unwinding the
  shadow stack.  Prefer raising an exception when clean unwinding matters.

## Free Functions

---

### `system_get_env`

**Description:** Retrieves the value of an environment variable.

**Syntax:**
```hoo
system_get_env(name: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable. |

**Returns:** `string` — The value of the variable, or `nil` if it is not set.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var path = system_get_env("PATH");
    if (!path) { return 0; }
    if (path.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_set_env`

**Description:** Sets an environment variable to the specified value, overwriting any existing value.

**Syntax:**
```hoo
system_set_env(name: string, value: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable. |
| `value` | `string` | The value to set. |

**Returns:** `int64` — `0` on success.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if `name` or `value` is `nil`, or if the
environment variable cannot be set.  `name == ""` removes the variable (same as
`system_unset_env`).

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    // NOTE: the old "!= 0" sentinel check is no longer needed; a failed set
    // raises, so success is assumed once control reaches the next statement.
    system_set_env("MY_APP_MODE", "test");
    return 1;
}
```

---

### `system_unset_env`

**Description:** Removes an environment variable from the process environment.

**Syntax:**
```hoo
system_unset_env(name: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The name of the environment variable to remove. |

**Returns:** `int64` — `0` on success.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if `name` is `nil` or the variable cannot be
removed.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_unset_env("MY_APP_MODE") != 0) { return 0; }
    return 1;
}
```

---

### `system_hostname`

**Description:** Returns the system's hostname.

**Syntax:**
```hoo
system_hostname() :string
```

**Parameters:** None.

**Returns:** `string` — The hostname, or `"unknown"` if it cannot be determined.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var host = system_hostname();
    if (host.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_os_name`

**Description:** Returns the name of the operating system.

**Syntax:**
```hoo
system_os_name() :string
```

**Parameters:** None.

**Returns:** `string` — `"Windows"`, `"Linux"`, `"macOS"`, or `"Unknown"`.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var os = system_os_name();
    if (os.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_os_version`

**Description:** Returns the version string of the operating system.

**Syntax:**
```hoo
system_os_version() :string
```

**Parameters:** None.

**Returns:** `string` — The OS version (e.g. `10.0.22000` on Windows, kernel release on Linux, product version on macOS), or `"unknown"`.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var ver = system_os_version();
    if (ver.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_cpu_count`

**Description:** Returns the number of logical CPU cores available.

**Syntax:**
```hoo
system_cpu_count() :int64
```

**Parameters:** None.

**Returns:** `int64` — The number of logical CPU cores, at least `1`.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_cpu_count() < 1) { return 0; }
    return 1;
}
```

---

### `system_process_id`

**Description:** Returns the process ID of the current process.

**Syntax:**
```hoo
system_process_id() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current process ID.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_process_id() < 1) { return 0; }
    return 1;
}
```

---

### `system_uptime_ms`

**Description:** Returns the system uptime in milliseconds.

**Syntax:**
```hoo
system_uptime_ms() :int64
```

**Parameters:** None.

**Returns:** `int64` — The uptime in milliseconds.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if the uptime cannot be determined (e.g. the
platform does not expose it).

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_uptime_ms() < 0) { return 0; }
    return 1;
}
```

---

### `system_exit`

**Description:** Terminates the current process with the specified exit code.

**Syntax:**
```hoo
system_exit(code: int64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `code` | `int64` | The exit code to return to the operating system. |

**Returns:** `void` — This function does not return.

**Errors:** None. The process exits immediately; the code is clamped to `0..255`
and pending `stdout`/`stderr` output is flushed first. The function bypasses
ARC/drop teardown and shadow-stack unwinding — embedders that need clean
unwinding should raise a Hoo exception instead.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    system_exit(0);
    return 0; // never reached
}
```

---

### `system_exec`

**Description:** Executes a shell command and returns its standard output.

**Syntax:**
```hoo
system_exec(command: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The shell command to execute. |

**Returns:** `string` — The captured text written to standard output.  On
failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if the command cannot be spawned (popen failure)
or if `command` is `nil`. Only standard output is captured; standard error is
inherited, so add `2>&1` to capture it too. There is no stdin pipe.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var out = system_exec("echo hello");
    if (out.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_exec_status`

**Description:** Executes a shell command, discards its output, and returns the child process exit code.

**Syntax:**
```hoo
system_exec_status(command: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The shell command to execute. |

**Returns:** `int64` — The exit code of the command (the same value on all
platforms).  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if the command cannot be spawned (popen failure)
or if `command` is `nil`.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_exec_status("exit 7") != 7) { return 0; }
    return 1;
}
```

---

### `system_user_home`

**Description:** Returns the current user's home directory path.

**Syntax:**
```hoo
system_user_home() :string
```

**Parameters:** None.

**Returns:** `string` — The home directory path, or an empty string if it cannot be determined.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var home = system_user_home();
    if (home.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_user_name`

**Description:** Returns the current user's login name.

**Syntax:**
```hoo
system_user_name() :string
```

**Parameters:** None.

**Returns:** `string` — The user name, or an empty string if it cannot be determined.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var user = system_user_name();
    if (user.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_current_dir`

**Description:** Returns the current working directory.

**Syntax:**
```hoo
system_current_dir() :string
```

**Parameters:** None.

**Returns:** `string` — The current working directory path, or an empty string on error.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    var dir = system_current_dir();
    if (dir.length() == 0) { return 0; }
    return 1;
}
```

---

### `system_set_current_dir`

**Description:** Changes the current working directory.

**Syntax:**
```hoo
system_set_current_dir(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The new working directory path. |

**Returns:** `int64` — `0` on success.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if `path` is `nil` or the directory cannot be
changed.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    try {
        system_set_current_dir(system_current_dir());
    } catch (e: Exception) {
        return 0;
    }
    return 1;
}
```

---

### `system_total_memory`

**Description:** Returns the total physical memory in bytes.

**Syntax:**
```hoo
system_total_memory() :int64
```

**Parameters:** None.

**Returns:** `int64` — Total physical memory in bytes.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if the memory size cannot be determined.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_total_memory() < 1) { return 0; }
    return 1;
}
```

---

### `system_free_memory`

**Description:** Returns the available (free) physical memory in bytes.

**Syntax:**
```hoo
system_free_memory() :int64
```

**Parameters:** None.

**Returns:** `int64` — Available memory in bytes.  On failure a `RuntimeException` is raised.

**Errors:** Raises `Exception` if the memory size cannot be determined.

**Complete Example:**
```hoo
import hoo.system;

func :int64 main() {
    if (system_free_memory() < 1) { return 0; }
    return 1;
}
```

## Usage Example

```hoo
import hoo.system;

func :int64 main() {
    system_set_env("MY_APP_MODE", "test");

    var stored = system_get_env("MY_APP_MODE");
    if (!stored) { return 0; }
    if (!stored.equals("test")) { return 0; }

    var host = system_hostname();
    if (host.length() == 0) { return 0; }

    try {
        system_unset_env("MY_APP_MODE");
    } catch (e: Exception) {
        return 0;
    }
    return 1;
}
```