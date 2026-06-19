# Args API Developer Reference

The `Args` runtime API provides an instance-based interface for command-line
arguments. Create an instance with `new Args()`.

Before calling `Args` methods from JIT-compiled Hoo code, the host must provide
raw command-line arguments by calling `hoo_args_init(argc, argv)` from C/C++.
The runtime parses:

- positional arguments, excluding the program name
- long options such as `--output=result.txt` and `--output result.txt`
- short options such as `-o result.txt`
- flags without values such as `--verbose`
- `--` as an end-of-options marker

The API has two layers:

- raw access methods for reading positional and named arguments directly
- argparse-style methods for declaring typed arguments and reading parsed values

## Host Setup

```cpp
const char* argv[] = {
    "/usr/bin/hoo",
    "input.txt",
    "--output=result.txt",
    "--verbose"
};
hoo_args_init(4, argv);
```

Call `hoo_args_shutdown()` from the host when the runtime no longer needs the
argument snapshot.

## `new Args`

### Description

Creates an `Args` parser handle for raw argument access and argparse-style
definitions.

### Syntax

```hoo
new Args() :Args
```

### Parameters

None.

### Return Type

`Args`
An argument parser instance.

### Errors

Returns a runtime object handle. If the host has not called `hoo_args_init`,
the native constructor returns null.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    return args.count();
}
```

## `count`

### Description

Returns the number of positional arguments. The program name and named flags are
not counted.

### Syntax

```hoo
args.count() :int64
```

### Parameters

None.

### Return Type

`int64`
The positional argument count.

### Errors

Returns `0` when no argument data is available.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    var count = args.count();
    println("positional args: " + count);
    return count;
}
```

## `get`

### Description

Returns a positional argument by zero-based index.

### Syntax

```hoo
args.get(index: int64) :string
```

### Parameters

`index`
Zero-based positional argument index.

### Return Type

`string`
The positional argument value. Out-of-range indexes return a null string handle
from the JIT wrapper.

### Errors

Does not throw for out-of-range indexes. Check `count()` before calling `get`
when the result will be used as a string.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    if args.count() == 0 {
        return 0;
    }

    var first = args.get(0);
    println(first);
    return first.length();
}
```

## `has`

### Description

Checks whether a named argument or flag exists.

### Syntax

```hoo
args.has(name: string) :int64
```

### Parameters

`name`
The argument name without leading dashes. For `--output=result.txt`, use
`"output"`. For `-v`, use `"v"`.

### Return Type

`int64`
Returns `1` when the argument exists, otherwise `0`.

### Errors

Returns `0` for missing names or unavailable argument data.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    if args.has("verbose") == 1 {
        println("verbose enabled");
        return 1;
    }
    return 0;
}
```

## `value`

### Description

Returns the raw value associated with a named argument.

For `--output=result.txt`, `args.value("output")` returns `"result.txt"`. For a
flag without a value, such as `--verbose`, it returns an empty string.

### Syntax

```hoo
args.value(name: string) :string
```

### Parameters

`name`
The argument name without leading dashes.

### Return Type

`string`
The raw value for the named argument. Flags without values return an empty
string. Missing names return a null string handle from the JIT wrapper.

### Errors

Does not throw for missing names.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    var output = args.value("output");
    println(output);
    return output.length();
}
```

## `programName`

### Description

Returns the program name/path from `argv[0]`.

### Syntax

```hoo
args.programName() :string
```

### Parameters

None.

### Return Type

`string`
The program name or path.

### Errors

Returns an empty string when no program name is available.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    var name = args.programName();
    println(name);
    return name.length();
}
```

## `addString`

### Description

Defines a string-valued optional argument for argparse-style parsing.

### Syntax

```hoo
args.addString(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: string)
```

### Parameters

`name`
The parsed value name used by `getString`.

`shortOpt`
The short option, such as `"-o"`. Pass `""` when there is no short option.

`longOpt`
The long option, such as `"--output"`. Pass `""` when there is no long option.

`help`
Help text used by `helpText`.

`defaultVal`
Value returned when the option is not provided.

### Return Type

`void`

### Errors

Invalid or nil strings are treated as empty strings by the native runtime.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addString("output", "-o", "--output", "Output file", "out.txt");
    args.parse();

    var output = args.getString("output");
    println(output);
    return output.length();
}
```

## `addInt`

### Description

Defines an integer-valued optional argument for argparse-style parsing.

### Syntax

```hoo
args.addInt(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: int64)
```

### Parameters

`name`
The parsed value name used by `getInt`.

`shortOpt`
The short option, such as `"-c"`, or `""`.

`longOpt`
The long option, such as `"--count"`, or `""`.

`help`
Help text used by `helpText`.

`defaultVal`
Integer returned when the option is not provided or cannot be parsed.

### Return Type

`void`

### Errors

Does not throw on parse conversion failure; the default value is used.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addInt("count", "-c", "--count", "Iteration count", 1);
    args.parse();

    var count = args.getInt("count");
    println("count: " + count);
    return count;
}
```

## `addFlag`

### Description

Defines a boolean flag. Flags do not consume a value.

### Syntax

```hoo
args.addFlag(name: string, shortOpt: string, longOpt: string, help: string)
```

### Parameters

`name`
The parsed flag name used by `getBool`.

`shortOpt`
The short option, such as `"-v"`, or `""`.

`longOpt`
The long option, such as `"--verbose"`, or `""`.

`help`
Help text used by `helpText`.

### Return Type

`void`

### Errors

Does not throw.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addFlag("verbose", "-v", "--verbose", "Enable verbose logging");
    args.parse();

    if args.getBool("verbose") == 1 {
        println("verbose");
        return 1;
    }
    return 0;
}
```

## `addFloat`

### Description

Defines a floating-point optional argument for argparse-style parsing.

### Syntax

```hoo
args.addFloat(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: f64)
```

### Parameters

`name`
The parsed value name used by `getFloat`.

`shortOpt`
The short option, such as `"-t"`, or `""`.

`longOpt`
The long option, such as `"--threshold"`, or `""`.

`help`
Help text used by `helpText`.

`defaultVal`
Floating-point value returned when the option is not provided or cannot be
parsed.

### Return Type

`void`

### Errors

Does not throw on parse conversion failure; the default value is used.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addFloat("threshold", "-t", "--threshold", "Threshold", 0.5);
    args.parse();

    var threshold = args.getFloat("threshold");
    println("threshold: " + threshold);
    return 0;
}
```

## `addPositional`

### Description

Defines a positional argument for argparse-style parsing. Positional arguments
are matched in declaration order.

### Syntax

```hoo
args.addPositional(name: string, help: string)
```

### Parameters

`name`
The parsed value name used by `getString`.

`help`
Help text used by `helpText`.

### Return Type

`void`

### Errors

Does not throw when the positional argument is missing; `getString` returns an
empty string.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addPositional("input", "Input file path");
    args.parse();

    var input = args.getString("input");
    println(input);
    return input.length();
}
```

## `parse`

### Description

Parses the host-provided command-line arguments against the definitions added
with `addString`, `addInt`, `addFlag`, `addFloat`, and `addPositional`.

### Syntax

```hoo
args.parse() :int64
```

### Parameters

None.

### Return Type

`int64`
Returns `1` on success. Returns `0` for parse failure or help flow, such as
`--help`.

### Errors

Does not throw for missing or malformed user input.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addString("output", "-o", "--output", "Output file", "out.txt");
    args.addFlag("verbose", "-v", "--verbose", "Verbose mode");

    if args.parse() == 0 {
        println(args.helpText());
        return 1;
    }

    return args.getBool("verbose");
}
```

## `getString`

### Description

Returns a parsed string or positional argument value by definition name.

### Syntax

```hoo
args.getString(name: string) :string
```

### Parameters

`name`
The argument definition name.

### Return Type

`string`
The parsed value, the configured default value, or an empty string.

### Errors

Does not throw for unknown names.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addString("name", "-n", "--name", "Display name", "world");
    args.parse();

    var name = args.getString("name");
    println("hello " + name);
    return name.length();
}
```

## `getInt`

### Description

Returns a parsed integer argument value by definition name.

### Syntax

```hoo
args.getInt(name: string) :int64
```

### Parameters

`name`
The argument definition name.

### Return Type

`int64`
The parsed integer value, the configured default value, or `0` for unknown
names.

### Errors

Does not throw for unknown names or failed integer conversion.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addInt("port", "-p", "--port", "Server port", 8080);
    args.parse();

    var port = args.getInt("port");
    println("port: " + port);
    return port;
}
```

## `getBool`

### Description

Returns whether a parsed flag was present.

### Syntax

```hoo
args.getBool(name: string) :int64
```

### Parameters

`name`
The flag definition name.

### Return Type

`int64`
Returns `1` when the flag was present, otherwise `0`.

### Errors

Does not throw for unknown names.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addFlag("dryRun", "-n", "--dry-run", "Do not write changes");
    args.parse();

    return args.getBool("dryRun");
}
```

## `getFloat`

### Description

Returns a parsed floating-point argument value by definition name.

### Syntax

```hoo
args.getFloat(name: string) :f64
```

### Parameters

`name`
The argument definition name.

### Return Type

`f64`
The parsed floating-point value, the configured default value, or `0.0` for
unknown names.

### Errors

Does not throw for unknown names or failed floating-point conversion.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addFloat("timeout", "", "--timeout", "Timeout seconds", 30.0);
    args.parse();

    var timeout = args.getFloat("timeout");
    println("timeout: " + timeout);
    return 0;
}
```

## `helpText`

### Description

Builds a formatted help string from the current argument definitions.

### Syntax

```hoo
args.helpText() :string
```

### Parameters

None.

### Return Type

`string`
Generated usage/help text.

### Errors

Returns help text for the definitions currently attached to the `Args` instance.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addString("output", "-o", "--output", "Output file", "out.txt");
    args.addFlag("verbose", "-v", "--verbose", "Verbose mode");

    var help = args.helpText();
    println(help);
    return help.length();
}
```

## `clear`

### Description

Clears argument definitions and parsed values so the same `Args` instance can
be reused.

### Syntax

```hoo
args.clear()
```

### Parameters

None.

### Return Type

`void`

### Errors

Does not throw.

### Complete Example

```hoo
func :int64 main() {
    var args = new Args();
    args.addString("output", "-o", "--output", "Output file", "out.txt");
    args.parse();

    var first = args.getString("output");
    args.clear();

    args.addString("name", "-n", "--name", "Name", "world");
    args.parse();

    var second = args.getString("name");
    return first.length() + second.length();
}
```
