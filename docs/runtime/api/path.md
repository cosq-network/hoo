# Path API Reference

## Module Name

`Path` — `hoo.io` module

## Import Statement

```hoo
import hoo.io;
```

## Module Description

The `Path` class provides static utility methods for manipulating filesystem paths in a platform-independent manner. It handles path parsing, composition, normalization, and conversion between relative and absolute forms.

---

## Class: `Path`

### Declaration

```hoo
class Path
```

`Path` is a static utility class. It has no constructor and cannot be instantiated — all operations are performed via class (static) methods.

### Public Fields

None — `Path` is a purely static utility class.

### Public Class (Static) Functions

---

### `Path.separator`

**Description:** Returns the platform path separator character.

**Syntax:**
```hoo
Path.separator() :char
```

**Parameters:** None.

**Returns:** `char` — `'/'` on Unix-like systems, `'\\'` on Windows.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var sep = Path.separator();
    println(sep); // "/"
}
```

---

### `Path.join`

**Description:** Joins two path components into a single path using the platform separator.

**Syntax:**
```hoo
Path.join(path1: string, path2: string) :string
```

**Parameters:**
- `path1: string` — The first path component.
- `path2: string` — The second path component.

**Returns:** `string` — The joined path.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var full = Path.join("usr", "local");
    println(full); // "usr/local"
}
```

---

### `Path.extension`

**Description:** Returns the file extension including the leading dot (e.g., `".txt"`). Returns an empty string if the path has no extension.

**Syntax:**
```hoo
Path.extension(path: string) :string
```

**Parameters:**
- `path: string` — The file path to inspect.

**Returns:** `string` — The file extension including the dot, or an empty string.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var ext = Path.extension("archive.tar.gz");
    println(ext); // ".gz"

    var noExt = Path.extension("Makefile");
    println(noExt); // ""
}
```

---

### `Path.stem`

**Description:** Returns the filename without its extension. For `"archive.tar.gz"` returns `"archive.tar"`.

**Syntax:**
```hoo
Path.stem(path: string) :string
```

**Parameters:**
- `path: string` — The file path to inspect.

**Returns:** `string` — The filename without the extension.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var name = Path.stem("archive.tar.gz");
    println(name); // "archive.tar"
}
```

---

### `Path.filename`

**Description:** Returns the last component of the path (the filename). For `"/home/user/file.txt"` returns `"file.txt"`.

**Syntax:**
```hoo
Path.filename(path: string) :string
```

**Parameters:**
- `path: string` — The path to extract the filename from.

**Returns:** `string` — The last path component.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var name = Path.filename("/home/user/resume.pdf");
    println(name); // "resume.pdf"
}
```

---

### `Path.parent`

**Description:** Returns the parent directory path. For `"/home/user/docs/file.txt"` returns `"/home/user/docs"`.

**Syntax:**
```hoo
Path.parent(path: string) :string
```

**Parameters:**
- `path: string` — The path whose parent directory to extract.

**Returns:** `string` — The parent directory path.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var parent = Path.parent("/home/user/docs/file.txt");
    println(parent); // "/home/user/docs"
}
```

---

### `Path.absolute`

**Description:** Resolves a path to an absolute path by expanding relative paths against the current working directory.

**Syntax:**
```hoo
Path.absolute(path: string) :string
```

**Parameters:**
- `path: string` — The path to resolve.

**Returns:** `string` — The absolute path.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var abs = Path.absolute("doc/readme.md");
    println(abs); // "/current/working/dir/doc/readme.md"
}
```

---

### `Path.normalize`

**Description:** Normalizes a path by collapsing redundant separators and resolving `"."` and `".."` components.

**Syntax:**
```hoo
Path.normalize(path: string) :string
```

**Parameters:**
- `path: string` — The path to normalize.

**Returns:** `string` — The normalized path.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var norm = Path.normalize("/home/user/docs/../file.txt");
    println(norm); // "/home/user/file.txt"
}
```

---

### `Path.root`

**Description:** Returns the root component of a path. On Unix returns `"/"`; on Windows returns the drive root such as `"C:\\"`.

**Syntax:**
```hoo
Path.root(path: string) :string
```

**Parameters:**
- `path: string` — The path to extract the root from.

**Returns:** `string` — The root component, or an empty string if the path is relative.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var r = Path.root("/home/user/file.txt");
    println(r); // "/"
}
```

---

### `Path.relative`

**Description:** Returns a relative path from `base` to `path`.

**Syntax:**
```hoo
Path.relative(path: string, base: string) :string
```

**Parameters:**
- `path: string` — The target path.
- `base: string` — The base path to make relative from.

**Returns:** `string` — The relative path.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var rel = Path.relative("/home/user/docs/file.txt", "/home/user");
    println(rel); // "docs/file.txt"
}
```

---

### `Path.has_extension`

**Description:** Checks whether a path has a file extension.

**Syntax:**
```hoo
Path.has_extension(path: string) :int64
```

**Parameters:**
- `path: string` — The path to check.

**Returns:** `int64` — `1` if the path has an extension, `0` otherwise.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var has = Path.has_extension("data.txt");
    println(has); // 1

    var no = Path.has_extension("Makefile");
    println(no); // 0
}
```

---

### `Path.split`

**Description:** Splits a path into its component parts.

**Syntax:**
```hoo
Path.split(path: string) :array
```

**Parameters:**
- `path: string` — The path to split.

**Returns:** `array` — An array of path component strings.

**Errors:** None.

**Complete Example:**
```hoo
import hoo.io;

func :void example() {
    var parts = Path.split("/usr/local/bin");
    println(parts.length()); // 3
    // parts[0] == "usr", parts[1] == "local", parts[2] == "bin"
}
```

---

## Usage Example

```hoo
import hoo.io;

func :int64 main() {
    var p = "/home/user/docs/../file.txt";

    println(Path.filename(p));      // "file.txt"
    println(Path.parent(p));        // "/home/user/docs/.."
    println(Path.extension(p));     // ".txt"
    println(Path.stem(p));          // "file"
    println(Path.normalize(p));     // "/home/user/file.txt"
    println(Path.separator());      // "/"
    println(Path.has_extension(p)); // 1
    println(Path.root(p));          // "/"

    var joined = Path.join("usr", "local");
    println(joined);                // "usr/local"

    var abs = Path.absolute("doc/readme.md");
    println(abs);                   // /current/working/dir/doc/readme.md

    var parts = Path.split("/usr/local/bin");
    println(parts.length());        // 3

    return 0;
}
```
