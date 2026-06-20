# System API Reference

## Module

`hoo.system`

## Import Statement

```hoo
import hoo.system;
```

## Module Description

The `system` module provides free functions for interacting with the operating system, including querying system information, reading and modifying environment variables, and accessing high-resolution timing.

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
import hoo.system;

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
import hoo.system;

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
import hoo.system;

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
import hoo.system;

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
import hoo.system;

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
import hoo.system;

func :void example() {
    var val = system_env("HOME");
    println("Home: " + val);
    system_free_string(val);
}
```

---

### `system_hostname`

Returns the system's hostname.

**Syntax:**
```hoo
system_hostname() :string
```
**Parameters:** None.
**Returns:** `string` — The hostname string. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the hostname cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var host = system_hostname();
    if host != 0 {
        println(host);
        system_free_string(host);
    }
}
```

---

### `system_os_name`

Returns the name of the operating system.

**Syntax:**
```hoo
system_os_name() :string
```
**Parameters:** None.
**Returns:** `string` — The OS name (e.g., "Darwin", "Linux", "Windows"). Must be freed with `system_free_string`.
**Errors:** Returns `0` if the OS name cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var os = system_os_name();
    if os != 0 {
        println(os);
        system_free_string(os);
    }
}
```

---

### `system_os_version`

Returns the version string of the operating system.

**Syntax:**
```hoo
system_os_version() :string
```
**Parameters:** None.
**Returns:** `string` — The OS version string. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the OS version cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var ver = system_os_version();
    if ver != 0 {
        println(ver);
        system_free_string(ver);
    }
}
```

---

### `system_cpu_count`

Returns the number of logical CPU cores available.

**Syntax:**
```hoo
system_cpu_count() :int64
```
**Parameters:** None.
**Returns:** `int64` — The number of logical CPU cores. Returns `1` if the count cannot be determined.
**Errors:** None.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var cpus = system_cpu_count();
    println("CPU cores: " + cpus);
}
```

---

### `system_process_id`

Returns the process ID of the current process.

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

func :void example() {
    var pid = system_process_id();
    println("PID: " + pid);
}
```

---

### `system_uptime_ms`

Returns the system uptime in milliseconds.

**Syntax:**
```hoo
system_uptime_ms() :int64
```
**Parameters:** None.
**Returns:** `int64` — The uptime in milliseconds. Returns `-1` on error.
**Errors:** Returns `-1` if the uptime cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var uptime = system_uptime_ms();
    if uptime >= 0 {
        println("Uptime: " + uptime + " ms");
    }
}
```

---

### `system_exit`

Terminates the current process with the specified exit code.

**Syntax:**
```hoo
system_exit(code: int64) :void
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `code` | `int64` | The exit code to return to the operating system. |
**Returns:** `void` — This function does not return.
**Errors:** None.
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

Executes a shell command and returns its standard output as a string.

**Syntax:**
```hoo
system_exec(command: string) :string
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The shell command to execute. |
**Returns:** `string` — The captured stdout output. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the command cannot be executed.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var out = system_exec("echo hello");
    if out != 0 {
        println(out);
        system_free_string(out);
    }
}
```

---

### `system_exec_status`

Executes a shell command and returns an array containing stdout, stderr, and the exit code.

**Syntax:**
```hoo
system_exec_status(command: string) :array
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The shell command to execute. |
**Returns:** `array` — An array of three elements: `[stdout: string, stderr: string, exit_code: int64]`.
**Errors:** Returns `0` if the command cannot be executed.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var result = system_exec_status("ls /nonexistent");
    // result[0] = ""  (stdout)
    // result[1] = "ls: /nonexistent: No such file or directory\n" (stderr)
    // result[2] = 1   (exit code)
    println(result.length()); // 3
}
```

---

### `system_user_home`

Returns the current user's home directory path.

**Syntax:**
```hoo
system_user_home() :string
```
**Parameters:** None.
**Returns:** `string` — The home directory path. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the home directory cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var home = system_user_home();
    if home != 0 {
        println(home);
        system_free_string(home);
    }
}
```

---

### `system_user_name`

Returns the current user's login name.

**Syntax:**
```hoo
system_user_name() :string
```
**Parameters:** None.
**Returns:** `string` — The user name. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the user name cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var user = system_user_name();
    if user != 0 {
        println(user);
        system_free_string(user);
    }
}
```

---

### `system_current_dir`

Returns the current working directory.

**Syntax:**
```hoo
system_current_dir() :string
```
**Parameters:** None.
**Returns:** `string` — The current working directory path. Must be freed with `system_free_string`.
**Errors:** Returns `0` if the current directory cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var cwd = system_current_dir();
    if cwd != 0 {
        println(cwd);
        system_free_string(cwd);
    }
}
```

---

### `system_set_current_dir`

Changes the current working directory.

**Syntax:**
```hoo
system_set_current_dir(path: string) :int64
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The new working directory path. |
**Returns:** `int64` — `0` on success, non-zero on failure.
**Errors:** Returns non-zero if `path` is nil or the directory cannot be changed.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var ok = system_set_current_dir("/tmp");
    println(ok); // 0 on success
}
```

---

### `system_total_memory`

Returns the total physical memory in bytes.

**Syntax:**
```hoo
system_total_memory() :int64
```
**Parameters:** None.
**Returns:** `int64` — Total physical memory in bytes. Returns `-1` on error.
**Errors:** Returns `-1` if the memory size cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var mem = system_total_memory();
    if mem >= 0 {
        println("Total memory: " + mem + " bytes");
    }
}
```

---

### `system_free_memory`

Returns the available (free) physical memory in bytes.

**Syntax:**
```hoo
system_free_memory() :int64
```
**Parameters:** None.
**Returns:** `int64` — Available memory in bytes. Returns `-1` on error.
**Errors:** Returns `-1` if the memory size cannot be determined.
**Complete Example:**
```hoo
import hoo.system;

func :void example() {
    var free = system_free_memory();
    if free >= 0 {
        println("Free memory: " + free + " bytes");
    }
}
```

## Usage Example

```hoo
import hoo.system;

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
