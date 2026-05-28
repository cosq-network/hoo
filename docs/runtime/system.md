# System Information (`hoo.system`)

The `hoo.system` module provides environment variables, OS name, hostname, CPU count, process ID, user info, and current working directory.

Functions returning `char*` allocate strings that the caller must free with `hoo_system_free_string`.

## 1. Environment Variables

- `hoo_system_get_env(name)` — Get environment variable value, or NULL if not set.
- `hoo_system_set_env(name, value)` — Set environment variable. Returns 0 on success, -1 on failure.
- `hoo_system_unset_env(name)` — Unset environment variable.

## 2. System Info

- `hoo_system_hostname()` — Hostname string.
- `hoo_system_os_name()` — OS name (`"macOS"`, `"Linux"`, `"Windows"`, or `"Unknown"`).
- `hoo_system_os_version()` — OS version string (e.g., kernel release).
- `hoo_system_cpu_count()` — Number of logical CPU cores.
- `hoo_system_process_id()` — Current process ID.
- `hoo_system_uptime_ms()` — System uptime in milliseconds, or -1 if unavailable.

## 3. Process Control

- `hoo_system_exit(code)` — Terminate the process with an exit code.
- `hoo_system_exec(command)` — Execute a shell command and capture stdout (returns allocated string).
- `hoo_system_exec_status(command)` — Execute a shell command and return the exit status.

## 4. User Info

- `hoo_system_user_home()` — User's home directory path.
- `hoo_system_user_name()` — User login name.
- `hoo_system_current_dir()` — Current working directory.
- `hoo_system_set_current_dir(path)` — Change working directory. Returns 0 on success.

## 5. Memory Info

- `hoo_system_total_memory()` — Total physical RAM in bytes, or -1 if unavailable.
- `hoo_system_free_memory()` — Free physical RAM in bytes, or -1 if unavailable.

## Memory Management

Allocated strings must be freed with `hoo_system_free_string(str)`.
