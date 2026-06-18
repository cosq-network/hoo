# File System (`hoo.fs`)

The `hoo.fs` module provides file I/O, directory traversal, temporary files, and file metadata queries, wrapping C++17 `<filesystem>`.

The module exposes two API layers:
- **C++ API** — object-oriented classes: `hoo::fs::File`, `hoo::fs::Directory`, `hoo::fs::Path`
- **C-ABI bridge** — flat `hoo_fs_*` functions used by the JIT / FFI layer

All Hoo language calls (`Fs.exists`, `Fs.delete`, etc.) resolve through the JIT bridge to the C-ABI layer, which delegates to the C++ classes.

---

## 1. `hoo::fs::File`

Instance methods on the `File` class. Construct with `File(path)`.

| Method | Signature | Description |
|---|---|---|
| `path` | `() -> string` | The path this instance was constructed with. |
| `exists` | `() -> bool` | Check if the path exists. |
| `isFile` | `() -> bool` | Check if path is a regular file. |
| `size` | `() -> i64` | File size in bytes; `-1` on error. |
| `lastModified` | `() -> i64` | Unix timestamp (seconds since epoch); `-1` on error. |
| `remove` | `() -> bool` | Delete the file or empty directory. |
| `rename` | `(newPath) -> bool` | Rename / move. Updates the stored path on success. |
| `readText` | `() -> string` | Read entire text file. Returns empty string on error. |
| `writeText` | `(content) -> bool` | Write string to file, overwriting. |
| `appendText` | `(content) -> bool` | Append string to file. Creates file if missing. |
| `readBytes` | `(out vector<u8>) -> bool` | Read entire binary file into vector. |
| `writeBytes` | `(vector<u8>) -> bool` | Write raw bytes to file, overwriting. |

## 2. `hoo::fs::Directory`

Instance methods on the `Directory` class. Construct with `Directory(path)`.

| Method | Signature | Description |
|---|---|---|
| `path` | `() -> string` | The path this instance was constructed with. |
| `exists` | `() -> bool` | Check if the path exists. |
| `isDirectory` | `() -> bool` | Check if path is a directory. |
| `create` | `() -> bool` | Create a single directory (parent must exist). |
| `createTree` | `() -> bool` | Create directory tree (mkdir -p). |
| `remove` | `() -> bool` | Remove an empty directory. |
| `list` | `() -> vector<string>` | List directory contents (filenames only, no paths). |

## 3. `hoo::fs::Path`

Instance methods on the `Path` class. Construct with `Path(path)`.

### Component Extraction

| Method | Signature | Description |
|---|---|---|
| `str` | `() -> string` | The original path string. |
| `dirname` | `() -> string` | Parent directory. `Path("a/b/c.txt").dirname()` → `"a/b"`. Returns `"."` for bare filenames. |
| `basename` | `() -> string` | Last path component. Returns `"c.txt"`. |
| `extension` | `() -> string` | Extension including dot. `".gz"`. |
| `stem` | `() -> string` | Filename without extension. `"resume"`. |
| `root` | `() -> string` | Root component. `"/"` or `"C:\"`. |

### Normalization & Resolution

| Method | Signature | Description |
|---|---|---|
| `normalized` | `() -> Path` | Resolve `.`/`..`, collapse redundant separators. Returns new `Path`. |
| `absolute` | `() -> Path` | Convert to absolute path relative to CWD. Returns new `Path`. |

### Properties

| Method | Signature | Description |
|---|---|---|
| `isAbsolute` | `() -> bool` | Returns true if path is absolute. |
| `isRelative` | `() -> bool` | Returns true if path is relative. |
| `hasExtension` | `() -> bool` | Returns true if path has an extension. |
| `hasRoot` | `() -> bool` | Returns true if path has a root component. |

### Split

| Method | Signature | Description |
|---|---|---|
| `split` | `() -> vector<string>` | Split path into individual components. |

## 4. Free Functions

Operations that don't naturally belong on a single instance:

| Function | Signature | Description |
|---|---|---|
| `join` | `(a, b) -> string` | Join two components with platform separator. |
| `joinMulti` | `(parts) -> string` | Join multiple components. |
| `relative` | `(path, base) -> string` | Compute relative path from `base` to `path`. |
| `separator` | `() -> char` | `'/'` on Unix, `'\\'` on Windows. |
| `listSeparator` | `() -> char` | `':'` on Unix, `';'` on Windows. |
| `tempDir` | `() -> string` | System temporary directory path. |
| `createTempFile` | `(prefix) -> string` | Create a temporary file. Returns the full path. |
| `copyFile` | `(src, dst) -> bool` | Copy a file (overwrites destination). |

---

## C-ABI Bridge Functions (JIT / FFI)

These functions are declared `extern "C"` and remain stable for JIT linkage.

| Function | Description |
|---|---|
| `hoo_fs_exists(path)` | Returns 1 if exists, 0 otherwise. |
| `hoo_fs_is_file(path)` | Returns 1 if regular file. |
| `hoo_fs_is_dir(path)` | Returns 1 if directory. |
| `hoo_fs_size(path)` | Returns size in bytes, or -1. |
| `hoo_fs_last_modified(path)` | Returns Unix timestamp, or -1. |
| `hoo_fs_delete(path)` | Deletes file, returns 1 on success. |
| `hoo_fs_rename(old, new)` | Renames file/directory, returns 1 on success. |
| `hoo_fs_copy(src, dst)` | Copies file, returns 1 on success. |
| `hoo_fs_read_text(path)` | Returns malloc'd string (free with `hoo_fs_free_string`), or NULL. |
| `hoo_fs_write_text(path, content)` | Writes string, returns 1 on success. |
| `hoo_fs_append_text(path, content)` | Appends string, returns 1 on success. |
| `hoo_fs_read_bytes(path, &out_data, &out_len)` | Reads binary into malloc'd buffer, returns 1 on success. |
| `hoo_fs_write_bytes(path, data, len)` | Writes binary, returns 1 on success. |
| `hoo_fs_mkdir(path)` | Creates single directory, returns 1 on success. |
| `hoo_fs_mkdirs(path)` | Creates directory tree, returns 1 on success. |
| `hoo_fs_rmdir(path)` | Removes empty directory, returns 1 on success. |
| `hoo_fs_list_dir(path, &count)` | Lists directory into malloc'd array (free with `hoo_fs_free_list`). |
| `hoo_fs_free_list(list, count)` | Frees array returned by `hoo_fs_list_dir`. |
| `hoo_fs_temp_dir()` | Returns malloc'd temp directory path string. |
| `hoo_fs_create_temp_file(prefix)` | Returns malloc'd temp file path string. |
| `hoo_fs_free_string(str)` | Frees a string returned by any `hoo_fs_*` function. |

### Path C-ABI Bridges

These functions (originally from `hoo_path.h`) are now defined in `hoo_fs.cpp`
and delegate to `hoo::fs::Path`. They remain stable for JIT/FFI linkage.

| Function | Description |
|---|---|
| `hoo_path_dirname(path)` | Parent directory (malloc'd, free with `hoo_path_free_string`). |
| `hoo_path_basename(path)` | Last component (malloc'd). |
| `hoo_path_extension(path)` | Extension with dot (malloc'd). |
| `hoo_path_stem(path)` | Filename without extension (malloc'd). |
| `hoo_path_root(path)` | Root component (malloc'd). |
| `hoo_path_join(a, b)` | Join two components (malloc'd). |
| `hoo_path_join_multi(parts, count)` | Join multiple components (malloc'd). |
| `hoo_path_normalize(path)` | Normalize path (malloc'd). |
| `hoo_path_absolute(path)` | Absolute path (malloc'd). |
| `hoo_path_relative(path, base)` | Relative path (malloc'd). |
| `hoo_path_is_absolute(path)` | Returns 1 if absolute. |
| `hoo_path_is_relative(path)` | Returns 1 if relative. |
| `hoo_path_has_extension(path)` | Returns 1 if has extension. |
| `hoo_path_has_root(path)` | Returns 1 if has root component. |
| `hoo_path_split(path, &count)` | Split into malloc'd array (free with `hoo_path_free_parts`). |
| `hoo_path_free_parts(parts, count)` | Free parts array. |
| `hoo_path_separator()` | Returns platform path separator character. |
| `hoo_path_list_separator()` | Returns platform list separator character. |
| `hoo_path_free_string(str)` | Frees a string returned by any `hoo_path_*` function. |

## Hoo Language API

Hoo code accesses FS operations through the `Fs` module class:

```
import hoo.fs

val ok = Fs.exists("myfile.txt")
val text = Fs.readText("myfile.txt")
val size = Fs.size("myfile.txt")
```

These calls resolve via the JIT bridge to the C-ABI functions above.

## Memory Management

- Strings returned from `hoo_fs_read_text`, `hoo_fs_temp_dir`, `hoo_fs_create_temp_file` must be freed with `hoo_fs_free_string`.
- Directory listings from `hoo_fs_list_dir` must be freed with `hoo_fs_free_list(list, count)`.
- The C++ API (`File(path).readText()`, `tempDir()`, etc.) returns `std::string` / `std::vector`, which manage their own memory.
