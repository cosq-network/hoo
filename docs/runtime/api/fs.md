# Fs — Filesystem Operations API Reference

The `Fs` module provides filesystem inspection, text/binary I/O, directory
management, and system path operations. All calls use namespace-prefixed
free-function dispatch resolved at compile time via the codegen's `Fs` → `fs_`
mapping — there is no `Fs` instance, constructor, or static class methods.
The `fs_methodName(...)` syntax is a namespace prefix, not a static method call.

---

## `exists`

### Description

Checks whether a file system path exists.

### Syntax

```hoo
fs_exists(path: string) :int64
```

### Parameters

`path`
The file system path to check.

### Return Type

`int64`
Returns `1` if the path exists, `0` otherwise.

### Errors

Returns `0` if `path` is nil.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_exists("/tmp");
    println(ok); // 1
    return ok;
}
```

---

## `isFile`

### Description

Checks whether a path points to a regular file.

### Syntax

```hoo
fs_isFile(path: string) :int64
```

### Parameters

`path`
The file system path to check.

### Return Type

`int64`
Returns `1` if the path is a regular file, `0` otherwise.

### Errors

Returns `0` if `path` is nil.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_isFile("/tmp/data.txt");
    println(ok);
    return ok;
}
```

---

## `isDir`

### Description

Checks whether a path points to a directory.

### Syntax

```hoo
fs_isDir(path: string) :int64
```

### Parameters

`path`
The file system path to check.

### Return Type

`int64`
Returns `1` if the path is a directory, `0` otherwise.

### Errors

Returns `0` if `path` is nil.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_isDir("/tmp");
    println(ok); // 1
    return ok;
}
```

---

## `size`

### Description

Returns the size of a file in bytes.

### Syntax

```hoo
fs_size(path: string) :int64
```

### Parameters

`path`
The file system path to the file.

### Return Type

`int64`
The file size in bytes. Returns `-1` if the file does not exist or cannot be
read.

### Errors

Returns `-1` if `path` is nil or the file cannot be accessed.

### Complete Example

```hoo
func :int64 main() {
    var sz = fs_size("/tmp/data.txt");
    if sz >= 0 {
        println("size: " + sz);
    }
    return sz >= 0 ? 0 : 1;
}
```

---

## `lastModified`

### Description

Returns the last modified time of a file or directory as a Unix timestamp
(seconds since epoch).

### Syntax

```hoo
fs_lastModified(path: string) :int64
```

### Parameters

`path`
The file system path to query.

### Return Type

`int64`
A Unix timestamp. Returns `-1` on error.

### Errors

Returns `-1` if `path` is nil or the path cannot be accessed.

### Complete Example

```hoo
func :int64 main() {
    var ts = fs_lastModified("/tmp");
    if ts >= 0 {
        println("last modified: " + ts);
    }
    return ts >= 0 ? 0 : 1;
}
```

---

## `readText`

### Description

Reads the entire contents of a text file as a string.

### Syntax

```hoo
fs_readText(path: string) :string
```

### Parameters

`path`
The file system path to read from.

### Return Type

`string`
The file contents. Returns `0` if the file does not exist, cannot be read, or
is empty.

### Errors

Returns `0` if `path` is nil, the file does not exist, cannot be read, or
contains no data.

### Complete Example

```hoo
func :int64 main() {
    var content = fs_readText("/tmp/hello.txt");
    if content != 0 {
        println(content);
    }
    return content != 0 ? 0 : 1;
}
```

---

## `writeText`

### Description

Writes a string to a text file, overwriting any existing content. Creates the
file if it does not exist.

### Syntax

```hoo
fs_writeText(path: string, content: string) :int64
```

### Parameters

`path`
The file system path to write to.

`content`
The text to write.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil or the file cannot be written.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_writeText("/tmp/hello.txt", "Hello, world!");
    println(ok); // 1
    return ok;
}
```

---

## `appendText`

### Description

Appends a string to the end of a text file. Creates the file if it does not
exist.

### Syntax

```hoo
fs_appendText(path: string, content: string) :int64
```

### Parameters

`path`
The file system path to append to.

`content`
The text to append.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil or the file cannot be written.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_appendText("/tmp/log.txt", "new entry\n");
    println(ok); // 1
    return ok;
}
```

---

## `readBytes`

### Description

Reads the entire contents of a file into a Buffer.

### Syntax

```hoo
fs_readBytes(path: string) :buffer
```

### Parameters

`path`
The file system path to read from.

### Return Type

`buffer`
A Buffer containing the file bytes. Returns `0` if the file does not exist,
cannot be read, or is empty.

### Errors

Returns `0` if `path` is nil, the file does not exist, cannot be read, or
contains no data.

### Complete Example

```hoo
func :int64 main() {
    var buf = fs_readBytes("/tmp/data.bin");
    if buf != 0 {
        println(buf.length());
    }
    return buf != 0 ? 0 : 1;
}
```

---

## `writeBytes`

### Description

Writes the contents of a Buffer to a file, overwriting any existing content.
Creates the file if it does not exist.

### Syntax

```hoo
fs_writeBytes(path: string, buf: buffer) :int64
```

### Parameters

`path`
The file system path to write to.

`buf`
The Buffer containing the bytes to write.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil or the file cannot be written.

### Complete Example

```hoo
func :int64 main() {
    var buf = new Buffer();
    buf.append("binary data");
    var ok = fs_writeBytes("/tmp/data.bin", buf);
    buf.release();
    println(ok); // 1
    return ok;
}
```

---

## `delete`

### Description

Deletes a file. Does not delete directories (use `rmdir` instead).

### Syntax

```hoo
fs_delete(path: string) :int64
```

### Parameters

`path`
The file system path to delete.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil or the file cannot be deleted.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_delete("/tmp/temp.txt");
    println(ok);
    return ok;
}
```

---

## `rename`

### Description

Renames or moves a file or directory.

### Syntax

```hoo
fs_rename(oldPath: string, newPath: string) :int64
```

### Parameters

`oldPath`
The current file system path.

`newPath`
The new file system path.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if either path is nil or the operation fails.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_rename("/tmp/old.txt", "/tmp/new.txt");
    println(ok);
    return ok;
}
```

---

## `copy`

### Description

Copies a file from a source path to a destination path. Overwrites the
destination if it already exists.

### Syntax

```hoo
fs_copy(src: string, dst: string) :int64
```

### Parameters

`src`
The source file path.

`dst`
The destination file path.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if either path is nil or the copy fails.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_copy("/tmp/source.txt", "/tmp/dest.txt");
    println(ok);
    return ok;
}
```

---

## `mkdir`

### Description

Creates a single directory. The parent directory must already exist.

### Syntax

```hoo
fs_mkdir(path: string) :int64
```

### Parameters

`path`
The file system path of the directory to create.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil, the parent does not exist, or the directory
cannot be created.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_mkdir("/tmp/newdir");
    println(ok); // 1
    return ok;
}
```

---

## `mkdirs`

### Description

Creates a directory and all missing parent directories (like `mkdir -p`).
Does not fail if the directory already exists.

### Syntax

```hoo
fs_mkdirs(path: string) :int64
```

### Parameters

`path`
The file system path of the directory tree to create.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil or the directory tree cannot be created.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_mkdirs("/tmp/a/b/c");
    println(ok); // 1
    return ok;
}
```

---

## `rmdir`

### Description

Removes an empty directory.

### Syntax

```hoo
fs_rmdir(path: string) :int64
```

### Parameters

`path`
The file system path of the directory to remove.

### Return Type

`int64`
Returns `1` on success, `0` on failure.

### Errors

Returns `0` if `path` is nil, the directory is not empty, or the directory
cannot be removed.

### Complete Example

```hoo
func :int64 main() {
    var ok = fs_rmdir("/tmp/emptydir");
    println(ok);
    return ok;
}
```

---

## `listDir`

### Description

Returns an array of filenames in a directory. The entries are the base names
of files and subdirectories (not full paths).

### Syntax

```hoo
fs_listDir(path: string) :array
```

### Parameters

`path`
The file system path of the directory to list.

### Return Type

`array`
An array of strings containing the entry names. Returns `0` if the directory
does not exist, is empty, or cannot be read.

### Errors

Returns `0` if `path` is nil, the directory does not exist, is empty, or
cannot be read.

### Complete Example

```hoo
func :int64 main() {
    var entries = fs_listDir("/tmp");
    if entries != 0 {
        println(entries.length());
    }
    return entries != 0 ? 0 : 1;
}
```

---

## `tempDir`

### Description

Returns the system's temporary directory path.

### Syntax

```hoo
fs_tempDir() :string
```

### Parameters

None.

### Return Type

`string`
The path to the system temporary directory. Returns `0` on error.

### Errors

Returns `0` if the temporary directory cannot be determined.

### Complete Example

```hoo
func :int64 main() {
    var tmp = fs_tempDir();
    if tmp != 0 {
        println(tmp);
    }
    return tmp != 0 ? 0 : 1;
}
```

---

## `createTempFile`

### Description

Creates a temporary file with the given prefix in the system's temporary
directory. The file is created empty and its path is returned.

### Syntax

```hoo
fs_createTempFile(prefix: string) :string
```

### Parameters

`prefix`
A string prefix for the temporary file name.

### Return Type

`string`
The full path to the newly created temporary file. Returns `0` on failure.

### Errors

Returns `0` if `prefix` is nil or the file cannot be created.

### Complete Example

```hoo
func :int64 main() {
    var tmp = fs_createTempFile("hoo_");
    if tmp != 0 {
        println(tmp);
        fs_delete(tmp);
    }
    return tmp != 0 ? 0 : 1;
}
```

---

# C++ API — Class Reference

The `hoo::fs` namespace provides three object-oriented classes (`Path`, `File`,
`Directory`) for filesystem operations. Each has a single explicit constructor
with no overloads. These classes live in the C++ layer; Hoo code accesses the
equivalent operations through the free functions documented above, which
delegate to these classes via the C-ABI bridge.

---

## `hoo::fs::Path`

### Description

Parses, inspects, and manipulates filesystem paths. All extraction methods
return the relevant component as a `std::string`. Mutating methods
(`normalized`, `absolute`) return a new `Path` object.

### Constructor

```cpp
explicit Path(const std::string& path);
```

### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `str` | `() -> string` | The original path string. |
| `dirname` | `() -> string` | Parent directory path. |
| `basename` | `() -> string` | File name with extension. |
| `extension` | `() -> string` | Extension including the dot, or empty. |
| `stem` | `() -> string` | File name without extension. |
| `root` | `() -> string` | Root component (e.g. `/`, `C:\`). |
| `normalized` | `() -> Path` | Path with `.`/`..` resolved. |
| `absolute` | `() -> Path` | Resolved to absolute path. |
| `isAbsolute` | `() -> bool` | Whether the path is absolute. |
| `isRelative` | `() -> bool` | Whether the path is relative. |
| `hasExtension` | `() -> bool` | Whether the path has an extension. |
| `hasRoot` | `() -> bool` | Whether the path has a root component. |
| `split` | `() -> vector<string>` | Path split into individual components. |

### Errors

- Constructor does not validate the path; validation happens on I/O.
- Extraction methods return empty strings for missing components.
- `normalized` / `absolute` throw `std::filesystem::filesystem_error` on
  resolution failure.

### Complete Example

```cpp
#include "hoo_fs.h"
#include <iostream>

int main() {
    hoo::fs::Path p("/home/user/docs/file.txt");

    std::cout << "dirname:    " << p.dirname()    << "\n"; // /home/user/docs
    std::cout << "basename:   " << p.basename()   << "\n"; // file.txt
    std::cout << "extension:  " << p.extension()  << "\n"; // .txt
    std::cout << "stem:       " << p.stem()       << "\n"; // file
    std::cout << "root:       " << p.root()       << "\n"; // /
    std::cout << "isAbsolute: " << p.isAbsolute() << "\n"; // 1

    hoo::fs::Path abs = p.absolute();
    std::cout << "absolute:   " << abs.str() << "\n";

    return 0;
}
```

---

## `hoo::fs::File`

### Description

Represents a file on the filesystem. Provides metadata queries and I/O
operations. The class does **not** open a file handle; it operates on the path
at each method call.

### Constructor

```cpp
explicit File(const std::string& path);
```

### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `path` | `() -> string` | The path used to construct this instance. |
| `exists` | `() -> bool` | Whether the path exists. |
| `isFile` | `() -> bool` | Whether the path is a regular file. |
| `size` | `() -> int64_t` | File size in bytes, `-1` on error. |
| `lastModified` | `() -> int64_t` | Unix timestamp (seconds since epoch), `-1` on error. |
| `remove` | `() -> bool` | Delete the file. |
| `rename` | `(newPath: string) -> bool` | Rename / move; updates stored path on success. |
| `readText` | `() -> string` | Read entire file as text; empty on error. |
| `writeText` | `(content: string) -> bool` | Write string to file, overwriting. |
| `appendText` | `(content: string) -> bool` | Append string to file; creates if missing. |
| `readBytes` | `(out vector<u8>&) -> bool` | Read binary file into `vector`. |
| `writeBytes` | `(vector<u8>&) -> bool` | Write bytes to file, overwriting. |

### Errors

- Constructor does not validate the path.
- `exists`, `isFile`, `size`, `lastModified` return `false` / `-1` if the
  path does not exist or is inaccessible.
- `remove`, `rename`, `writeText`, `appendText`, `writeBytes` return `false`
  on failure (permissions, read-only filesystem, etc.).
- `readText` returns an empty string if the file does not exist or cannot be
  read.
- `readBytes` returns `false` and an empty vector on failure.

### Complete Example

```cpp
#include "hoo_fs.h"
#include <iostream>

int main() {
    hoo::fs::File f("/tmp/example.txt");

    if (f.writeText("Hello, world!")) {
        std::cout << "wrote " << f.size() << " bytes\n";
    }

    std::string content = f.readText();
    std::cout << "read: " << content << "\n";

    if (f.remove()) {
        std::cout << "removed\n";
    }

    return 0;
}
```

---

## `hoo::fs::Directory`

### Description

Represents a directory on the filesystem. Provides creation, removal, and
listing operations.

### Constructor

```cpp
explicit Directory(const std::string& path);
```

### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `path` | `() -> string` | The path used to construct this instance. |
| `exists` | `() -> bool` | Whether the directory path exists. |
| `isDirectory` | `() -> bool` | Whether the path is a directory. |
| `create` | `() -> bool` | Create a single directory (parent must exist). |
| `createTree` | `() -> bool` | Create directory tree (`mkdir -p`). |
| `remove` | `() -> bool` | Remove an empty directory. |
| `list` | `() -> vector<string>` | List entry names (filenames only, not full paths). |

### Errors

- Constructor does not validate the path.
- `exists`, `isDirectory` return `false` if the path does not exist or is
  inaccessible.
- `create` returns `false` if the parent does not exist.
- `createTree` returns `false` if the tree cannot be created.
- `remove` returns `false` if the directory is not empty.
- `list` returns an empty vector if the directory does not exist or cannot be
  read.

### Complete Example

```cpp
#include "hoo_fs.h"
#include <iostream>

int main() {
    hoo::fs::Directory d("/tmp/myapp/data");

    if (d.createTree()) {
        std::cout << "created tree\n";

        auto entries = d.list();
        std::cout << entries.size() << " entries\n";

        d.remove();
    }

    return 0;
}
```

---

## Free Functions (`hoo::fs`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `join` | `(a: string, b: string) -> string` | Concatenate two path components. |
| `joinMulti` | `(parts: vector<string>&) -> string` | Join multiple path components. |
| `relative` | `(path: string, base: string) -> string` | Compute relative path. |
| `separator` | `() -> char` | System path separator (`/` or `\`). |
| `listSeparator` | `() -> char` | System path list separator (`:` or `;`). |
| `tempDir` | `() -> string` | System temporary directory path. |
| `createTempFile` | `(prefix: string) -> string` | Create a temporary file, returns path. |
| `copyFile` | `(src: string, dst: string) -> bool` | Copy a file. |

