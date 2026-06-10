# System — Operating System Information

The `System` class provides static methods to query the operating system and hardware environment.

## Methods

`System.getEnv(name: string) :string`
Returns the value of an environment variable, or empty string if not set.

`System.setEnv(name: string, value: string) :int64`
Sets an environment variable. Returns 1 on success, 0 on failure.

`System.unsetEnv(name: string) :int64`
Unsets an environment variable. Returns 1 on success, 0 on failure.

`System.hostname() :string`
Returns the system hostname.

`System.osName() :string`
Returns the operating system name (e.g. `"darwin"`, `"linux"`, `"windows"`).

`System.osVersion() :string`
Returns the operating system version string.

`System.cpuCount() :int64`
Returns the number of logical CPUs.

`System.processId() :int64`
Returns the current process ID.

`System.uptimeMs() :int64`
Returns the system uptime in milliseconds.

`System.exit(code: int64)`
Terminates the program with the given exit code.

`System.exec(command: string) :string`
Executes a shell command and returns its stdout output.

`System.execStatus(command: string) :int64`
Executes a shell command and returns its exit status.

`System.userHome() :string`
Returns the current user's home directory path.

`System.userName() :string`
Returns the current user's login name.

`System.currentDir() :string`
Returns the current working directory.

`System.setCurrentDir(path: string) :int64`
Changes the current working directory. Returns 1 on success, 0 on failure.

`System.totalMemory() :int64`
Returns the total physical memory in bytes.

`System.freeMemory() :int64`
Returns the amount of free memory in bytes.

## Example

```hoo
println("Hostname: " + System.hostname())
println("OS: " + System.osName())
println("User: " + System.userName())
println("Home: " + System.userHome())
println("CPUs: " + System.cpuCount())
println("Memory: " + System.totalMemory() + " bytes")
```
