# System Information (`hoo.system`)

The `hoo.system` module provides environment variables, OS name, hostname, CPU count, process ID, user info, and current working directory.

Functions returning `char*` allocate strings that the caller must free with `System.free_string`.

## 1. Environment Variables

- `System.get_env(name)` — Get environment variable value, or NULL if not set.
- `System.set_env(name, value)` — Set environment variable. Returns 0 on success, -1 on failure.
- `System.unset_env(name)` — Unset environment variable.

## 2. System Info

- `System.hostname()` — Hostname string.
- `System.os_name()` — OS name (`"macOS"`, `"Linux"`, `"Windows"`, or `"Unknown"`).
- `System.os_version()` — OS version string (e.g., kernel release).
- `System.cpu_count()` — Number of logical CPU cores.
- `System.pid()` — Current process ID.
- `System.uptime_ms()` — System uptime in milliseconds, or -1 if unavailable.

## 3. Process Control

- `System.exit(code)` — Terminate the process with an exit code.
- `System.exec(command)` — Execute a shell command and capture stdout (returns allocated string).
- `System.exec_status(command)` — Execute a shell command and return the exit status.

## 4. User Info

- `System.user_home()` — User's home directory path.
- `System.user_name()` — User login name.
- `System.cwd()` — Current working directory.
- `System.set_cwd(path)` — Change working directory. Returns 0 on success.

## 5. Memory Info

- `System.total_memory()` — Total physical RAM in bytes, or -1 if unavailable.
- `System.free_memory()` — Free physical RAM in bytes, or -1 if unavailable.

## Memory Management

Allocated strings must be freed with `System.free_string(str)`.
