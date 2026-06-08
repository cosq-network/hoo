# Process (`hoo.process`)

The `hoo.process` module provides spawn (fork/exec), wait, kill, self-pid, and command capture via POSIX APIs and `popen`. **Not available on Windows.**

## 1. Spawn & Control

- `Process.spawn(command, argv)` — Fork and exec a process. Returns PID on success, -1 on failure.
- `Process.wait(pid)` — Wait for process to exit. Returns exit code on success, -1 on failure.
- `Process.kill(pid, signal)` — Send signal to process. Returns 0 on success.
- `Process.self()` — Returns current process ID.

## 2. Command Capture

- `Process.capture(command)` — Execute command and capture stdout into allocated string (free with `Process.free_string`).
- `Process.capture_status(command)` — Execute command, capture both stdout and exit status.

## Usage from Hoo Source

All `Process.*` functions are available on the `Process` class:

```hoo
func :int64 demo() {
    var pid = Process.self();
    var out = Process.capture("echo hello");         // captured stdout
    var ok = Process.kill(pid, 0);                    // signal 0 = existence check
    return string_length(out);
}
```

## Memory Management

Output strings must be freed with `Process.free_string(str)`.
