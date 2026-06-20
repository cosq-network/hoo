# Process — Process Control

The `process` module provides free functions for process control and command execution.

## Functions

`process_self_pid() :int64`
Returns the current process ID.

`process_capture(command: string) :string`
Executes a shell command and captures its stdout output as a string.

`process_kill(pid: int64, signal: int64) :int64`
Sends a signal to a process. Returns 0 on success, -1 on error. Signal 0 checks if the process exists.

`process_spawn(command: string, argv: array) :int64`
Spawns a child process. Returns the PID, or -1 on error.

`process_wait(pid: int64) :int64`
Waits for a child process to exit. Returns the exit code, or -1 on error.

## Example

```hoo
import hoo;
import hoo.process;

let pid = process_self_pid()
println("PID: " + pid)

let out = process_capture("echo hello")
println(out)  // "hello\n"

-- Check if a process exists
let exists = process_kill(pid, 0)  // 0 if exists
```
