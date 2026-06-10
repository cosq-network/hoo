# Args — Command-Line Arguments

The `Args` class provides an instance-based interface for reading command-line arguments passed to the program. Obtain an instance via `Args.new()`.

It offers two tiers of API:

1. **Low-level raw access** — direct access to parsed positional and named arguments.
2. **Argparse-style API** — define expected arguments (typed, with defaults) and access parsed values with automatic type conversion.

Before calling any `Args` methods from JIT-compiled Hooc code, you must call `hoo_args_init(argc, argv)` from C/C++ to provide the raw command-line arguments. The examples below are complete, self-contained Hooc programs with `func :int64 main()` as the entry point.

## Low-Level Methods

`count() :int64`
Returns the number of positional arguments (excluding the program name and named flags).

`get(index: int64) :string`
Returns the positional argument at the given 0-based index. Returns an empty string if the index is out of range.

`has(name: string) :int64`
Returns 1 if a named argument with the given name exists (e.g. `--output`), 0 otherwise.

`value(name: string) :string`
Returns the value associated with a named argument. For `--output=result.txt`, calling `value("output")` returns `"result.txt"`. For `--verbose` (no value), returns an empty string.

`programName() :string`
Returns the name or path of the running program (argv[0]).

### Low-Level Example

Given the C++ setup code:

```cpp
const char* argv[] = {"/usr/bin/hoo", "input.txt", "--output=result.txt", "--verbose"};
hoo_args_init(4, argv);
// ... compile and run the Hooc function below
```

The following Hooc program counts positional args, prints each one, checks for named flags, and prints the program name:

```hoo
func :int64 main() {
    var args = Args.new()

    var count = args.count()
    println("Positional args: " + count)

    for i in 0..count {
        println("  " + i + ": " + args.get(i))
    }

    if args.has("output") == 1 {
        var out = args.value("output")
        println("Output: " + out)
    }

    if args.has("verbose") == 1 {
        println("Verbose mode enabled")
    }

    println("Program: " + args.programName())
    return 0
}
```

## Argparse-Style API

The argparse-style API (inspired by Python's `argparse` module) lets you declare typed arguments with flags, defaults, and help text, then parse all arguments at once.

### Argument Definition Methods

`addString(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: string)`
Define a string-valued optional argument. `shortOpt` is the short flag (e.g. `"-o"`); `longOpt` is the long flag (e.g. `"--output"`). Pass `""` for either to omit.

`addInt(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: int64)`
Define an integer-valued optional argument.

`addFlag(name: string, shortOpt: string, longOpt: string, help: string)`
Define a boolean flag (no value expected). `getBool()` returns 1 if the flag was present, 0 otherwise.

`addFloat(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: float)`
Define a float-valued optional argument.

`addPositional(name: string, help: string)`
Define a positional argument. Positional arguments are matched in declaration order.

### Parse & Access Methods

`parse() :int64`
Parse command-line arguments against the defined argument list. Returns 1 on success, 0 on failure (e.g. when `--help` is passed). When `--help` is present, the result is still accessible via `helpText()`.

`getString(name: string) :string`
Get the parsed value of a string or positional argument by its declared name. Returns the default if the argument was not provided.

`getInt(name: string) :int64`
Get the parsed value of an integer argument by its declared name. Returns the default if not provided.

`getBool(name: string) :int64`
Get whether a flag argument was set. Returns 1 if the flag was present, 0 otherwise.

`getFloat(name: string) :float`
Get the parsed value of a float argument by its declared name. Returns the default if not provided.

`helpText() :string`
Generate a formatted help/usage string describing all defined arguments.

`clear()`
Clear all argument definitions and parsed state, allowing the instance to be reused.

### Example 1: All Argument Types

Given the argv `{"program", "data.csv", "--output=report.txt", "--verbose", "--count=10", "--threshold=0.75"}`:

```hoo
func :int64 main() {
    var args = Args.new()

    args.addString("output", "-o", "--output", "Output file path", "out.txt")
    args.addFlag("verbose", "-v", "--verbose", "Enable verbose output")
    args.addInt("count", "-c", "--count", "Number of iterations", 1)
    args.addFloat("threshold", "-t", "--threshold", "Threshold value", 0.5)
    args.addPositional("input", "Input file path")

    if args.parse() == 0 {
        println(args.helpText())
        return 1
    }

    var input = args.getString("input")
    var output = args.getString("output")
    var verbose = args.getBool("verbose")
    var count = args.getInt("count")
    var threshold = args.getFloat("threshold")

    println("Input: " + input)
    println("Output: " + output)
    println("Verbose: " + verbose)
    println("Count: " + count)
    println("Threshold: " + threshold)
    return 0
}
```

### Example 2: Flags and Conditionals

Given the argv `{"program", "--verbose"}`:

```hoo
func :int64 main() {
    var args = Args.new()

    args.addFlag("verbose", "-v", "--verbose", "Enable verbose mode")
    args.addFlag("quiet", "-q", "--quiet", "Suppress output")
    args.addFlag("dryRun", "-n", "--dry-run", "Dry run mode")

    args.parse()

    var verbose = args.getBool("verbose")
    var quiet = args.getBool("quiet")
    var dryRun = args.getBool("dryRun")

    if quiet == 1 {
        println("Quiet mode — no output")
        return 0
    }

    if verbose == 1 {
        println("Verbose logging enabled")
    }

    if dryRun == 1 {
        println("Dry run — no changes will be made")
    }

    println("Verbose=" + verbose + " Quiet=" + quiet + " DryRun=" + dryRun)
    return 0
}
```

### Example 3: Short-Only and Long-Only Options

Given the argv `{"program", "-o", "short.txt", "--name", "alice"}`:

```hoo
func :int64 main() {
    var args = Args.new()

    args.addString("output", "-o", "", "Short-only option", "")
    args.addString("name", "", "--name", "Long-only option", "unknown")

    args.parse()

    var out = args.getString("output")
    var name = args.getString("name")

    println("Output: " + out)
    println("Name: " + name)
    return out.length() + name.length()
}
```

### Example 4: Default Values When Args Are Missing

Given the argv `{"program"}` (no additional arguments):

```hoo
func :int64 main() {
    var args = Args.new()

    args.addString("host", "-h", "--host", "Server host", "localhost")
    args.addInt("port", "-p", "--port", "Server port", 8080)
    args.addFlag("tls", "-t", "--tls", "Enable TLS")
    args.addFloat("timeout", "", "--timeout", "Connection timeout", 30.0)

    args.parse()

    var host = args.getString("host")
    var port = args.getInt("port")
    var tls = args.getBool("tls")
    var timeout = args.getFloat("timeout")

    println("Host: " + host)
    println("Port: " + port)
    println("TLS: " + tls)
    println("Timeout: " + timeout)

    // All defaults used: host="localhost", port=8080, tls=0, timeout=30.0
    return port
}
```

### Example 5: Clear and Reuse

Given the argv `{"program", "--output=file.txt"}`:

```hoo
func :int64 main() {
    var args = Args.new()

    args.addString("output", "-o", "--output", "Output path", "")
    args.parse()
    var first = args.getString("output")
    println("First output: " + first)

    args.clear()

    args.addString("name", "-n", "--name", "Name", "world")
    args.parse()
    var second = args.getString("name")
    println("Hello, " + second)

    return first.length() + second.length()
}
```

### Example 6: Auto-Generated Help Text

Given the argv `{"program", "--help"}`:

```hoo
func :int64 main() {
    var args = Args.new()

    args.addString("output", "-o", "--output", "Output file path", "out.txt")
    args.addFlag("verbose", "-v", "--verbose", "Enable verbose output")
    args.addInt("count", "-c", "--count", "Number of iterations", 1)
    args.addFloat("threshold", "-t", "--threshold", "Threshold value", 0.5)
    args.addPositional("input", "Input file path")

    if args.parse() == 0 {
        var help = args.helpText()
        println(help)
        return help.length()
    }
    return 0
}
```

The generated help text looks like:

```
usage: program [--output OUTPUT] [--verbose] [--count COUNT] [--threshold THRESHOLD] input

positional arguments:
  input                 Input file path

optional arguments:
  -h, --help            Show this help message and exit
  -o, --output          Output file path (default: out.txt)
  -v, --verbose         Enable verbose output
  -c, --count           Number of iterations (default: 1)
  -t, --threshold       Threshold value (default: 0.5)
```
