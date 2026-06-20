# Args API Reference

## Module Name

`hoo.args`

## Import Statement

```hoo
import hoo.args;
```

## Module Description

The `args` module provides two ways to access command-line arguments:

1. **Free functions** `args_get` and `args_count` for quick positional argument access using the global runtime state.
2. **`Args` class** — a full argparse-style parser with named flags, positional arguments, typed access, and help text generation.

---

## Free Functions

Simple positional argument access without creating a parser instance. Index `0` returns the program name.

---

#### `args_get`

**Description:** Returns the command-line argument at the given zero-based index from the global argument list.

**Syntax:**

```hoo
args_get(index: int64) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | Zero-based argument index. `0` returns the program name. |

**Returns:** `string` — The argument value at the specified index.

**Errors:** Returns an empty string if the index is out of range.

**Complete Example:**

```hoo
import hoo.args;

func :int64 main() {
    var name = args_get(0);
    println(name);
    return name.length();
}
```

---

#### `args_count`

**Description:** Returns the total number of command-line arguments from the global argument list, including the program name.

**Syntax:**

```hoo
args_count() :int64
```

**Parameters:** None.

**Returns:** `int64` — The total number of arguments.

**Errors:** Returns `0` when no argument data is available.

**Complete Example:**

```hoo
import hoo.args;

func :int64 main() {
    var argc = args_count();
    println("arguments: " + argc);
    return argc;
}
```

---

## Class: `Args`

### Declaration

```hoo
class Args
```

### Constructor

#### `new Args`

Creates a new argument parser instance.

**Syntax:**

```hoo
new Args() :Args
```

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
}
```

---

### Methods

#### `addString`

Defines a string-valued named argument (flag).

**Syntax:**

```hoo
parser.addString(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | Destination key name. |
| `shortOpt` | `string` | Short flag (e.g. `"-o"`). Pass `""` if none. |
| `longOpt` | `string` | Long flag (e.g. `"--output"`). Pass `""` if none. |
| `help` | `string` | Help text description. |
| `defaultVal` | `string` | Default value if the flag is not provided. |

**Returns:** `void`

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addString("output", "-o", "--output", "Output file", "default.txt");
}
```

---

#### `addInt`

Defines an integer-valued named argument (flag).

**Syntax:**

```hoo
parser.addInt(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: int64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | Destination key name. |
| `shortOpt` | `string` | Short flag (e.g. `"-c"`). Pass `""` if none. |
| `longOpt` | `string` | Long flag (e.g. `"--count"`). Pass `""` if none. |
| `help` | `string` | Help text description. |
| `defaultVal` | `int64` | Default value if the flag is not provided. |

**Returns:** `void`

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addInt("count", "-c", "--count", "Number of items", 1);
}
```

---

#### `addFlag`

Defines a boolean flag (present or absent).

**Syntax:**

```hoo
parser.addFlag(name: string, shortOpt: string, longOpt: string, help: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | Destination key name. |
| `shortOpt` | `string` | Short flag (e.g. `"-v"`). Pass `""` if none. |
| `longOpt` | `string` | Long flag (e.g. `"--verbose"`). Pass `""` if none. |
| `help` | `string` | Help text description. |

**Returns:** `void`

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addFlag("verbose", "-v", "--verbose", "Enable verbose output");
}
```

---

#### `addFloat`

Defines a float-valued named argument (flag).

**Syntax:**

```hoo
parser.addFloat(name: string, shortOpt: string, longOpt: string, help: string, defaultVal: float64) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | Destination key name. |
| `shortOpt` | `string` | Short flag (e.g. `"-t"`). Pass `""` if none. |
| `longOpt` | `string` | Long flag (e.g. `"--threshold"`). Pass `""` if none. |
| `help` | `string` | Help text description. |
| `defaultVal` | `float64` | Default value if the flag is not provided. |

**Returns:** `void`

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addFloat("threshold", "-t", "--threshold", "Threshold value", 0.5);
}
```

---

#### `addPositional`

Defines a positional argument (no leading dash).

**Syntax:**

```hoo
parser.addPositional(name: string, help: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | Destination key name. |
| `help` | `string` | Help text description. |

**Returns:** `void`

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addPositional("input", "Input file path");
}
```

---

#### `parse`

Parses the actual command-line arguments against the defined argument specifications. Returns 1 on success, 0 on failure (e.g. `--help` was passed).

**Syntax:**

```hoo
parser.parse() :int64
```

**Parameters:** None.

**Returns:** `int64` — 1 if parsing succeeded, 0 if `--help` was requested or a required argument is missing.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addString("output", "-o", "--output", "Output file", "out.txt");
    parser.addFlag("verbose", "-v", "--verbose", "Verbose mode");
    parser.addPositional("input", "Input file");
    if parser.parse() {
        var out = parser.getString("output");
        println(out);
    }
}
```

---

#### `getString`

Retrieves a parsed string value by key name.

**Syntax:**

```hoo
parser.getString(name: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The key name used in `addString` or `addPositional`. |

**Returns:** `string` — The parsed value, or the default if not provided.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addString("output", "-o", "--output", "Output file", "default.txt");
    parser.parse();
    var out = parser.getString("output");
    println(out);
}
```

---

#### `getInt`

Retrieves a parsed integer value by key name.

**Syntax:**

```hoo
parser.getInt(name: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The key name used in `addInt`. |

**Returns:** `int64` — The parsed integer value, or the default if not provided.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addInt("count", "-c", "--count", "Count", 0);
    parser.parse();
    var count = parser.getInt("count");
    println(count);
}
```

---

#### `getBool`

Retrieves a parsed boolean flag value by key name.

**Syntax:**

```hoo
parser.getBool(name: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The key name used in `addFlag`. |

**Returns:** `int64` — 1 if the flag was present, 0 otherwise.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addFlag("verbose", "-v", "--verbose", "Verbose mode");
    parser.parse();
    if parser.getBool("verbose") {
        println("Verbose enabled");
    }
}
```

---

#### `getFloat`

Retrieves a parsed float value by key name.

**Syntax:**

```hoo
parser.getFloat(name: string) :float64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | `string` | The key name used in `addFloat`. |

**Returns:** `float64` — The parsed float value, or the default if not provided.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addFloat("threshold", "-t", "--threshold", "Threshold", 1.0);
    parser.parse();
    var t = parser.getFloat("threshold");
    println(t);
}
```

---

#### `has`

Checks whether a named argument was explicitly provided on the command line.

**Syntax:**

```hoo
parser.has(key: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `key` | `string` | The key or flag name to check. |

**Returns:** `int64` — 1 if the argument was provided, 0 otherwise.

**Complete Example:**

```hoo
import hoo.args;

func :void example() {
    var parser = new Args();
    parser.addFlag("verbose", "-v", "--verbose", "Verbose");
    parser.parse();
    if parser.has("verbose") {
        println("verbose was set");
    }
}
```

---

#### `value`

Retrieves the raw string value of a named argument as provided on the command line.

**Syntax:**

```hoo
parser.value(key: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `key` | `string` | The key or flag name. |

**Returns:** `string` — The raw value, or empty string if not found.

---

#### `programName`

Returns the program name as provided by the runtime.

**Syntax:**

```hoo
parser.programName() :string
```

**Parameters:** None.

**Returns:** `string` — The program name.

---

#### `helpText`

Generates a formatted help string describing all defined arguments.

**Syntax:**

```hoo
parser.helpText() :string
```

**Parameters:** None.

**Returns:** `string` — A formatted help text string.

---

#### `clear`

Clears all parsed values and argument definitions, allowing the parser to be reused.

**Syntax:**

```hoo
parser.clear() :void
```

**Parameters:** None.

**Returns:** `void`

---

## Usage Examples

### Basic positional access

```hoo
import hoo.args;

func :int64 main() {
    var argc = args_count();
    var i: int64 = 0;
    while i < argc {
        var arg = args_get(i);
        println(arg);
        i = i + 1;
    }
    return argc;
}
```

### Full argparse-style parsing

```hoo
import hoo.args;

func :void main() {
    var parser = new Args();
    parser.addString("output", "-o", "--output", "Output file path", "out.txt");
    parser.addFlag("verbose", "-v", "--verbose", "Enable verbose logging");
    parser.addInt("retries", "-r", "--retries", "Retry count", 3);
    parser.addFloat("threshold", "-t", "--threshold", "Error threshold", 0.01);
    parser.addPositional("input", "Input file");

    if parser.parse() {
        var input = parser.getString("input");
        var output = parser.getString("output");
        var verbose = parser.getBool("verbose");
        var retries = parser.getInt("retries");
        var threshold = parser.getFloat("threshold");

        println("Input: " + input);
        println("Output: " + output);
        if verbose {
            println("Retries: " + retries);
        }
    } else {
        var help = parser.helpText();
        println(help);
    }
}
```
