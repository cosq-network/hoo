# Process API Reference

## Module Name

`process` — part of the `hoo` module.

## Import Statement

```hoo
import hoo.process;
```

## Module Description

The `process` module provides free functions for process control and command execution, including spawning child processes, capturing command output, querying process identity, and terminating the current process.

---

## Free Functions

---

#### `process_execute`

**Description:** Executes a shell command and returns its standard output as a string.

**Syntax:**
```hoo
process_execute(command: string) :string
```

**Parameters:**
- `command: string` — The shell command to execute.

**Returns:** `string` — The captured stdout output.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo.process;

func :void example() {
    var out = process_execute("echo hello");
    println(out); // "hello\n"
}
```

---

#### `process_capture`

**Description:** Executes a shell command with stdin input and returns its standard output as a string.

**Syntax:**
```hoo
process_capture(command: string, input: string) :string
```

**Parameters:**
- `command: string` — The shell command to execute.
- `input: string` — The stdin input to send to the command.

**Returns:** `string` — The captured stdout output.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo.process;

func :void example() {
    var out = process_capture("wc -c", "hello");
    println(out); // "6\n"
}
```

---

#### `process_capture_status`

**Description:** Executes a shell command with stdin input and returns an array containing the stdout, stderr, and exit code.

**Syntax:**
```hoo
process_capture_status(command: string, input: string) :array
```

**Parameters:**
- `command: string` — The shell command to execute.
- `input: string` — The stdin input to send to the command.

**Returns:** `array` — An array of three elements: `[stdout: string, stderr: string, exit_code: int64]`.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo.process;

func :void example() {
    var result = process_capture_status("grep foo", "hello\nworld");
    // result[0] = "hello\nworld"  (if both match) or "" (if neither matches)
    // result[1] = ""              (stderr, empty on success)
    // result[2] = 0               (exit code)
    println(result.length()); // 3
}
```

---

#### `process_exit`

**Description:** Terminates the current process with the specified exit code.

**Syntax:**
```hoo
process_exit(exit_code: int64) :void
```

**Parameters:**
- `exit_code: int64` — The exit code to return to the operating system.

**Returns:** `void` — This function does not return.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.process;

func :int64 main() {
    process_exit(0);
    return 0; // never reached
}
```

---

#### `process_pid`

**Description:** Returns the process ID of the current process.

**Syntax:**
```hoo
process_pid() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current process ID.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.process;

func :void example() {
    var pid = process_pid();
    println("Current PID: " + pid);
}
```

---

## Usage Example

```hoo
import hoo.process;

func :int64 main() {
    var pid = process_pid();
    println("PID: " + pid);

    var out = process_execute("echo hello world");
    println(out); // "hello world\n"

    var result = process_capture_status("cat", "line1\nline2");
    println(result.length()); // 3

    return 0;
}
```
