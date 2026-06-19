# System API Reference (`hoo.system`)

**Import Requirement:**
```hoo
import hoo.system;
```

The `hoo.system` module provides free functions to query the operating system, memory info, process execution, and environment.

## Functions

### `system_get_env(name: string) :string`
Returns the value of an environment variable, or empty string if not set.

### `system_set_env(name: string, value: string) :int64`
Sets an environment variable. Returns 1 on success, 0 on failure.

### `system_unset_env(name: string) :int64`
Unsets an environment variable. Returns 1 on success, 0 on failure.

### `system_hostname() :string`
Returns the system hostname.

### `system_os_name() :string`
Returns the operating system name (e.g. `"darwin"`, `"linux"`, `"windows"`).

### `system_os_version() :string`
Returns the operating system version string.

### `system_cpu_count() :int64`
Returns the number of logical CPUs.

### `system_process_id() :int64`
Returns the current process ID.

### `system_uptime_ms() :int64`
Returns the system uptime in milliseconds.

### `system_exit(code: int64)`
Terminates the program with the given exit code.

### `system_exec(command: string) :string`
Executes a shell command and returns its stdout output.

### `system_exec_status(command: string) :int64`
Executes a shell command and returns its exit status.

### `system_user_home() :string`
Returns the current user's home directory path.

### `system_user_name() :string`
Returns the current user's login name.

### `system_current_dir() :string`
Returns the current working directory.

### `system_set_current_dir(path: string) :int64`
Changes the current working directory. Returns 1 on success, 0 on failure.

### `system_total_memory() :int64`
Returns the total physical memory in bytes.

### `system_free_memory() :int64`
Returns the amount of free memory in bytes.

## Example

```hoo
import hoo.system;

func :void example() {
    println("Hostname: " + system_hostname());
    println("OS: " + system_os_name());
    println("User: " + system_user_name());
    println("Home: " + system_user_home());
    println("CPUs: " + system_cpu_count().toString());
    println("Memory: " + system_total_memory().toString() + " bytes");
}
```
