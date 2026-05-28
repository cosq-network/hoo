# Process (`hoo.process`)

The `hoo.process` module provides spawn (fork/exec), wait, kill, self-pid, and command capture via POSIX APIs and `popen`. **Not available on Windows.**

## 1. Spawn & Control

- `hoo_process_spawn(command, argv, &out_pid)` — Fork and exec a process. Returns 0 on success, -1 on failure.
- `hoo_process_wait(pid, &out_exit_code)` — Wait for process to exit. Returns 0 on success.
- `hoo_process_kill(pid, signal)` — Send signal to process. Returns 0 on success.
- `hoo_process_self_pid()` — Returns current process ID.

## 2. Command Capture

- `hoo_process_capture(command)` — Execute command and capture stdout into allocated string (free with `hoo_process_free_string`).
- `hoo_process_capture_status(command, &out_stdout, &out_exit_code)` — Execute command, capture both stdout and exit status. Returns 0 on success.

## Memory Management

Output strings must be freed with `hoo_process_free_string(str)`.
