# System Information (`hoo.system`)

The `hoo.system` module provides free functions for environment variables, OS name, hostname, CPU count, process ID, user info, process execution, and memory info.

## 1. Environment Variables

- `system_get_env(name)` — Get environment variable value, or empty string if not set.
- `system_set_env(name, value)` — Set environment variable. Returns 1 on success, 0 on failure.
- `system_unset_env(name)` — Unset environment variable.

## 2. System Info

- `system_hostname()` — Hostname string.
- `system_os_name()` — OS name (`"darwin"`, `"linux"`, `"windows"`, or `"unknown"`).
- `system_os_version()` — OS version string (e.g., kernel release).
- `system_cpu_count()` — Number of logical CPU cores.
- `system_process_id()` — Current process ID.
- `system_uptime_ms()` — System uptime in milliseconds.

## 3. Process Control

- `system_exit(code)` — Terminate the process with an exit code.
- `system_exec(command)` — Execute a shell command and capture stdout.
- `system_exec_status(command)` — Execute a shell command and return the exit status.

## 4. User Info

- `system_user_home()` — User's home directory path.
- `system_user_name()` — User login name.
- `system_current_dir()` — Current working directory.
- `system_set_current_dir(path)` — Change working directory. Returns 1 on success.

## 5. Memory Info

- `system_total_memory()` — Total physical RAM in bytes.
- `system_free_memory()` — Free physical RAM in bytes.
