# Path API Reference

## Module Name

Part of the `hoo.path` module.

## Import Statement

```hoo
import hoo.path;
```

## Module Description

The path module provides utility functions for manipulating filesystem paths in a platform-independent manner. It handles path parsing, composition, normalization, and conversion between relative and absolute forms.

## Functions

### `path_separator`

Returns the platform path separator character.

**Syntax:**

```hoo
path_separator() :char
```

**Parameters:** None.

**Returns:** `char` — `'/'` on Unix-like systems, `'\\'` on Windows.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var sep = path_separator();
    println(sep); // "/"
}
```

---

### `path_join`

Joins two path components into a single path using the platform separator.

**Syntax:**

```hoo
path_join(path1: string, path2: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path1` | `string` | The first path component. |
| `path2` | `string` | The second path component. |

**Returns:** `string` — The joined path.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var full = path_join("usr", "local");
    println(full); // "usr/local"
}
```

---

### `path_extension`

Returns the file extension including the leading dot (e.g., `".txt"`). Returns an empty string if the path has no extension.

**Syntax:**

```hoo
path_extension(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file path to inspect. |

**Returns:** `string` — The file extension including the dot, or an empty string.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var ext = path_extension("archive.tar.gz");
    println(ext); // ".gz"

    var noExt = path_extension("Makefile");
    println(noExt); // ""
}
```

---

### `path_stem`

Returns the filename without its extension. For `"archive.tar.gz"` returns `"archive.tar"`.

**Syntax:**

```hoo
path_stem(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file path to inspect. |

**Returns:** `string` — The filename without the extension.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var name = path_stem("archive.tar.gz");
    println(name); // "archive.tar"
}
```

---

### `path_filename`

Returns the last component of the path (the filename). For `"/home/user/file.txt"` returns `"file.txt"`.

**Syntax:**

```hoo
path_filename(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to extract the filename from. |

**Returns:** `string` — The last path component.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var name = path_filename("/home/user/resume.pdf");
    println(name); // "resume.pdf"
}
```

---

### `path_parent`

Returns the parent directory path. For `"/home/user/docs/file.txt"` returns `"/home/user/docs"`.

**Syntax:**

```hoo
path_parent(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path whose parent directory to extract. |

**Returns:** `string` — The parent directory path.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var parent = path_parent("/home/user/docs/file.txt");
    println(parent); // "/home/user/docs"
}
```

---

### `path_absolute`

Resolves a path to an absolute path by expanding relative paths against the current working directory.

**Syntax:**

```hoo
path_absolute(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to resolve. |

**Returns:** `string` — The absolute path.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var abs = path_absolute("doc/readme.md");
    println(abs); // "/current/working/dir/doc/readme.md"
}
```

---

### `path_normalize`

Normalizes a path by collapsing redundant separators and resolving `"."` and `".."` components.

**Syntax:**

```hoo
path_normalize(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to normalize. |

**Returns:** `string` — The normalized path.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var norm = path_normalize("/home/user/docs/../file.txt");
    println(norm); // "/home/user/file.txt"
}
```

---

### `path_root`

Returns the root component of a path. On Unix returns `"/"`; on Windows returns the drive root such as `"C:\\"`.

**Syntax:**

```hoo
path_root(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to extract the root from. |

**Returns:** `string` — The root component, or an empty string if the path is relative.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var r = path_root("/home/user/file.txt");
    println(r); // "/"
}
```

---

### `path_relative`

Returns a relative path from `base` to `path`.

**Syntax:**

```hoo
path_relative(path: string, base: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The target path. |
| `base` | `string` | The base path to make relative from. |

**Returns:** `string` — The relative path.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var rel = path_relative("/home/user/docs/file.txt", "/home/user");
    println(rel); // "docs/file.txt"
}
```

---

### `path_has_extension`

Checks whether a path has a file extension.

**Syntax:**

```hoo
path_has_extension(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to check. |

**Returns:** `int64` — `1` if the path has an extension, `0` otherwise.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var has = path_has_extension("data.txt");
    println(has); // 1

    var no = path_has_extension("Makefile");
    println(no); // 0
}
```

---

### `path_split`

Splits a path into its component parts.

**Syntax:**

```hoo
path_split(path: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The path to split. |

**Returns:** `array` — An array of path component strings.

**Errors:** None.

**Complete Example:**

```hoo
import hoo.path;

func :void example() {
    var parts = path_split("/usr/local/bin");
    println(parts.length()); // 3
    // parts[0] == "usr", parts[1] == "local", parts[2] == "bin"
}
```

## Usage Example

```hoo
import hoo.path;

func :int64 main() {
    var p = "/home/user/docs/../file.txt";

    println(path_filename(p));      // "file.txt"
    println(path_parent(p));        // "/home/user/docs/.."
    println(path_extension(p));     // ".txt"
    println(path_stem(p));          // "file"
    println(path_normalize(p));     // "/home/user/file.txt"
    println(path_separator());      // "/"
    println(path_has_extension(p)); // 1
    println(path_root(p));          // "/"

    var joined = path_join("usr", "local");
    println(joined);                // "usr/local"

    var abs = path_absolute("doc/readme.md");
    println(abs);                   // /current/working/dir/doc/readme.md

    var parts = path_split("/usr/local/bin");
    println(parts.length());        // 3

    return 0;
}
```
