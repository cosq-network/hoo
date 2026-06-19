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

# C++ API — Class and Function Reference

The `hoo::fs` namespace provides three object-oriented classes (`Path`, `File`,
`Directory`) and several free functions for filesystem operations. Each class
has a single explicit constructor with no overloads.

These classes live in the C++ layer. Hoo code accesses the equivalent
operations through the free functions documented above, which delegate to these
classes via the C-ABI bridge.

---

## `hoo::fs::Path::Path`

### Description

Constructs a `Path` object from a path string. The constructor does not
validate the path; validation and resolution happen on I/O or method calls.

### Syntax

```cpp
Path(const std::string& path);
```

### Parameters

`path`
A filesystem path string.

### Return Type

`Path`
A new `Path` instance.

### Errors

The constructor does not validate the path. Resolution methods
(`absolute`, `normalized`) throw `std::filesystem::filesystem_error` on
failure.

### Complete Example

```cpp
#include "hoo_fs.h"
hoo::fs::Path p("/home/user/docs/file.txt");
```

---

## `hoo::fs::Path::str`

### Description

Returns the original path string used to construct this `Path` instance.

### Syntax

```cpp
p.str() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The original path string.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Path p("/tmp");
std::string s = p.str(); // "/tmp"
```

---

## `hoo::fs::Path::dirname`

### Description

Returns the parent directory path. Equivalent to Python's
`os.path.dirname()`.

### Syntax

```cpp
p.dirname() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The parent directory path. Returns an empty string if there is no parent.

### Errors

Returns an empty string for paths without a parent (e.g. `"file.txt"`).

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
std::string d = p.dirname(); // "/home/user/docs"
```

---

## `hoo::fs::Path::basename`

### Description

Returns the file name with extension. Equivalent to Python's
`os.path.basename()`.

### Syntax

```cpp
p.basename() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The file name including extension.

### Errors

Returns an empty string for root paths.

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
std::string b = p.basename(); // "file.txt"
```

---

## `hoo::fs::Path::extension`

### Description

Returns the file extension including the leading dot. Equivalent to
Python's `os.path.splitext()[1]`.

### Syntax

```cpp
p.extension() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The extension including the dot (e.g. `".txt"`). Returns an empty string if
the path has no extension.

### Errors

Returns an empty string for paths without an extension or paths ending in a
dot.

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
std::string e = p.extension(); // ".txt"
```

---

## `hoo::fs::Path::stem`

### Description

Returns the file name without the extension.

### Syntax

```cpp
p.stem() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The file name without extension.

### Errors

Returns an empty string for paths with no filename.

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
std::string s = p.stem(); // "file"
```

---

## `hoo::fs::Path::root`

### Description

Returns the root component of the path (e.g. `"/"` on POSIX,
`"C:\\"` on Windows).

### Syntax

```cpp
p.root() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The root component. Returns an empty string for relative paths.

### Errors

Returns an empty string for relative paths.

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
std::string r = p.root(); // "/"
```

---

## `hoo::fs::Path::normalized`

### Description

Resolves `.` and `..` components without accessing the filesystem. Returns a
new `Path`.

### Syntax

```cpp
p.normalized() -> Path
```

### Parameters

None.

### Return Type

`Path`
A new `Path` with `.` and `..` resolved.

### Errors

Throws `std::filesystem::filesystem_error` on resolution failure.

### Complete Example

```cpp
hoo::fs::Path p("/home/../tmp/./file.txt");
hoo::fs::Path n = p.normalized(); // "/tmp/file.txt"
```

---

## `hoo::fs::Path::absolute`

### Description

Resolves the path to an absolute path relative to the current working
directory. Requires the filesystem to be accessible. Returns a new `Path`.

### Syntax

```cpp
p.absolute() -> Path
```

### Parameters

None.

### Return Type

`Path`
A new `Path` containing the absolute path.

### Errors

Throws `std::filesystem::filesystem_error` if the current working directory
cannot be determined.

### Complete Example

```cpp
hoo::fs::Path p("docs/file.txt");
hoo::fs::Path a = p.absolute(); // e.g. "/home/user/proj/docs/file.txt"
```

---

## `hoo::fs::Path::isAbsolute`

### Description

Checks whether the path is an absolute path.

### Syntax

```cpp
p.isAbsolute() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path is absolute, `false` otherwise.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Path p1("/tmp");
bool a1 = p1.isAbsolute(); // true

hoo::fs::Path p2("relative/path");
bool a2 = p2.isAbsolute(); // false
```

---

## `hoo::fs::Path::isRelative`

### Description

Checks whether the path is a relative path.

### Syntax

```cpp
p.isRelative() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path is relative, `false` otherwise.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Path p("relative/path");
bool r = p.isRelative(); // true
```

---

## `hoo::fs::Path::hasExtension`

### Description

Checks whether the path has a file extension.

### Syntax

```cpp
p.hasExtension() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path has an extension, `false` otherwise.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Path p1("file.txt");
bool e1 = p1.hasExtension(); // true

hoo::fs::Path p2("file");
bool e2 = p2.hasExtension(); // false
```

---

## `hoo::fs::Path::hasRoot`

### Description

Checks whether the path has a root component.

### Syntax

```cpp
p.hasRoot() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path has a root component, `false` otherwise.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Path p1("/tmp");
bool r1 = p1.hasRoot(); // true

hoo::fs::Path p2("tmp");
bool r2 = p2.hasRoot(); // false
```

---

## `hoo::fs::Path::split`

### Description

Splits the path into individual components.

### Syntax

```cpp
p.split() -> std::vector<std::string>
```

### Parameters

None.

### Return Type

`std::vector<std::string>`
A vector of path components.

### Errors

Returns an empty vector for an empty path.

### Complete Example

```cpp
hoo::fs::Path p("/home/user/docs/file.txt");
auto parts = p.split();
// parts = {"home", "user", "docs", "file.txt"}
```

---

## `hoo::fs::File::File`

### Description

Constructs a `File` object from a path string. The class does **not** open
a file handle; it operates on the path at each method call.

### Syntax

```cpp
File(const std::string& path);
```

### Parameters

`path`
The filesystem path to the file.

### Return Type

`File`
A new `File` instance.

### Errors

The constructor does not validate the path.

### Complete Example

```cpp
#include "hoo_fs.h"
hoo::fs::File f("/tmp/example.txt");
```

---

## `hoo::fs::File::path`

### Description

Returns the path used to construct this `File` instance.

### Syntax

```cpp
f.path() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The path string.

### Errors

None.

### Complete Example

```cpp
hoo::fs::File f("/tmp/example.txt");
std::string p = f.path(); // "/tmp/example.txt"
```

---

## `hoo::fs::File::exists`

### Description

Checks whether the file path exists on the filesystem.

### Syntax

```cpp
f.exists() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path exists, `false` otherwise.

### Errors

Returns `false` if the path is inaccessible.

### Complete Example

```cpp
hoo::fs::File f("/tmp/example.txt");
if (f.exists()) {
    // file exists
}
```

---

## `hoo::fs::File::isFile`

### Description

Checks whether the path points to a regular file.

### Syntax

```cpp
f.isFile() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path is a regular file, `false` otherwise.

### Errors

Returns `false` if the path does not exist or is inaccessible.

### Complete Example

```cpp
hoo::fs::File f("/tmp/example.txt");
bool ok = f.isFile();
```

---

## `hoo::fs::File::size`

### Description

Returns the size of the file in bytes.

### Syntax

```cpp
f.size() -> int64_t
```

### Parameters

None.

### Return Type

`int64_t`
The file size in bytes. Returns `-1` on error.

### Errors

Returns `-1` if the file does not exist or cannot be read.

### Complete Example

```cpp
hoo::fs::File f("/tmp/example.txt");
int64_t sz = f.size();
if (sz >= 0) {
    // use sz
}
```

---

## `hoo::fs::File::lastModified`

### Description

Returns the last modified time of the file as a Unix timestamp (seconds
since epoch).

### Syntax

```cpp
f.lastModified() -> int64_t
```

### Parameters

None.

### Return Type

`int64_t`
A Unix timestamp. Returns `-1` on error.

### Errors

Returns `-1` if the file does not exist or cannot be accessed.

### Complete Example

```cpp
hoo::fs::File f("/tmp/example.txt");
int64_t ts = f.lastModified();
```

---

## `hoo::fs::File::remove`

### Description

Deletes the file from the filesystem.

### Syntax

```cpp
f.remove() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the file does not exist or cannot be deleted.

### Complete Example

```cpp
hoo::fs::File f("/tmp/temp.txt");
if (f.remove()) {
    // file deleted
}
```

---

## `hoo::fs::File::rename`

### Description

Renames or moves the file. Updates the stored path on success.

### Syntax

```cpp
f.rename(const std::string& newPath) -> bool
```

### Parameters

`newPath`
The new filesystem path.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the source does not exist or the destination cannot be
created.

### Complete Example

```cpp
hoo::fs::File f("/tmp/old.txt");
if (f.rename("/tmp/new.txt")) {
    // f.path() is now "/tmp/new.txt"
}
```

---

## `hoo::fs::File::readText`

### Description

Reads the entire contents of the file as a string.

### Syntax

```cpp
f.readText() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The file contents. Returns an empty string on error.

### Errors

Returns an empty string if the file does not exist or cannot be read.

### Complete Example

```cpp
hoo::fs::File f("/tmp/hello.txt");
std::string content = f.readText();
```

---

## `hoo::fs::File::writeText`

### Description

Writes a string to the file, overwriting any existing content. Creates the
file if it does not exist.

### Syntax

```cpp
f.writeText(const std::string& content) -> bool
```

### Parameters

`content`
The text to write.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the file cannot be written.

### Complete Example

```cpp
hoo::fs::File f("/tmp/hello.txt");
if (f.writeText("Hello, world!")) {
    // file written
}
```

---

## `hoo::fs::File::appendText`

### Description

Appends a string to the end of the file. Creates the file if it does not
exist.

### Syntax

```cpp
f.appendText(const std::string& content) -> bool
```

### Parameters

`content`
The text to append.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the file cannot be written.

### Complete Example

```cpp
hoo::fs::File f("/tmp/log.txt");
f.appendText("new entry\n");
```

---

## `hoo::fs::File::readBytes`

### Description

Reads the entire file into a byte vector.

### Syntax

```cpp
f.readBytes(std::vector<uint8_t>& outData) -> bool
```

### Parameters

`outData`
Output vector that receives the file bytes.

### Return Type

`bool`
Returns `true` on success, `false` on failure. The vector is empty on
failure.

### Errors

Returns `false` if the file does not exist or cannot be read.

### Complete Example

```cpp
hoo::fs::File f("/tmp/data.bin");
std::vector<uint8_t> data;
if (f.readBytes(data)) {
    // data contains the file bytes
}
```

---

## `hoo::fs::File::writeBytes`

### Description

Writes raw bytes to the file, overwriting any existing content.

### Syntax

```cpp
f.writeBytes(const std::vector<uint8_t>& data) -> bool
```

### Parameters

`data`
The bytes to write.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the file cannot be written.

### Complete Example

```cpp
hoo::fs::File f("/tmp/data.bin");
std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
f.writeBytes(data);
```

---

## `hoo::fs::Directory::Directory`

### Description

Constructs a `Directory` object from a path string.

### Syntax

```cpp
Directory(const std::string& path);
```

### Parameters

`path`
The filesystem path to the directory.

### Return Type

`Directory`
A new `Directory` instance.

### Errors

The constructor does not validate the path.

### Complete Example

```cpp
#include "hoo_fs.h"
hoo::fs::Directory d("/tmp/mydir");
```

---

## `hoo::fs::Directory::path`

### Description

Returns the path used to construct this `Directory` instance.

### Syntax

```cpp
d.path() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The path string.

### Errors

None.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/mydir");
std::string p = d.path(); // "/tmp/mydir"
```

---

## `hoo::fs::Directory::exists`

### Description

Checks whether the directory path exists on the filesystem.

### Syntax

```cpp
d.exists() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path exists, `false` otherwise.

### Errors

Returns `false` if the path is inaccessible.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/mydir");
if (d.exists()) {
    // directory exists
}
```

---

## `hoo::fs::Directory::isDirectory`

### Description

Checks whether the path points to a directory.

### Syntax

```cpp
d.isDirectory() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` if the path is a directory, `false` otherwise.

### Errors

Returns `false` if the path does not exist or is inaccessible.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/mydir");
bool ok = d.isDirectory();
```

---

## `hoo::fs::Directory::create`

### Description

Creates a single directory. The parent directory must already exist.

### Syntax

```cpp
d.create() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the parent does not exist or the directory cannot be
created.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/mydir");
if (d.create()) {
    // directory created
}
```

---

## `hoo::fs::Directory::createTree`

### Description

Creates the directory and all missing parent directories (like `mkdir -p`).
Does not fail if the directory already exists.

### Syntax

```cpp
d.createTree() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the directory tree cannot be created.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/a/b/c");
if (d.createTree()) {
    // whole tree created
}
```

---

## `hoo::fs::Directory::remove`

### Description

Removes an empty directory.

### Syntax

```cpp
d.remove() -> bool
```

### Parameters

None.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the directory is not empty or cannot be removed.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp/emptydir");
if (d.remove()) {
    // directory removed
}
```

---

## `hoo::fs::Directory::list`

### Description

Returns the names of entries inside the directory. The entries are base
names only (not full paths).

### Syntax

```cpp
d.list() -> std::vector<std::string>
```

### Parameters

None.

### Return Type

`std::vector<std::string>`
A vector of entry names.

### Errors

Returns an empty vector if the directory does not exist or cannot be read.

### Complete Example

```cpp
hoo::fs::Directory d("/tmp");
auto entries = d.list();
for (const auto& name : entries) {
    // process name
}
```

---

## Free Functions (`hoo::fs`)

### `join`

### Description

Concatenates two path components using the system path separator.

### Syntax

```cpp
join(const std::string& a, const std::string& b) -> std::string
```

### Parameters

`a`
First path component.

`b`
Second path component.

### Return Type

`std::string`
The joined path.

### Errors

None.

### Complete Example

```cpp
std::string p = hoo::fs::join("a", "b"); // "a/b"
```

---

### `joinMulti`

### Description

Joins multiple path components using the system path separator.

### Syntax

```cpp
joinMulti(const std::vector<std::string>& parts) -> std::string
```

### Parameters

`parts`
A vector of path components.

### Return Type

`std::string`
The joined path.

### Errors

Returns an empty string if `parts` is empty.

### Complete Example

```cpp
std::vector<std::string> parts = {"a", "b", "c"};
std::string p = hoo::fs::joinMulti(parts); // "a/b/c"
```

---

### `relative`

### Description

Computes the relative path from `base` to `path`.

### Syntax

```cpp
relative(const std::string& path, const std::string& base) -> std::string
```

### Parameters

`path`
The target path.

`base`
The base path.

### Return Type

`std::string`
The relative path.

### Errors

Returns an empty string if no relative path can be computed.

### Complete Example

```cpp
std::string r = hoo::fs::relative("/a/b/c", "/a/b"); // "c"
```

---

### `separator`

### Description

Returns the system path separator character (`/` on POSIX, `\\` on
Windows).

### Syntax

```cpp
separator() -> char
```

### Parameters

None.

### Return Type

`char`
The path separator.

### Errors

None.

### Complete Example

```cpp
char sep = hoo::fs::separator(); // '/'
```

---

### `listSeparator`

### Description

Returns the system path list separator character (`:` on POSIX, `;` on
Windows).

### Syntax

```cpp
listSeparator() -> char
```

### Parameters

None.

### Return Type

`char`
The path list separator.

### Errors

None.

### Complete Example

```cpp
char sep = hoo::fs::listSeparator(); // ':'
```

---

### `tempDir`

### Description

Returns the system temporary directory path.

### Syntax

```cpp
tempDir() -> std::string
```

### Parameters

None.

### Return Type

`std::string`
The temporary directory path.

### Errors

Returns an empty string if the temp directory cannot be determined.

### Complete Example

```cpp
std::string tmp = hoo::fs::tempDir(); // e.g. "/tmp"
```

---

### `createTempFile`

### Description

Creates a temporary file with the given prefix in the system temporary
directory. The file is created empty and its path is returned.

### Syntax

```cpp
createTempFile(const std::string& prefix) -> std::string
```

### Parameters

`prefix`
A string prefix for the temporary file name.

### Return Type

`std::string`
The full path to the newly created temporary file.

### Errors

Returns an empty string if the file cannot be created.

### Complete Example

```cpp
std::string path = hoo::fs::createTempFile("hoo_");
// use path, then remove the file manually
```

---

### `copyFile`

### Description

Copies a file from source to destination. Overwrites the destination if it
exists.

### Syntax

```cpp
copyFile(const std::string& src, const std::string& dst) -> bool
```

### Parameters

`src`
Source file path.

`dst`
Destination file path.

### Return Type

`bool`
Returns `true` on success, `false` on failure.

### Errors

Returns `false` if the source does not exist or the destination cannot be
written.

### Complete Example

```cpp
bool ok = hoo::fs::copyFile("/tmp/src.txt", "/tmp/dst.txt");
```
