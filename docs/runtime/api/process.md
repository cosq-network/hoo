# Process API Reference

## Module Name

`Process` — `hoo` module

## Import Statement

```hoo
import hoo;
```

## Module Description

The `Process` class provides static utility methods for process control and command execution, including spawning child processes, capturing command output, querying process identity, and terminating the current process.

---

## Class: `Process`

### Declaration

```hoo
class Process
```

`Process` is a static utility class. It has no constructor and cannot be instantiated — all operations are performed via class (static) methods.

### Public Fields

None — `Process` is a purely static utility class.

### Public Class (Static) Functions

---

### `Process.execute`

**Description:** Executes a shell command and returns its standard output as a string.

**Syntax:**
```hoo
Process.execute(command: string) :string
```

**Parameters:**
- `command: string` — The shell command to execute.

**Returns:** `string` — The captured stdout output.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var out = Process.execute("echo hello");
    println(out); // "hello\n"
}
```

---

### `Process.capture`

**Description:** Executes a shell command with stdin input and returns its standard output as a string.

**Syntax:**
```hoo
Process.capture(command: string, input: string) :string
```

**Parameters:**
- `command: string` — The shell command to execute.
- `input: string` — The stdin input to send to the command.

**Returns:** `string` — The captured stdout output.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var out = Process.capture("wc -c", "hello");
    println(out); // "6\n"
}
```

---

### `Process.capture_status`

**Description:** Executes a shell command with stdin input and returns an array containing the stdout, stderr, and exit code.

**Syntax:**
```hoo
Process.capture_status(command: string, input: string) :array
```

**Parameters:**
- `command: string` — The shell command to execute.
- `input: string` — The stdin input to send to the command.

**Returns:** `array` — An array of three elements: `[stdout: string, stderr: string, exit_code: int64]`.

**Errors:** Returns `0` if the command cannot be executed.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var result = Process.capture_status("grep foo", "hello\nworld");
    // result[0] = "hello\nworld"  (if both match) or "" (if neither matches)
    // result[1] = ""              (stderr, empty on success)
    // result[2] = 0               (exit code)
    println(result.length()); // 3
}
```

---

### `Process.exit`

**Description:** Terminates the current process with the specified exit code.

**Syntax:**
```hoo
Process.exit(exit_code: int64) :void
```

**Parameters:**
- `exit_code: int64` — The exit code to return to the operating system.

**Returns:** `void` — This function does not return.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :int64 main() {
    Process.exit(0);
    return 0; // never reached
}
```

---

### `Process.pid`

**Description:** Returns the process ID of the current process.

**Syntax:**
```hoo
Process.pid() :int64
```

**Parameters:** None.

**Returns:** `int64` — The current process ID.

**Errors:** None.

**Complete Example:**
```hoo
import hoo;

func :void example() {
    var pid = Process.pid();
    println("Current PID: " + pid);
}
```

---

## Usage Example

```hoo
import hoo;

func :int64 main() {
    var pid = Process.pid();
    println("PID: " + pid);

    var out = Process.execute("echo hello world");
    println(out); // "hello world\n"

    var result = Process.capture_status("cat", "line1\nline2");
    println(result.length()); // 3

    return 0;
}
```
