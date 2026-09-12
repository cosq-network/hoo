# Process API Reference

## Module Name

`process` — part of the `hoo` module.

## Import Statement

```hoo
import hoo.process;
```

## Module Description

The `process` module provides free functions for process control and command
execution: spawning a child process, capturing its standard output, waiting
for it to finish, signaling it, and querying the current process ID.
Spawning, waiting, and sharing stdin/stdout are done through the operating
system process API (no intermediate shell for `process_spawn`).

## Call conventions

* String-returning functions return managed Hoo strings.  They are garbage
  collected; never call a manual "free" function.
* `process_capture` returns `nil` when the command cannot be spawned.  It
  captures only standard output; standard error is inherited by the parent.
* Status functions report failure through their return value (an error value
  or `-1`), so check the result — unlike `hoo.system`, the `process` module
  does not raise on failure.

---

## Free Functions

---

#### `process_self_pid`

**Description:** Returns the process ID of the current process.

**Syntax:**
```hoo
process_self_pid() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current process ID.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    if (process_self_pid() < 1) { return 0; }
    return 1;
}
```

---

#### `process_spawn`

**Description:** Spawns a child process with the given command line arguments,
without going through a shell.

**Syntax:**
```hoo
process_spawn(command: string, argv: array) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The executable (or script) to run. |
| `argv` | `array` | The arguments to pass to the process (strings). |

**Returns:** `int64` — The PID of the spawned child process, or `-1` if the
process could not be spawned.

**Errors:** Returns `-1` (no exception is raised).

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    var pid = process_spawn("cmd.exe", ["/c", "exit 0"]);
    if (pid < 1) { return 0; }
    return 1;
}
```

> **Portability:** `process_spawn` invokes the executable directly (no shell).
> The examples use `cmd.exe` for illustration; on Linux/macOS use the actual
> executable path instead, e.g. `process_spawn("/bin/sh", ["-c", "exit 0"])`.

---

#### `process_wait`

**Description:** Waits for a previously spawned child process to finish and
returns its exit code.

**Syntax:**
```hoo
process_wait(pid: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pid` | `int64` | The PID of the child process to wait for. |

**Returns:** `int64` — The exit code of the child process, or `-1` if the wait
failed (no such process, etc.).

**Errors:** Returns `-1` (no exception is raised).

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    var pid = process_spawn("cmd.exe", ["/c", "exit 7"]);
    if (pid < 1) { return 0; }
    if (process_wait(pid) != 7) { return 0; }
    return 1;
}
```

---

#### `process_kill`

**Description:** Sends a signal to a process.

**Syntax:**
```hoo
process_kill(pid: int64, signal: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `pid` | `int64` | The PID of the target process. |
| `signal` | `int64` | The signal number to send. |

**Returns:** `int64` — `0` on success, or a non-zero error indicator on failure.

**Errors:** Returns a non-zero value on failure (no exception is raised).

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    var pid = process_spawn("cmd.exe", ["/c", "ping -n 5 127.0.0.1 >nul"]);
    if (pid < 1) { return 0; }
    // Signal 0 checks that the process exists (POSIX semantics).
    if (process_kill(pid, 0) != 0) { return 0; }
    process_wait(pid);
    return 1;
}
```

---

#### `process_capture`

**Description:** Executes a command through the system shell and returns its
standard output (text).

**Syntax:**
```hoo
process_capture(command: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `command` | `string` | The shell command to execute. |

**Returns:** `string` — The captured standard output, or `nil` if the command
cannot be spawned.

**Errors:** Returns `nil` if the command cannot be spawned (no exception is
raised). Only standard output is captured; standard error is inherited, so add
`2>&1` to capture it too.

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    var out = process_capture("echo hello");
    if (!out) { return 0; }
    if (out.length() == 0) { return 0; }
    return 1;
}
```

---

## Usage Example

```hoo
import hoo.process;

func :int64 main() {
    if (process_self_pid() < 1) { return 0; }

    var out = process_capture("echo hello world");
    if (!out) { return 0; }
    if (out.length() == 0) { return 0; }

    var pid = process_spawn("cmd.exe", ["/c", "exit 3"]);
    if (pid < 1) { return 0; }
    if (process_wait(pid) != 3) { return 0; }

    return 1;
}
```