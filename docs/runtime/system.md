# System Information (`hoo.system`)

The `hoo.system` module provides environment variables, OS name, hostname, CPU count, process ID, user info, and current working directory.

Functions returning `char*` allocate strings that the caller must free with `System.freeString`.

## 1. Environment Variables

- `System.getEnv(name)` — Get environment variable value, or NULL if not set.
- `System.setEnv(name, value)` — Set environment variable. Returns 0 on success, -1 on failure.
- `System.unsetEnv(name)` — Unset environment variable.

## 2. System Info

- `System.hostname()` — Hostname string.
- `System.osName()` — OS name (`"macOS"`, `"Linux"`, `"Windows"`, or `"Unknown"`).
- `System.osVersion()` — OS version string (e.g., kernel release).
- `System.cpuCount()` — Number of logical CPU cores.
- `System.pid()` — Current process ID.
- `System.uptimeMs()` — System uptime in milliseconds, or -1 if unavailable.

## 3. Process Control

- `System.exit(code)` — Terminate the process with an exit code.
- `System.exec(command)` — Execute a shell command and capture stdout (returns allocated string).
- `System.execStatus(command)` — Execute a shell command and return the exit status.

## 4. User Info

- `System.userHome()` — User's home directory path.
- `System.userName()` — User login name.
- `System.cwd()` — Current working directory.
- `System.setCwd(path)` — Change working directory. Returns 0 on success.

## 5. Memory Info

- `System.totalMemory()` — Total physical RAM in bytes, or -1 if unavailable.
- `System.freeMemory()` — Free physical RAM in bytes, or -1 if unavailable.

## Memory Management

Allocated strings must be freed with `System.freeString(str)`.
