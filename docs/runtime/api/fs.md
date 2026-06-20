# Fs API Reference

## Module Name

Part of the `hoo.io` module.

## Import Statement

```hoo
import hoo.io;
```

## Module Description

The fs module provides filesystem operations including file reading and writing, directory management, path queries, and file metadata inspection. All functions are free functions in the `hoo.io` module.

## Functions

### `fs_read`

Reads the entire contents of a text file as a string.

**Syntax:**

```hoo
fs_read(path: string) :string
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to read from. |

**Returns:**

`string` — The file contents. Returns `0` if the file does not exist, cannot be read, or is empty.

**Errors:**

Returns `0` if `path` is nil, the file does not exist, cannot be read, or contains no data.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var content = fs_read("/tmp/hello.txt");
    if content != 0 {
        println(content);
    }
    return content != 0 ? 0 : 1;
}
```

---

### `fs_write`

Writes a string to a text file, overwriting any existing content. Creates the file if it does not exist.

**Syntax:**

```hoo
fs_write(path: string, data: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to write to. |
| `data` | `string` | The text to write. |

**Returns:**

`void`

**Errors:**

No return value; if the file cannot be written the operation silently fails.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    fs_write("/tmp/hello.txt", "Hello, world!");
    return 0;
}
```

---

### `fs_append`

Appends a string to the end of a text file. Creates the file if it does not exist.

**Syntax:**

```hoo
fs_append(path: string, data: string) :void
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to append to. |
| `data` | `string` | The text to append. |

**Returns:**

`void`

**Errors:**

No return value; if the file cannot be written the operation silently fails.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    fs_append("/tmp/log.txt", "new entry\n");
    return 0;
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

**Errors:**

Returns `0` if `path` is nil.

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

**Errors:**

Returns `0` if `path` is nil.

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

**Errors:**

Returns `0` if `path` is nil.

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

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if `path` is nil or the file cannot be deleted.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_delete("/tmp/temp.txt");
    println(ok); // 0 on success
    return ok;
}
```

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

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if `path` is nil, the parent does not exist, or the directory cannot be created.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_mkdir("/tmp/newdir");
    println(ok); // 0 on success
    return ok;
}
```

---

### `fs_mkdirs`

Creates a directory and all missing parent directories (like `mkdir -p`). Does not fail if the directory already exists.

**Syntax:**

```hoo
fs_mkdirs(path: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory tree to create. |

**Returns:**

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if `path` is nil or the directory tree cannot be created.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_mkdirs("/tmp/a/b/c");
    println(ok); // 0 on success
    return ok;
}
```

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

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if `path` is nil, the directory is not empty, or the directory cannot be removed.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_rmdir("/tmp/emptydir");
    println(ok);
    return ok;
}
```

---

### `fs_list`

Returns an array of filenames in a directory. The entries are the base names of files and subdirectories (not full paths).

**Syntax:**

```hoo
fs_list(path: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path of the directory to list. |

**Returns:**

`array` — An array of strings containing the entry names. Returns `0` if the directory does not exist, is empty, or cannot be read.

**Errors:**

Returns `0` if `path` is nil, the directory does not exist, is empty, or cannot be read.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var entries = fs_list("/tmp");
    if entries != 0 {
        println(entries.length());
    }
    return entries != 0 ? 0 : 1;
}
```

---

### `fs_cwd`

Returns the current working directory.

**Syntax:**

```hoo
fs_cwd() :string
```

**Parameters:**

None.

**Returns:**

`string` — The current working directory path. Returns `0` on error.

**Errors:**

Returns `0` if the current working directory cannot be determined.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var dir = fs_cwd();
    if dir != 0 {
        println(dir);
    }
    return dir != 0 ? 0 : 1;
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

**Errors:**

Returns `-1` if `path` is nil or the file cannot be accessed.

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

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if either path is nil or the copy fails.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_copy("/tmp/source.txt", "/tmp/dest.txt");
    println(ok);
    return ok;
}
```

---

### `fs_move`

Moves or renames a file or directory.

**Syntax:**

```hoo
fs_move(source: string, dest: string) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `source` | `string` | The current file system path. |
| `dest` | `string` | The new file system path. |

**Returns:**

`int64` — Returns `0` on success, non-zero on failure.

**Errors:**

Returns a non-zero value if either path is nil or the operation fails.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_move("/tmp/old.txt", "/tmp/new.txt");
    println(ok);
    return ok;
}
```

---

### `fs_read_bytes`

Reads the entire contents of a file into a byte array.

**Syntax:**

```hoo
fs_read_bytes(path: string) :array
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to read from. |

**Returns:**

`array` — An array of bytes (int64 values) containing the file data. Returns `0` if the file does not exist, cannot be read, or is empty.

**Errors:**

Returns `0` if `path` is nil, the file does not exist, cannot be read, or contains no data.

**Complete Example:**

```hoo
import hoo.io;

func :int64 main() {
    var bytes = fs_read_bytes("/tmp/data.bin");
    if bytes != 0 {
        println(bytes.length());
    }
    return bytes != 0 ? 0 : 1;
}
```

---

### `fs_read_bytes_buffer`

Reads file content into an existing `Buffer` at the specified offset and length.

**Syntax:**

```hoo
fs_read_bytes_buffer(path: string, buffer: Buffer, offset: int64, length: int64) :int64
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to read from. |
| `buffer` | `Buffer` | The target buffer to read data into. |
| `offset` | `int64` | The byte offset in the file to start reading from. |
| `length` | `int64` | The number of bytes to read. |

**Returns:**

`int64` — The number of bytes actually read. Returns `-1` on error.

**Errors:**

Returns `-1` if `path` is nil, the file cannot be read, or the buffer is invalid.

**Complete Example:**

```hoo
import hoo.io;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer();
    var bytes_read = fs_read_bytes_buffer("/tmp/data.bin", buf, 0, 1024);
    if bytes_read > 0 {
        println("read " + bytes_read + " bytes");
    }
    return bytes_read > 0 ? 0 : 1;
}
```

---

### `fs_last_modified`

Returns the last modification timestamp of a file in milliseconds since the Unix epoch.

**Syntax:**
```hoo
fs_last_modified(path: string) :int64
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path. |
**Returns:** `int64` — The modification timestamp in milliseconds since the Unix epoch. Returns `-1` on error.
**Errors:** Returns `-1` if `path` is nil or the file cannot be accessed.
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
**Returns:** `int64` — `0` on success, non-zero on failure.
**Errors:** Returns non-zero if either path is nil or the rename fails.
**Complete Example:**
```hoo
import hoo.io;

func :int64 main() {
    var ok = fs_rename("/tmp/old.txt", "/tmp/new.txt");
    println(ok);
    return ok;
}
```

---

### `fs_write_bytes_buffer`

Writes the contents of a Buffer to a file, overwriting any existing content.

**Syntax:**
```hoo
fs_write_bytes_buffer(path: string, buf: Buffer) :int64
```
**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `string` | The file system path to write to. |
| `buf` | `Buffer` | The buffer containing data to write. |
**Returns:** `int64` — The number of bytes written. Returns `-1` on error.
**Errors:** Returns `-1` if `path` is nil, `buf` is null, or the file cannot be written.
**Complete Example:**
```hoo
import hoo.io;
import hoo.buffer;

func :int64 main() {
    var buf = Buffer(64);
    buf.write("binary data");
    var written = fs_write_bytes_buffer("/tmp/data.bin", buf);
    if written >= 0 {
        println("wrote " + written + " bytes");
    }
    buf.release();
    return written >= 0 ? 0 : 1;
}
```

---

### `fs_temp_dir`

Returns the system's temporary directory path.

**Syntax:**
```hoo
fs_temp_dir() :string
```
**Parameters:** None.
**Returns:** `string` — The temporary directory path. Returns `0` on error.
**Errors:** Returns `0` if the temp directory cannot be determined.
**Complete Example:**
```hoo
import hoo.io;

func :int64 main() {
    var tmp = fs_temp_dir();
    if tmp != 0 {
        println("Temp dir: " + tmp);
    }
    return tmp != 0 ? 0 : 1;
}
```

---

### `fs_create_temp_file`

Creates a temporary file and returns its path.

**Syntax:**
```hoo
fs_create_temp_file() :string
```
**Parameters:** None.
**Returns:** `string` — The path to the newly created temporary file. Returns `0` on error.
**Errors:** Returns `0` if the temporary file cannot be created.
**Complete Example:**
```hoo
import hoo.io;

func :int64 main() {
    var tmpfile = fs_create_temp_file();
    if tmpfile != 0 {
        println("Temp file: " + tmpfile);
        // Use tmpfile, then delete it
        fs_delete(tmpfile);
    }
    return tmpfile != 0 ? 0 : 1;
}
```

## Usage Example

```hoo
import hoo.io;

func :int64 main() {
    // Write a file
    fs_write("/tmp/example.txt", "Hello, Hoo!");

    // Check it exists
    if fs_exists("/tmp/example.txt") == 1 {
        println("file exists");
    }

    // Read it back
    var content = fs_read("/tmp/example.txt");
    if content != 0 {
        println(content);
    }

    // Get file size
    var sz = fs_size("/tmp/example.txt");
    println("size: " + sz);

    // List directory
    var entries = fs_list("/tmp");
    if entries != 0 {
        println("entries: " + entries.length());
    }

    // Clean up
    fs_delete("/tmp/example.txt");

    return 0;
}
```
