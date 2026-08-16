# Fs API Reference

## Module Name

Part of the `hoo.io` module.

## Import Statement

```hoo
import hoo.io;
```

## Module Description

The fs module provides filesystem operations including file reading and writing, directory management, path queries, and file metadata inspection. All functions are free functions in the `hoo.io` module.

## Return Value Conventions

- Functions that mutate the filesystem (`fs_write_text`, `fs_delete`, `fs_mkdir`, `fs_move`, ...) return `int64` `1` on success and `0` on failure.
- Functions that query the filesystem (`fs_exists`, `fs_is_dir`, ...) return `int64` `1`/`0` as a boolean.
- Functions that return data (`fs_read_text`, `fs_read_bytes`, ...) return the value or `0` (null) when the operation cannot be performed. `fs_read_text` returns an empty string (`""`) for a file that exists but is empty.

## Functions

### `fs_read_text`

Reads the entire contents of a text file as a string.

**Syntax:**

```hoo
fs_read_text(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to read from. |

**Returns:**

`string` — The file contents. Returns an empty string (`""`) if the file exists but is empty, and `0` (null) if the file does not exist or cannot be read.

**Errors:**

Returns `0` if `path` is nil, the file does not exist, or the file cannot be read.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var content = fs_read_text("/tmp/hello.txt");
    if content != 0 {
        println(content);
    }
    return content != 0 ? 0 : 1;
}
```

---

### `fs_read_text` (with fallback)

Reads a text file, returning a caller-supplied fallback string when the file does not exist.

**Syntax:**

```hoo
fs_read_text(path: string, fallback: string) :string
```

**Returns:**

`string` — The file contents, or `fallback` if the file does not exist or cannot be read. An existing-but-empty file still returns `""`.

---

### `fs_write_text`

Writes a string to a text file, overwriting any existing content. Creates the file if it does not exist.

**Syntax:**

```hoo
fs_write_text(path: string, data: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to write to. |
| `data` | `string` | The text to write. |

**Returns:**

`int64` — `1` on success, `0` on failure.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    if (fs_write_text("/tmp/hello.txt", "Hello, world!") == 0) {
        return 0;
    }
    return 1;
}
```

---

### `fs_append_text`

Appends a string to the end of a text file. Creates the file if it does not exist.

**Syntax:**

```hoo
fs_append_text(path: string, data: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to append to. |
| `data` | `string` | The text to append. |

**Returns:**

`int64` — `1` on success, `0` on failure.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    if (fs_append_text("/tmp/log.txt", "new entry\n") == 0) {
        return 0;
    }
    return 1;
}
```

---

### `fs_exists`

Checks whether a file system path exists.

**Syntax:**

```hoo
fs_exists(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to check. |

**Returns:**

`int64` — Returns `1` if the path exists, `0` otherwise.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_exists("/tmp");
    println(ok); // 1
    return ok;
}
```

---

### `fs_is_dir`

Checks whether a path points to a directory.

**Syntax:**

```hoo
fs_is_dir(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to check. |

**Returns:**

`int64` — Returns `1` if the path is a directory, `0` otherwise.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_is_dir("/tmp");
    println(ok); // 1
    return ok;
}
```

---

### `fs_is_file`

Checks whether a path points to a regular file.

**Syntax:**

```hoo
fs_is_file(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to check. |

**Returns:**

`int64` — Returns `1` if the path is a regular file, `0` otherwise.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_is_file("/tmp/data.txt");
    println(ok);
    return ok;
}
```

---

### `fs_size`

Returns the size of a file in bytes.

**Syntax:**

```hoo
fs_size(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to the file. |

**Returns:**

`int64` — The file size in bytes. Returns `-1` if the file does not exist or cannot be read.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var sz = fs_size("/tmp/data.txt");
    if sz >= 0 {
        println("size: " + sz);
    }
    return sz >= 0 ? 0 : 1;
}
```

---

### `fs_last_modified`

Returns the last modification timestamp of a file in seconds since the Unix epoch.

**Syntax:**

```hoo
fs_last_modified(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path. |

**Returns:**

`int64` — The modification timestamp in seconds since the Unix epoch. Returns `-1` if `path` is nil or the file cannot be accessed.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var mtime = fs_last_modified("/tmp/data.txt");
    if mtime >= 0 {
        println("Last modified: " + mtime);
    }
    return mtime >= 0 ? 0 : 1;
}
```

---

### `fs_delete`

Deletes a file or empty directory.

**Syntax:**

```hoo
fs_delete(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to delete. |

**Returns:**

`int64` — `1` on success, `0` on failure.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_delete("/tmp/temp.txt");
    println(ok); // 1 on success
    return ok;
}
```

---

### `fs_remove`

Alias of `fs_delete` that removes a file or empty directory.

**Syntax:**

```hoo
fs_remove(path: string) :int64
```

**Returns:**

`int64` — `1` on success, `0` on failure.

---

### `fs_rename`

Renames or moves a file or directory.

**Syntax:**

```hoo
fs_rename(old_path: string, new_path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `old_path` | `string` | The current path. |
| `new_path` | `string` | The new path. |

**Returns:**

`int64` — `1` on success, `0` on failure.

---

### `fs_move`

Moves or renames a file or directory. Alias of `fs_rename`.

**Syntax:**

```hoo
fs_move(source: string, dest: string) :int64
```

**Returns:**

`int64` — `1` on success, `0` on failure.

---

### `fs_copy`

Copies a file from a source path to a destination path. Overwrites the destination if it already exists.

**Syntax:**

```hoo
fs_copy(source: string, dest: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `source` | `string` | The source file path. |
| `dest` | `string` | The destination file path. |

**Returns:**

`int64` — `1` on success, `0` on failure.

---

### `fs_mkdir`

Creates a single directory. The parent directory must already exist.

**Syntax:**

```hoo
fs_mkdir(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory to create. |

**Returns:**

`int64` — `1` on success, `0` on failure (including when the parent does not exist).

---

### `fs_mkdirs`

Creates a directory and all missing parent directories (like `mkdir -p`).

**Syntax:**

```hoo
fs_mkdirs(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory tree to create. |

**Returns:**

`int64` — `1` if a directory was created, `0` on failure or if the directory already exists.

---

### `fs_rmdir`

Removes an empty directory.

**Syntax:**

```hoo
fs_rmdir(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory to remove. |

**Returns:**

`int64` — `1` on success, `0` on failure (including when the directory is not empty).

---

### `fs_list_dir`

Returns an array of the entry names in a directory. The entries are the base names of files and subdirectories (not full paths).

**Syntax:**

```hoo
fs_list_dir(path: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory to list. |

**Returns:**

`array` — An array of strings containing the entry names. Returns an empty array for an empty directory, and `0` (null) if the directory does not exist or cannot be read.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var entries = fs_list_dir("/tmp");
    if entries != 0 {
        println(entries.length());
        for i in 0..entries.length() {
            var name: string = entries.getString(i);
            println(name);
        }
    }
    return entries != 0 ? 0 : 1;
}
```

---

### `fs_temp_dir`

Returns the system's temporary directory path.

**Syntax:**

```hoo
fs_temp_dir() :string
```

**Returns:**

`string` — The temporary directory path. Returns `0` on error.

---

### `fs_create_temp_file`

Creates a temporary file and returns its path. The file is created and must be deleted by the caller.

**Syntax:**

```hoo
fs_create_temp_file(prefix: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `prefix` | `string` | A prefix for the temporary file name (may be empty). |

**Returns:**

`string` — The path to the newly created temporary file. Returns `0` on error.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var tmpfile = fs_create_temp_file("myapp");
    if tmpfile != 0 {
        println("Temp file: " + tmpfile);
        fs_delete(tmpfile);
    }
    return tmpfile != 0 ? 0 : 1;
}
```

---

### `fs_create_temp_dir`

Creates a temporary directory and returns its path. The directory is created and must be removed by the caller.

**Syntax:**

```hoo
fs_create_temp_dir() :string
```

**Returns:**

`string` — The path to the newly created temporary directory. Returns `0` on error.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var tmpdir = fs_create_temp_dir();
    if tmpdir != 0 {
        println("Temp dir: " + tmpdir);
        fs_rmdir(tmpdir);
    }
    return tmpdir != 0 ? 0 : 1;
}
```

---

### `fs_current_dir`

Returns the current working directory.

**Syntax:**

```hoo
fs_current_dir() :string
```

**Returns:**

`string` — The current working directory path. Returns `0` on error.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var dir = fs_current_dir();
    if dir != 0 {
        println(dir);
    }
    return dir != 0 ? 0 : 1;
}
```

---

### `fs_current_exe_dir`

Returns the directory containing the running executable.

**Syntax:**

```hoo
fs_current_exe_dir() :string
```

**Returns:**

`string` — The directory path of the running executable. Returns `0` on error.

---

### `fs_read_bytes`

Reads the entire contents of a file into a `Buffer`.

**Syntax:**

```hoo
fs_read_bytes(path: string) :Buffer
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to read from. |

**Returns:**

`Buffer` — A buffer containing the file data. Returns `0` (null) if the file does not exist or cannot be read. An existing-but-empty file yields an empty buffer (length `0`).

**Complete Example:**

```hoo
import hoo.io;
import hoo.buffer;

func :int64 main() {
    var bytes = fs_read_bytes("/tmp/data.bin");
    if bytes != 0 {
        println(bytes.length());
        bytes.release();
    }
    return bytes != 0 ? 0 : 1;
}
```

---

### `fs_read_bytes` (with fallback)

Reads a file into a `Buffer`, returning a caller-supplied fallback buffer when the file does not exist.

**Syntax:**

```hoo
fs_read_bytes(path: string, fallback: Buffer) :Buffer
```

**Returns:**

`Buffer` — The file contents, or `fallback` if the file does not exist or cannot be read.

---

### `fs_read_bytes_buffer`

Reads a file's entire contents into a new `Buffer`. With a fallback argument, returns the fallback when the file does not exist.

**Syntax:**

```hoo
fs_read_bytes_buffer(path: string) :Buffer
fs_read_bytes_buffer(path: string, fallback: Buffer) :Buffer
```

**Returns:**

`Buffer` — A buffer containing the file data, or `0` (null) if the file cannot be read (or `fallback` when provided).

---

### `fs_write_bytes`

Writes the contents of a `Buffer` to a file, overwriting any existing content.

**Syntax:**

```hoo
fs_write_bytes(path: string, buf: Buffer) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to write to. |
| `buf` | `Buffer` | The buffer containing the data to write. |

**Returns:**

`int64` — `1` on success, `0` on failure.

---

### `fs_write_bytes_buffer`

Alias of `fs_write_bytes` that writes the contents of a `Buffer` to a file.

**Syntax:**

```hoo
fs_write_bytes_buffer(path: string, buf: Buffer) :int64
```

**Returns:**

`int64` — `1` on success, `0` on failure.

**Complete Example:**

```hoo
import hoo.io;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("binary data");
    var written = fs_write_bytes_buffer("/tmp/data.bin", buf);
    buf.release();
    return written;
}
```

---

## Usage Example

```hoo
import hoo.io;

func :int64 main() {
    // Write a file
    fs_write_text("/tmp/example.txt", "Hello, Hoo!");

    // Check it exists
    if fs_exists("/tmp/example.txt") == 1 {
        println("file exists");
    }

    // Read it back
    var content = fs_read_text("/tmp/example.txt");
    if content != 0 {
        println(content);
    }

    // Get file size
    var sz = fs_size("/tmp/example.txt");
    println("size: " + sz);

    // List directory
    var entries = fs_list_dir("/tmp");
    if entries != 0 {
        println("entries: " + entries.length());
    }

    // Clean up
    fs_delete("/tmp/example.txt");

    return 0;
}
```
