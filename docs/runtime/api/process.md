# Process — Process Control

The `Process` class provides static methods for process control and command execution.

## Methods

`Process.self_pid() :int64`
Returns the current process ID.

`Process.capture(command: string) :string`
Executes a shell command and captures its stdout output as a string.

`Process.kill(pid: int64, signal: int64) :int64`
Sends a signal to a process. Returns 0 on success, -1 on error. Signal 0 checks if the process exists.

`Process.spawn(command: string, argv: array) :int64`
Spawns a child process. Returns the PID.

`Process.wait(pid: int64) :int64`
Waits for a child process to exit. Returns the exit code.

## Example

```hoo
let pid = Process.self_pid()
println("PID: " + pid)

let out = Process.capture("echo hello")
println(out)  // "hello\n"

-- Check if a process exists
let exists = Process.kill(pid, 0)  // 0 if exists
```
